/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file camera.h
 * @brief JC4880P433C Camera Hardware Support
 * 
 * This file provides camera hardware initialization and configuration
 * for the JC4880P433C board. The BSP provides I2C bus and pin configuration
 * for camera sensors. Sensor-specific logic should be handled by the application.
 */

#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Camera hardware configuration for JC4880P433C board
 * 
 * @note I2C bus is shared with GT911 touch controller and managed by bsp_i2c.c
 */
typedef struct {
    int reset_pin;      /*!< Camera reset pin number (-1 if not used) */
    int pwdn_pin;       /*!< Camera power down pin number (-1 if not used) */
} bsp_camera_config_t;

/**
 * @brief Get default camera hardware configuration for JC4880P433C board
 * 
 * @return Default camera hardware configuration structure
 */
bsp_camera_config_t bsp_camera_get_default_config(void);

/**
 * @brief Initialize camera I2C bus and hardware pins
 * 
 * This function initializes the I2C bus for SCCB communication and configures
 * camera control pins. It does NOT initialize camera sensors - that should be
 * done by the application using esp_cam_sensor APIs.
 * 
 * @param[in] config Camera hardware configuration. If NULL, default config is used.
 * @param[out] i2c_handle Pointer to store the created I2C bus handle
 * 
 * @return
 *      - ESP_OK: Camera hardware initialized successfully
 *      - ESP_ERR_INVALID_ARG: Invalid arguments
 *      - Other ESP error codes on failure
 */
esp_err_t bsp_camera_init(const bsp_camera_config_t *config, i2c_master_bus_handle_t *i2c_handle);

/**
 * @brief Deinitialize camera hardware
 * 
 * @param[in] i2c_handle I2C bus handle to deinitialize
 * 
 * @return
 *      - ESP_OK: Camera hardware deinitialized successfully
 *      - Other ESP error codes on failure
 */
esp_err_t bsp_camera_deinit(i2c_master_bus_handle_t i2c_handle);

#ifdef __cplusplus
}
#endif