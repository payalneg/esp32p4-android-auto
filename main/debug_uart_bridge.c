#include "debug_uart_bridge.h"

#include "sdkconfig.h"

#if CONFIG_DEBUG_UART_BRIDGE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "linenoise/linenoise.h"
#include "mbedtls/base64.h"
#include "driver/jpeg_encode.h"
#include "driver/uart.h"

#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "touch_input.h"

static const char *TAG = "dbg_uart";

/* lv_scr_act() is the 800x480 landscape screen. We size the snapshot buffer for
 * the full screen; a smaller active object just fills less of it (we trust the
 * dsc dimensions, not these constants, for the wire header / encode). */
#define SHOT_W            800
#define SHOT_H            480
#define SHOT_BUF_SIZE     (SHOT_W * SHOT_H * 2)   /* RGB565, 768000 B */
#define JPEG_OUT_CAP      (384 * 1024)            /* worst-case q100 fits easily */
#define CHUNK_RAW         768                     /* 768 raw -> exactly 1024 b64 */
#define B64_LINE_CAP      1025                    /* 1024 + NUL */

/* Lazily-allocated (first screenshot) so an idle debug build doesn't hold ~1 MB
 * PSRAM. The JPEG engine is shared with no one else here (the album-art decoder
 * in music_info_view.c uses a separate decoder engine). */
static uint8_t              *s_shot_buf;          /* RGB565 snapshot target */
static uint8_t              *s_jpeg_out;          /* HW-JPEG bitstream out */
static size_t                s_jpeg_cap;
static jpeg_encoder_handle_t s_enc;

/* Last injected point, so `touchup` can release at where the finger was. */
static uint16_t s_last_x, s_last_y;

static bool s_inited;

/* ---- helpers ---- */

static inline int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* Allocate the snapshot buffer (needed for both jpeg + raw) and, best-effort,
 * the JPEG encoder + output buffer (only needed for jpeg). Returns true if at
 * least the snapshot buffer is ready. */
