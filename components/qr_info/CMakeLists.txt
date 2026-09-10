idf_component_register(
    SRCS "qr_info.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_netif esp_wifi
)

# Hostname the on-device web UI is reachable under (http://<host>.local/).
# Comes from CONFIG_WEB_MDNS_HOSTNAME (main/Kconfig.projbuild) — "android-auto"
# on the AA head units, "vesc-display" on the dashboard-only S3 board. qr_info
# is a leaf component and can't include main/config.h without a cycle (main
# already privately requires qr_info via vesc_ui), so the value is injected
# here; mdns_advertise.c reads the same Kconfig symbol.
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    AA_MDNS_HOSTNAME="${CONFIG_WEB_MDNS_HOSTNAME}")
