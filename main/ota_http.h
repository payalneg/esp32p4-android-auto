#pragma once

#include "esp_err.h"

/* Start the HTTP OTA server. Idempotent; safe to call twice (second call
 * returns ESP_OK without restarting the server). No-op when
 * CONFIG_OTA_HTTP_ENABLED is unset.
 *
 * Endpoints (all on CONFIG_OTA_HTTP_PORT):
 *   GET  /       small upload form / curl hint
 *   GET  /info   running partition + app desc as text
 *   POST /ota    raw app binary (Content-Length required); on success
 *                the device flashes the inactive OTA slot, sets it as
 *                boot partition, replies "OK rebooting" and restarts.
 */
esp_err_t ota_http_start(void);
