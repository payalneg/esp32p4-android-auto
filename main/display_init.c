#include "display_init.h"

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "display_init";

static lv_display_t *s_display;

/* ---- Render-performance instrumentation ------------------------------------
 * Hooked onto the LVGL driver's monitor_cb. lv_refr.c calls it once per
 * *actual* refresh (only when something was invalidated), passing the elapsed
 * render+flush-wait time and the pixel count. We aggregate over a 1 s window
 * and emit a single line so the serial log shows the real frame rate, the
 * per-frame cost, and how much of each frame is full-screen (px≈384000 in the
 * DOUBLE_FULL/full_refresh path).
 *
 * Flip DISPLAY_PERF_LOG to 0 to silence once the cause is found. */
#define DISPLAY_PERF_LOG 1

#if DISPLAY_PERF_LOG
/* monitor_cb only fires when LVGL actually renders (something invalidated), so
 * a render-driven logger goes silent on a static screen — and that's
 * indistinguishable from a hang. Decouple it: monitor_cb just accumulates, and
 * a 1 Hz esp_timer (separate task) emits the line every second regardless. So
 * "render: 0 fps" = idle or hung (still alive), nonzero = rendering. */
static volatile uint32_t s_perf_frames;
static volatile uint32_t s_perf_time_sum_ms;
static volatile uint32_t s_perf_time_max_ms;
static volatile uint64_t s_perf_px_sum;

static void display_perf_monitor_cb(lv_disp_drv_t *drv, uint32_t time_ms, uint32_t px)
{
    (void)drv;
    s_perf_frames++;
    s_perf_time_sum_ms += time_ms;
    if (time_ms > s_perf_time_max_ms) {
        s_perf_time_max_ms = time_ms;
    }
    s_perf_px_sum += px;
}

static void display_perf_timer_cb(void *arg)
{
    (void)arg;
    uint32_t frames = s_perf_frames;
    uint32_t tsum   = s_perf_time_sum_ms;
    uint32_t tmax   = s_perf_time_max_ms;
    uint64_t px     = s_perf_px_sum;
    s_perf_frames      = 0;
    s_perf_time_sum_ms = 0;
    s_perf_time_max_ms = 0;
    s_perf_px_sum      = 0;

    ESP_LOGI(TAG,
             "render: %u fps | avg %ums max %ums | %uk px/frame | busy %u%%",
             (unsigned)frames,
             frames ? (unsigned)(tsum / frames) : 0u,
             (unsigned)tmax,
             frames ? (unsigned)((px / frames) / 1000) : 0u,
             (unsigned)(tsum / 10));   /* tsum ms out of the ~1000 ms window */
}
#endif /* DISPLAY_PERF_LOG */

esp_err_t display_init(void)
{
    if (s_display) {
        return ESP_OK;
    }

    /* Rotate 90° clockwise so the 800×480 panel reads landscape (480 high
     * × 800 wide on the user side).
     *
     * DOUBLE_DIRECT (experiment 2026-06-15): partial rendering. Measured DOUBLE_FULL
     * = ~51 ms per dirty frame because it software-re-renders the WHOLE 384k px
     * dashboard into PSRAM every time anything changes → caps everything at ~19 fps
     * (laggy swipes/scrolls). DIRECT mode makes LVGL redraw only the invalidated
     * region, and the rotate flush (display_bridge_v8_flush_direct_rotate →
     * flush_dirty_copy → rotate_copy_region) rotates ONLY the dirty region via
     * PPA SRM. Crucially it does NOT use the DMA2D front→back copy
     * (copy_unrendered_area_from_front_to_back / portMAX_DELAY) that froze
     * TRIPLE_PARTIAL — only the PPA SRM path, which the Espressif PPA patch
     * (sr_macro_bk_ro_bypass in ppa_srm.c) fixes. So the known freeze cause is
     * avoided. Still needs HW verification for: (a) no freeze, (b) no stale-region
     * ghosting from the 2-framebuffer sync. Revert to ..._DOUBLE_FULL if it
     * freezes or corrupts. */
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_90,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_DIRECT,
    };
    /* Pin the LVGL worker to core 0. The H.264 decoder library spawns a
     * helper task at priority 17 pinned to core 1 (CONFIG_ESP_H264_DUAL_TASK*)
     * and h264_pipe's wrapper task is also pinned to core 1; without this
     * affinity the no-affinity LVGL adapter would migrate onto core 1 and get
     * preempted by the decoder for the entire duration of a frame, blocking
     * the bsp_display_lock for hundreds of ms and stalling vesc_ui_updater. */
    cfg.lv_adapter_cfg.task_core_id = 0;
    s_display = bsp_display_start_with_config(&cfg);
    if (!s_display) {
        ESP_LOGE(TAG, "bsp_display_start failed");
        return ESP_FAIL;
    }

