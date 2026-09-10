#include <stdio.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_idf_version.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_lcd_st7701.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lv_adapter.h"

#include "bsp/display.h"
#include "bsp/touch.h"
#include "bsp/esp32_s3_touch_lcd_4.h"
#include "bsp_err_check.h"

static const char *TAG = "ESP32-S3-Touch-LCD-4";

static i2c_master_bus_handle_t i2c_handle = NULL; // I2C Handle
static bool i2c_initialized = false;
static esp_io_expander_handle_t custom_io_expander = NULL;
static lv_display_t *disp;
static lv_indev_t *disp_indev = NULL;
uint8_t brightness;
sdmmc_card_t *bsp_sdcard = NULL; // Global uSD card handler
static esp_lcd_touch_handle_t tp = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL; // LCD panel handle

static const st7701_lcd_init_cmd_t lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {0x11, (uint8_t[]){0x00}, 0, 120},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]){0x3B, 0x00}, 2, 0},
    {0xC1, (uint8_t[]){0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t[]){0x21, 0x08}, 2, 0},
    {0xCD, (uint8_t[]){0x08}, 1, 0},
    {0xB0, (uint8_t[]){0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18}, 16, 0},
    {0xB1, (uint8_t[]){0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18}, 16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t[]){0x60}, 1, 0},
    {0xB1, (uint8_t[]){0x30}, 1, 0},
    {0xB2, (uint8_t[]){0x87}, 1, 0},
    {0xB3, (uint8_t[]){0x80}, 1, 0},
    {0xB5, (uint8_t[]){0x49}, 1, 0},
    {0xB7, (uint8_t[]){0x85}, 1, 0},
    {0xB8, (uint8_t[]){0x21}, 1, 0},
    {0xC1, (uint8_t[]){0x78}, 1, 0},
    {0xC2, (uint8_t[]){0x78}, 1, 20},
    {0xE0, (uint8_t[]){0x00, 0x1B, 0x02}, 3, 0},
    {0xE1, (uint8_t[]){0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}, 11, 0},
    {0xE2, (uint8_t[]){0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}, 12, 0},
    {0xE3, (uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t[]){0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0}, 16, 0},
    {0xE6, (uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t[]){0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0}, 16, 0},
    {0xEB, (uint8_t[]){0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}, 7, 0},
    {0xEC, (uint8_t[]){0x3C, 0x00}, 2, 0},
    {0xED, (uint8_t[]){0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA}, 16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x3A, (uint8_t[]){0x66}, 1, 0},
    {0x21, (uint8_t[]){0x00}, 0, 120},
    {0x29, (uint8_t[]){0x00}, 0, 0},
};

/**************************************************************************************************
 *
 * I2C Function
 *
 **************************************************************************************************/
esp_err_t bsp_i2c_init(void)
{
    /* I2C was initialized before */
    if (i2c_initialized)
    {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .i2c_port = BSP_I2C_NUM,
    };
    BSP_ERROR_CHECK_RETURN_ERR(i2c_new_master_bus(&i2c_bus_conf, &i2c_handle));

    i2c_initialized = true;

    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    BSP_ERROR_CHECK_RETURN_ERR(i2c_del_master_bus(i2c_handle));
    i2c_initialized = false;
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    bsp_i2c_init();
    return i2c_handle;
}

static esp_err_t bsp_i2c_device_probe(uint8_t addr)
{
    return i2c_master_probe(i2c_handle, addr, 100);
}

esp_err_t bsp_spiffs_mount(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_BSP_SPIFFS_MOUNT_POINT,
        .partition_label = CONFIG_BSP_SPIFFS_PARTITION_LABEL,
        .max_files = CONFIG_BSP_SPIFFS_MAX_FILES,
#ifdef CONFIG_BSP_SPIFFS_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
    };

    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);

    BSP_ERROR_CHECK_RETURN_ERR(ret_val);

    size_t total = 0, used = 0;
    ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret_val));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret_val;
}

esp_err_t bsp_spiffs_unmount(void)
{
    return esp_vfs_spiffs_unregister(CONFIG_BSP_SPIFFS_PARTITION_LABEL);
}

/**************************************************************************************************
 *
 * IO Expander Function
 *
 **************************************************************************************************/
