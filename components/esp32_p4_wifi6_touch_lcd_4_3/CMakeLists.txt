# ESP32-P4 only (see EXCLUDE_COMPONENTS in the root CMakeLists). Registering
# empty on any other chip keeps a stray build from tripping over this
# component's P4-specific dependencies or embedded blobs. IDF_TARGET is
# available in the early requirements pass; CONFIG_* symbols are not.
if(NOT "${IDF_TARGET}" STREQUAL "esp32p4")
    idf_component_register()
    return()
endif()

idf_component_register(
    SRCS "esp32_p4_wifi6_touch_lcd_4_3.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES driver
    PRIV_REQUIRES esp_lcd usb spiffs fatfs
)
