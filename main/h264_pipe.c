#include "h264_pipe.h"

#include <string.h>

#include "display_video.h"
#include "driver/uart.h"
#include "esp_h264_dec.h"
#include "esp_h264_dec_param.h"
#include "esp_h264_dec_sw.h"
#include "esp_h264_types.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"

static const char *TAG = "h264_pipe";

/* One-shot debug: dump the 10th decoded I420 frame as base64 to the console
 * so we can extract it on the host and inspect/play it. The output is framed
 * by sentinel lines so scripts/extract_yuv.py can pick it out of capture
 * logs. Disabled now that the decoder has been verified — flip back to 1
 * if we need another sample. */
#define H264_DUMP_FIRST_FRAME 0

#if H264_DUMP_FIRST_FRAME
/* Base64 dump straight to UART0 (the IDF console). Going through printf /
 * stdio caused silent byte loss past ~120 KiB even with fflush — VFS
 * console driver has a finite TX buffer it doesn't backpressure on. The
 * driver-API uart_write_bytes blocks until the bytes are queued in the
 * UART TX ring, which is what we want for a one-shot frame dump.
 *
 * 64 base64 chars per line (= 48 raw bytes). Throttle every 256 lines so
 * the TX ring drains — at 921600 baud the throughput is ~92 KiB/s, so
 * 16 KiB of console output per 256 lines takes ~180 ms; 100 ms vTaskDelay
 * has plenty of margin. */
static void dump_i420_base64(const uint8_t *buf, size_t len,
                             uint16_t w, uint16_t h)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    /* IDF v5 console uses esp_rom_printf for UART0 by default and never
     * installs the regular UART driver — uart_write_bytes returns
     * "uart driver error" until we set it up ourselves. Install on first
     * dump (idempotent), 16 KiB TX ring buffer is enough headroom. */
    static bool s_uart_installed;
    if (!s_uart_installed) {
        if (!uart_is_driver_installed(UART_NUM_0)) {
            esp_err_t e = uart_driver_install(UART_NUM_0,
                                              256, 16 * 1024, 0, NULL, 0);
            if (e != ESP_OK) {
                ESP_LOGE(TAG, "uart_driver_install: %s", esp_err_to_name(e));
                return;
            }
        }
        s_uart_installed = true;
    }

    char hdr[80];
    int hl = snprintf(hdr, sizeof(hdr),
                      "\n=== I420_DUMP_BEGIN w=%u h=%u size=%u ===\n",
                      (unsigned)w, (unsigned)h, (unsigned)len);
    uart_write_bytes(UART_NUM_0, hdr, (size_t)hl);
    uart_wait_tx_done(UART_NUM_0, pdMS_TO_TICKS(200));

    char line[68];
    size_t lp = 0;
    size_t lines = 0;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t v = (uint32_t)buf[i] << 16;
        if (i + 1 < len) v |= (uint32_t)buf[i + 1] << 8;
        if (i + 2 < len) v |= (uint32_t)buf[i + 2];
        line[lp++] = alphabet[(v >> 18) & 0x3F];
        line[lp++] = alphabet[(v >> 12) & 0x3F];
        line[lp++] = (i + 1 < len) ? alphabet[(v >> 6) & 0x3F] : '=';
        line[lp++] = (i + 2 < len) ? alphabet[v & 0x3F]        : '=';
        if (lp >= 64) {
            line[lp++] = '\n';
            uart_write_bytes(UART_NUM_0, line, lp);
            lp = 0;
            /* Yield every 32 lines so IDLE can run and pet the IDLE-task
             * watchdog (~10 s total dump vs 5 s WDT). uart_write_bytes
             * already blocks on TX ring full, but its block path doesn't
             * always give IDLE enough CPU. */
            ++lines;
            if (lines % 32 == 0) {
                vTaskDelay(1);
            }
            if (lines % 256 == 0) {
                uart_wait_tx_done(UART_NUM_0, pdMS_TO_TICKS(500));
            }
        }
    }
    if (lp > 0) {
        line[lp++] = '\n';
        uart_write_bytes(UART_NUM_0, line, lp);
    }
    uart_wait_tx_done(UART_NUM_0, pdMS_TO_TICKS(500));
    static const char tail[] = "=== I420_DUMP_END ===\n";
    uart_write_bytes(UART_NUM_0, tail, sizeof(tail) - 1);
    uart_wait_tx_done(UART_NUM_0, pdMS_TO_TICKS(500));
}
#endif

