idf_component_register(
    SRCS
        "main.c"
        "wifi_manager.c"
        "tcp_server.c"
        "mdns_advertise.c"
        "ota_screen.c"
        "c6_ota.c"
        "aa_frame.c"
        "aa_tls.c"
        "aa_handshake.c"
        "aa_proto.c"
        "aa_service.c"
        "aa_x509_patch.c"
        "bt_link.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_event
        esp_netif
        esp_wifi
        esp_hosted
        esp_timer
        lwip
        mdns
        mbedtls
        driver
        c6_ota_partition
        esp32_p4_wifi6_touch_lcd_4_3
)

# Reroute mbedTLS's UTCTime parser to our offset-tolerant wrapper. Required
# to load the original aasdk head-unit certificate (whose validity uses
# `YYMMDDHHMMSS±HHMM` instead of Zulu) — see aa_x509_patch.c.
#
#   --wrap=mbedtls_x509_get_time : route every internal call to our wrapper
#   -u __wrap_mbedtls_x509_get_time : force the symbol undefined up front so
#       the linker pulls aa_x509_patch.o out of libmain.a — without this it
#       skips main entirely (nothing else references __wrap_*) and then
#       libmbedx509.a fails with "undefined reference".
idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=mbedtls_x509_get_time" APPEND)
idf_build_set_property(LINK_OPTIONS "-u" APPEND)
idf_build_set_property(LINK_OPTIONS "__wrap_mbedtls_x509_get_time" APPEND)
