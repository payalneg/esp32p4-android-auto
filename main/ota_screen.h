#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* C6 firmware update overlay. Builds a hidden full-screen container with
 * a title, subtitle, progress bar, percent label and status line. Sits on
 * lv_scr_act() — display_init() must have run first.
 *
 * Idempotent. Compiled-out when CONFIG_C6_OTA_DISPLAY_PROGRESS=n. */
esp_err_t ota_screen_init(void);

void ota_screen_show(const char *subtitle);

void ota_screen_set_progress(uint32_t bytes_done, uint32_t bytes_total);

void ota_screen_set_status(const char *line);

void ota_screen_hide(void);

#ifdef __cplusplus
}
#endif
