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

static TaskHandle_t      s_task;
static touch_send_fn     s_aa_cb;
static void             *s_aa_ctx;
static touch_gesture_fn  s_gest_cb;

/* Mode flag — default LVGL because the dashboard is the active screen at
 * boot. Flipped to AA by ui_mode_set when the user triple-taps. */
static _Atomic int       s_mode = TOUCH_MODE_LVGL;

/* Landscape-rotated coords for the LVGL indev. Updated by poll_task in
 * TOUCH_MODE_LVGL; LVGL pulls them via touch_input_lvgl_read(). */
static _Atomic uint16_t  s_lvgl_x;
static _Atomic uint16_t  s_lvgl_y;
static _Atomic bool      s_lvgl_pressed;

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

            const touch_mode_t mode = (touch_mode_t)atomic_load(&s_mode);

            if (mode == TOUCH_MODE_LVGL) {
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
                /* AA-side state must not survive a mode flip — reset every
                 * cycle so the next AA session starts clean. */
                was_pressed = false;
            } else {
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
    if (out_pressed) *out_pressed = atomic_load(&s_lvgl_pressed);
    if (out_x)       *out_x       = atomic_load(&s_lvgl_x);
    if (out_y)       *out_y       = atomic_load(&s_lvgl_y);
}
