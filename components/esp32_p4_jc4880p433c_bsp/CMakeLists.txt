idf_component_register(
    SRCS "src/jc4880p433c_bsp.c"
         "src/bsp_display.c"
         "src/bsp_touch.c"
         "src/bsp_i2c.c"
         "src/bsp_camera.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd_touch esp_lvgl_port
    PRIV_REQUIRES spiffs esp_lcd esp_lcd_st7701 esp_lcd_touch_gt911 esp_driver_i2c esp_driver_ledc esp_hw_support
)
