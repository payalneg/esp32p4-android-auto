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

/* Called when the rider has picked somewhere to go by tapping the map and
 * confirming it. Runs on the LVGL task, so the handler must only enqueue. */
typedef void (*nav_screen_dest_cb_t)(int32_t lat_e7, int32_t lon_e7);
void nav_screen_set_dest_cb(nav_screen_dest_cb_t cb);

/* Somewhere the phone found for a typed query. Names come from the offline
 * search index the phone downloaded with its map, so this works with no
 * signal — which is the point of having it on the panel at all. */
#define NAV_SEARCH_MAX   6
#define NAV_SEARCH_NAME  56
typedef struct {
    int32_t lat_e7, lon_e7;         /* the wire's units, no conversion needed */
    char    name[NAV_SEARCH_NAME];  /* UTF-8, NUL-terminated */
} nav_search_hit_t;

/* Show what the phone found (n == 0 means "nothing"), or clear the list. */
void nav_screen_set_results(const nav_search_hit_t *hits, size_t n);

/* The rider typed something to look for. Runs on the LVGL task, so the
 * handler must only enqueue. */
typedef void (*nav_screen_search_cb_t)(const char *query);
void nav_screen_set_search_cb(nav_screen_search_cb_t cb);

/* The rider's zoom buttons. Called with the level they asked for, already
 * clamped to what the composer can draw. */
typedef void (*nav_screen_zoom_cb_t)(uint8_t zoom);
void nav_screen_set_zoom_cb(nav_screen_zoom_cb_t cb);

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
