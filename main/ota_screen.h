#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t ota_screen_init(void);

void ota_screen_show(const char *subtitle);

void ota_screen_set_progress(uint32_t bytes_done, uint32_t bytes_total);

void ota_screen_set_status(const char *line);

void ota_screen_hide(void);
