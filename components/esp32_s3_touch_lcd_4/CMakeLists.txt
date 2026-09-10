# ESP32-S3 only (see EXCLUDE_COMPONENTS in the root CMakeLists). Registering
# empty on any other chip keeps a stray build from pulling this board's
# dependencies. IDF_TARGET is available in the early requirements pass;
# CONFIG_* symbols are not.
if(NOT "${IDF_TARGET}" STREQUAL "esp32s3")
    idf_component_register()
    return()
endif()

idf_component_register(
    SRCS "esp32_s3_touch_lcd_4.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_sdmmc esp_lcd
    PRIV_REQUIRES esp_timer spiffs esp_psram fatfs
)
