/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "bsp_i2c";

/* 
 * Shared I2C bus configuration for JC4880P433C board
 * This I2C bus is shared between:
 * - GT911 touch controller (I2C address: 0x5D/0x14) 
 * - Camera sensors via SCCB interface (e.g., OV02C10 at 0x36)
 * 
 * Physical connection: Both GT911 and camera sensors are wired to the same I2C pins
 * Hardware pins: SCL=GPIO8, SDA=GPIO7 (from manufacturer schematic)
 */
#define BSP_I2C_NUM         I2C_NUM_0
#define BSP_I2C_SCL_GPIO    GPIO_NUM_8  // Shared SCL: GT911 + Camera sensors
#define BSP_I2C_SDA_GPIO    GPIO_NUM_7  // Shared SDA: GT911 + Camera sensors  
#define BSP_I2C_CLK_SPEED   400000      // 400kHz: Compatible with both GT911 and camera sensors

static i2c_master_bus_handle_t i2c_bus_handle = NULL;

esp_err_t bsp_i2c_init(void)
{
    if (i2c_bus_handle != NULL) {
        ESP_LOGD(TAG, "I2C already initialized");
        return ESP_OK;
    }

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = BSP_I2C_NUM,
        .sda_io_num = BSP_I2C_SDA_GPIO,
        .scl_io_num = BSP_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 0,  // External pull-ups present on JC4880P433C
        },
    };

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &i2c_bus_handle), 
                        TAG, "I2C master bus init failed");

    ESP_LOGI(TAG, "Shared I2C bus initialized (SCL=%d, SDA=%d, Speed=%dHz)", 
             BSP_I2C_SCL_GPIO, BSP_I2C_SDA_GPIO, BSP_I2C_CLK_SPEED);
    ESP_LOGI(TAG, "I2C bus shared between: GT911 touch + Camera sensors (via SCCB)");
    
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    return i2c_bus_handle;
}
