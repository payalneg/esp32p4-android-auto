#include "mic_capture.h"

#include "sdkconfig.h"

#if CONFIG_AA_MIC_ENABLE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/esp32_p4_wifi6_touch_lcd_4_3.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"   /* ES7120_SEL_MIC* */
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "mic";

/* Waveshare 4.3: the two MEMS mics sit on ES7210 inputs MIC1 and MIC3 (NOT
 * MIC1/MIC2 — verified on hardware 2026-09-09 by tapping the mics with all
 * four inputs captured: only ADC1 and ADC3 moved; MIC2 is the AEC reference
 * from the ES8311 output, MIC4 is unconnected). In plain I2S mode SDOUT1 only
 * carries ADC1/ADC2, so MIC3 is unreachable; selecting three inputs makes the
 * esp_codec_dev driver switch the chip to 16-bit TDM and all four ADCs arrive
 * on SDOUT1. We open the device as 2 ch × 32 bit and read each 32-bit slot as
 * two 16-bit samples — memory lane order is [ADC2, ADC1, ADC4, ADC3] (LE
 * words, MSB-first slots), the same layout Waveshare's own demo consumes as
 * AFE format "RMNM". The ES8311 ADC on the JC4880 is plain mono. */
#if CONFIG_BOARD_JC4880P443C
#define MIC_HW_CHANNELS   1
#define MIC_SLOT_MODE     I2S_SLOT_MODE_MONO
#define MIC_TDM_SELECT    0   /* ES8311: ignored by the BSP */
#else
#define MIC_HW_CHANNELS   2
#define MIC_SLOT_MODE     I2S_SLOT_MODE_STEREO
#define MIC_TDM_SELECT    (ES7120_SEL_MIC1 | ES7120_SEL_MIC2 | ES7120_SEL_MIC3)
#define MIC_TDM_LANE_A    1   /* ADC1 = MIC1 */
#define MIC_TDM_LANE_B    3   /* ADC3 = MIC3 */
#endif

/* Analog input gain. Waveshare's demo runs the ES7210 at 30 dB (the driver's
 * maximum); taps on the mic holes peaked ~1400 at 24 dB, so there is ample
 * headroom for speech at 30. */
#define MIC_GAIN_DB       30.0f

/* The capture task calls into aa_service → mbedTLS encrypt → lwIP send.
 * touch_input does the same chain on 4 KiB; give the mic some margin since
 * its payload is 1.3 KiB per message instead of ~30 bytes. */
#define MIC_TASK_STACK    6144
#define MIC_TASK_PRIO     8

static esp_codec_dev_handle_t s_dev;
static TaskHandle_t           s_task;
static SemaphoreHandle_t      s_done;      /* task → stop(): "I'm out" */
static volatile bool          s_run;
static mic_pcm_cb_t           s_cb;
static void                  *s_cb_ctx;

/* int16 lanes per I2S frame: 4 on the ES7210 TDM read, 1 on the ES8311. */
#if CONFIG_BOARD_JC4880P443C
#define MIC_LANES         1
#else
#define MIC_LANES         4
#endif

static esp_err_t hw_init(void)
{
    if (s_dev) return ESP_OK;

    /* Bring the I2S port up already at the mic's format so esp_codec_dev
     * doesn't have to reconfigure the channel on open. bsp_audio_init is a
     * no-op if audio was initialised before (e.g. a future speaker path). */
    i2s_std_config_t cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(MIC_CAPTURE_SAMPLE_RATE),
        /* Plain token, not an expression: the macro compares its argument
         * with == inside, and a ?: would bind wrong there. */
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                        MIC_SLOT_MODE),
        .gpio_cfg = {
            .mclk = BSP_I2S_MCLK,
            .bclk = BSP_I2S_SCLK,
            .ws   = BSP_I2S_LCLK,
            .dout = BSP_I2S_DOUT,
            .din  = BSP_I2S_DSIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    esp_err_t err = bsp_audio_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_audio_init: %s", esp_err_to_name(err));
        return err;
    }
#if CONFIG_BOARD_JC4880P443C
    s_dev = bsp_audio_codec_microphone_init();
#else
    s_dev = bsp_audio_codec_microphone_init_sel(MIC_TDM_SELECT);
#endif
    if (!s_dev) {
        ESP_LOGE(TAG, "microphone codec init failed (ADC not answering on I2C?)");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "mic codec up: %d hw ch, %d Hz", MIC_HW_CHANNELS, MIC_CAPTURE_SAMPLE_RATE);
    return ESP_OK;
}

