# GUI Guider VESC dashboard, sourced **in-place** from
# <repo>/Super_VESC_Display/{generated,custom}/. There are no copies in
# this component on purpose — when GUI Guider regenerates the project,
# the only sync step needed is "rebuild ESP-IDF". Embed-only patches
# (g_kb_top_layer struct field, ta_event_cb NULL-guard, -Werror fixes,
# VESC_UI_PROFILE-gated timing markers) live in those Super_VESC_Display
# files; they are additive and the desktop simulator keeps building.
#
# LV_REALDEVICE is set below (post-register). It switches settings_wrapper.c
# off SIMULATOR_MODE onto the NVS-backed dev_settings, and enables the
# vesc_limits / vesc_battery_calc include sites in custom.c. Both backends
# are stubbed in components/dev_settings until vesc_app lands.

set(SVD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../Super_VESC_Display")

# Pull every generated/custom .c, then drop _grid_480_800x480.c — that's
# a 1.15 MB invisible (bg_img_opa=0) background that doesn't fit the
# factory partition without bloat.
file(GLOB SVD_GENERATED  "${SVD_ROOT}/generated/*.c")
list(FILTER SVD_GENERATED EXCLUDE REGEX ".*_grid_480_800x480\\.c$")
file(GLOB SVD_FONTS      "${SVD_ROOT}/generated/guider_fonts/*.c")
file(GLOB SVD_IMAGES     "${SVD_ROOT}/generated/images/*.c")
list(FILTER SVD_IMAGES   EXCLUDE REGEX ".*_grid_480_800x480\\.c$")
file(GLOB SVD_CUSTOM     "${SVD_ROOT}/custom/*.c")

idf_component_register(
    SRCS
        ${SVD_GENERATED}
        ${SVD_FONTS}
        ${SVD_IMAGES}
        ${SVD_CUSTOM}
        "vesc_ui.c"
    INCLUDE_DIRS
        "include"
        "${SVD_ROOT}/generated"
        "${SVD_ROOT}/custom"
    REQUIRES
        lvgl__lvgl
    PRIV_REQUIRES
        esp_timer
        dev_settings
        log_capture
)

# LV_REALDEVICE flips settings_wrapper.c from in-RAM SIMULATOR_MODE to the
# real NVS-backed dev_settings backend, and turns on the vesc_limits /
# vesc_battery_calc include sites in custom.c. Stubs in components/dev_settings
# keep the build linkable until the vesc_app backend lands.
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_REALDEVICE)

# Mirror of main/config.h's ENABLE_WALL_CLOCK — vesc_ui can't include
# main/config.h cleanly, so the flag is duplicated here. Must stay in
# sync. When 0, custom_init hides dashboard_cur_time_label and the
# Time row in Settings is skipped.
target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_WALL_CLOCK=0)

# Generated images use:
#   #if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#   #include "lvgl.h"
#   #else
#   #include "lvgl/lvgl.h"
#   #endif
# IDF's lvgl component exposes "lvgl.h", not "lvgl/lvgl.h".
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)

# Per-widget timing markers in setup_scr_dashboard.c (Super_VESC_Display).
# Off by default; flip to ON to log per-widget cost via ESP_LOGI("svd_prof").
option(VESC_UI_PROFILE "Profile per-widget LVGL build time" ON)
if(VESC_UI_PROFILE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE VESC_UI_PROFILE)
endif()

# The dashboard generated/_grid_480_800x480.c was excluded from the build,
# but gui_guider.h still has LV_IMG_DECLARE(_grid_480_800x480) and the
# original setup_scr_dashboard.c sets it as bg_img with opa=0. Provide a
# tiny stub so the linker is happy — the image is never drawn.
target_sources(${COMPONENT_LIB} PRIVATE "grid_480_stub.c")
