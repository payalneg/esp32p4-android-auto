#include "display_init.h"

#include "bsp/esp-bsp.h"
#include "dev_settings.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "display_init";

static lv_display_t *s_display;

/* Orientation actually applied at bring-up. Latched from NVS once in
 * display_init() and never changed afterwards — the LVGL adapter's rotation
 * is start-time-only, so a toggle mid-session must NOT leak into the touch /
 * video / overlay mappings or they'd flip while the panel doesn't. */
static bool s_flip_active;

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

/* Flush sequence — increments on every actual LVGL refresh. Always compiled:
 * the render watchdog in scr_scrub_timer_cb keys off it. The perf aggregation
 * below stays behind DISPLAY_PERF_LOG. */
static volatile uint32_t s_flush_seq;

#if DISPLAY_PERF_LOG
/* monitor_cb only fires when LVGL actually renders (something invalidated), so
 * a render-driven logger goes silent on a static screen — and that's
 * indistinguishable from a hang. Decouple it: monitor_cb just accumulates, and
 * a periodic esp_timer (separate task) emits the line regardless of rendering.
 *
 * Cadence: 5 s, same as the AA video pipeline's stats (aa_svc rx / h264_pipe
 * dec / display_video show). It was 1 s — one line every second filled the
 * Logs screen's 16 KB tail in ~3 min and buried everything else (field photo
 * 2026-09-03: a phone-side TCP reset with nothing but "render: 0 fps" around
 * it). Idle windows are logged once — the line that marks the moment rendering
 * stopped — and then suppressed until it resumes: the render watchdog below
 * has its own line for "LVGL alive but not refreshing", so a stream of zeros
 * added nothing. While the AA video owns the panel LVGL is paused and this
 * logger is therefore silent by design. */
#define DISPLAY_PERF_PERIOD_MS 5000
static volatile uint32_t s_perf_frames;
static volatile uint32_t s_perf_time_sum_ms;
static volatile uint32_t s_perf_time_max_ms;
static volatile uint64_t s_perf_px_sum;
#endif

static void display_monitor_cb(lv_disp_drv_t *drv, uint32_t time_ms, uint32_t px)
{
    (void)drv;
    s_flush_seq++;
#if DISPLAY_PERF_LOG
    s_perf_frames++;
    s_perf_time_sum_ms += time_ms;
    if (time_ms > s_perf_time_max_ms) {
        s_perf_time_max_ms = time_ms;
    }
    s_perf_px_sum += px;
#else
    (void)time_ms; (void)px;
#endif
}

#if DISPLAY_PERF_LOG
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

    static bool was_idle;
    if (frames == 0) {
        if (was_idle) return;      /* still idle — the transition was logged */
        was_idle = true;
    } else {
        was_idle = false;
    }

    ESP_LOGI(TAG,
             "render %us: %u fr %u.%u fps | avg %u ms max %u ms | %uk px/frame | busy %u%%",
             (unsigned)(DISPLAY_PERF_PERIOD_MS / 1000),
             (unsigned)frames,
             (unsigned)(frames * 1000u / DISPLAY_PERF_PERIOD_MS),
             (unsigned)((frames * 10000u / DISPLAY_PERF_PERIOD_MS) % 10u),
             frames ? (unsigned)(tsum / frames) : 0u,
             (unsigned)tmax,
             frames ? (unsigned)((px / frames) / 1000) : 0u,
             (unsigned)(tsum * 100u / DISPLAY_PERF_PERIOD_MS));  /* render ms out of the window */
}
#endif /* DISPLAY_PERF_LOG */

