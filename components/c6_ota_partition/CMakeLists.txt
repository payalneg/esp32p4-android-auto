# ESP32-P4 only (see EXCLUDE_COMPONENTS in the root CMakeLists). Registering
# empty on any other chip keeps a stray build from tripping over this
# component's P4-specific dependencies or embedded blobs. IDF_TARGET is
# available in the early requirements pass; CONFIG_* symbols are not.
if(NOT "${IDF_TARGET}" STREQUAL "esp32p4")
    idf_component_register()
    return()
endif()

set(_slave_gz "slave_fw_bin/network_adapter.bin.gz")
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_slave_gz}")
    message(FATAL_ERROR
        "C6 OTA: ${CMAKE_CURRENT_LIST_DIR}/${_slave_gz} not found — "
        "drop the C6 slave firmware (network_adapter.bin) at "
        "${CMAKE_CURRENT_LIST_DIR}/slave_fw_bin/ and run "
        "tools/pack_fw_blobs.sh to gzip it.")
endif()

idf_component_register(
    SRCS "ota_partition.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_hosted bootloader_support esp_app_format espressif__zlib esp_psram
    EMBED_FILES "${_slave_gz}"
)
