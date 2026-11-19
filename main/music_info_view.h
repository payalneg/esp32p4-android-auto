#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the current track (title / artist / play-pause state) into a
 * caller-provided LVGL container. Designed to plug into the
 * `dashboard_music_info_tile` widget that GUI Guider generates inside
 * Super_VESC_Display — call once after the dashboard is built:
 *
 *     music_info_view_attach(ui->dashboard_music_info_tile);
 *
 * The module then polls notif_bridge_get_media() in the LVGL task and
 * keeps the labels in sync. Idempotent — safe to call multiple times
 * (no-op after the first successful attach). */
esp_err_t music_info_view_attach(lv_obj_t *parent);

/* Detach from the current tile: stop the poller and drop widget references.
 * The widgets are children of the hosting dashboard screen and are freed when
 * that screen is deleted — this must NOT lv_obj_del them. Used by the dashboard
 * theme switcher before the hosting screen is rebuilt; pair with a fresh
 * music_info_view_attach() on the new theme's tile. */
void music_info_view_detach(void);

/* Detach AND remove the widgets the view created. Used when the rider turns
 * the music widget off in Settings: the hosting screen stays, the card must
 * actually disappear (detach alone only drops our references). Safe to call
 * when nothing is attached. */
void music_info_view_destroy(void);

/* Show/hide the prev/play/next row inside the card. The card, album art and
 * title/artist are untouched. Safe before attach: the state is kept and applied
 * when the view is created. */
void music_info_view_set_buttons_visible(bool on);

#ifdef __cplusplus
}
#endif