/* ---- Stale-region scrub after screen transitions ---------------------------
 * DOUBLE_DIRECT keeps two framebuffers and only copies invalidated regions
 * into them. A screen-load *animation* (lv_scr_load_anim) repaints the new
 * screen frame-by-frame; on the frames whose dirty area spans the whole width
 * the adapter's flush takes the SKIP_COPY path and writes only ONE buffer, so
 * the animation's penultimate frame stays behind in the buffer that wasn't its
 * final target. That buffer is hidden until the next partial flush toggles it
 * back onto the panel — which is why the old dashboard ("power") flashed
 * through Statistics about once a second (its live-totals timer ticks at 1 Hz).
 *
 * Fix: watch the active screen and, once a transition has settled (longer than
 * the 200 ms load anim), force ONE full-screen invalidate. A full repaint goes
 * through the adapter's PART_COPY path, which writes the whole screen into BOTH
 * framebuffers in a single flush, so no stale region survives. Costs one extra
 * full frame on a rare, user-initiated transition — imperceptible. Centralised
 * here so it covers every screen switch without touching the call sites. */
#define SCR_SCRUB_PERIOD_MS   50
#define SCR_SCRUB_DELAY_TICKS 6     /* ~300 ms — outlasts the 200 ms screen-load anim */

/* Render watchdog — the "automatic tap". Field bug (v1.3.6): the dashboard
 * occasionally stops refreshing until the user taps the screen; the tap heals
 * it because the press invalidates a widget and the refresh pipeline runs
 * again. Root cause not yet pinned down (this timer running at all proves the
 * LVGL task itself is alive), so heal it the same way the tap does: if not a
 * single refresh happened for RENDER_WDT_TICKS, force one full-screen
 * invalidate. On a genuinely static screen that costs one extra full frame
 * every ~4 s (the forced frame bumps s_flush_seq, so fires alternate) — and
 * the full repaint goes through the PART_COPY path into BOTH framebuffers,
 * which also scrubs any stale-FB ghosting as a side effect. */
#define RENDER_WDT_TICKS 40   /* 40 × 50 ms = 2 s without a refresh */

