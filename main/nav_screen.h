/* Navigator screen — the map picture the companion app renders on the phone
 * and streams to us over BLE (see ble_nav.c for the wire side).
 *
 * The head unit draws nothing itself: it decodes a JPEG into an RGB565
 * framebuffer and shows it full-screen through a single lv_img. That keeps
 * the notification toast, the 3-finger gesture and every other LVGL citizen
 * working exactly as they do over the dashboard, which a direct-to-panel
 * blit (splash_screen.c / display_video.c) would bypass. At the one frame a
 * second this link can carry, the cost of going through LVGL does not matter.
 *
 * Buffers: two 800x480 RGB565 in PSRAM, front (what lv_img points at) and
 * back (what the BLE worker fills). The worker asks for the back buffer, and
 * gets NULL while a committed frame is still waiting to be shown or while
 * LVGL may still be reading it — a single-producer / single-consumer swap
 * with no lock in the DMA path. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NAV_SCREEN_W 800
#define NAV_SCREEN_H 480

/* Build the screen and its framebuffers. Takes the LVGL lock itself; call
 * after ui_mode_init(), from a task that does not already hold it. */
esp_err_t nav_screen_init(void);

/* The screen object for ui_mode_set to load. NULL before a successful init. */
lv_obj_t *nav_screen_get(void);

/* Back framebuffer to decode/scale the next frame into (NAV_SCREEN_W *
 * NAV_SCREEN_H * 2 bytes, cache-line aligned), or NULL when the previous
 * frame has not been shown yet — the caller should drop the frame and say so.
 * BLE worker task only. */
uint16_t *nav_screen_back_buffer(void);
size_t    nav_screen_back_buffer_bytes(void);

/* The back buffer now holds a complete frame; show it on the next LVGL tick.
 * BLE worker task only. */
void nav_screen_commit(void);

/* Whether the Navigator screen is the one the rider is looking at. Set by
 * ui_mode_set; read by the BLE side to tell the phone whether to stream. */
void nav_screen_set_active(bool active);
bool nav_screen_active(void);

/* Link/stream state, for the placeholder text. */
void nav_screen_set_phone(bool connected);
void nav_screen_set_streaming(bool streaming);

/* Frames actually shown, and how long ago the last one was — diagnostics for
 * the `navstat` debug command. */
uint32_t nav_screen_frames_shown(void);
int64_t  nav_screen_last_frame_us(void);

#ifdef __cplusplus
}
#endif
