#pragma once

#include "esp_err.h"
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

/* Send callback. timestamp_us is monotonic microseconds (esp_timer_get_time);
 * (x, y) are already mapped to AA landscape coordinates (0..799, 0..479). The
 * callback is responsible for protobuf encoding and TLS send — touch_input
 * only does GT911 polling and coordinate mapping. */
typedef esp_err_t (*touch_send_fn)(uint64_t timestamp_us,
                                   touch_action_t action,
                                   uint16_t x, uint16_t y,
                                   void *ctx);

/* Start the polling task. The task captures the current touch handle from BSP
 * (must already be initialised via bsp_display_start_with_config). Idempotent:
 * if a task is already running, returns ESP_OK without changing the cb. */
esp_err_t touch_input_start(touch_send_fn cb, void *ctx);

/* Stop the polling task and release the handle reference. After this returns,
 * cb will not be called any more. */
void touch_input_stop(void);

#ifdef __cplusplus
}
#endif
