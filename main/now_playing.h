#pragma once

#include <stdbool.h>

#include "lvgl.h"

/* Now Playing — current track metadata received over UART from the WROOM
 * AVRCP CT. Used only in CONN_AVRCP mode; the rest of the firmware never
 * touches it.
 *
 * UI is intentionally decoupled: this module owns the strings, the GUI
 * Guider–generated VESC dashboard will add label widgets later and call
 * now_playing_attach_labels() to bind them. Until that happens, updates
 * just stay in the RAM cache and the ESP_LOG line — flipping the bind on
 * later picks up the most recent track automatically. */

void now_playing_init(void);

/* Bind LVGL label widgets to the cached strings. Any may be NULL — only
 * widgets the user actually added in GUI Guider need to be passed. After
 * attach, current cached values are pushed once so the labels reflect
 * whatever track was already playing. Safe to call multiple times (e.g.
 * after a screen reload) — replaces the previous binding. */
void now_playing_attach_labels(lv_obj_t *title,
                               lv_obj_t *artist,
                               lv_obj_t *album,
                               lv_obj_t *play_icon);

/* Update the cached track. Any field may be NULL or empty — the
 * corresponding label is cleared. Thread-safe; can be called from the
 * UART rx_task. */
void now_playing_set_track(const char *title,
                           const char *artist,
                           const char *album);

/* Update playback state (true = playing, false = paused/stopped). */
void now_playing_set_state(bool playing);
