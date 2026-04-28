/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "bsp/camera.h"
#include "bsp/config.h"
#include "bsp/esp-bsp.h"  // For shared I2C bus

static const char *TAG = "bsp_camera";

// Camera hardware pin configuration for JC4880P433C board
// NOTE: I2C bus (GPIO7/GPIO8) is shared with GT911 touch controller and managed by bsp_i2c.c
#ifndef CONFIG_BSP_CAMERA_RESET_PIN
#define CONFIG_BSP_CAMERA_RESET_PIN      -1  // No reset pin by default
#endif

#ifndef CONFIG_BSP_CAMERA_PWDN_PIN
#define CONFIG_BSP_CAMERA_PWDN_PIN       -1  // No power down pin by default
#endif

bsp_camera_config_t bsp_camera_get_default_config(void)
{
    bsp_camera_config_t config = {
        .reset_pin = CONFIG_BSP_CAMERA_RESET_PIN,
        .pwdn_pin = CONFIG_BSP_CAMERA_PWDN_PIN,
    };
    return config;
}

esp_err_t bsp_camera_init(const bsp_camera_config_t *config, i2c_master_bus_handle_t *i2c_handle)
{
    ESP_RETURN_ON_FALSE(i2c_handle, ESP_ERR_INVALID_ARG, TAG, "i2c_handle cannot be NULL");

    // Use default config if none provided
    bsp_camera_config_t default_config;
    if (config == NULL) {
        default_config = bsp_camera_get_default_config();
        config = &default_config;
    }

    /* Initialize I2C (shared with GT911 touch controller) */
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "I2C init failed");
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle();
    ESP_RETURN_ON_FALSE(i2c_bus, ESP_ERR_INVALID_STATE, TAG, "I2C handle is NULL");

    // Configure reset pin if specified
    if (config->reset_pin >= 0) {
        gpio_config_t reset_config = {
            .pin_bit_mask = BIT64(config->reset_pin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&reset_config), TAG, "Failed to configure reset pin");
        
        // Set reset pin high (inactive)
        gpio_set_level(config->reset_pin, 1);
    }

    // Configure power down pin if specified
    if (config->pwdn_pin >= 0) {
        gpio_config_t pwdn_config = {
            .pin_bit_mask = BIT64(config->pwdn_pin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&pwdn_config), TAG, "Failed to configure power down pin");
        
        // Set power down pin low (inactive)
        gpio_set_level(config->pwdn_pin, 0);
    }

    // Return the shared I2C bus handle for sensor initialization
    *i2c_handle = i2c_bus;

    ESP_LOGI(TAG, "Camera hardware initialized (using shared I2C bus with GT911 touch)");
    ESP_LOGI(TAG, "Control pins: RESET=%d, PWDN=%d", config->reset_pin, config->pwdn_pin);
    
    return ESP_OK;
}

esp_err_t bsp_camera_deinit(i2c_master_bus_handle_t i2c_handle)
{
    // NOTE: We don't delete the shared I2C bus here since it's also used by GT911 touch
    // The shared I2C bus is managed by bsp_i2c_init() and should persist
    // Only GPIO pins specific to camera are cleaned up here
    
    ESP_LOGI(TAG, "Camera hardware deinitialized (shared I2C bus preserved for touch)");
    return ESP_OK;
}
