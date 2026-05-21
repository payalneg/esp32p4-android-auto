idf_component_register(
    SRCS "qr_info.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_netif esp_wifi
)

# Mirror of main/config.h's AA_MDNS_HOSTNAME. qr_info is a leaf component
# and can't include main/config.h without creating a cycle (main already
# privately requires qr_info via vesc_ui). Same pattern as ENABLE_WALL_CLOCK
# in components/vesc_ui — keep this value in lockstep with main/config.h.
target_compile_definitions(${COMPONENT_LIB} PRIVATE AA_MDNS_HOSTNAME="android-auto")
