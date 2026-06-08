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

# ── Cockpit Antonio fonts: subsetted from the TTF at build time ──
# GUI Guider exports Antonio with the full ASCII range (95 glyphs) + 60 icons
# at every size — Antonio@200 alone is 1.4 MB of flash, and the firmware only
# renders digits / a few letters. So we DROP the GUI Guider Antonio copies from
# the build and regenerate hard-subset versions into the build dir via
# tools/regen_cockpit_fonts.py. Because the compiled fonts live in the build dir
# and are derived from the .ttf, a GUI Guider "Generate Code" (which overwrites
# generated/guider_fonts/) can never reintroduce the bloated copies here.
# Needs node/npx; when absent the tool copies the committed subsets as fallback
# so CI / clean checkouts still link. Keep COCKPIT_FONT_NAMES in sync with JOBS
# in tools/regen_cockpit_fonts.py.
# Also drop the DSEG7/DSEG14 segment fonts (amber theme): GUI Guider's full
# export is even larger (DSEG7_200 = 9.7 MB; DSEG14 carries A-Z too).
# regen_cockpit_fonts.py rebuilds subsets (digit-only for the big numeric sizes,
# full charset for the small label sizes) into the build dir, same mechanism as
# Antonio. DSEG7 is no longer referenced by the amber screen (now all-DSEG14) but
# kept here so the subset still builds if any screen reintroduces it.
list(FILTER SVD_FONTS EXCLUDE REGEX "/lv_font_(Antonio_Regular|DSEG7Classic_Regular|DSEG14Classic_Regular)_[0-9]+\\.c$")

set(COCKPIT_FONT_NAMES
    lv_font_Antonio_Regular_200 lv_font_Antonio_Regular_50
    lv_font_Antonio_Regular_40  lv_font_Antonio_Regular_22
    lv_font_Antonio_Regular_64  lv_font_Antonio_Regular_32
    lv_font_DSEG7Classic_Regular_200 lv_font_DSEG7Classic_Regular_64
    lv_font_DSEG7Classic_Regular_40  lv_font_DSEG7Classic_Regular_32
    lv_font_DSEG7Classic_Regular_24
    # DSEG14 Classic — the amber dashboard's single 14-segment font (labels +
    # numbers). Letter sizes (full charset) + numeric-only big sizes.
    lv_font_DSEG14Classic_Regular_11  lv_font_DSEG14Classic_Regular_12
    lv_font_DSEG14Classic_Regular_16  lv_font_DSEG14Classic_Regular_18
    lv_font_DSEG14Classic_Regular_20  lv_font_DSEG14Classic_Regular_22
    lv_font_DSEG14Classic_Regular_24  lv_font_DSEG14Classic_Regular_25
    lv_font_DSEG14Classic_Regular_26  lv_font_DSEG14Classic_Regular_50
    lv_font_DSEG14Classic_Regular_64  lv_font_DSEG14Classic_Regular_180
    lv_font_DSEG14Classic_Regular_200)
set(COCKPIT_FONT_DIR "${CMAKE_CURRENT_BINARY_DIR}/cockpit_fonts")
set(COCKPIT_FONT_SRCS "")
foreach(_cf ${COCKPIT_FONT_NAMES})
    list(APPEND COCKPIT_FONT_SRCS "${COCKPIT_FONT_DIR}/${_cf}.c")
endforeach()
# The add_custom_command that produces these lives AFTER idf_component_register
# below — add_custom_command is not available in the script-mode pass ESP-IDF
# uses to extract component requirements, and everything before the register
# call runs in that pass too.

idf_component_register(
    SRCS
        ${SVD_GENERATED}
        ${SVD_FONTS}
        ${COCKPIT_FONT_SRCS}
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
        qr_info
        vesc_config
        vesc_can
        trip_log
)

# Build-time subsetting of the cockpit Antonio fonts (see the comment block
# above the glob). Placed here because add_custom_command is not scriptable and
# must run only in the normal configure pass, after idf_component_register.
set(_regen_tool  "${SVD_ROOT}/tools/regen_cockpit_fonts.py")
set(_antonio_ttf "${SVD_ROOT}/import/font/Antonio-Regular.ttf")
set_source_files_properties(${COCKPIT_FONT_SRCS} PROPERTIES GENERATED TRUE)
add_custom_command(
    OUTPUT ${COCKPIT_FONT_SRCS}
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${COCKPIT_FONT_DIR}"
    COMMAND python3 "${_regen_tool}"
            --out-dir "${COCKPIT_FONT_DIR}"
            --fallback-from "${SVD_ROOT}/generated/guider_fonts"
    DEPENDS "${_antonio_ttf}" "${_regen_tool}"
    COMMENT "Subsetting cockpit Antonio fonts -> ${COCKPIT_FONT_DIR}"
    VERBATIM)

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

# Antonio_Regular_200 (the giant speedometer font) has 32-bit glyph
# offsets and refuses to compile unless this is set — see the #error
# at the bottom of the generated .c. Match main/lv_conf.h here in case
# the build picks up a different lv_conf via global include order.
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_FONT_FMT_TXT_LARGE=1)

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