esp_io_expander_handle_t bsp_io_expander_init(void)
{
    if (!i2c_initialized)
    {
        /* Initialize I2C (returns a handle, so NULL on failure — the vendor
         * BSP returns an esp_err_t here and relies on int-to-pointer). */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_i2c_init());
    }
    if (!custom_io_expander)
    {
        /* The helper occasionally misses the very first transaction after a
         * cold power-up (it is a microcontroller on V4.0 and has to boot
         * too) — retry rather than come up with no display resets. */
        esp_err_t err = ESP_FAIL;
        for (int attempt = 1; attempt <= 3; attempt++)
        {
#if CONFIG_BSP_S3T4_HW_V4
            err = custom_io_expander_new_i2c_ch32v003(i2c_handle, BSP_IO_EXPANDER_I2C_ADDRESS, &custom_io_expander);
#else
            err = esp_io_expander_new_i2c_tca9554(i2c_handle, BSP_IO_EXPANDER_I2C_ADDRESS, &custom_io_expander);
#endif
            if (err == ESP_OK && custom_io_expander)
            {
                break;
            }
            ESP_LOGW(TAG, "IO expander 0x%02X attempt %d/3: %s",
                     BSP_IO_EXPANDER_I2C_ADDRESS, attempt, esp_err_to_name(err));
            custom_io_expander = NULL;
            vTaskDelay(pdMS_TO_TICKS(20 * attempt));
        }
        if (!custom_io_expander)
        {
            ESP_LOGE(TAG, "no IO expander at 0x%02X — wrong CONFIG_BSP_S3T4_HW_V* "
                          "for this board revision?", BSP_IO_EXPANDER_I2C_ADDRESS);
        }
    }
    return custom_io_expander;
}

esp_err_t bsp_sdcard_mount(void)
{
    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_BSP_SD_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    const sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    const sdmmc_slot_config_t slot_config = {
        .clk = BSP_SD_CLK,
        .cmd = BSP_SD_CMD,
        .d0 = BSP_SD_D0,
        .d1 = GPIO_NUM_NC,
        .d2 = GPIO_NUM_NC,
        .d3 = GPIO_NUM_NC,
        .d4 = GPIO_NUM_NC,
        .d5 = GPIO_NUM_NC,
        .d6 = GPIO_NUM_NC,
        .d7 = GPIO_NUM_NC,
        .cd = SDMMC_SLOT_NO_CD,
        .wp = SDMMC_SLOT_NO_WP,
        .width = 1,
        .flags = 0,
    };

#if !CONFIG_FATFS_LONG_FILENAMES
    ESP_LOGW(TAG, "Warning: Long filenames on SD card are disabled in menuconfig!");
#endif

    return esp_vfs_fat_sdmmc_mount(BSP_SD_MOUNT_POINT, &host, &slot_config, &mount_config, &bsp_sdcard);
}

esp_err_t bsp_sdcard_unmount(void)
{
    return esp_vfs_fat_sdcard_unmount(BSP_SD_MOUNT_POINT, bsp_sdcard);
}

#define LCD_BRIGHTNESS_MAX 0xFF

esp_err_t bsp_display_brightness_init(void)
{
    if (!custom_io_expander)
    {
        BSP_NULL_CHECK(bsp_io_expander_init(), ESP_FAIL);
    }
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    if (brightness_percent > 100)
    {
        brightness_percent = 100;
    }
    else if (brightness_percent < 0)
    {
        brightness_percent = 0;
    }
    if (!custom_io_expander)
    {
        return ESP_ERR_INVALID_STATE;
    }
    brightness = (uint8_t)brightness_percent;

#if CONFIG_BSP_S3T4_HW_V4
    /* The CH32V003 drives the backlight with an inverted PWM duty: 0 is full
     * brightness, 0xFF is off (vendor BSP does the same subtraction). */
    return custom_io_expander_set_pwm(custom_io_expander,
                                      (100 - brightness_percent) * LCD_BRIGHTNESS_MAX / 100);
#else
    /* V3.0 has a plain enable line — no dimming. Anything above zero is "on",
     * so the Settings brightness slider degrades to a switch on this
     * revision. */
    return esp_io_expander_set_level(custom_io_expander, BSP_LCD_BL_EN,
                                     brightness_percent > 0 ? 1 : 0);
#endif
}
esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

