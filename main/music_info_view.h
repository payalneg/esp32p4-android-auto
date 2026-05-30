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

#ifdef __cplusplus
}
#endif
