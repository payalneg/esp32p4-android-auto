#include "touch_input.h"

#include <stdatomic.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "touch_input";

/* GT911 docs warn that reads faster than ~10 ms can return stale/duplicate
 * frames. 20 ms = 50 Hz which is well above touch perception (~30 Hz is
 * already smooth) and lines up with the BSP example's polling cadence. */
#define POLL_INTERVAL_MS    20

/* Coordinate mapping: panel native is 480×800 portrait. The video pipeline
 * (display_video.c BYPASS path) writes panel pixel (dx, dy) from AA pixel
 * (sx=dy, sy=479-dx). Inverting: panel touch (tx, ty) shows AA pixel
 * (ax=ty, ay=479-tx). AA frame is 800×480 landscape, which matches the
 * touch_screen_config we advertise in InputChannel (width=800, height=480). */
#define PANEL_NATIVE_W      480
#define PANEL_NATIVE_H      800
#define AA_W                800
#define AA_H                480

#define MAX_POINTS          5
#define GESTURE_FINGERS     3
#define GESTURE_HOLD_MS     100  /* dwell before firing toggle */

/* Left-edge swipe → dashboard quick-action panel. Measured in LVGL landscape
 * space (x = panel_y). The press must start within START_PX of the left edge
 * and travel at least DIST_PX to the right to fire (once per touch). */
#define EDGE_SWIPE_START_PX 40
#define EDGE_SWIPE_DIST_PX  100

static TaskHandle_t      s_task;
static touch_send_fn     s_aa_cb;
static void             *s_aa_ctx;
static touch_gesture_fn  s_gest_cb;
static touch_gesture_fn  s_edge_swipe_cb;

/* Mode flag — default LVGL because the dashboard is the active screen at
 * boot. Flipped to AA by ui_mode_set when the user triple-taps. */
static _Atomic int       s_mode = TOUCH_MODE_LVGL;

/* Landscape-rotated coords for the LVGL indev. Updated by poll_task in
 * TOUCH_MODE_LVGL; LVGL pulls them via touch_input_lvgl_read(). */
static _Atomic uint16_t  s_lvgl_x;
static _Atomic uint16_t  s_lvgl_y;
static _Atomic bool      s_lvgl_pressed;

/* Synthetic injection override (debug bridge). While s_inject_active and the
 * current time is below s_inject_expiry_us, poll_task skips writing real GT911
 * coords and touch_input_lvgl_read returns the injected sample. The expiry is
 * a watchdog: a host that dies mid-gesture can't strand the panel — the
 * override lapses on its own and the real finger takes over again. */
static _Atomic bool      s_inject_active;
static _Atomic uint16_t  s_inject_x;
static _Atomic uint16_t  s_inject_y;
static _Atomic bool      s_inject_pressed;
static _Atomic int64_t   s_inject_expiry_us;

/* True iff an injection sample is currently authoritative. Reads the expiry
 * after the active flag so we never honour a stale deadline. */
static inline bool inject_live(void)
{
    return atomic_load(&s_inject_active) &&
           esp_timer_get_time() < atomic_load(&s_inject_expiry_us);
}