/* The ring is byte-stream — one xRingbufferReceive call returns whatever
 * contiguous bytes are available, which we then feed to the decoder which
 * does its own NAL framing via Annex B start codes. This decouples AAP
 * frame boundaries from decoder calls, so a phone-side fragmented frame
 * that we already reassembled into one push() can still be split across
 * several decode loops without losing bytes. 256 KiB headroom — observed
 * "ring full, dropped 15 KiB" with a smaller ring when an I-frame burst
 * arrives faster than the SW decoder can drain. */
#define H264_RING_SIZE_BYTES   (256 * 1024)
#define H264_TASK_STACK_BYTES  (8 * 1024)
#define H264_TASK_PRIORITY     5
/* No core pin: ESP_H264_DUAL_TASK is on so the decoder spawns its own
 * worker on CPU1 internally. Our outer driver task can run wherever the
 * scheduler puts it; that's plenty since esp_h264_dec_process is the
 * heavy bit and it dispatches across both cores on its own. */

static RingbufHandle_t       s_ring;
static esp_h264_dec_handle_t s_dec;
static esp_h264_dec_param_handle_t s_dec_param;
static TaskHandle_t          s_task;

/* Stats — reported once per second from the decoder task. */
static uint32_t  s_decoded_frames;
static uint32_t  s_decode_errors;
static uint64_t  s_decode_total_us;

static void log_stats_once_per_second(void)
{
    static int64_t window_start_us;
    int64_t now = esp_timer_get_time();
    if (window_start_us == 0) {
        window_start_us = now;
        return;
    }
    if (now - window_start_us < 1000000) return;

    if (s_decoded_frames > 0 || s_decode_errors > 0) {
        ESP_LOGI(TAG, "decoded %u frames (avg %llu us/frame), errors %u",
                 (unsigned)s_decoded_frames,
                 (unsigned long long)(s_decoded_frames
                     ? (s_decode_total_us / s_decoded_frames) : 0),
                 (unsigned)s_decode_errors);
    }
    s_decoded_frames  = 0;
    s_decode_errors   = 0;
    s_decode_total_us = 0;
    window_start_us   = now;
}

