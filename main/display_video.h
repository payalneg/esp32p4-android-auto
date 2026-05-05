#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/* Display sink for the H.264 decoder. Owns the PPA client + the dummy-draw
 * handoff to the LVGL adapter. The Waveshare 800×480 panel is wired in
 * portrait native (480×800), so the OTA UI is drawn rotated by LVGL; for
 * video we run PPA's own scale+rotate so what reaches the panel is already
 * native-orientation RGB565.
 *
 * Init sequence: BSP must be up first (ota_screen_init or whatever else
 * called bsp_display_start*). Calling display_video_init() flips the LVGL
 * adapter into dummy-draw mode and disables the OTA-screen widgets so the
 * frame buffers we write are what's actually shown. */

esp_err_t display_video_init(void);

/* Render one decoded I420 frame. Buffer layout is the contiguous YUV420
 * planar block esp_h264 produces (Y plane W*H, U plane W*H/4, V plane W*H/4).
 * PPA scales+rotates and the result is blitted to the panel via the LVGL
 * adapter's dummy-draw path.
 *
 * Safe to call from the decoder task — internally locks LVGL only for the
 * duration of the blit. */
esp_err_t display_video_show_yuv420(const uint8_t *yuv,
                                    uint16_t src_w, uint16_t src_h);

/* Yield the panel back to LVGL — resumes the adapter worker if we'd
 * paused it on the first video frame. Called from ui_mode_set when
 * switching to the VESC dashboard so the user doesn't have to wait for
 * the next decoded frame to trigger the resume. No-op if the adapter
 * isn't paused. */
void display_video_yield_panel(void);
