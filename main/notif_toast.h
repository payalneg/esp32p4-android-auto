#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Top-of-screen toast banner for incoming phone notifications.
 *
 * After init the module subscribes to notif_bridge's ring buffer and, on
 * every new entry, slides a card down from the top, holds for 4 s, then
 * slides it back. Stacks at most one card — newer notifications replace
 * the visible one. The card is parented to lv_layer_top() so it sits over
 * the dashboard, AA video, navigation map, anything.
 *
 * Album/track metadata: nothing in this module. The companion app keeps
 * pushing MEDIA frames, and notif_bridge_get_media() returns the current
 * snapshot. Wire that snapshot into the dashboard's music_info widget
 * (see Super_VESC_Display/generated/setup_scr_dashboard.c) — call
 * notif_toast_init() once at boot and read media state from the LVGL
 * task that owns dashboard_music_info. */
esp_err_t notif_toast_init(void);

#ifdef __cplusplus
}
#endif
