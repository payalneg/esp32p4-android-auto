idf_component_register(
    SRCS
        "main.c"
        "wifi_manager.c"
        "tcp_server.c"
        "aa_reconnect.c"
        "mdns_advertise.c"
        "display_init.c"
        "idle_screen.c"
        "ota_screen.c"
        "c6_ota.c"
        "ota_http.c"
        "files_http.c"
        "lisp_http.c"
        "aa_frame.c"
        "aa_tls.c"
        "aa_handshake.c"
        "aa_proto.c"
        "aa_service.c"
        "aa_x509_patch.c"
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
        "music_info_view.c"
        "fonts/aabridge_font_24.c"
        "fonts/aabridge_font_32.c"
        "fonts/aabridge_font_38.c"
        "fonts/aabridge_font_48.c"
        "bt_link.c"
        "bt_agent_ota.c"
        "h264_pipe.c"
        "display_video.c"
        "aa_overlay.c"
        "touch_input.c"
        "aa_overclock.c"
        "ui_mode.c"
        "vesc_ui_updater.c"
        "vesc_sim.c"
        "vbat_routing.c"
        "files_screen.c"
        "splash_screen.c"
        "debug_uart_bridge.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_event
        esp_netif
        esp_wifi
        esp_hw_support
        hal
        esp_hosted
        bt
        esp_timer
        esp_partition
        esp_ringbuf
        esp_lcd
        esp_mm
        esp_driver_ppa
        esp_driver_jpeg
        lwip
        mdns
        mbedtls
        driver
        console
        app_update
        esp_http_server
        c6_ota_partition
        bt_agent_fw
        esp32_p4_wifi6_touch_lcd_4_3
        espressif__esp_h264
        h264dec
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

# Phone-free video self-test clip (CONFIG_AA_VIDEO_SELFTEST). Embedded as-is;
# the file is a dev asset (gitignored), regenerate with the ffmpeg line in
# scripts or aa_selftest.c's header comment.
if(CONFIG_AA_VIDEO_SELFTEST)
    target_add_binary_data(${COMPONENT_LIB}
        "${CMAKE_CURRENT_LIST_DIR}/aa_selftest.h264" BINARY)
endif()

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
