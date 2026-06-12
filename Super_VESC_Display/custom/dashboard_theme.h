/*
    Copyright 2026 Super VESC Display

    Dashboard theme framework.

    The VESC dashboard can be drawn in several visually distinct layouts
    ("themes") — not just a recolour, but genuinely different screens (a
    segmented cockpit, a round speedometer, a minimal readout, …). Each theme
    is a self-contained screen plus a table of render callbacks whose
    signatures mirror the canonical data feed (update_speed, update_battery…).

    The data pump (main/vesc_ui_updater.c) and the demo loop keep calling the
    global update_*() functions; those are dispatchers (see dashboard_theme.c)
    that forward to the active theme's ops. Switching theme rebuilds the target
    screen, repoints the dashboard slot (guider_ui.dashboard_Classic) and frees the
    previous screen — all on the LVGL thread.

    Themes are registered once at boot (custom_init_once). The active index is
    persisted in NVS (dev_settings "dash_theme") and chosen from the Settings
    screen.

    --- Adding a GUI-Guider-authored theme ------------------------------------
      1. In GUI Guider add a screen `dashboardN` and lay it out. REUSE the
         existing fonts (Antonio / Montserrat subsets) — new TTFs bloat the
         image and overflow the 6 MB OTA slot on jc4880.
      2. Generate code: you get setup_scr_dashboardN() and guider_ui.dashboardN*
      3. Add theme_dashboardN.c: ops fns that read guider_ui.dashboardN_*; a
         create() that calls setup_scr_dashboardN(&guider_ui) + the shared
         chrome wiring + returns guider_ui.dashboardN; screen()/music_tile();
         and a theme_dashboardN_register() that calls dashboard_theme_register().
      4. Call theme_dashboardN_register() from custom_init_once() in custom.c.
      5. The Settings dropdown and NVS persistence pick it up from the registry
         automatically.
*/

#ifndef DASHBOARD_THEME_H_
#define DASHBOARD_THEME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ---- Invalidation-deduping setters --------------------------------------
 * The display runs in DOUBLE_FULL / full_refresh (TRIPLE_PARTIAL freezes on
 * P4 — see main/display_init.c), so ANY widget invalidation re-renders the
 * whole 800×480 screen in software. The 10 Hz data pump re-sets ~15 labels
 * every cycle even when the displayed digits are identical, forcing a full
 * re-render ~10×/s at rest. These helpers skip the LVGL call (and its
 * invalidation) when the value is unchanged, so a steady dashboard produces
 * zero re-renders. Drop-in: same arg order as the lv_* funcs they replace.
 * Only valid for default-state (part-only) selectors — all dashboard setters
 * use LV_PART_MAIN/INDICATOR, never LV_STATE_* bits. */
static inline void dash_label_set(lv_obj_t *label, const char *txt)
{
    if (!label) return;
    const char *cur = lv_label_get_text(label);
    if (cur && strcmp(cur, txt) == 0) return;
    lv_label_set_text(label, txt);
}

static inline void dash_set_bg_color(lv_obj_t *obj, lv_color_t c, lv_style_selector_t sel)
{
    if (!obj) return;
    if (lv_obj_get_style_bg_color(obj, sel).full == c.full) return;
    lv_obj_set_style_bg_color(obj, c, sel);
}

static inline void dash_set_text_color(lv_obj_t *obj, lv_color_t c, lv_style_selector_t sel)
{
    if (!obj) return;
    if (lv_obj_get_style_text_color(obj, sel).full == c.full) return;
    lv_obj_set_style_text_color(obj, c, sel);
}

/* Render operations — same canonical values the data feed produces. Every hook
 * is optional: a NULL slot means "this theme doesn't show that field" and the
 * dispatcher silently skips it. */
typedef struct {
    void (*speed)(float kmh);
    void (*current)(float a);
    void (*battery_proc)(float pct);
    void (*battery_voltage)(float v);
    void (*battery_temp)(float c);
    void (*temp_fet)(float c);
    void (*temp_motor)(float c);
    void (*trip)(float km);
    void (*range)(float km);
    void (*odometer)(float km);
    void (*amp_hours)(float ah);
    void (*uptime)(uint32_t ms);
    void (*fps)(int fps);
    void (*ble_status)(bool connected);
    void (*esc_connection_status)(bool connected);
    void (*cruise_control_status)(bool active);
    void (*cruise_speed)(float kmh);
    void (*mode_text)(uint8_t mode);
    void (*units_changed)(void);
    void (*cur_time)(int hour, int minute, int second);
    void (*cur_time_hm)(int hour, int minute);
    void (*hide_cur_time)(void);
    void (*hide_mode_text)(void);   /* hide the ride-mode label (no Lisp data) */
    void (*navigation_icon)(const uint8_t *img_data, uint32_t data_size,
                            uint16_t width, uint16_t height, lv_img_cf_t cf);
    void (*navigation_text)(const char *text);
    void (*music_text)(const char *text);
} dashboard_theme_ops_t;

typedef struct {
    const char *id;                 /* stable machine id, e.g. "cockpit"      */
    const char *name;               /* human label shown in the dropdown      */
    lv_obj_t *(*create)(void);      /* build the screen (+ chrome), return root */
    void      (*destroy)(void);     /* free per-theme state; the switcher frees
                                       the screen object itself (may be NULL)  */
    lv_obj_t *(*music_tile)(void);  /* attach point for music_info_view, NULL  */
    const dashboard_theme_ops_t *ops;
} dashboard_theme_t;

/* Invoked on the LVGL thread right after a theme is built/switched, so the app
 * can re-attach chrome that lives outside the theme (the music tile). The
 * music tile may be NULL when the active theme has none. Registered by main on
 * device; absent in the simulator. */
typedef void (*dashboard_theme_switch_cb_t)(lv_obj_t *new_screen,
                                             lv_obj_t *music_tile);

void dashboard_theme_register(const dashboard_theme_t *t);
int  dashboard_theme_count(void);
const dashboard_theme_t *dashboard_theme_get(int idx);
int  dashboard_theme_active_index(void);
lv_obj_t *dashboard_theme_active_screen(void);
void dashboard_theme_set_switch_cb(dashboard_theme_switch_cb_t cb);

/* Build the theme at idx and make it active. Used on device (vesc_ui_init)
 * with the NVS-saved index; calls the theme's create(). idx is clamped into
 * [0, count). No-op if no theme is registered. */
void dashboard_theme_build(int idx);

/* Adopt an already-built screen as the active theme WITHOUT calling create() —
 * used by the simulator path where setup_ui() already built+loaded the cockpit
 * before custom_init() runs. */
void dashboard_theme_adopt(int idx);

/* Live-switch to the theme at idx. Must run on the LVGL thread (the Settings
 * dropdown callback already does). Rebuilds the target screen, repoints the
 * dashboard slot, frees the previous screen and fires the switch callback.
 * No-op if idx is already active or out of range. */
void dashboard_theme_set(int idx);

#ifdef __cplusplus
}
#endif
#endif /* DASHBOARD_THEME_H_ */