#if DISPLAY_PERF_LOG
    /* Attach the render-time monitor. The adapter sets flush_cb / full_refresh
     * but never touches monitor_cb, and lv_refr.c re-reads driver->monitor_cb
     * every cycle, so setting it post-register is safe. lv_display_t == lv_disp_t
     * in LVGL v8.4. */
    {
        lv_disp_t *d = (lv_disp_t *)s_display;
        if (d && d->driver) {
            d->driver->monitor_cb = display_perf_monitor_cb;

            /* 1 Hz logger, decoupled from rendering — always emits a line so an
             * idle screen ("0 fps") is distinguishable from a hang, and we get
             * steady-state numbers (the render-driven version only logged during
             * boot churn). */
            const esp_timer_create_args_t targs = {
                .callback = display_perf_timer_cb,
                .name     = "disp_perf",
            };
            esp_timer_handle_t th;
            if (esp_timer_create(&targs, &th) == ESP_OK) {
                esp_timer_start_periodic(th, 1000000);
            }
            ESP_LOGI(TAG, "render perf monitor attached (1 Hz logger)");

            /* Confirm where LVGL's software renderer actually writes. On this
             * board the MIPI-DSI framebuffers (and the ROTATE_90 scratch buffer
             * LVGL draws into) live in PSRAM, so every full_refresh sweep is a
             * ~768 KB software write into PSRAM — the prime suspect for the slow
             * full-frame render. This line states it as fact from the device. */
            lv_disp_draw_buf_t *db = d->driver->draw_buf;
            if (db && db->buf1) {
                bool psram = esp_ptr_external_ram(db->buf1);
                ESP_LOGI(TAG,
                         "LVGL draw buf: %s, %u px (%u KB), full_refresh=%d direct=%d buf2=%s",
                         psram ? "PSRAM (slow SW render)" : "internal SRAM",
                         (unsigned)db->size,
                         (unsigned)(db->size * sizeof(lv_color_t) / 1024),
                         (int)d->driver->full_refresh,
                         (int)d->driver->direct_mode,
                         db->buf2 ? "yes(double)" : "no(single)");
            }
        }
    }
#endif

    /* Backlight off while the framebuffer is still uninitialised — that's
     * what causes the 1-2 s of white flash at boot. We turn it on once
     * the first frame has rendered our black background. */
    bsp_display_backlight_off();

    /* Generous timeout — LVGL task may still be holding the lock right
     * after esp_lv_adapter_start() returns. */
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl lock timeout during init");
        return ESP_FAIL;
    }

    /* Paint the LVGL active screen black so later widgets land on black,
     * not LVGL's default white. */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    /* Activate PPA draw acceleration (the .enable_ppa_accel=true we set in the
     * BSP profile). Necessary because the adapter installs the PPA draw_ctx_init
     * AFTER lv_disp_drv_register has already built a plain software draw_ctx, so
     * the PPA blend hook (->blend = lv_draw_ppa_blend) never actually replaces
     * the SW one. Re-running draw_ctx_init on the live ctx installs it — done
     * under the LVGL lock so no render is in flight. If PPA accel were off this
     * is a harmless SW-ctx re-init (draw_ctx_init would still be the SW one). */
    {
        lv_disp_t *d = (lv_disp_t *)s_display;
        if (d && d->driver && d->driver->draw_ctx && d->driver->draw_ctx_init) {
            d->driver->draw_ctx_init(d->driver, d->driver->draw_ctx);
            ESP_LOGI(TAG, "draw_ctx re-init (PPA accel hook install)");
        }
    }

    bsp_display_unlock();

    /* Give LVGL a couple of render cycles to push the black background to
     * the panel before lighting the backlight — partial-rotate flush can
     * take more than one frame to settle. Otherwise we'd briefly unmask
     * whatever stale data was in the framebuffer. */
    vTaskDelay(pdMS_TO_TICKS(200));
    bsp_display_backlight_on();

    return ESP_OK;
}

struct _lv_display_t *display_get(void)
{
    return (struct _lv_display_t *)s_display;
}
