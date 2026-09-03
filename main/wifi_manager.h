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

/* SoftAP only: deauthenticate the station that holds `ip_str` (looked up in
 * the DHCP lease table) so its Wi-Fi state machine starts over. With ip_str
 * NULL, or when the lease lookup fails, the single connected station is taken
 * to be the one; with several stations and no match nothing is kicked.
 * Returns ESP_OK when a deauth was issued, ESP_ERR_NOT_FOUND when no station
 * matched, ESP_ERR_NOT_SUPPORTED in STA (bench) builds. */
esp_err_t wifi_manager_kick_sta(const char *ip_str);
