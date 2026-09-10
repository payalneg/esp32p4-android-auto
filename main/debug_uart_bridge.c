#include "debug_uart_bridge.h"

#include "sdkconfig.h"

#if CONFIG_DEBUG_UART_BRIDGE

#include <math.h>
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
#if CONFIG_ESP_CONSOLE_UART
#include "driver/uart.h"
#endif

#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "img_jpeg.h"
#include "touch_input.h"
#if CONFIG_AA_MIC_ENABLE
#include "mic_capture.h"
#endif
/* LVGL internals: the timer list, for the lvtimers dump below. */
#include "misc/lv_gc.h"
#include "misc/lv_ll.h"

static const char *TAG = "dbg_uart";

/* lv_scr_act() is the whole screen (800x480 landscape on the P4 head units,
 * 480x480 on the S3 board). The snapshot buffer is sized for it from the BSP's
 * panel dimensions — the product is the same whichever axis the panel scans —
 * and a smaller active object just fills less of it: the wire header and the
 * encode take their size from the LVGL descriptor, not from these constants. */
#define SHOT_BUF_SIZE     (BSP_LCD_H_RES * BSP_LCD_V_RES * 2)   /* RGB565 */
#define JPEG_OUT_CAP      (384 * 1024)            /* worst-case q100 fits easily */
#define CHUNK_RAW         768                     /* 768 raw -> exactly 1024 b64 */
#define B64_LINE_CAP      1025                    /* 1024 + NUL */

/* Lazily-allocated (first screenshot) so an idle debug build doesn't hold ~1 MB
 * PSRAM. The JPEG engine is shared with no one else here (the album-art decoder
 * in music_info_view.c uses a separate decoder engine). */
