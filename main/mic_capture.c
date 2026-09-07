#include "mic_capture.h"

#include "sdkconfig.h"

#if CONFIG_AA_MIC_ENABLE

#include <stdlib.h>
#include <string.h>

#include "bsp/esp32_p4_wifi6_touch_lcd_4_3.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "mic";

/* ES7210 hands MIC1/MIC2 as an I2S stereo pair; the ES8311 ADC is mono. */
#if CONFIG_BOARD_JC4880P443C
#define MIC_HW_CHANNELS   1
#define MIC_SLOT_MODE     I2S_SLOT_MODE_MONO
#else
#define MIC_HW_CHANNELS   2
#define MIC_SLOT_MODE     I2S_SLOT_MODE_STEREO
#endif

/* Analog input gain. The ES7210 driver programs 30 dB at open; Waveshare's
 * own record demo settles on 24 dB, which is what the MEMS mics on this
 * board were tuned against. */
#define MIC_GAIN_DB       24.0f

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
    s_dev = bsp_audio_codec_microphone_init();
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
    const size_t raw_bytes = MIC_CAPTURE_CHUNK_SAMPLES * MIC_HW_CHANNELS * sizeof(int16_t);
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
#if MIC_HW_CHANNELS == 2
        for (size_t i = 0; i < MIC_CAPTURE_CHUNK_SAMPLES; i++) mono[i] = raw[2 * i];
#else
        memcpy(mono, raw, MIC_CAPTURE_CHUNK_SAMPLES * sizeof(int16_t));
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

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
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
