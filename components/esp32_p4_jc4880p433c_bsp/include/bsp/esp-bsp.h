/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP BSP: JC4880P433C Board Support Package
 */

#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "hal/lcd_types.h"
#include "bsp/config.h"
#include "bsp/display.h"

#if CONFIG_BSP_TOUCH_ENABLED
#include "bsp/touch.h"
#include "esp_lvgl_port.h"
#endif //CONFIG_BSP_TOUCH_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
 *  BSP Board Name
 **************************************************************************************************/

/** @defgroup boardname Board Name
 *  @brief BSP Board Name
 *  @{
 */
#define BSP_BOARD_JC4880P433C
/** @} */ // end of boardname

/**************************************************************************************************
 *  BSP Capabilities
 **************************************************************************************************/

/** @defgroup capabilities Capabilities
 *  @brief BSP Capabilities
 *  @{
 */
#define BSP_CAPS_DISPLAY        1
#define BSP_CAPS_TOUCH          1
#define BSP_CAPS_BUTTONS        0
#define BSP_CAPS_AUDIO          1
#define BSP_CAPS_AUDIO_SPEAKER  1
#define BSP_CAPS_AUDIO_MIC      1
#define BSP_CAPS_SDCARD         0
#define BSP_CAPS_IMU            0
/** @} */ // end of capabilities

/**************************************************************************************************
 *  JC4880P433C pinout
 **************************************************************************************************/

/** @defgroup g01_i2c I2C
 *  @brief I2C BSP API
 *  @{
 */
#define BSP_I2C_SCL                     ((gpio_num_t)CONFIG_BSP_JC4880P443C_I2C_SCL_GPIO)
#define BSP_I2C_SDA                     ((gpio_num_t)CONFIG_BSP_JC4880P443C_I2C_SDA_GPIO)
/** @} */ // end of i2c

/** @defgroup g04_display Display and Touch
 *  @brief Display and Touch BSP API
 *  @{
 */

/* Display configuration (configurable via menuconfig) */
#define BSP_LCD_H_RES                   CONFIG_BSP_JC4880P443C_LCD_H_RES
#define BSP_LCD_V_RES                   CONFIG_BSP_JC4880P443C_LCD_V_RES
#define BSP_LCD_COLOR_SPACE             LCD_RGB_ELEMENT_ORDER_RGB

/* LCD GPIO pins */
#define BSP_LCD_BACKLIGHT               ((gpio_num_t)CONFIG_BSP_JC4880P443C_LCD_BL_GPIO)
#define BSP_LCD_RST                     ((gpio_num_t)CONFIG_BSP_JC4880P443C_LCD_RST_GPIO)

/* Touch GPIO pins (not connected to dedicated pins on this board) */
#define BSP_LCD_TOUCH_RST               (GPIO_NUM_NC)
#define BSP_LCD_TOUCH_INT               (GPIO_NUM_NC)
/** @} */ // end of display

/** @defgroup g03_audio Audio
 *  @brief Audio BSP API  
 *  @{
 */
/* I2S GPIO pins (board specific - not configurable) */
#define BSP_I2S_SCLK                    (GPIO_NUM_12)
#define BSP_I2S_MCLK                    (GPIO_NUM_13)
#define BSP_I2S_LCLK                    (GPIO_NUM_10)
#define BSP_I2S_DOUT                    (GPIO_NUM_9)
#define BSP_I2S_DSIN                    (GPIO_NUM_48)
#define BSP_POWER_AMP_IO                (GPIO_NUM_11)
/** @} */ // end of audio

/**************************************************************************************************
 *  BSP Public API
 **************************************************************************************************/

/**
 * @brief Initialize shared I2C bus (for GT911 touch + camera sensors)
 * 
 * Initializes I2C bus shared between GT911 touch controller and camera sensors.
 * Physical pins: SCL=GPIO8, SDA=GPIO7 (per JC4880P433C board design)
 * 
 * @note Call this once before using either touch or camera functionality
 * @note Safe to call multiple times - will return ESP_OK if already initialized
 * 
 * @return
 *      - ESP_OK                On success or already initialized
 *      - ESP_ERR_INVALID_ARG   I2C parameter error
 *      - ESP_FAIL              I2C driver installation error
 */
esp_err_t bsp_i2c_init(void);

/**
 * @brief Get shared I2C bus handle
 *
 * @note Must call bsp_i2c_init() first before getting handle
 * @return I2C bus handle (shared between GT911 touch + camera sensors)
 */
i2c_master_bus_handle_t bsp_i2c_get_handle(void);

/* API surface compatible with esp32_p4_function_ev_board BSP (subset) */
esp_err_t bsp_spiffs_mount(void);
esp_err_t bsp_sdcard_mount(void);
esp_err_t bsp_display_backlight_on(void);
esp_err_t bsp_display_brightness_set(uint8_t brightness_percent);

/* Display API */
bsp_display_cfg_t bsp_display_get_default_config(void);
lv_display_t *bsp_display_start(void);
lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg);
bool bsp_display_lock(uint32_t timeout_ms);
void bsp_display_unlock(void);

/* Audio extra API (stubbed for now) */
esp_err_t bsp_extra_codec_init();

#ifdef __cplusplus
}
#endif