static uint8_t              *s_shot_buf;          /* RGB565 snapshot target */
static uint8_t              *s_jpeg_out;          /* JPEG bitstream out */
static size_t                s_jpeg_cap;

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
        /* PSRAM with the alignment/caps this chip's JPEG backend wants to read
         * from (DMA-capable on the P4, 16-byte aligned on the S3). */
        s_shot_buf = img_jpeg_alloc_rgb565(SHOT_BUF_SIZE);
        if (!s_shot_buf) {
            ESP_LOGE(TAG, "no PSRAM for %d-byte snapshot buffer", SHOT_BUF_SIZE);
            return false;
        }
    }
    if (need_jpeg && !s_jpeg_out) {
        s_jpeg_out = img_jpeg_alloc_encoded(JPEG_OUT_CAP);
        s_jpeg_cap = s_jpeg_out ? JPEG_OUT_CAP : 0;   /* raw mode still works */
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
#if CONFIG_ESP_CONSOLE_UART
    /* Drain the UART TX FIFO/ring before re-enabling logs so a queued log line
     * can't tail-splice into the last chunk. The REPL installed this driver.
     * The USB-Serial-JTAG console (S3 board) has no such driver — fflush plus
     * the host-side framing checks cover it there. */
    uart_wait_tx_done(CONFIG_ESP_CONSOLE_UART_NUM, pdMS_TO_TICKS(3000));
#endif
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

    if (jpeg && s_jpeg_out) {
        size_t out_len = 0;
        if (img_jpeg_encode_rgb565(s_shot_buf, (uint16_t)w, (uint16_t)h,
                                   quality, s_jpeg_out, s_jpeg_cap, &out_len)) {
            send_frame("jpeg", w, h, s_jpeg_out, out_len);
            return;
        }
        ESP_LOGW(TAG, "jpeg encode failed — falling back to raw");
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

/* Per-task CPU usage over a 1 s window (two uxTaskGetSystemState snapshots,
 * delta of ulRunTimeCounter). Needs CONFIG_FREERTOS_USE_TRACE_FACILITY +
 * CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS (diagnostic build). Percentages are
 * of wall-clock time — two cores means the column can total ~200%. */
static int cmd_tasks(int argc, char **argv)
{
    (void)argc; (void)argv;
#if !(configUSE_TRACE_FACILITY && configGENERATE_RUN_TIME_STATS)
    printf("ERR: rebuild with FREERTOS_USE_TRACE_FACILITY + GENERATE_RUN_TIME_STATS\n");
    return 1;
#else
    UBaseType_t cap = uxTaskGetNumberOfTasks() + 6;
    TaskStatus_t *a = calloc(cap, sizeof(TaskStatus_t));
    TaskStatus_t *b = calloc(cap, sizeof(TaskStatus_t));
    if (!a || !b) { free(a); free(b); printf("ERR: alloc\n"); return 1; }

    configRUN_TIME_COUNTER_TYPE rt_a = 0, rt_b = 0;
    UBaseType_t na = uxTaskGetSystemState(a, cap, &rt_a);
    vTaskDelay(pdMS_TO_TICKS(1000));
    UBaseType_t nb = uxTaskGetSystemState(b, cap, &rt_b);
    uint32_t win = (uint32_t)(rt_b - rt_a);   /* µs of wall clock (esp_timer) */
    if (!win) win = 1;

    printf("%-18s core prio state hwm(B)  cpu%%/1s\n", "task");
    for (UBaseType_t i = 0; i < nb; i++) {
        uint32_t prev = 0;
        for (UBaseType_t j = 0; j < na; j++) {
            if (a[j].xTaskNumber == b[i].xTaskNumber) {
                prev = a[j].ulRunTimeCounter;
                break;
            }
        }
        uint32_t d = b[i].ulRunTimeCounter - prev;
        unsigned pct10 = (unsigned)((uint64_t)d * 1000 / win);
        char st = '?';
        switch (b[i].eCurrentState) {
        case eRunning:   st = 'X'; break;
        case eReady:     st = 'R'; break;
        case eBlocked:   st = 'B'; break;
        case eSuspended: st = 'S'; break;
        case eDeleted:   st = 'D'; break;
        default: break;
        }
        int core = (int)xTaskGetCoreID(b[i].xHandle);
        printf("%-18s %4d %4u   %c   %6u  %4u.%u\n",
               b[i].pcTaskName,
               core == tskNO_AFFINITY ? -1 : core,
               (unsigned)b[i].uxCurrentPriority, st,
               (unsigned)b[i].usStackHighWaterMark * (unsigned)sizeof(StackType_t),
               pct10 / 10, pct10 % 10);
    }
    printf("OK tasks window=%uus\n", (unsigned)win);
    free(a);
    free(b);
    return 0;
#endif
}

/* Dump every LVGL timer with its period and how overdue it is. The LVGL
 * worker calls lv_timer_handler() in a loop and sleeps for whatever it
 * returns — the time until the next timer is due — so a timer that is always
 * due keeps the task spinning. At CONFIG_FREERTOS_HZ=100 anything under 10 ms
 * rounds to vTaskDelay(0), which does not block at all, and every task on
 * core 0 below the worker's priority 6 stops running. This says which timer
 * it is; addr2line the callback against the elf for a name. */
static int cmd_lvtimers(int argc, char **argv)
{
    (void)argc; (void)argv;
    if (!bsp_display_lock(1000)) { printf("lvtimers: display busy\n"); return 1; }
    uint32_t now = lv_tick_get();
    lv_timer_t *t = _lv_ll_get_head(&LV_GC_ROOT(_lv_timer_ll));
    int n = 0;
    uint32_t soonest = UINT32_MAX;
    while (t) {
        uint32_t elapsed = now - t->last_run;
        int32_t  left    = (int32_t)t->period - (int32_t)elapsed;
        if (!t->paused && left < (int32_t)soonest) soonest = left < 0 ? 0 : (uint32_t)left;
        printf("  cb=%p period=%4u ms  elapsed=%6u  due in %6d ms%s%s\n",
               (void *)t->timer_cb, (unsigned)t->period, (unsigned)elapsed,
               (int)left, t->paused ? "  [paused]" : "",
               t->repeat_count == 0 ? "  [spent]" : "");
        t = _lv_ll_get_next(&LV_GC_ROOT(_lv_timer_ll), t);
        n++;
    }
    bsp_display_unlock();
    printf("%d timers; soonest due in %u ms -> worker sleeps pdMS_TO_TICKS(%u) = %u tick(s)\n",
           n, (unsigned)soonest, (unsigned)soonest,
           (unsigned)pdMS_TO_TICKS(soonest > 15 ? 15 : (soonest < 1 ? 1 : soonest)));
    return 0;
}

/* ---- mic: run the AA microphone capture without a phone ---- */
#if CONFIG_AA_MIC_ENABLE

typedef struct {
    uint64_t sq;        /* sum of squares, current 1 s window */
    int32_t  peak;      /* |max| sample, current window */
    uint32_t samples;   /* samples in the window */
    uint32_t chunks;    /* total chunks delivered */
} mic_stat_t;

static void mic_stat_cb(const int16_t *pcm, size_t n, uint64_t ts, void *ctx)
{
    (void)ts;
    mic_stat_t *st = (mic_stat_t *)ctx;
    uint64_t sq = 0;
    int32_t  peak = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t v = pcm[i];
        sq += (uint64_t)(v * v);
        if (v < 0) v = -v;
        if (v > peak) peak = v;
    }
    /* Single producer (capture task) / single consumer (REPL task) — the
     * numbers are diagnostic, a torn read costs one slightly-off line. */
    st->sq += sq;
    if (peak > st->peak) st->peak = peak;
    st->samples += (uint32_t)n;
    st->chunks++;
}

static int cmd_mic(int argc, char **argv)
{
    int secs = argc > 1 ? clampi(atoi(argv[1]), 1, 60) : 5;
    if (mic_capture_is_running()) {
        printf("ERR: mic busy (an AA session has it open)\n");
        return 1;
    }
    static mic_stat_t st;
    memset(&st, 0, sizeof(st));
    esp_err_t err = mic_capture_start(mic_stat_cb, &st);
    if (err != ESP_OK) {
        printf("ERR: mic_capture_start: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("capturing %d s\n", secs);
    for (int s = 1; s <= secs; s++) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        uint64_t sq = st.sq;
        uint32_t n = st.samples;
        int32_t  peak = st.peak;
        st.sq = 0; st.samples = 0; st.peak = 0;
        unsigned rms = n ? (unsigned)sqrt((double)sq / n) : 0;
        printf("t=%2ds chunks=%u rms=%u peak=%d%s\n", s, (unsigned)st.chunks, rms, (int)peak,
               n == 0 ? "  <-- no data from the codec" : "");
    }
    mic_capture_stop();
    printf("done: %u chunks in %d s (expect %d/s)\n",
           (unsigned)st.chunks, secs, 1000 / MIC_CAPTURE_CHUNK_MS);
    return 0;
}
#endif /* CONFIG_AA_MIC_ENABLE */

static void register_cmds(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "screenshot",
          .help = "Capture lv_scr_act() and stream it back (base64-framed). "
                  "[--fmt jpeg|rgb565] [--quality N]",
          .hint = NULL, .func = cmd_screenshot },
        { .command = "tap",   .help = "Tap at <x> <y> (screen coords)",
          .hint = NULL, .func = cmd_tap },
        { .command = "swipe", .help = "Swipe <x1> <y1> <x2> <y2> [ms]",
          .hint = NULL, .func = cmd_swipe },
        { .command = "touchdown", .help = "Press at <x> <y> (held)",
          .hint = NULL, .func = cmd_touchdown },
        { .command = "touchmove", .help = "Move held press to <x> <y>",
          .hint = NULL, .func = cmd_touchmove },
        { .command = "touchup",   .help = "Release the held press",
          .hint = NULL, .func = cmd_touchup },
        { .command = "lvtimers",
          .help = "List LVGL timers with period and time until due",
          .hint = NULL, .func = cmd_lvtimers },
        { .command = "tasks",
          .help = "Per-task CPU%% over a 1 s window + prio/core/stack HWM",
          .hint = NULL, .func = cmd_tasks },
#if CONFIG_AA_MIC_ENABLE
        { .command = "mic",
          .help = "Capture the AA microphone for [seconds] (default 5) without a "
                  "phone; prints RMS/peak per second",
          .hint = NULL, .func = cmd_mic },
#endif
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

    /* Whichever console this board has. The P4 head units talk over UART0
     * (a bridge chip on the USB port); the S3 board's USB-C is the chip's own
     * USB peripheral, so its console is USB-Serial-JTAG and scripts/uart_debug.py
     * opens /dev/cu.usbmodem* instead of a USB-serial adapter. */
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t dev_cfg =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&dev_cfg, &repl_cfg, &repl);
#else
    esp_console_dev_uart_config_t dev_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_uart(&dev_cfg, &repl_cfg, &repl);
#endif
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
    ESP_LOGI(TAG, "serial debug bridge up (screenshot + touch injection)");
    return ESP_OK;
}

#else  /* !CONFIG_DEBUG_UART_BRIDGE */

esp_err_t debug_uart_bridge_init(void)
{
    return ESP_OK;
}

#endif /* CONFIG_DEBUG_UART_BRIDGE */
