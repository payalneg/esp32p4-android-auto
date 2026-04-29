#pragma once

#include <stdint.h>

#include "esp_err.h"

typedef struct {
    char ssid[33];
    char password[65];
    char bssid_str[18];
    uint8_t channel;
} wifi_ap_info_t;

esp_err_t wifi_manager_start(void);

esp_err_t wifi_manager_wait_ready(uint32_t timeout_ms);

const wifi_ap_info_t *wifi_manager_get_ap_info(void);
