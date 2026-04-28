/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file jc4880p433c_bsp.c
 * @brief Board support package for JC4880P433C integrated components
 * 
 * This module handles board-level integrated/soldered components:
 * - SPIFFS flash storage
 * - SD card slot
 * - Speaker/microphone (audio codec)
 * - RS485 interface
 * - Other non-modular peripherals
 * 
 * Modular components (connected via FPC/connectors) are in separate files:
 * - Display: bsp_display.c
 * - Touch: bsp_touch.c
 * - Camera: bsp_camera.c
 * - I2C bus: bsp_i2c.c
 */

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_spiffs.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "JC4880P433C_BSP";

/**
 * @brief Mount SPIFFS flash storage
 * 
 * SPIFFS is integrated into the board's flash partition table.
 * Auto-formats on first mount if needed.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bsp_spiffs_mount(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true,  // Auto-format on first mount
    };
    ESP_RETURN_ON_ERROR(esp_vfs_spiffs_register(&conf), TAG, "spiffs mount failed");
    size_t total = 0, used = 0;
    esp_spiffs_info(conf.partition_label, &total, &used);
    ESP_LOGI(TAG, "SPIFFS total=%zu used=%zu", total, used);
    return ESP_OK;
}

/**
 * @brief Mount SD card
 * 
 * TODO: Implement SD card mounting for the integrated SD card slot.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bsp_sdcard_mount(void)
{
    // TODO: Implement SD card mounting
    ESP_LOGW(TAG, "SD card mount not yet implemented");
    return ESP_OK;
}

/**
 * @brief Initialize audio codec
 * 
 * TODO: Implement audio codec initialization for integrated speaker/microphone.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bsp_extra_codec_init(void)
{
    // TODO: Implement audio codec initialization
    ESP_LOGW(TAG, "Audio codec init not yet implemented");
    return ESP_OK;
}
