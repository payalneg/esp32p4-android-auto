cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
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
