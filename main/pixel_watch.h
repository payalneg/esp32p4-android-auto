#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Spawn a low-priority task that polls pixel (0,0) of each LVGL
 * framebuffer at 50 Hz and ESP_LOGWs any time it goes light blue.
 * Use to diagnose blue-flash episodes — see pixel_watch.c header. */
esp_err_t pixel_watch_start(void);

#ifdef __cplusplus
}
#endif
