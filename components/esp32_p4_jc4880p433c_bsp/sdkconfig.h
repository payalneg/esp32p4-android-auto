/*
 * Minimal sdkconfig.h for ESP32-P4 BSP component development
 * This file provides basic ESP32-P4 configuration for IntelliSense
 */

#pragma once

/* ESP32-P4 Target Configuration */
#define CONFIG_IDF_TARGET_ESP32P4 1
#define CONFIG_IDF_TARGET "esp32p4"

/* Memory Configuration */
#define CONFIG_ESP32P4_DEFAULT_CPU_FREQ_MHZ 400
#define CONFIG_MMU_PAGE_SIZE 65536
#define CONFIG_XTAL_FREQ 40

/* LCD Configuration */
#define CONFIG_LCD_PANEL_IO_FORMAT_BUF_SIZE 32
#define CONFIG_LCD_RGB_ISR_IRAM_SAFE 1

/* GPIO Configuration */
#define CONFIG_GPIO_ESP32_SUPPORT_SWITCH_SLP_PULL 1

/* I2C Configuration */
#define CONFIG_I2C_ENABLE_DEBUG_LOG 0

/* LEDC Configuration */
#define CONFIG_LEDC_CTRL_FUNC_IN_IRAM 0

/* SPIFFS Configuration */
#define CONFIG_SPIFFS_MAX_PARTITIONS 3

/* Logging Configuration */
#define CONFIG_LOG_DEFAULT_LEVEL 3
#define CONFIG_LOG_MAXIMUM_LEVEL 5

/* FreeRTOS Configuration */
#define CONFIG_FREERTOS_HZ 1000

/* ESP-IDF Version */
#define ESP_IDF_VERSION_MAJOR 5
#define ESP_IDF_VERSION_MINOR 5
#define ESP_IDF_VERSION_PATCH 1

/* Compiler Attributes */
#define IRAM_ATTR __attribute__((section(".iram1")))
#define DRAM_ATTR __attribute__((section(".dram1")))