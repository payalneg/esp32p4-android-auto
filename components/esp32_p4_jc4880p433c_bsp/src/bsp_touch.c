/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"
#include "bsp/esp-bsp.h"
#include "bsp/touch.h"

static const char *TAG = "bsp_touch";

esp_err_t bsp_touch_new(const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch)
{
    ESP_RETURN_ON_FALSE(ret_touch, ESP_ERR_INVALID_ARG, TAG, "ret_touch is NULL");
    
    /* Initialize I2C */
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "I2C init failed");
    i2c_master_bus_handle_t i2c_handle = bsp_i2c_get_handle();
    ESP_RETURN_ON_FALSE(i2c_handle, ESP_ERR_INVALID_STATE, TAG, "I2C handle is NULL");

    /* Initialize touch panel IO */
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = 400000;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_handle, &tp_io_config, &tp_io_handle), 
                        TAG, "Touch panel IO init failed");

    /* Initialize touch controller */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = BSP_LCD_TOUCH_RST,
        .int_gpio_num = BSP_LCD_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, ret_touch), 
                        TAG, "Touch controller init failed");

    ESP_LOGI(TAG, "Touch initialized successfully");
    return ESP_OK;
}