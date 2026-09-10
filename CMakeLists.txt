cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# ---- Per-target component set ------------------------------------------------
# The firmware builds for two different chips from one source tree:
#   esp32p4  — the Android Auto head units (Waveshare 4.3", Guition JC4880)
#   esp32s3  — the VESC-dashboard-only board (Waveshare ESP32-S3-Touch-LCD-4)
# Each has its own BSP, and the P4-only blob components (C6 slave firmware,
# BT-agent firmware) make no sense on the S3. Excluding them here — rather than
# only guarding their CMakeLists — also keeps their idf_component.yml manifests
# out of the component-manager solve, so an S3 build doesn't trip over the P4
# BSP's `targets: [esp32p4]` pin or download the audio-codec dependencies it
# pulls in. scripts/build_board.sh exports IDF_TARGET; a plain `idf.py build`
# with nothing set stays the Waveshare P4 build it has always been.
set(_board_target "$ENV{IDF_TARGET}")
if(NOT _board_target)
    set(_board_target "esp32p4")
endif()
if(_board_target STREQUAL "esp32s3")
    list(APPEND EXCLUDE_COMPONENTS
        esp32_p4_wifi6_touch_lcd_4_3
        c6_ota_partition
        bt_agent_fw)
else()
    list(APPEND EXCLUDE_COMPONENTS
        esp32_s3_touch_lcd_4)
endif()

# ---- Per-target dependency lock ---------------------------------------------
# One lock file per chip (dependencies.lock.esp32p4 / .esp32s3): the managed
# dependency set differs (esp_hosted / esp_h264 / esp_wifi_remote are P4-only),
# and a shared lock would be rewritten on every board switch. Must be set
# BEFORE project(): idf_build_process reads this property imperatively while
# preparing the component manager, unlike the compile options appended below.
# scripts/build_board.sh passes -D DEPENDENCIES_LOCK=... explicitly, which the
# component manager itself also reads from the CMake cache variable.
if(NOT DEPENDENCIES_LOCK)
    set(DEPENDENCIES_LOCK "${CMAKE_CURRENT_LIST_DIR}/dependencies.lock.${_board_target}")
endif()
idf_build_set_property(DEPENDENCIES_LOCK "${DEPENDENCIES_LOCK}")

project(esp32p4_android_auto)

# main/lv_conf.h is the single source of truth for LVGL configuration.
# It must reach every component that does '#include "lv_conf.h"' or
# '#include "lvgl.h"' (BSP, esp_lv_adapter, vesc_ui, Super_VESC_Display
# generated/custom files). Putting main/ on the global include path is
# the simplest way to satisfy that. Pair this with CONFIG_LV_CONF_SKIP=n
# in sdkconfig so the LVGL package picks up our header instead of
# generating its own from Kconfig.
idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_SOURCE_DIR}/main" APPEND)

# Auto-tag the firmware artifact with PROJECT_VER (read from version.txt).
# IDF emits build/esp32p4_android_auto.bin; we keep that for `idf.py flash`
# and additionally drop a versioned twin alongside it so release artifacts
# don't need manual renaming. POST_BUILD on gen_project_binary fires right
# after the raw .bin is produced.
add_custom_command(
    TARGET gen_project_binary POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin"
            "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-v${PROJECT_VER}.bin"
    COMMENT "Tagged: ${CMAKE_PROJECT_NAME}-v${PROJECT_VER}.bin"
    VERBATIM)