esp_err_t bsp_display_new(const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io)
{
    esp_lcd_panel_io_handle_t io_handle = NULL;

    // ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG, "Brightness init failed");

    esp_io_expander_handle_t expander = NULL;

    // ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG, "Brightness init failed");
    BSP_NULL_CHECK(expander = bsp_io_expander_init(), ESP_FAIL);

    /* Hold the panel and the touch controller in reset, then release them.
     * Both revisions need this before the ST7701 will answer on the 3-wire
     * SPI link; a warm reset happens to work without it, which is exactly
     * how a missing reset turns into "works after reflash, black after a
     * power cycle". */
#if CONFIG_BSP_S3T4_HW_V4
    esp_io_expander_set_dir(custom_io_expander,
                            BSP_SYS_EN | BSP_BEE_EN | BSP_LCD_RST | BSP_LCD_TOUCH_RST,
                            IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(custom_io_expander, BSP_RTC_INT, IO_EXPANDER_INPUT);
    esp_io_expander_set_level(custom_io_expander,
                              BSP_BEE_EN | BSP_LCD_RST | BSP_LCD_TOUCH_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_io_expander_set_level(custom_io_expander,
                              BSP_SYS_EN | BSP_LCD_RST | BSP_LCD_TOUCH_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
#else
    esp_io_expander_set_dir(custom_io_expander,
                            BSP_LCD_RST | BSP_LCD_TOUCH_RST | BSP_LCD_BL_EN,
                            IO_EXPANDER_OUTPUT);
    /* Backlight stays off here — display_init() turns it on after the first
     * frame so the boot flash of uninitialised framebuffer never shows. */
    esp_io_expander_set_level(custom_io_expander, BSP_LCD_BL_EN, 0);
    esp_io_expander_set_level(custom_io_expander,
                              BSP_LCD_RST | BSP_LCD_TOUCH_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_io_expander_set_level(custom_io_expander,
                              BSP_LCD_RST | BSP_LCD_TOUCH_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));
#endif

    ESP_LOGI(TAG, "Install 3-wire SPI panel IO");
    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO,
        .cs_gpio_num = BSP_LCD_IO_SPI_CS,
        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = BSP_LCD_IO_SPI_SCL,
        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = BSP_LCD_IO_SPI_SDA,
    };
    esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

    esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .dma_burst_size = 64,
#else
        .psram_trans_align = 64,
#endif
        .data_width = BSP_RGB_DATA_WIDTH,
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
#endif
        .de_gpio_num = BSP_LCD_DE,
        .pclk_gpio_num = BSP_LCD_PCLK,
        .vsync_gpio_num = BSP_LCD_VSYNC,
        .hsync_gpio_num = BSP_LCD_HSYNC,
        .disp_gpio_num = BSP_LCD_DISP,
        .data_gpio_nums = {
            BSP_LCD_DATA0,
            BSP_LCD_DATA1,
            BSP_LCD_DATA2,
            BSP_LCD_DATA3,
            BSP_LCD_DATA4,
            BSP_LCD_DATA5,
            BSP_LCD_DATA6,
            BSP_LCD_DATA7,
            BSP_LCD_DATA8,
            BSP_LCD_DATA9,
            BSP_LCD_DATA10,
            BSP_LCD_DATA11,
            BSP_LCD_DATA12,
            BSP_LCD_DATA13,
            BSP_LCD_DATA14,
            BSP_LCD_DATA15,
        },
        .timings = ST7701_480_480_PANEL_60HZ_RGB_TIMING(),
        .flags.fb_in_psram = 1,
        /* Two framebuffers: what the LVGL adapter's DOUBLE_DIRECT tear-avoid
         * mode wants at rotation 0 (it asks for a third only when it has to
         * rotate). Both live in PSRAM, and the panel streams them out through
         * internal-RAM bounce buffers so a PSRAM stall can't shift the
         * picture — that is also why CONFIG_LCD_RGB_ISR_IRAM_SAFE is on. */
        .num_fbs = CONFIG_BSP_LCD_RGB_BUFFER_NUMS,
        .bounce_buffer_size_px = BSP_LCD_H_RES * CONFIG_BSP_LCD_RGB_BOUNCE_BUFFER_HEIGHT,
    };
    rgb_config.timings.pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ;
    rgb_config.timings.h_res = BSP_LCD_H_RES;
    rgb_config.timings.v_res = BSP_LCD_V_RES;
    st7701_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,
        .init_cmds = lcd_init_cmds, // Uncomment these line if use custom initialization commands
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = {
            .auto_del_panel_io = 0,
            .mirror_by_cmd = 1,
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        /* Reset is an IO-expander pin, not a GPIO — already pulsed above. */
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BSP_LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, &panel_handle));
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_disp_on_off(panel_handle, true);

    if (ret_panel)
    {
        *ret_panel = panel_handle;
    }
    if (ret_io)
    {
        *ret_io = io_handle;
    }

    return ESP_OK;
}

esp_err_t bsp_touch_new(const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch)
{
    /* Initilize I2C */
    BSP_ERROR_CHECK_RETURN_ERR(bsp_i2c_init());

    /* Initialize touch. Reset is on the IO expander (pulsed in
     * bsp_display_new), and the firmware polls GT911 rather than using its
     * interrupt — main/touch_input.c is the single reader and demuxes to
     * LVGL. Axis flags come from the caller so a mirrored mount is a config
     * change, not a code change. */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy  = config ? config->swap_xy  : 0,
            .mirror_x = config ? config->mirror_x : 0,
            .mirror_y = config ? config->mirror_y : 0,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config;
    if (ESP_OK == bsp_i2c_device_probe(ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS))
    {
        ESP_LOGI(TAG, "Touch 0x5d found");
        esp_lcd_panel_io_i2c_config_t config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
        memcpy(&tp_io_config, &config, sizeof(config));
    }
    else if (ESP_OK == bsp_i2c_device_probe(ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP))
    {
        ESP_LOGI(TAG, "Touch 0x14 found");
        esp_lcd_panel_io_i2c_config_t config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
        config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
        memcpy(&tp_io_config, &config, sizeof(config));
    }
    else
    {
        ESP_LOGE(TAG, "Touch not found");
        return ESP_ERR_NOT_FOUND;
    }
    tp_io_config.scl_speed_hz = CONFIG_BSP_I2C_CLK_SPEED_HZ;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_handle, &tp_io_config, &tp_io_handle), TAG, "");
    return esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, ret_touch);
}