static bool ensure_resources(bool need_jpeg)
{
    if (!s_shot_buf) {
        /* DMA-capable PSRAM, cache-line aligned — matches the JPEG decoder
         * scratch in music_info_view.c and satisfies the encoder's DMA read. */
        s_shot_buf = heap_caps_aligned_calloc(64, 1, SHOT_BUF_SIZE,
                                              MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
        if (!s_shot_buf) {
            ESP_LOGE(TAG, "no PSRAM for %d-byte snapshot buffer", SHOT_BUF_SIZE);
            return false;
        }
    }
    if (need_jpeg && !s_enc) {
        jpeg_encode_engine_cfg_t cfg = { .intr_priority = 0, .timeout_ms = 1000 };
        if (jpeg_new_encoder_engine(&cfg, &s_enc) != ESP_OK) {
            s_enc = NULL;   /* raw mode still works */
        }
    }
    if (need_jpeg && s_enc && !s_jpeg_out) {
        jpeg_encode_memory_alloc_cfg_t mcfg = {
            .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER,
        };
        size_t got = 0;
        s_jpeg_out = jpeg_alloc_encoder_mem(JPEG_OUT_CAP, &mcfg, &got);
        if (s_jpeg_out) s_jpeg_cap = got;
    }
    return true;
}

/* Emit one captured image as a base64-framed block on the console UART. Logging
 * is muted for the whole frame so no ESP_LOG line splices the payload; the host
 * additionally ignores any non-"SCR-" line and verifies len + crc32. */
static void send_frame(const char *fmt, int w, int h,
                       const uint8_t *data, size_t len)
{
    /* zlib-compatible CRC32 (host: zlib.crc32). Treated as advisory host-side
     * in case the ROM CRC convention differs across chips. */
    uint32_t crc = ~esp_rom_crc32_le(~0u, data, (uint32_t)len);
    int chunks = (int)((len + CHUNK_RAW - 1) / CHUNK_RAW);

    esp_log_level_t prev = esp_log_level_get("*");
    esp_log_level_set("*", ESP_LOG_NONE);

    printf("\nSCR-BEGIN w=%d h=%d fmt=%s len=%u crc32=0x%08x chunks=%d\n",
           w, h, fmt, (unsigned)len, (unsigned)crc, chunks);

    static char b64[B64_LINE_CAP];
    for (int i = 0; i < chunks; i++) {
        size_t off = (size_t)i * CHUNK_RAW;
        size_t n = (len - off) < CHUNK_RAW ? (len - off) : CHUNK_RAW;
        size_t olen = 0;
        if (mbedtls_base64_encode((unsigned char *)b64, sizeof(b64), &olen,
                                  data + off, n) != 0) {
            /* Should never happen at this chunk size; bail with a marker. */
            printf("SCR-ERR b64\n");
            break;
        }
        b64[olen] = '\0';
        printf("SCR-DATA %04d %s\n", i, b64);
    }
    printf("SCR-END\n");

    fflush(stdout);
    /* Drain the UART TX FIFO/ring before re-enabling logs so a queued log line
     * can't tail-splice into the last chunk. The REPL installed this driver. */
    uart_wait_tx_done(CONFIG_ESP_CONSOLE_UART_NUM, pdMS_TO_TICKS(3000));
    esp_log_level_set("*", prev);
}

static void do_screenshot(bool jpeg, int quality)
{
    if (!ensure_resources(jpeg)) {
        printf("SCR-ERR nomem\n");
        return;
    }

    lv_img_dsc_t dsc;
    memset(&dsc, 0, sizeof dsc);

    /* lv_snapshot re-renders the object tree into our buffer via LVGL's own
     * draw path — hold the display lock just for that, then release before the
     * (slow) encode + send so we don't starve the renderer. */
    if (bsp_display_lock(1000) != ESP_OK) {
        printf("SCR-ERR busy\n");
        return;
    }
    lv_res_t res = lv_snapshot_take_to_buf(lv_scr_act(), LV_IMG_CF_TRUE_COLOR,
                                           &dsc, s_shot_buf, SHOT_BUF_SIZE);
    bsp_display_unlock();
    if (res != LV_RES_OK) {
        printf("SCR-ERR snapshot\n");
        return;
    }

    int w = (int)dsc.header.w;
    int h = (int)dsc.header.h;
    size_t raw_len = (size_t)w * h * 2;

    if (jpeg && s_enc && s_jpeg_out) {
        jpeg_encode_cfg_t ecfg = {
            .width        = (uint32_t)w,
            .height       = (uint32_t)h,
            .src_type     = JPEG_ENCODE_IN_FORMAT_RGB565,
            .sub_sample   = JPEG_DOWN_SAMPLING_YUV420,
            .image_quality = (uint32_t)quality,
        };
        uint32_t out_len = 0;
        esp_err_t e = jpeg_encoder_process(s_enc, &ecfg, s_shot_buf,
                                           (uint32_t)raw_len,
                                           s_jpeg_out, s_jpeg_cap, &out_len);
        if (e == ESP_OK && out_len > 0) {
            send_frame("jpeg", w, h, s_jpeg_out, out_len);
            return;
        }
        ESP_LOGW(TAG, "jpeg encode failed (%s) — falling back to raw",
                 esp_err_to_name(e));
    }

    /* raw RGB565 (little-endian uint16, R in high bits — host unpacks 5-6-5). */
    send_frame("rgb565", w, h, s_shot_buf, raw_len);
}

/* ---- console commands (manual argv parsing — host is a script) ---- */

static int cmd_screenshot(int argc, char **argv)
{
    bool jpeg = true;
    int quality = CONFIG_DEBUG_UART_BRIDGE_JPEG_QUALITY;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fmt") == 0 && i + 1 < argc) {
            jpeg = (strcmp(argv[++i], "rgb565") != 0);
        } else if (strcmp(argv[i], "rgb565") == 0) {
            jpeg = false;
        } else if (strcmp(argv[i], "jpeg") == 0) {
            jpeg = true;
        } else if (strcmp(argv[i], "--quality") == 0 && i + 1 < argc) {
            quality = atoi(argv[++i]);
        }
    }
    do_screenshot(jpeg, clampi(quality, 1, 100));
    return 0;
}

static int cmd_tap(int argc, char **argv)
{
    if (argc < 3) { printf("usage: tap <x> <y>\n"); return 1; }
    uint16_t x = (uint16_t)clampi(atoi(argv[1]), 0, SHOT_W - 1);
    uint16_t y = (uint16_t)clampi(atoi(argv[2]), 0, SHOT_H - 1);
    s_last_x = x; s_last_y = y;
    /* press -> hold one+ LVGL read period -> release -> let LVGL fire CLICKED */
    touch_input_inject(x, y, true, 300);
    vTaskDelay(pdMS_TO_TICKS(80));
    touch_input_inject(x, y, false, 300);
    vTaskDelay(pdMS_TO_TICKS(80));
    touch_input_inject_clear();
    printf("OK tap %u %u\n", x, y);
    return 0;
}

