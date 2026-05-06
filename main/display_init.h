#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One-shot panel + LVGL bring-up. Starts the BSP display, paints lv_scr_act
 * black, then turns the backlight on once a render cycle has had time to
 * push that black background out to the panel — avoids the 1-2 s of white
 * flash you'd otherwise see at boot.
 *
 * Pins the LVGL adapter task to core 0 so it can't be preempted by the
 * H.264 decoder (which owns core 1).
 *
 * Idempotent — safe to call more than once. */
esp_err_t display_init(void);

/* Returns the lv_display_t* the BSP gave us at init time, or NULL if
 * display_init() hasn't run / failed. The video sink needs this to drive
 * the LVGL adapter into dummy-draw mode while it owns the panel. */
struct _lv_display_t;
struct _lv_display_t *display_get(void);

#ifdef __cplusplus
}
#endif
