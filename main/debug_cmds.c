#include "debug_cmds.h"

#include "sdkconfig.h"

/* The commands are only compiled into a build that can actually reach one of
 * the bridges; otherwise both headers expand to stubs and nothing here exists. */
#if CONFIG_DEBUG_UART_BRIDGE || CONFIG_DEBUG_WIFI_BRIDGE

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_cache.h"
#include "esp_rom_crc.h"
#include "mbedtls/base64.h"
#if CONFIG_ESP_CONSOLE_UART
#include "driver/uart.h"
#endif

#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "esp_timer.h"
#include "img_jpeg.h"
#include "notif_bridge.h"
#include "touch_input.h"
#include "ui_mode.h"
#if CONFIG_AA_MIC_ENABLE
#include "mic_capture.h"
#endif
/* The navigator screen (phone map tiles over BLE) is built for the 800x480
 * P4 head units — its framebuffers, JPEG path and mode switch all live behind
 * CONFIG_AA_ENABLE, and so do the console commands that poke at it. */
#if CONFIG_AA_ENABLE
#include "ble_nav.h"
#include "nav_map.h"
#include "nav_screen.h"
#include "nav_route.h"
#include "nav_tiles.h"
#include "nav_tile_store.h"
#include "nav_vector.h"
#endif
/* LVGL internals: the timer list, for the lvtimers dump below. */
#include "misc/lv_gc.h"
#include "misc/lv_ll.h"

static const char *TAG = "dbg";

/* lv_scr_act() is the whole screen (800x480 landscape on the P4 head units,
 * 480x480 on the S3 board). The snapshot buffer is sized for it from the BSP's
 * panel dimensions — the product is the same whichever axis the panel scans —
 * and a smaller active object just fills less of it: the wire header and the
 * encode take their size from the LVGL descriptor, not from these constants. */
#define SHOT_BUF_SIZE     (BSP_LCD_H_RES * BSP_LCD_V_RES * 2)   /* RGB565 */
/* Where a tap may land. The panel as the rider sees it, which on the P4 head
 * units is the BSP's dimensions swapped: the display is scanned portrait and
 * rotated in the adapter, so BSP_LCD_H_RES is the short axis there. Taken from
 * LVGL rather than the BSP for that reason — it is the one that knows which
 * way round the picture ended up. */
#define SHOT_W            ((int)lv_disp_get_hor_res(NULL))
#define SHOT_H            ((int)lv_disp_get_ver_res(NULL))
#define JPEG_OUT_CAP      (384 * 1024)            /* worst-case q100 fits easily */
#define CHUNK_RAW         768                     /* 768 raw -> exactly 1024 b64 */
#define B64_LINE_CAP      1025                    /* 1024 + NUL */
#define TEXT_LINE_CAP     512

/* One quality knob shared by both transports; the Kconfig option came with the
 * serial bridge, the Wi-Fi bridge reuses it when that one is off. */
#ifdef CONFIG_DEBUG_UART_BRIDGE_JPEG_QUALITY
#define SHOT_JPEG_QUALITY CONFIG_DEBUG_UART_BRIDGE_JPEG_QUALITY
#else
#define SHOT_JPEG_QUALITY 80
#endif

/* Lazily-allocated (first screenshot) so an idle debug build doesn't hold ~1 MB
 * PSRAM. The JPEG engine is shared with no one else here (the album-art decoder
 * in music_info_view.c uses a separate decoder engine). */
static uint8_t              *s_shot_buf;          /* RGB565 snapshot target */
static uint8_t              *s_jpeg_out;          /* JPEG bitstream out */
static size_t                s_jpeg_cap;

/* Last injected point, so `touchup` can release at where the finger was. */
static uint16_t s_last_x, s_last_y;

/* Output sink of the command currently running; NULL = the serial console, i.e.
 * plain stdout. Set under s_lock, which serialises the two transports. */
