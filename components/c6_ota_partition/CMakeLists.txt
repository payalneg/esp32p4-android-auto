set(_slave_bin "slave_fw_bin/network_adapter.bin")
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_slave_bin}")
    message(FATAL_ERROR
        "C6 OTA: ${CMAKE_CURRENT_LIST_DIR}/${_slave_bin} not found — "
        "drop the C6 slave firmware there before building")
endif()

idf_component_register(
    SRCS "ota_partition.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_hosted bootloader_support esp_app_format
    EMBED_FILES "${_slave_bin}"
)
