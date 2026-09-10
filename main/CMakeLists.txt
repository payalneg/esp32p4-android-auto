# ---- Source / requirement split by chip -------------------------------------
# Everything that is not Android Auto builds for both chips. The AA tract (TCP
# server, TLS handshake, protobuf service, H.264 pipe, panel video sink, AA idle
# screen, external BT-agent link) and the P4-only hardware helpers (C6 OTA,
# 400 MHz overclock, VBAT routing, the AA mic input) are ESP32-P4 only — see
# CONFIG_AA_ENABLE in Kconfig.projbuild. Keep this list and that symbol in
# lockstep: the sources are dropped here, the call sites in main.c are dropped
# by the #if.
set(MAIN_SRCS
    "main.c"
    "wifi_manager.c"
    "mdns_advertise.c"
    "display_init.c"
    "img_jpeg.c"
    "ota_screen.c"
    "ota_http.c"
    "files_http.c"
    "lisp_http.c"
    "ble_host.c"
    "ble_cadence_client.c"
    "ble_central_arb.c"
    "ble_speed_client.c"
    "ble_nus.c"
    "notif_bridge.c"
    "pas.c"
    "speed_sensor.c"
    "ble_ota.c"
    "ble_files.c"
    "notif_toast.c"
    "charge_prompt.c"
    "music_info_view.c"
    "fonts/aabridge_font_24.c"
    "fonts/aabridge_font_32.c"
    "fonts/aabridge_font_38.c"
    "fonts/aabridge_font_48.c"
    "touch_input.c"
    "ui_mode.c"
    "vesc_ui_updater.c"
    "vesc_sim.c"
    "files_screen.c"
    "splash_screen.c"
    "debug_uart_bridge.c"
)

set(MAIN_REQUIRES
    nvs_flash
    spi_flash
    esp_event
    esp_netif
    esp_wifi
    esp_hw_support
    hal
    bt
    esp_timer
    esp_partition
    esp_ringbuf
    esp_lcd
    esp_mm
    lwip
    mdns
    mbedtls
    driver
    console
    app_update
    esp_http_server
    espressif__esp_lvgl_adapter
    vesc_ui
    vesc_can
    vesc_config
    dev_settings
    log_capture
    trip_log
    joltwallet__littlefs
    json
)

if("${IDF_TARGET}" STREQUAL "esp32s3")
    list(APPEND MAIN_REQUIRES
        esp32_s3_touch_lcd_4
        espressif__esp_new_jpeg)
else()
    list(APPEND MAIN_SRCS
        "flash_shade.c"
        "tcp_server.c"
        "aa_frame.c"
        "aa_tls.c"
        "aa_handshake.c"
        "aa_proto.c"
        "aa_service.c"
        "aa_x509_patch.c"
        "aa_link_status.c"
        "aa_reconnect.c"
        "aa_overlay.c"
        "h264_pipe.c"
        "display_video.c"
        "idle_screen.c"
        "bt_link.c"
        "bt_agent_ota.c"
        "c6_ota.c"
        "mic_capture.c"
        "aa_overclock.c"
        "vbat_routing.c")
    list(APPEND MAIN_REQUIRES
        esp_hosted
        esp_driver_ppa
        esp_driver_jpeg
        c6_ota_partition
        bt_agent_fw
        esp32_p4_wifi6_touch_lcd_4_3
        espressif__esp_h264)
endif()

idf_component_register(
    SRCS ${MAIN_SRCS}
    INCLUDE_DIRS "."
    REQUIRES ${MAIN_REQUIRES}
)

# Web LISP editor page (main/web/lisp_editor.html) — gzipped at build time and
# embedded in .rodata, served with Content-Encoding: gzip by lisp_http.c. Kept
# as a real .html file instead of a C string literal (the way ota_http.c and
# files_http.c hold their much smaller pages) so it stays editable; gzip takes
# ~50 KiB of markup down to ~15 KiB of flash.
#
# target_add_binary_data rather than EMBED_FILES in the registration above:
# EMBED_FILES needs the file to exist at configure time, and this one is
# generated. add_dependencies makes the gzip run before the object is built.
set(_lisp_web_src "${CMAKE_CURRENT_LIST_DIR}/web/lisp_editor.html")
set(_lisp_web_gz  "${CMAKE_CURRENT_BINARY_DIR}/lisp_editor.html.gz")
# (scripts/compress_gz.py rather than `gzip -9 -n > out`: add_custom_command
#  runs the command directly, without a shell to interpret the redirection —
#  and Windows has no gzip at all. The script reproduces `-n` exactly. ${_py} is
#  the IDF venv interpreter; a bare `python3`/`python` is missing on one host OS
#  or the other.)
idf_build_get_property(_py PYTHON)
add_custom_command(
    OUTPUT "${_lisp_web_gz}"
    COMMAND "${_py}" "${CMAKE_CURRENT_LIST_DIR}/../scripts/compress_gz.py"
            "${_lisp_web_src}" "${_lisp_web_gz}"
    DEPENDS "${_lisp_web_src}" "${CMAKE_CURRENT_LIST_DIR}/../scripts/compress_gz.py"
    COMMENT "Compressing web LISP editor -> lisp_editor.html.gz"
    VERBATIM)
add_custom_target(lisp_web_gz DEPENDS "${_lisp_web_gz}")
add_dependencies(${COMPONENT_LIB} lisp_web_gz)
target_add_binary_data(${COMPONENT_LIB} "${_lisp_web_gz}" BINARY)

# Reroute mbedTLS's UTCTime parser to our offset-tolerant wrapper. Required
# to load the original aasdk head-unit certificate (whose validity uses
# `YYMMDDHHMMSS±HHMM` instead of Zulu) — see aa_x509_patch.c. AA-only, so the
# wrap goes away with the source file on the S3 board (the -u would otherwise
# ask the linker for a symbol nothing defines).
#
#   --wrap=mbedtls_x509_get_time : route every internal call to our wrapper
#   -u __wrap_mbedtls_x509_get_time : force the symbol undefined up front so
#       the linker pulls aa_x509_patch.o out of libmain.a — without this it
#       skips main entirely (nothing else references __wrap_*) and then
#       libmbedx509.a fails with "undefined reference".
if(NOT "${IDF_TARGET}" STREQUAL "esp32s3")
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=mbedtls_x509_get_time" APPEND)
    idf_build_set_property(LINK_OPTIONS "-u" APPEND)
    idf_build_set_property(LINK_OPTIONS "__wrap_mbedtls_x509_get_time" APPEND)
endif()

# flash_shade.c sits between every caller and the flash driver's write/erase
# entry points so the backlight can go dark for the duration of the op (each
# one stalls the DSI scan-out — see main/flash_shade.h). The wrap applies at
# the final link, so NVS, littlefs, esp_partition and OTA are all routed
# through it without any call-site changes.
#
# P4 only. On the S3 board the panel survives a flash write on its own
# (CONFIG_SPIRAM_FETCH_INSTRUCTIONS/RODATA keep the cache the LCD DMA reads
# through alive), so there is nothing to shade — and asking the linker to wrap
# symbols whose wrapper is not compiled in fails the link.
if(NOT "${IDF_TARGET}" STREQUAL "esp32s3")
    target_link_options(${COMPONENT_LIB} INTERFACE
        "-Wl,--wrap=esp_flash_write"
        "-Wl,--wrap=esp_flash_erase_region"
        "-Wl,--wrap=esp_flash_erase_chip")
endif()