static void poll_task(void *arg)
{
    (void)arg;
    esp_lcd_touch_handle_t tp = bsp_display_get_touch_handle();
    if (!tp) {
        ESP_LOGE(TAG, "no touch handle from BSP — task exiting");
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    bool was_pressed = false;       /* AA single-finger state */
    uint16_t last_x = 0, last_y = 0;
    bool gesture_latched = false;   /* latched until all fingers lift */
    int64_t gesture_armed_us = 0;

    /* Left-edge swipe state (LVGL mode). */
    bool     edge_prev_pressed = false; /* effective-touch pressed last cycle */
    bool     edge_candidate    = false; /* this touch began at the left edge */
    bool     edge_fired        = false; /* swipe already fired this touch */
    uint16_t edge_start_x      = 0;

    /* The polling task lives for the lifetime of the program. We used to
     * tear it down on AA disconnect via touch_input_stop, but that left the
     * VESC dashboard unresponsive (no LVGL touch + no 3-finger gesture)
     * between AA sessions. Now stop only clears the AA cb. */
    while (true) {
        if (esp_lcd_touch_read_data(tp) == ESP_OK) {
            uint16_t tx[MAX_POINTS], ty[MAX_POINTS], strength[MAX_POINTS];
            uint8_t  cnt = 0;
            bool any = esp_lcd_touch_get_coordinates(tp, tx, ty, strength,
                                                     &cnt, MAX_POINTS);
            uint64_t ts = (uint64_t)esp_timer_get_time();

            if (!any) cnt = 0;

            /* --- gesture state machine --- */
            if (cnt >= GESTURE_FINGERS) {
                if (gesture_armed_us == 0) {
                    gesture_armed_us = (int64_t)ts;
                } else if (!gesture_latched &&
                           (int64_t)ts - gesture_armed_us >= GESTURE_HOLD_MS * 1000) {
                    gesture_latched = true;
                    if (s_gest_cb) s_gest_cb();
                }
            } else if (cnt == 0) {
                gesture_armed_us = 0;
                gesture_latched  = false;
            }
            /* When 1 <= cnt < GESTURE_FINGERS we keep the armed timer running
             * but don't reset until all fingers lift — this lets a brief
             * release+regrab still register without bouncing to IDLE. */

            /* --- single-finger handling, demuxed by mode --- */
            /* Common: pre-compute panel-native coords once. */
            const bool single = (cnt == 1 && !gesture_latched);
            uint16_t panel_x = 0, panel_y = 0;
            if (single) {
                panel_x = tx[0] < PANEL_NATIVE_W ? tx[0] : PANEL_NATIVE_W - 1;
                panel_y = ty[0] < PANEL_NATIVE_H ? ty[0] : PANEL_NATIVE_H - 1;
            }

            /* In AA mode but with no live projection session, GT911 events
             * have nowhere to go: the phone's input channel isn't open yet
             * (s_aa_cb still NULL) and LVGL is starved. That strands the idle
             * screen's "Connect" button — taps get read and dropped. Fall
             * back to LVGL routing whenever the AA callback isn't installed so
             * on-screen buttons stay live until the phone actually projects. */
            touch_mode_t mode = (touch_mode_t)atomic_load(&s_mode);
            if (mode == TOUCH_MODE_AA && s_aa_cb == NULL) {
                mode = TOUCH_MODE_LVGL;
            }

            /* Press/release transitions in raw panel coords — independent of
             * mode-specific rotation. Useful for figuring out whether GT911
             * is firing the events the rest of the pipeline expects. */
            static bool     prev_any;
            static uint8_t  prev_cnt;
            const bool any_now = (cnt > 0);
            if (any_now != prev_any) {
                // if (any_now) {
                //     ESP_LOGI(TAG, "DOWN cnt=%u panel=(%u,%u) mode=%s",
                //              cnt, tx[0], ty[0],
                //              mode == TOUCH_MODE_AA ? "AA" : "LVGL");
                // } else {
                //     ESP_LOGI(TAG, "UP   mode=%s",
                //              mode == TOUCH_MODE_AA ? "AA" : "LVGL");
                // }
                prev_any = any_now;
            } else if (any_now && cnt != prev_cnt) {
                // ESP_LOGI(TAG, "CNT  %u->%u panel=(%u,%u)",
                //          prev_cnt, cnt, tx[0], ty[0]);
            }
            prev_cnt = cnt;

            if (mode == TOUCH_MODE_LVGL) {
                /* Debug injection wins: while a synthetic sample is live, leave
                 * the LVGL atomics untouched so the injected (x, y, pressed)
                 * survives and the real finger is ignored. Clear an expired
                 * override here so it never lingers past its deadline. */
                if (inject_live()) {
                    was_pressed = false;
                } else {
                    if (atomic_load(&s_inject_active)) {
                        atomic_store(&s_inject_active, false);
                    }
                    /* Rotate panel-native portrait (480x800) to LVGL landscape
                     * (800x480) — matches BSP's ESP_LV_ADAPTER_ROTATE_90.
                     *  lx = panel_y, ly = (PANEL_NATIVE_W - 1) - panel_x. */
                    if (single) {
                        uint16_t lvgl_x = panel_y;
                        uint16_t lvgl_y = (PANEL_NATIVE_W - 1) - panel_x;
                        if (lvgl_x >= AA_W) lvgl_x = AA_W - 1;
                        if (lvgl_y >= AA_H) lvgl_y = AA_H - 1;
                        atomic_store(&s_lvgl_x, lvgl_x);
                        atomic_store(&s_lvgl_y, lvgl_y);
                    }
                    atomic_store(&s_lvgl_pressed, single);
                }
                /* AA-side state must not survive a mode flip — reset every
                 * cycle so the next AA session starts clean. */
                was_pressed = false;

                /* --- left-edge swipe detector (opens the LISP panel) ---
                 * Runs on the effective landscape coords reported to LVGL this
                 * cycle (real finger OR a live injection), so UI-test swipes
                 * trigger it the same as a real one. Landscape x = panel_y. */
                bool     eff_pressed;
                uint16_t eff_x;
                if (inject_live()) {
                    eff_pressed = atomic_load(&s_inject_pressed);
                    eff_x       = atomic_load(&s_inject_x);
                } else {
                    eff_pressed = single;
                    eff_x       = panel_y;
                }
                if (eff_pressed && !edge_prev_pressed) {
                    edge_candidate = (eff_x < EDGE_SWIPE_START_PX);
                    edge_fired     = false;
                    edge_start_x   = eff_x;
                } else if (eff_pressed && edge_candidate && !edge_fired &&
                           (int)eff_x - (int)edge_start_x >= EDGE_SWIPE_DIST_PX) {
                    edge_fired = true;
                    if (s_edge_swipe_cb) s_edge_swipe_cb();
                } else if (!eff_pressed) {
                    edge_candidate = false;
                    edge_fired     = false;
                }
                edge_prev_pressed = eff_pressed;
            } else {
                /* AA mode: drop any half-tracked edge swipe so it can't carry
                 * across a mode flip into the dashboard. */
                edge_prev_pressed = false;
                edge_candidate    = false;
                edge_fired        = false;
                /* TOUCH_MODE_AA: same rotation, plus PRESS/DRAG/RELEASE state
                 * machine. Multi-touch (>=2) suppresses AA events. */
                if (single) {
                    uint16_t aa_x = panel_y;
                    uint16_t aa_y = (PANEL_NATIVE_W - 1) - panel_x;
                    if (aa_x >= AA_W) aa_x = AA_W - 1;
                    if (aa_y >= AA_H) aa_y = AA_H - 1;

                    touch_action_t action;
                    if (!was_pressed) {
                        action = TOUCH_ACTION_PRESS;
                    } else if (aa_x != last_x || aa_y != last_y) {
                        action = TOUCH_ACTION_DRAG;
                    } else {
                        /* Same position, still touching — phone doesn't need
                         * an event for this. Skip to avoid spamming the input
                         * channel at 50 Hz with redundant DRAGs. */
                        goto next;
                    }

                    if (s_aa_cb) s_aa_cb(ts, action, aa_x, aa_y, s_aa_ctx);
                    last_x = aa_x;
                    last_y = aa_y;
                    was_pressed = true;
                } else if (cnt == 0 && was_pressed) {
                    /* Final position with RELEASE — openauto sends the last
                     * (x, y) we saw, not (0, 0). */
                    if (s_aa_cb) s_aa_cb(ts, TOUCH_ACTION_RELEASE, last_x, last_y, s_aa_ctx);
                    was_pressed = false;
                } else if (cnt >= 2 && was_pressed) {
                    /* Multi-touch begun while we had a single-finger press
                     * pending — release it cleanly so the phone doesn't see
                     * the gesture as a stuck-finger drag. */
                    if (s_aa_cb) s_aa_cb(ts, TOUCH_ACTION_RELEASE, last_x, last_y, s_aa_ctx);
                    was_pressed = false;
                }
            }
        }
next:
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}

esp_err_t touch_input_start(touch_send_fn cb, void *ctx)
{
    /* ctx must land before cb so a poll-task read that sees the new cb never
     * pairs it with a stale/NULL ctx (touch_send_event would deref it). */
    if (s_task) {
        s_aa_ctx = ctx;
        s_aa_cb  = cb;
        return ESP_OK;
    }
    if (!bsp_display_get_touch_handle()) {
        ESP_LOGE(TAG, "BSP touch not initialised — call bsp_display_start first");
        return ESP_ERR_INVALID_STATE;
    }
    s_aa_ctx = ctx;
    s_aa_cb  = cb;
    /* Pinned to core 0 alongside LVGL (prio 6) — we want touch to be on the
     * same core as the UI it drives. Priority 10 keeps us above LVGL so the
     * dashboard render never starves the input poll. Decoder + AA recv-loop
     * live on core 1, so we don't need to outprioritise them anymore. The
     * poll loop is mostly vTaskDelay anyway. */
    BaseType_t ok = xTaskCreatePinnedToCore(poll_task, "touch_input", 4096, NULL, 10, &s_task, 0);
    if (ok != pdPASS) {
        s_aa_cb  = NULL;
        s_aa_ctx = NULL;
        s_task   = NULL;
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "polling started (%d ms)", POLL_INTERVAL_MS);
    return ESP_OK;
}

void touch_input_set_gesture_cb(touch_gesture_fn cb)
{
    s_gest_cb = cb;
}

void touch_input_set_edge_swipe_cb(touch_gesture_fn cb)
{
    s_edge_swipe_cb = cb;
}

void touch_input_stop(void)
{
    /* Clear cb first (close the gate), then ctx — a poll-task read that
     * sees cb=NULL takes the early-out and never touches ctx. The poll
     * task itself keeps running so LVGL/gesture stay alive. */
    s_aa_cb  = NULL;
    s_aa_ctx = NULL;
}

void touch_input_set_mode(touch_mode_t mode)
{
    atomic_store(&s_mode, (int)mode);
    if (mode == TOUCH_MODE_LVGL) {
        /* Drop any pending AA state so a stale RELEASE doesn't fire when we
         * come back. The LVGL-side pressed flag is also cleared so a tap
         * mid-gesture doesn't leak as a phantom press. */
        atomic_store(&s_lvgl_pressed, false);
    }
    ESP_LOGI(TAG, "mode -> %s", mode == TOUCH_MODE_AA ? "AA" : "LVGL");
}

void touch_input_lvgl_read(uint16_t *out_x, uint16_t *out_y, bool *out_pressed)
{
    /* Honour a live injection override regardless of what poll_task last
     * wrote — this is the read-side half of the override (poll_task's skip is
     * the write-side half that keeps the real finger from racing the write). */
    if (inject_live()) {
        if (out_pressed) *out_pressed = atomic_load(&s_inject_pressed);
        if (out_x)       *out_x       = atomic_load(&s_inject_x);
        if (out_y)       *out_y       = atomic_load(&s_inject_y);
        return;
    }
    if (out_pressed) *out_pressed = atomic_load(&s_lvgl_pressed);
    if (out_x)       *out_x       = atomic_load(&s_lvgl_x);
    if (out_y)       *out_y       = atomic_load(&s_lvgl_y);
}

void touch_input_inject(uint16_t x, uint16_t y, bool pressed, uint32_t hold_ms)
{
    if (x >= AA_W) x = AA_W - 1;
    if (y >= AA_H) y = AA_H - 1;
    /* Order matters: stash the sample + deadline first, flip active last, so a
     * concurrent reader never sees active=true paired with a stale coord or
     * expiry. (Mirror of inject_live() reading active before expiry.) */
    atomic_store(&s_inject_x, x);
    atomic_store(&s_inject_y, y);
    atomic_store(&s_inject_pressed, pressed);
    atomic_store(&s_inject_expiry_us,
                 esp_timer_get_time() + (int64_t)hold_ms * 1000);
    atomic_store(&s_inject_active, true);
}

void touch_input_inject_clear(void)
{
    atomic_store(&s_inject_active, false);
}
