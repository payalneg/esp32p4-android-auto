#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AA touch action enum (TouchActionEnum.proto). */
typedef enum {
    TOUCH_ACTION_PRESS   = 0,
    TOUCH_ACTION_RELEASE = 1,
    TOUCH_ACTION_DRAG    = 2,
} touch_action_t;

/* Single-finger AA event delivery. timestamp_us is monotonic microseconds
 * (esp_timer_get_time); (x, y) are already mapped to AA landscape coordinates
 * (0..799, 0..479). The callback is responsible for protobuf encoding and TLS
 * send — touch_input only does GT911 polling and coordinate mapping.
 *
 * Multi-finger touches (>=2 fingers down) suppress AA events while present:
 * any pending press is released and no further events are forwarded until the
 * user lifts all fingers. This keeps gesture detection from leaking through
 * to the phone as a button press. */
typedef esp_err_t (*touch_send_fn)(uint64_t timestamp_us,
                                   touch_action_t action,
                                   uint16_t x, uint16_t y,
                                   void *ctx);

/* Gesture callback: fires once per gesture. The poll loop tracks 3-finger
 * touches and, after a hold threshold, calls this. State resets when all
 * fingers lift. */
typedef void (*touch_gesture_fn)(void);

/* Install / clear the single-finger AA callback. Idempotent — if the polling
 * task is already running this just swaps the cb pointer. Calling with cb=NULL
 * keeps the task running but stops AA event delivery. */
esp_err_t touch_input_start(touch_send_fn cb, void *ctx);

/* Install or clear the 3-finger gesture callback. Independent of the AA cb. */
void touch_input_set_gesture_cb(touch_gesture_fn cb);

/* Detach the AA single-finger callback. Polling task and gesture detection
 * keep running so the LVGL dashboard and the 3-finger toggle stay live
 * between AA sessions. Idempotent. */
void touch_input_stop(void);

/* Multiplex the GT911 reader between two consumers:
 *  - TOUCH_MODE_AA   : map to AA landscape coords + invoke the touch_send_fn
 *                      registered via touch_input_start.
 *  - TOUCH_MODE_LVGL : update the rotated landscape shared state read by the
 *                      LVGL input device hooked up in main.c.
 * Switching is atomic; the next poll cycle uses the new mode. */
typedef enum {
    TOUCH_MODE_AA   = 0,
    TOUCH_MODE_LVGL = 1,
} touch_mode_t;

void touch_input_set_mode(touch_mode_t mode);

/* LVGL indev hook. Returns the latest landscape-rotated (x, y) (already
 * mapped from panel-native portrait to 800x480 LVGL space, matching the
 * BSP's ESP_LV_ADAPTER_ROTATE_90 setting) and whether the panel is being
 * touched. Read without locking — values are independent so a torn read
 * yields at most a one-sample-stale coord (LVGL re-reads next tick). */
void touch_input_lvgl_read(uint16_t *out_x, uint16_t *out_y, bool *out_pressed);

#ifdef __cplusplus
}
#endif