static void decoder_task(void *arg)
{
    (void)arg;
    bool seen_resolution = false;

    while (true) {
        size_t got = 0;
        uint8_t *chunk = (uint8_t *)xRingbufferReceive(s_ring, &got,
                                                      pdMS_TO_TICKS(100));
        log_stats_once_per_second();
        if (!chunk || got == 0) {
            if (chunk) vRingbufferReturnItem(s_ring, chunk);
            continue;
        }

        /* Drain all NAL units the decoder can find in this chunk. The
         * upstream code treats `consume` as how many input bytes the
         * decoder ate — loop until empty or the decoder rejects. */
        esp_h264_dec_in_frame_t  in  = { .raw_data = { chunk, (uint32_t)got } };
        esp_h264_dec_out_frame_t out = { 0 };
        while (in.raw_data.len > 0) {
            int64_t t0 = esp_timer_get_time();
            esp_h264_err_t e = esp_h264_dec_process(s_dec, &in, &out);
            int64_t dt = esp_timer_get_time() - t0;

            if (e != ESP_H264_ERR_OK) {
                s_decode_errors++;
                ESP_LOGW(TAG, "dec_process err=%d at %u/%u bytes",
                         (int)e,
                         (unsigned)in.consume,
                         (unsigned)in.raw_data.len);
                break;
            }

            if (out.out_size > 0) {
                s_decoded_frames++;
                s_decode_total_us += (uint64_t)dt;
                esp_h264_resolution_t res = {0};
                bool have_res = (s_dec_param &&
                    esp_h264_dec_get_resolution(s_dec_param, &res)
                        == ESP_H264_ERR_OK);
                if (!seen_resolution && have_res) {
                    ESP_LOGI(TAG, "first frame %ux%u, %u bytes I420",
                             res.width, res.height, (unsigned)out.out_size);
                    seen_resolution = true;
                }
#if H264_DUMP_FIRST_FRAME
                /* 10th decoded frame, not the 1st: I-frame at start of a
                 * stream can be partially-decoded or lossy until reference
                 * frames build up — and the artefact we're chasing is more
                 * obvious on a real picture than a freshly-keyed one. */
                static uint32_t s_dump_seq;
                s_dump_seq++;
                if (s_dump_seq == 10 && have_res) {
                    dump_i420_base64(out.outbuf, out.out_size,
                                     res.width, res.height);
                }
#endif
                if (have_res) {
                    /* Hand off to the display sink. PPA + dummy_draw_blit
                     * happen synchronously inside; if it stalls we'd see
                     * dropped frames before phone ack timeouts hit. */
                    display_video_show_yuv420(out.outbuf,
                                              res.width, res.height);
                }
            }

            if (in.consume == 0) {
                /* Decoder didn't make progress — bail to avoid spinning. */
                break;
            }
            in.raw_data.buffer += in.consume;
            in.raw_data.len    -= in.consume;
            in.consume = 0;
        }

        vRingbufferReturnItem(s_ring, chunk);
    }
}

esp_err_t h264_pipe_init(void)
{
    if (s_dec) return ESP_OK;

    esp_h264_dec_cfg_sw_t cfg = {
        .pic_type = ESP_H264_RAW_FMT_I420,
    };
    esp_h264_err_t e = esp_h264_dec_sw_new(&cfg, &s_dec);
    if (e != ESP_H264_ERR_OK) {
        ESP_LOGE(TAG, "esp_h264_dec_sw_new: %d", (int)e);
        return ESP_FAIL;
    }
    e = esp_h264_dec_open(s_dec);
    if (e != ESP_H264_ERR_OK) {
        ESP_LOGE(TAG, "esp_h264_dec_open: %d", (int)e);
        esp_h264_dec_del(s_dec);
        s_dec = NULL;
        return ESP_FAIL;
    }
    /* Param handle is optional (only used to read out resolution after the
     * first IDR). Failure here just means we won't log the resolution. */
    if (esp_h264_dec_sw_get_param_hd(s_dec, &s_dec_param) != ESP_H264_ERR_OK) {
        s_dec_param = NULL;
    }

    s_ring = xRingbufferCreate(H264_RING_SIZE_BYTES, RINGBUF_TYPE_BYTEBUF);
    if (!s_ring) {
        ESP_LOGE(TAG, "xRingbufferCreate failed");
        esp_h264_dec_close(s_dec);
        esp_h264_dec_del(s_dec);
        s_dec = NULL;
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreate(decoder_task, "h264_dec",
                                H264_TASK_STACK_BYTES, NULL,
                                H264_TASK_PRIORITY, &s_task);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "decoder ready (ring %u KiB)",
             (unsigned)(H264_RING_SIZE_BYTES / 1024));
    return ESP_OK;
}

void h264_pipe_push(const uint8_t *data, size_t len)
{
    if (!s_ring || !data || len == 0) return;
    /* Non-blocking: if the ring is full we'd rather drop a frame than
     * stall the AAP receive loop and trigger phone ack timeouts. */
    BaseType_t ok = xRingbufferSend(s_ring, data, len, 0);
    if (ok != pdTRUE) {
        ESP_LOGW(TAG, "ring full, dropped %u bytes", (unsigned)len);
    }
}
