#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef enum {
    C6_OTA_STATUS_NOT_REQUIRED,
    C6_OTA_STATUS_UPDATED,
    C6_OTA_STATUS_FAILED,
} c6_ota_status_t;

c6_ota_status_t c6_ota_check_and_update(void);

/* Slave (C6) firmware version captured during the last check_and_update()
 * call, formatted as "MAJ.MIN.PAT". Returns "" if a read hasn't been
 * attempted yet or it failed. Buffer is module-owned, do not free. */
const char *c6_ota_get_slave_version_str(void);
