#pragma once
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "hal/lcd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lvgl_port_cfg_t lvgl_port_cfg;
    uint32_t        buffer_size;
    bool            double_buffer;
    struct {
        unsigned int buff_dma: 1;
        unsigned int buff_spiram: 1;
        unsigned int sw_rotate: 1;
    } flags;
} bsp_display_cfg_t;

/**
 * @brief Get default display configuration
 * 
 * @return bsp_display_cfg_t Default configuration with standard buffer settings
 */
bsp_display_cfg_t bsp_display_get_default_config(void);

/**
 * @brief Start display with default configuration
 * 
 * @return lv_display_t* LVGL display object, or NULL on failure
 */
lv_display_t *bsp_display_start(void);

/**
 * @brief Start display with custom configuration
 * 
 * @param cfg Display configuration structure
 * @return lv_display_t* LVGL display object, or NULL on failure
 */
lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg);

/**
 * @brief Lock LVGL mutex for thread-safe display operations
 * 
 * @param timeout_ms Timeout in milliseconds
 * @return true Lock acquired
 * @return false Timeout occurred
 */
bool bsp_display_lock(uint32_t timeout_ms);

/**
 * @brief Unlock LVGL mutex after display operations
 */
void bsp_display_unlock(void);

/**
 * @brief Turn on display backlight at full brightness
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bsp_display_backlight_on(void);

/**
 * @brief Set display backlight brightness
 * 
 * @param brightness_percent Brightness level (0-100%)
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if brightness > 100
 */
esp_err_t bsp_display_brightness_set(uint8_t brightness_percent);

#ifdef __cplusplus
}
#endif

