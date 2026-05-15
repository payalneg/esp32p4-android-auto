idf_component_register(
    SRCS "main.c" "wifi_setup_proto.c" "uart_link.c"
    INCLUDE_DIRS "."
    REQUIRES bt nvs_flash driver
)
