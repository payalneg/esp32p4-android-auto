/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OTA_PARTITION_STAGE_VALIDATE = 0,
    OTA_PARTITION_STAGE_BEGIN,
    OTA_PARTITION_STAGE_WRITE,
    OTA_PARTITION_STAGE_END,
    OTA_PARTITION_STAGE_DONE,
} ota_partition_stage_t;

typedef void (*ota_partition_progress_cb_t)(ota_partition_stage_t stage,
                                            uint32_t bytes_done,
                                            uint32_t total_bytes,
                                            void *ctx);

esp_err_t ota_partition_perform(const char *partition_label);

esp_err_t ota_partition_perform_with_cb(const char *partition_label,
                                        ota_partition_progress_cb_t cb,
                                        void *ctx);

#ifdef __cplusplus
}
#endif
