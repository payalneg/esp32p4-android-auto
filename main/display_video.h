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
 * Init sequence: display_init() must run first to bring the panel up.
 * Calling display_video_init() flips the LVGL adapter into dummy-draw
 * mode so the frame buffers we write are what's actually shown. */

esp_err_t display_video_init(void);

/* Render one decoded I420 frame synchronously (reference / experimental
 * path — see the DISPLAY_* switches in display_video.c). Buffer layout is
 * the contiguous YUV420 planar block esp_h264 produces (Y plane W*H, U plane
 * W*H/4, V plane W*H/4). */
esp_err_t display_video_show_yuv420(const uint8_t *yuv,
                                    uint16_t src_w, uint16_t src_h);

/* Production path: two-stage pipeline. The caller (decoder task, core 1)
 * only pays for the CPU shuffle of `yuv` into a staging buffer — the frame
 * is then converted, overlaid and presented by a task on core 0 while the
 * decoder moves on to the next frame. Blocks when the display is two frames
 * behind (back-pressure). `yuv` may be reused by the caller as soon as this
 * returns.
 *
 * Returns ESP_OK when the frame was queued: done_cb (may be NULL) fires from
 * the display task once the frame has been presented — or dropped, if LVGL
 * took the panel back in the meantime. Returns ESP_ERR_INVALID_STATE when the
 * frame was dropped right away because LVGL owns the panel (VESC dashboard
 * mode); the caller handles its own acknowledgement in that case. */
typedef void (*display_video_done_cb_t)(void *ctx);
esp_err_t display_video_submit_yuv420(const uint8_t *yuv,
                                      uint16_t src_w, uint16_t src_h,
                                      display_video_done_cb_t done_cb,
                                      void *done_ctx);

/* Queue a callback that fires once every frame submitted before it has been
 * presented (or dropped). Used to ack an AA message after its last frame. */
esp_err_t display_video_fence(display_video_done_cb_t done_cb, void *done_ctx);

/* Yield the panel back to LVGL — resumes the adapter worker if we'd
 * paused it on the first video frame. Called from ui_mode_set when
 * switching to the VESC dashboard so the user doesn't have to wait for
 * the next decoded frame to trigger the resume. No-op if the adapter
 * isn't paused. */
void display_video_yield_panel(void);
