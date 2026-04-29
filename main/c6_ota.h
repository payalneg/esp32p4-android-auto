#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef enum {
    C6_OTA_STATUS_NOT_REQUIRED,
    C6_OTA_STATUS_UPDATED,
    C6_OTA_STATUS_FAILED,
} c6_ota_status_t;

c6_ota_status_t c6_ota_check_and_update(void);
