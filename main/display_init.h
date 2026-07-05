#pragma once

#include <stdbool.h>

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

/* Flip the panel output 180° (upside-down mounting). Implemented as an
 * ST7701 scan-direction change (SDIR + MADCTL/ML over DSI-DBI), so it costs
 * nothing at render time and flips every pixel source at once — LVGL, the
 * AA video bypass path and the boot splash. Touch stays un-flipped here;
 * touch_input.c inverts GT911 coords itself from the same setting.
 * display_init() applies the persisted setting; this is the hot-apply hook
 * for the Settings switch. Safe to call from the LVGL task. */
esp_err_t display_set_flip(bool flip);

#ifdef __cplusplus
}
#endif