/**********************************************************************************************************
 *
 * Display / LVGL bring-up (espressif/esp_lvgl_adapter)
 *
 **********************************************************************************************************/

static lv_display_t *bsp_display_lcd_init(const bsp_display_cfg_t *cfg)
{
    esp_lcd_panel_io_handle_t io_handle = NULL;
    bsp_display_config_t disp_config = {0};

    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_new(&disp_config, &panel_handle, &io_handle));

    esp_lv_adapter_display_config_t disp_cfg = {
        .panel = panel_handle,
        .panel_io = io_handle,
        .profile = {
            .interface = ESP_LV_ADAPTER_PANEL_IF_RGB,
            .rotation = cfg->rotation,
            .hor_res = BSP_LCD_H_RES,
            .ver_res = BSP_LCD_V_RES,
            /* Ignored by the tear-avoid paths, which draw into the panel's own
             * framebuffers; it only matters for the partial-buffer mode. */
            .buffer_height = 50,
            .use_psram = false,
            /* No PPA on this chip — the adapter's software block rotate is the
             * only option, and at rotation 0 it is a straight row copy. */
            .enable_ppa_accel = false,
            .require_double_buffer = false,
        },
        .tear_avoid_mode = cfg->tear_avoid_mode,
    };

    return esp_lv_adapter_register_display(&disp_cfg);
}

static lv_indev_t *bsp_display_indev_init(const bsp_display_cfg_t *cfg, lv_display_t *disp)
{
    assert(cfg != NULL);

    bsp_touch_config_t touch_config = {
        .swap_xy  = cfg->touch_flags.swap_xy,
        .mirror_x = cfg->touch_flags.mirror_x,
        .mirror_y = cfg->touch_flags.mirror_y,
    };

    /* GT911 cold-boot is timing-sensitive: the first I2C config read can fail
     * while the chip is still latching its address from the INT pin. Retry
     * instead of coming up with no touch at all. */
    esp_err_t err = ESP_FAIL;
    for (int attempt = 1; attempt <= 3; attempt++)
    {
        tp = NULL;
        err = bsp_touch_new(&touch_config, &tp);
        if (err == ESP_OK && tp)
        {
            break;
        }
        ESP_LOGW(TAG, "GT911 init attempt %d/3 failed (%s), retrying",
                 attempt, esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (err != ESP_OK || !tp)
    {
        ESP_LOGE(TAG, "GT911 init gave up after 3 attempts");
        return NULL;
    }

    const esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, tp);
    return esp_lv_adapter_register_touch(&touch_cfg);
}

lv_display_t *bsp_display_start(void)
{
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg  = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation        = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_DIRECT,
    };
    return bsp_display_start_with_config(&cfg);
}

lv_display_t *bsp_display_start_with_config(bsp_display_cfg_t *cfg)
{
    lv_display_t *display;

    assert(cfg != NULL);
    BSP_ERROR_CHECK_RETURN_NULL(esp_lv_adapter_init(&cfg->lv_adapter_cfg));

    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_brightness_init());

    BSP_NULL_CHECK(display = bsp_display_lcd_init(cfg), NULL);

    /* Keep the display alive without touch rather than failing the whole BSP. */
    disp_indev = bsp_display_indev_init(cfg, display);
    if (!disp_indev)
    {
        ESP_LOGW(TAG, "Touch init failed — display will run without touch");
    }

    ESP_ERROR_CHECK(esp_lv_adapter_start());

    disp = display;
    return display;
}

lv_indev_t *bsp_display_get_input_dev(void)
{
    return disp_indev;
}

esp_err_t bsp_display_lock(uint32_t timeout_ms)
{
    return esp_lv_adapter_lock(timeout_ms);
}

void bsp_display_unlock(void)
{
    esp_lv_adapter_unlock();
}

esp_lcd_panel_handle_t bsp_display_get_panel_handle(void)
{
    return panel_handle;
}

esp_lcd_touch_handle_t bsp_display_get_touch_handle(void)
{
    return tp;
}