static void scr_scrub_timer_cb(lv_timer_t *t)
{
    (void)t;
    static lv_obj_t *last_scr;
    static int       ticks = -1;    /* -1 = idle; else ticks since last screen change */
    static uint32_t  wdt_ticks;
    static uint32_t  wdt_last_seq;

    lv_obj_t *act = lv_scr_act();

    /* ---- render watchdog (runs every tick, independent of the scrub) ---- */
    if (++wdt_ticks >= RENDER_WDT_TICKS) {
        wdt_ticks = 0;
        uint32_t seq = s_flush_seq;
        if (seq == wdt_last_seq && act) {
            lv_obj_invalidate(act);
            static uint32_t log_gate;
            if ((log_gate++ % 30) == 0) {   /* ≳2 min between lines at worst */
                ESP_LOGI(TAG, "render watchdog: no refresh in 2 s — forced repaint");
            }
        }
        wdt_last_seq = seq;
    }

    /* ---- stale-region scrub after a screen transition ---- */
    if (act != last_scr) {
        last_scr = act;
        ticks = 0;
        return;
    }
    if (ticks < 0) {
        return;
    }
    if (++ticks >= SCR_SCRUB_DELAY_TICKS) {
        if (act) {
            lv_obj_invalidate(act);
        }
        ticks = -1;
    }
}

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
    /* Upside-down mounting is a render-time flip: rotate the LVGL output
     * 270° instead of 90° (the AA video and splash PPA paths flip their
     * angle from display_flip_active() the same way). Panel-level mirror
     * (ST7701 SDIR/MADCTL) was tried first and produced garbage on the
     * jc4880 panel — the DPI stream's scan order is fixed, so the data has
     * to be flipped, not the panel. settings_init() has run by now (main
     * calls it before display_init). */
    s_flip_active = settings_get_display_flip();
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = s_flip_active ? ESP_LV_ADAPTER_ROTATE_270
                                  : ESP_LV_ADAPTER_ROTATE_90,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_DIRECT,
    };
    if (s_flip_active) {
        ESP_LOGI(TAG, "display flip 180 active (ROTATE_270)");
    }
    /* Pin the LVGL worker to core 0. The H.264 decoder library spawns a
     * helper task at priority 17 pinned to core 1 (CONFIG_ESP_H264_DUAL_TASK*)
     * and h264_pipe's wrapper task is also pinned to core 1; without this
     * affinity the no-affinity LVGL adapter would migrate onto core 1 and get
     * preempted by the decoder for the entire duration of a frame, blocking
     * the bsp_display_lock for hundreds of ms and stalling vesc_ui_updater. */
    cfg.lv_adapter_cfg.task_core_id = 0;
    /* Make the worker's sleep survive the tick rate. It runs
     * lv_timer_handler() in a loop and then vTaskDelay()s for however long
     * LVGL says the next timer is away, clamped to [min, max] = [1, 15] ms by
     * default. At CONFIG_FREERTOS_HZ=100 a tick is 10 ms, so pdMS_TO_TICKS()
     * of anything under 10 rounds to ZERO and vTaskDelay(0) does not block —
     * it only yields to tasks at the worker's own priority or above. The
     * worker sits at priority 6 on core 0, so for as long as LVGL keeps
     * reporting work due within 9 ms the loop spins and everything below it
     * on that core stops: BLE and the BT-agent link at 5, the VESC sim at 4,
     * the splash worker at 3, and IDLE0 — which is what trips the task
     * watchdog every 5 s.
     *
     * And it is the steady state, not a rare coincidence: lv_conf.h sets both
     * LV_DISP_DEF_REFR_PERIOD and LV_INDEV_DEF_READ_PERIOD to 10 ms. Two
     * 10 ms timers running out of phase mean the next one due is always less
     * than 10 ms away — fire one at t=0 and the other at t=5 and the handler
     * answers "5 ms" forever, never the 10 it would take to round up to a
     * tick. Asking for one whole tick costs nothing (the worker measures 2%
     * busy, and 10 ms is the period those two timers already want) and gives
     * the rest of core 0 its time back. */
    cfg.lv_adapter_cfg.task_min_delay_ms = portTICK_PERIOD_MS;
    if (cfg.lv_adapter_cfg.task_max_delay_ms < portTICK_PERIOD_MS) {
        cfg.lv_adapter_cfg.task_max_delay_ms = portTICK_PERIOD_MS;
    }
    s_display = bsp_display_start_with_config(&cfg);
    if (!s_display) {
        ESP_LOGE(TAG, "bsp_display_start failed");
        return ESP_FAIL;
    }

    /* Attach the render-time monitor. The adapter sets flush_cb / full_refresh
     * but never touches monitor_cb, and lv_refr.c re-reads driver->monitor_cb
     * every cycle, so setting it post-register is safe. lv_display_t == lv_disp_t
     * in LVGL v8.4. Always attached: the render watchdog needs the flush
     * counter even when the perf log is compiled out. */
    {
        lv_disp_t *d = (lv_disp_t *)s_display;
        if (d && d->driver) {
            d->driver->monitor_cb = display_monitor_cb;
        }
    }

#if DISPLAY_PERF_LOG
    {
        lv_disp_t *d = (lv_disp_t *)s_display;
        if (d && d->driver) {
            /* Periodic logger, decoupled from rendering — see the note at
             * DISPLAY_PERF_PERIOD_MS for cadence and idle suppression. Gives
             * steady-state numbers (the render-driven version only logged
             * during boot churn). */
            const esp_timer_create_args_t targs = {
                .callback = display_perf_timer_cb,
                .name     = "disp_perf",
            };
            esp_timer_handle_t th;
            if (esp_timer_create(&targs, &th) == ESP_OK) {
                esp_timer_start_periodic(th, (uint64_t)DISPLAY_PERF_PERIOD_MS * 1000);
            }
            ESP_LOGI(TAG, "render perf monitor attached (every %u s, idle suppressed)",
                     (unsigned)(DISPLAY_PERF_PERIOD_MS / 1000));

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

    /* Scrub stale framebuffer regions left by screen-load animations (see
     * scr_scrub_timer_cb). Runs on the LVGL task; created under the lock. */
    lv_timer_create(scr_scrub_timer_cb, SCR_SCRUB_PERIOD_MS, NULL);

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

bool display_flip_active(void)
{
    return s_flip_active;
}