static int cmd_swipe(int argc, char **argv)
{
    if (argc < 5) { printf("usage: swipe <x1> <y1> <x2> <y2> [ms]\n"); return 1; }
    int x1 = clampi(atoi(argv[1]), 0, SHOT_W - 1);
    int y1 = clampi(atoi(argv[2]), 0, SHOT_H - 1);
    int x2 = clampi(atoi(argv[3]), 0, SHOT_W - 1);
    int y2 = clampi(atoi(argv[4]), 0, SHOT_H - 1);
    int ms = argc >= 6 ? atoi(argv[5]) : 300;
    if (ms < 32) ms = 32;
    int steps = ms / 16;
    if (steps < 2) steps = 2;

    touch_input_inject((uint16_t)x1, (uint16_t)y1, true, (uint32_t)ms + 300);
    for (int i = 1; i <= steps; i++) {
        int xi = x1 + (x2 - x1) * i / steps;
        int yi = y1 + (y2 - y1) * i / steps;
        touch_input_inject((uint16_t)xi, (uint16_t)yi, true, (uint32_t)ms + 300);
        vTaskDelay(pdMS_TO_TICKS(16));
    }
    touch_input_inject((uint16_t)x2, (uint16_t)y2, false, 300);
    vTaskDelay(pdMS_TO_TICKS(80));
    touch_input_inject_clear();
    s_last_x = (uint16_t)x2; s_last_y = (uint16_t)y2;
    printf("OK swipe %d %d -> %d %d %dms\n", x1, y1, x2, y2, ms);
    return 0;
}

static int cmd_touchdown(int argc, char **argv)
{
    if (argc < 3) { printf("usage: touchdown <x> <y>\n"); return 1; }
    s_last_x = (uint16_t)clampi(atoi(argv[1]), 0, SHOT_W - 1);
    s_last_y = (uint16_t)clampi(atoi(argv[2]), 0, SHOT_H - 1);
    /* Long hold so the host can pace the next move/up itself. */
    touch_input_inject(s_last_x, s_last_y, true, 2000);
    printf("OK down %u %u\n", s_last_x, s_last_y);
    return 0;
}

static int cmd_touchmove(int argc, char **argv)
{
    if (argc < 3) { printf("usage: touchmove <x> <y>\n"); return 1; }
    s_last_x = (uint16_t)clampi(atoi(argv[1]), 0, SHOT_W - 1);
    s_last_y = (uint16_t)clampi(atoi(argv[2]), 0, SHOT_H - 1);
    touch_input_inject(s_last_x, s_last_y, true, 2000);
    printf("OK move %u %u\n", s_last_x, s_last_y);
    return 0;
}

static int cmd_touchup(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* Release at the last point; the override expires shortly after, handing
     * control back to the real GT911. */
    touch_input_inject(s_last_x, s_last_y, false, 300);
    printf("OK up %u %u\n", s_last_x, s_last_y);
    return 0;
}

static void register_cmds(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "screenshot",
          .help = "Capture lv_scr_act() and stream it back (base64-framed). "
                  "[--fmt jpeg|rgb565] [--quality N]",
          .hint = NULL, .func = cmd_screenshot },
        { .command = "tap",   .help = "Tap at <x> <y> (0..799 x 0..479)",
          .hint = NULL, .func = cmd_tap },
        { .command = "swipe", .help = "Swipe <x1> <y1> <x2> <y2> [ms]",
          .hint = NULL, .func = cmd_swipe },
        { .command = "touchdown", .help = "Press at <x> <y> (held)",
          .hint = NULL, .func = cmd_touchdown },
        { .command = "touchmove", .help = "Move held press to <x> <y>",
          .hint = NULL, .func = cmd_touchmove },
        { .command = "touchup",   .help = "Release the held press",
          .hint = NULL, .func = cmd_touchup },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}

esp_err_t debug_uart_bridge_init(void)
{
    if (s_inited) return ESP_ERR_INVALID_STATE;

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "p4>";
    repl_cfg.task_stack_size = 8192;   /* room for base64 line + handlers */
    repl_cfg.max_cmdline_length = 256;

    esp_console_dev_uart_config_t uart_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_uart(&uart_cfg, &repl_cfg, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "repl init failed: %s", esp_err_to_name(err));
        return err;
    }

    register_cmds();

    /* Host is a script, not a human: force dumb mode so linenoise doesn't emit
     * cursor-probe escape sequences and command output stays deterministic. */
    linenoiseSetDumbMode(1);

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "repl start failed: %s", esp_err_to_name(err));
        return err;
    }

    s_inited = true;
    ESP_LOGI(TAG, "UART debug bridge up (screenshot + touch injection)");
    return ESP_OK;
}

#else  /* !CONFIG_DEBUG_UART_BRIDGE */

esp_err_t debug_uart_bridge_init(void)
{
    return ESP_OK;
}

#endif /* CONFIG_DEBUG_UART_BRIDGE */