static void mic_task(void *arg)
{
    (void)arg;
    const size_t raw_bytes = MIC_CAPTURE_CHUNK_SAMPLES * MIC_LANES * sizeof(int16_t);
    int16_t *raw  = malloc(raw_bytes);
    int16_t *mono = malloc(MIC_CAPTURE_CHUNK_SAMPLES * sizeof(int16_t));
    if (!raw || !mono) {
        ESP_LOGE(TAG, "no memory for capture buffers");
        goto out;
    }

    uint32_t chunks = 0, errors = 0;
    while (s_run) {
        int r = esp_codec_dev_read(s_dev, raw, (int)raw_bytes);
        if (r != ESP_CODEC_DEV_OK) {
            if (errors++ < 3) ESP_LOGW(TAG, "codec read failed: %d", r);
            vTaskDelay(pdMS_TO_TICKS(MIC_CAPTURE_CHUNK_MS));
            continue;
        }
        if (!s_run) break;
        uint64_t ts = (uint64_t)esp_timer_get_time();
#if CONFIG_BOARD_JC4880P443C
        memcpy(mono, raw, MIC_CAPTURE_CHUNK_SAMPLES * sizeof(int16_t));
#else
        /* Both MEMS mics, averaged: +3 dB on uncorrelated noise, no comb
         * filtering worth noting at speech wavelengths for mics this close. */
        for (size_t i = 0; i < MIC_CAPTURE_CHUNK_SAMPLES; i++) {
            int32_t a = raw[MIC_LANES * i + MIC_TDM_LANE_A];
            int32_t b = raw[MIC_LANES * i + MIC_TDM_LANE_B];
            mono[i] = (int16_t)((a + b) / 2);
        }
#endif
        if (s_cb) s_cb(mono, MIC_CAPTURE_CHUNK_SAMPLES, ts, s_cb_ctx);
        chunks++;

    }
    ESP_LOGI(TAG, "capture stopped after %u chunks (%u read errors)",
             (unsigned)chunks, (unsigned)errors);

out:
    free(raw);
    free(mono);
    s_task = NULL;
    xSemaphoreGive(s_done);
    vTaskDelete(NULL);
}

esp_err_t mic_capture_start(mic_pcm_cb_t cb, void *ctx)
{
    if (s_task) {
        ESP_LOGW(TAG, "start while running — keeping the current capture");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = hw_init();
    if (err != ESP_OK) return err;

    /* ES7210: with ≥3 inputs selected the driver puts the chip in 16-bit TDM
     * and halves the requested bit depth (es7210_set_fs), so ask for
     * 2 ch × 32 bit: each I2S slot then carries two 16-bit ADCs. */
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = MIC_LANES == 4 ? 32 : 16,
        .channel         = MIC_HW_CHANNELS,
        .channel_mask    = 0,
        .sample_rate     = MIC_CAPTURE_SAMPLE_RATE,
        .mclk_multiple   = 0,
    };
    int r = esp_codec_dev_open(s_dev, &fs);
    if (r != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "codec open failed: %d", r);
        return ESP_FAIL;
    }
    /* After open: the ES7210 driver resets gain to 30 dB inside open(). */
    esp_codec_dev_set_in_gain(s_dev, MIC_GAIN_DB);

    if (!s_done) s_done = xSemaphoreCreateBinary();
    s_cb     = cb;
    s_cb_ctx = ctx;
    s_run    = true;
    BaseType_t ok = xTaskCreatePinnedToCore(mic_task, "aa_mic", MIC_TASK_STACK, NULL,
                                            MIC_TASK_PRIO, &s_task, tskNO_AFFINITY);
    if (ok != pdPASS) {
        s_run  = false;
        s_task = NULL;
        esp_codec_dev_close(s_dev);
        ESP_LOGE(TAG, "task create failed");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "capture started: %d ms chunks", MIC_CAPTURE_CHUNK_MS);
    return ESP_OK;
}

void mic_capture_stop(void)
{
    if (!s_task) return;
    s_run = false;
    /* esp_codec_dev_read blocks up to 1 s on the I2S driver; one TLS send
     * on a dying socket can add the socket send timeout on top. */
    if (xSemaphoreTake(s_done, pdMS_TO_TICKS(4000)) != pdTRUE) {
        ESP_LOGE(TAG, "capture task did not exit in time — leaving it be");
        return;
    }
    s_cb = NULL;
    esp_codec_dev_close(s_dev);
}

bool mic_capture_is_running(void)
{
    return s_task != NULL;
}

#else /* !CONFIG_AA_MIC_ENABLE */

esp_err_t mic_capture_start(mic_pcm_cb_t cb, void *ctx)
{
    (void)cb; (void)ctx;
    return ESP_ERR_NOT_SUPPORTED;
}

void mic_capture_stop(void) {}

bool mic_capture_is_running(void) { return false; }

#endif /* CONFIG_AA_MIC_ENABLE */