static const debug_out_t *s_out;
static SemaphoreHandle_t  s_lock;

/* ---- output plumbing ---- */

static void emit(const char *fmt, ...)
{
    char buf[TEXT_LINE_CAP];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    size_t len = (size_t)n < sizeof(buf) ? (size_t)n : sizeof(buf) - 1;
    if (s_out && s_out->text) {
        s_out->text(s_out->ctx, buf, len);
    } else {
        fwrite(buf, 1, len, stdout);
    }
}

/* Console fallback for a captured image: base64-framed blocks on the UART, the
 * wire format scripts/uart_debug.py parses. Logging is muted for the whole
 * frame so no ESP_LOG line splices the payload; the host additionally ignores
 * any non-"SCR-" line and verifies len + crc32. */
static void console_image(const char *fmt, int w, int h,
                          const uint8_t *data, size_t len)
{
    /* The ROM API already handles the CRC complements, as zlib does.
     * Complementing it again reported false corruption on every capture. */
    uint32_t crc = esp_rom_crc32_le(0, data, (uint32_t)len);
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

/* Hand a captured image to whichever transport is running the command. */
static void out_image(const char *fmt, int w, int h,
                      const uint8_t *data, size_t len)
{
    if (s_out && s_out->image) {
        s_out->image(s_out->ctx, fmt, w, h, data, len);
        return;
    }
    console_image(fmt, w, h, data, len);
}

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

static void do_screenshot(bool jpeg, int quality)
{
    if (!ensure_resources(jpeg)) {
        emit("SCR-ERR nomem\n");
        return;
    }

    lv_img_dsc_t dsc;
    memset(&dsc, 0, sizeof dsc);

    /* lv_snapshot re-renders the object tree into our buffer via LVGL's own
     * draw path — hold the display lock just for that, then release before the
     * (slow) encode + send so we don't starve the renderer. */
    if (bsp_display_lock(1000) != ESP_OK) {
        emit("SCR-ERR busy\n");
        return;
    }
    lv_res_t res = lv_snapshot_take_to_buf(lv_scr_act(), LV_IMG_CF_TRUE_COLOR,
                                           &dsc, s_shot_buf, SHOT_BUF_SIZE);
    bsp_display_unlock();
    if (res != LV_RES_OK) {
        emit("SCR-ERR snapshot\n");
        return;
    }

    int w = (int)dsc.header.w;
    int h = (int)dsc.header.h;
    size_t raw_len = (size_t)w * h * 2;

    if (jpeg && s_jpeg_out) {
        size_t out_len = 0;
        if (img_jpeg_encode_rgb565(s_shot_buf, (uint16_t)w, (uint16_t)h,
                                   quality, s_jpeg_out, s_jpeg_cap, &out_len)) {
            out_image("jpeg", w, h, s_jpeg_out, out_len);
            return;
        }
        ESP_LOGW(TAG, "jpeg encode failed — falling back to raw");
    }

    /* raw RGB565 (little-endian uint16, R in high bits — host unpacks 5-6-5). */
    out_image("rgb565", w, h, s_shot_buf, raw_len);
}

/* ---- commands (manual argv parsing — both hosts are scripts) ---- */

static int cmd_screenshot(int argc, char **argv)
{
    bool jpeg = true;
    int quality = SHOT_JPEG_QUALITY;
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
    if (argc < 3) { emit("usage: tap <x> <y>\n"); return 1; }
    uint16_t x = (uint16_t)clampi(atoi(argv[1]), 0, SHOT_W - 1);
    uint16_t y = (uint16_t)clampi(atoi(argv[2]), 0, SHOT_H - 1);
    s_last_x = x; s_last_y = y;
    /* press -> hold one+ LVGL read period -> release -> let LVGL fire CLICKED */
    touch_input_inject(x, y, true, 300);
    vTaskDelay(pdMS_TO_TICKS(80));
    touch_input_inject(x, y, false, 300);
    vTaskDelay(pdMS_TO_TICKS(80));
    touch_input_inject_clear();
    emit("OK tap %u %u\n", x, y);
    return 0;
}

static int cmd_swipe(int argc, char **argv)
{
    if (argc < 5) { emit("usage: swipe <x1> <y1> <x2> <y2> [ms]\n"); return 1; }
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
    emit("OK swipe %d %d -> %d %d %dms\n", x1, y1, x2, y2, ms);
    return 0;
}

static int cmd_touchdown(int argc, char **argv)
{
    if (argc < 3) { emit("usage: touchdown <x> <y>\n"); return 1; }
    s_last_x = (uint16_t)clampi(atoi(argv[1]), 0, SHOT_W - 1);
    s_last_y = (uint16_t)clampi(atoi(argv[2]), 0, SHOT_H - 1);
    /* Long hold so the host can pace the next move/up itself. */
    touch_input_inject(s_last_x, s_last_y, true, 2000);
    emit("OK down %u %u\n", s_last_x, s_last_y);
    return 0;
}

static int cmd_touchmove(int argc, char **argv)
{
    if (argc < 3) { emit("usage: touchmove <x> <y>\n"); return 1; }
    s_last_x = (uint16_t)clampi(atoi(argv[1]), 0, SHOT_W - 1);
    s_last_y = (uint16_t)clampi(atoi(argv[2]), 0, SHOT_H - 1);
    touch_input_inject(s_last_x, s_last_y, true, 2000);
    emit("OK move %u %u\n", s_last_x, s_last_y);
    return 0;
}

static int cmd_touchup(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* Release at the last point; the override expires shortly after, handing
     * control back to the real GT911. */
    touch_input_inject(s_last_x, s_last_y, false, 300);
    emit("OK up %u %u\n", s_last_x, s_last_y);
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
    emit("ERR: rebuild with FREERTOS_USE_TRACE_FACILITY + GENERATE_RUN_TIME_STATS\n");
    return 1;
#else
    UBaseType_t cap = uxTaskGetNumberOfTasks() + 6;
    TaskStatus_t *a = calloc(cap, sizeof(TaskStatus_t));
    TaskStatus_t *b = calloc(cap, sizeof(TaskStatus_t));
    if (!a || !b) { free(a); free(b); emit("ERR: alloc\n"); return 1; }

    configRUN_TIME_COUNTER_TYPE rt_a = 0, rt_b = 0;
    UBaseType_t na = uxTaskGetSystemState(a, cap, &rt_a);
    vTaskDelay(pdMS_TO_TICKS(1000));
    UBaseType_t nb = uxTaskGetSystemState(b, cap, &rt_b);
    uint32_t win = (uint32_t)(rt_b - rt_a);   /* µs of wall clock (esp_timer) */
    if (!win) win = 1;

    emit("%-18s core prio state hwm(B)  cpu%%/1s\n", "task");
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
        emit("%-18s %4d %4u   %c   %6u  %4u.%u\n",
             b[i].pcTaskName,
             core == tskNO_AFFINITY ? -1 : core,
             (unsigned)b[i].uxCurrentPriority, st,
             (unsigned)b[i].usStackHighWaterMark * (unsigned)sizeof(StackType_t),
             pct10 / 10, pct10 % 10);
    }
    emit("OK tasks window=%uus\n", (unsigned)win);
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
    if (!bsp_display_lock(1000)) { emit("lvtimers: display busy\n"); return 1; }
    uint32_t now = lv_tick_get();
    lv_timer_t *t = _lv_ll_get_head(&LV_GC_ROOT(_lv_timer_ll));
    int n = 0;
    uint32_t soonest = UINT32_MAX;
    while (t) {
        uint32_t elapsed = now - t->last_run;
        int32_t  left    = (int32_t)t->period - (int32_t)elapsed;
        if (!t->paused && left < (int32_t)soonest) soonest = left < 0 ? 0 : (uint32_t)left;
        emit("  cb=%p period=%4u ms  elapsed=%6u  due in %6d ms%s%s\n",
             (void *)t->timer_cb, (unsigned)t->period, (unsigned)elapsed,
             (int)left, t->paused ? "  [paused]" : "",
             t->repeat_count == 0 ?  "  [spent]" : "");
        t = _lv_ll_get_next(&LV_GC_ROOT(_lv_timer_ll), t);
        n++;
    }
    bsp_display_unlock();
    emit("%d timers; soonest due in %u ms -> worker sleeps pdMS_TO_TICKS(%u) = %u tick(s)\n",
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
    /* Single producer (capture task) / single consumer (command task) — the
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
        emit("ERR: mic busy (an AA session has it open)\n");
        return 1;
    }
    static mic_stat_t st;
    memset(&st, 0, sizeof(st));
    esp_err_t err = mic_capture_start(mic_stat_cb, &st);
    if (err != ESP_OK) {
        emit("ERR: mic_capture_start: %s\n", esp_err_to_name(err));
        return 1;
    }
    emit("capturing %d s\n", secs);
    for (int s = 1; s <= secs; s++) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        uint64_t sq = st.sq;
        uint32_t n = st.samples;
        int32_t  peak = st.peak;
        st.sq = 0; st.samples = 0; st.peak = 0;
        unsigned rms = n ? (unsigned)sqrtf((float)sq / (float)n) : 0;
        emit("t=%2ds chunks=%u rms=%u peak=%d%s\n", s, (unsigned)st.chunks, rms, (int)peak,
             n == 0 ? "  <-- no data from the codec" : "");
    }
    mic_capture_stop();
    emit("done: %u chunks in %d s (expect %d/s)\n",
         (unsigned)st.chunks, secs, 1000 / MIC_CAPTURE_CHUNK_MS);
    return 0;
}
#endif /* CONFIG_AA_MIC_ENABLE */

/* The 3-finger hold can't be injected through the touch override (it needs
 * three simultaneous contacts the GT911 shim doesn't fake), so the bridge gets
 * its own way into every full-screen mode. */
static int cmd_uimode(int argc, char **argv)
{
    if (argc > 1) {
        if      (strcmp(argv[1], "vesc") == 0)   ui_mode_set(UI_MODE_VESC);
        else if (strcmp(argv[1], "aa") == 0)     ui_mode_set(UI_MODE_AA);
        else if (strcmp(argv[1], "nav") == 0)    ui_mode_set(UI_MODE_NAV);
        else if (strcmp(argv[1], "toggle") == 0) ui_mode_toggle();
        else { emit("ERR: expected vesc|aa|nav|toggle\n"); return 1; }
    }
    emit("mode=%s\n", ui_mode_name(ui_mode_get()));
    return 0;
}

#if CONFIG_AA_ENABLE
/* What a heading-up map would cost.
 *
 * The panel has no hardware for an arbitrary-angle rotation (the PPA turns in
 * 90-degree steps only), so track-up means warping every one of the 384000
 * pixels from one PSRAM buffer into another each frame. This measures exactly
 * that, in the block size a real implementation would use, so the decision is
 * a number rather than an opinion: `navwarp [angle] [block]`.
 */
static int cmd_navwarp(int argc, char **argv)
{
    const int deg = (argc > 1) ? clampi(atoi(argv[1]), 0, 359) : 30;
    const int blk = (argc > 2) ? clampi(atoi(argv[2]), 1, 64) : 8;
    uint16_t *dst = nav_screen_back_buffer();
    if (!dst) {
        emit("ERR: no back buffer (navigator screen not up?)\n");
        return 1;
    }
    /* Source is the same buffer read at a rotated offset — the access pattern
     * is what costs, and it is identical to a real source buffer. */
    const uint16_t *src = dst;
    const int w = nav_screen_width(), h = nav_screen_height();
    const float rad = (float)deg * 0.0174532925f;
    const int32_t cos_q = (int32_t)lroundf(cosf(rad) * 65536.0f);
    const int32_t sin_q = (int32_t)lroundf(sinf(rad) * 65536.0f);
    const int64_t t0 = esp_timer_get_time();
    for (int by = 0; by < h; by += blk) {
        for (int bx = 0; bx < w; bx += blk) {
            /* Source position of this block's corner, stepped per pixel. */
            const int dx0 = bx - w / 2, dy0 = by - h / 2;
            int32_t sx = (int32_t)(w / 2 * 65536) + dx0 * cos_q - dy0 * sin_q;
            int32_t sy = (int32_t)(h / 2 * 65536) + dx0 * sin_q + dy0 * cos_q;
            for (int y = 0; y < blk && by + y < h; y++) {
                int32_t px = sx, py = sy;
                uint16_t *drow = dst + (size_t)(by + y) * w;
                for (int x = 0; x < blk && bx + x < w; x++) {
                    const int ix = px >> 16, iy = py >> 16;
                    if (ix >= 0 && ix < w && iy >= 0 && iy < h) {
                        drow[bx + x] = src[(size_t)iy * w + ix];
                    }
                    px += cos_q;
                    py += sin_q;
                }
                sx -= sin_q;
                sy += cos_q;
            }
        }
    }
    const int64_t us = esp_timer_get_time() - t0;
    emit("warp %d deg, %dx%d block: %lld us (%lld ms) for %d px\n",
         deg, blk, blk, (long long)us, (long long)(us / 1000), w * h);
    return 0;
}

static int cmd_navheading(int argc, char **argv)
{
    if (argc == 2) {
        if (strcmp(argv[1], "auto") == 0) {
            nav_map_set_heading_override(-1);
        } else {
            char *end;
            const long angle = strtol(argv[1], &end, 10);
            if (end == argv[1] || *end || angle < 0 || angle >= 360) {
                emit("Usage: navheading [auto|0..359]\n");
                return 1;
            }
            nav_map_set_heading_override((int)angle);
        }
    }
    emit("camera heading %.1f deg\n", nav_map_camera_heading());
    return 0;
}

/* Rasterize one NVT1 tile from the card straight into the panel buffer and
 * time it. This is the bench command for the vector path: it skips BLE, the
 * compositor and the cache, so the number is the tile cost itself. */
static int cmd_navvec(int argc, char **argv)
{
    if (argc < 4) {
        emit("Usage: navvec <z> <x> <y>\n");
        return 1;
    }
    const int z = atoi(argv[1]), x = atoi(argv[2]), y = atoi(argv[3]);
    if (z < 0 || z > 22 || x < 0 || y < 0) return 1;
    uint8_t fmt = 0;
    size_t len = 0;
    uint8_t *data = nav_tile_store_read((uint8_t)z, (uint32_t)x, (uint32_t)y,
                                        &fmt, &len);
    if (!data) {
        emit("no tile %d/%d/%d on the card\n", z, x, y);
        return 1;
    }
    emit("tile %d/%d/%d: %u bytes, format %u, labels %d\n", z, x, y,
         (unsigned)len, fmt, nav_vector_label_count(data, len));
    if (fmt != NAV_TILE_FMT_VECTOR) {
        emit("not a vector tile\n");
        heap_caps_free(data);
        return 1;
    }

    uint16_t *dst = nav_screen_back_buffer();
    if (!dst) {
        emit("no back buffer (navigator screen not up?)\n");
        heap_caps_free(data);
        return 1;
    }
    nav_vector_label_t labels[NAV_TILE_LABELS];
    uint8_t nlabels = 0;
    const int64_t t0 = esp_timer_get_time();
    const bool ok = nav_vector_rasterize(data, len, dst, labels, &nlabels,
                                         NAV_TILE_LABELS);
    const int64_t t1 = esp_timer_get_time();
    heap_caps_free(data);
    if (!ok) {
        emit("rasterize failed\n");
        return 1;
    }
    esp_cache_msync(dst, nav_screen_back_buffer_bytes(),
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    nav_screen_commit();
    emit("rasterized in %u us, %u label(s)\n",
         (unsigned)(t1 - t0), (unsigned)nlabels);
    for (uint8_t i = 0; i < nlabels; i++) {
        emit("  [%u] %s (%u,%u) kind=%u rank=%u\n", i, labels[i].name,
             labels[i].x, labels[i].y, labels[i].kind, labels[i].rank);
    }
    return 0;
}

static int cmd_navstat(int argc, char **argv)
{
    (void)argc; (void)argv;
    ble_nav_stats_t st;
    ble_nav_get_stats(&st);
    const int64_t last = nav_screen_last_frame_us();
    const int64_t age_ms = last ? (esp_timer_get_time() - last) / 1000 : -1;
    emit("mode=%s live=%d streaming=%d\n", ui_mode_name(ui_mode_get()),
         nav_screen_active() ? 1 : 0, st.streaming ? 1 : 0);
    emit("frames shown=%u last_shown=%lld ms ago\n",
         (unsigned)nav_screen_frames_shown(), (long long)age_ms);
    nav_tiles_stats_t ts;
    nav_tiles_get_stats(&ts);
    nav_map_view_t v;
    nav_map_get_view(&v);
    emit("worker stack free=%u B | nimble_host free=%u words\n",
         (unsigned)st.stack_free,
         (unsigned)notif_bridge_host_stack_free());
    emit("tiles ok=%u failed=%u stored=%u/%u evicted=%u last=%u B in %u ms\n",
         (unsigned)st.tiles_ok, (unsigned)st.tiles_failed,
         (unsigned)ts.stored, (unsigned)ts.capacity, (unsigned)ts.evicted,
         (unsigned)st.tile_last_bytes, (unsigned)st.tile_last_ms);
    emit("vector ok=%u last=%u ms labels=%d (last tile %u)\n",
         (unsigned)st.vector_ok, (unsigned)st.vector_last_ms,
         st.last_labels, (unsigned)ts.labels_last);
    nav_tile_store_stats_t ss;
    nav_tile_store_get_stats(&ss);
    emit("sd cache %s saved=%u loaded=%u missing=%u errors=%u vector=%s | "
         "evictions not re-asked=%u\n",
         ss.ready ? "on" : "no card",
         (unsigned)ss.saved, (unsigned)ss.loaded, (unsigned)ss.missing,
         (unsigned)ss.errors, ss.vector ? "yes" : "no",
         (unsigned)ts.evicted_quiet);
    emit("frame: compose %u us + cache flush %u us\n",
         (unsigned)st.compose_us, (unsigned)st.msync_us);
    emit("route %u points, last draw %u us\n",
         (unsigned)nav_route_count(), (unsigned)nav_map_last_route_us());
    emit("view %s %s%ld.%07ld,%s%ld.%07ld z%u hdg=%u | views=%u renders=%u "
         "last=%u ms tiles %d/%d\n",
         v.valid ? "set" : "unset",
         v.lat_e7 < 0 ? "-" : "", (long)labs(v.lat_e7) / 10000000,
         (long)labs(v.lat_e7) % 10000000,
         v.lon_e7 < 0 ? "-" : "", (long)labs(v.lon_e7) / 10000000,
         (long)labs(v.lon_e7) % 10000000,
         (unsigned)v.zoom,
         (unsigned)v.heading_deg, (unsigned)st.views, (unsigned)st.renders,
         (unsigned)st.render_last_ms, st.last_have, st.last_wanted);
    return 0;
}

#endif /* CONFIG_AA_ENABLE — navigator commands */

/* ---- table + dispatch ---- */

typedef struct {
    const char *name;
    const char *help;
    int (*func)(int argc, char **argv);
} debug_cmd_t;

static const debug_cmd_t s_cmds[] = {
    { "screenshot", "Capture lv_scr_act() and stream it back "
                    "[--fmt jpeg|rgb565] [--quality N]", cmd_screenshot },
    { "tap",        "Tap at <x> <y> (screen coords)", cmd_tap },
    { "swipe",      "Swipe <x1> <y1> <x2> <y2> [ms]", cmd_swipe },
    { "touchdown",  "Press at <x> <y> (held)", cmd_touchdown },
    { "touchmove",  "Move held press to <x> <y>", cmd_touchmove },
    { "touchup",    "Release the held press", cmd_touchup },
    { "lvtimers",   "List LVGL timers with period and time until due", cmd_lvtimers },
    { "tasks",      "Per-task CPU% over a 1 s window + prio/core/stack HWM", cmd_tasks },
    { "uimode",     "Show or set the full-screen mode: vesc|aa|nav|toggle", cmd_uimode },
#if CONFIG_AA_ENABLE
    { "navwarp",    "Time a full-screen rotated copy, as a heading-up map would "
                    "need every frame [degrees] [block]", cmd_navwarp },
    { "navstat",    "Navigator frame stream: mode, frames, last frame size/time", cmd_navstat },
    { "navvec",     "Rasterize one NVT1 tile from the card and time it <z> <x> <y>", cmd_navvec },
    { "navheading", "Navigator camera heading [auto|0..359]", cmd_navheading },
#endif
#if CONFIG_AA_MIC_ENABLE
    { "mic",        "Capture the AA microphone for [seconds] (default 5) without a "
                    "phone; prints RMS/peak per second", cmd_mic },
#endif
};

#define CMD_COUNT (sizeof(s_cmds) / sizeof(s_cmds[0]))

void debug_cmds_init(void)
{
    if (!s_lock) s_lock = xSemaphoreCreateRecursiveMutex();
}

size_t debug_cmds_count(void) { return CMD_COUNT; }

const char *debug_cmds_name(size_t i)
{
    return i < CMD_COUNT ? s_cmds[i].name : NULL;
}

const char *debug_cmds_help(size_t i)
{
    return i < CMD_COUNT ? s_cmds[i].help : NULL;
}

static void print_help(void)
{
    emit("commands:\n");
    for (size_t i = 0; i < CMD_COUNT; i++) {
        emit("  %-11s %s\n", s_cmds[i].name, s_cmds[i].help);
    }
}

int debug_cmds_run(int argc, char **argv, const debug_out_t *out)
{
    if (argc < 1 || !argv || !argv[0]) return -1;
    if (!s_lock) debug_cmds_init();
    if (!s_lock) return -1;

    xSemaphoreTakeRecursive(s_lock, portMAX_DELAY);
    const debug_out_t *prev = s_out;
    s_out = out;

    int rc = -1;
    if (strcmp(argv[0], "help") == 0) {
        print_help();
        rc = 0;
    } else {
        for (size_t i = 0; i < CMD_COUNT; i++) {
            if (strcmp(argv[0], s_cmds[i].name) == 0) {
                rc = s_cmds[i].func(argc, argv);
                break;
            }
        }
        if (rc == -1) {
            emit("ERR: unknown command '%s' (try 'help')\n", argv[0]);
        }
    }

    s_out = prev;
    xSemaphoreGiveRecursive(s_lock);
    return rc;
}

#else  /* neither bridge */

void debug_cmds_init(void) {}
int debug_cmds_run(int argc, char **argv, const debug_out_t *out)
{
    (void)argc; (void)argv; (void)out;
    return -1;
}
size_t debug_cmds_count(void) { return 0; }
const char *debug_cmds_name(size_t i) { (void)i; return NULL; }
const char *debug_cmds_help(size_t i) { (void)i; return NULL; }

#endif
