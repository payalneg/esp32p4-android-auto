/*
    Copyright 2026 Super VESC Display

    Reference dashboard theme — a deliberately minimal, hand-written screen
    (big centred speed, a battery bar, live current) used to:
      • validate the theme switcher end-to-end (two entries in the dropdown,
        a genuinely different layout to switch to), and
      • serve as a copy-paste TEMPLATE for real themes.

    PRODUCTION themes are meant to be authored in GUI Guider as their own
    screen (see the recipe in dashboard_theme.h). This file shows the same
    shape a GUI-Guider theme module takes: a create() that builds the screen
    and wires the shared chrome, a render-ops table, and a register() hook.
    It is wired up purely for demonstration — delete the theme_ref_register()
    call in custom_init_once() (custom.c) to drop it, the framework is
    unaffected.

    Reuses only fonts already compiled into the image (Antonio_Regular_64 +
    Montserrat) so it adds nothing to the flash budget.
*/

#include "theme_ref.h"
#include "dashboard_theme.h"
#include "settings_wrapper.h"
#include "gui_guider.h"   /* guider_ui, setup_scr_settings, ui_load_scr_animation */

#include <stdio.h>

/* Palette — kept local, intentionally distinct from the cockpit's. */
#define REF_BG        lv_color_hex(0x05070A)
#define REF_ACCENT    lv_color_hex(0x39B6FF)   /* cyan */
#define REF_TEXT      lv_color_hex(0xE6EEF2)
#define REF_DIM       lv_color_hex(0x6A767D)
#define REF_BAR_BG    lv_color_hex(0x172127)

static lv_obj_t *s_screen;
static lv_obj_t *s_speed;
static lv_obj_t *s_speed_unit;
static lv_obj_t *s_batt_bar;
static lv_obj_t *s_batt_lbl;
static lv_obj_t *s_current;

/* Shared chrome: open the Settings screen. Mirrors the generated dashboard→
 * settings nav (events_init.c) so the user can reach Settings (and switch the
 * theme back) from here. */
static void ref_settings_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_load_scr_animation(&guider_ui, &guider_ui.settings, guider_ui.settings_del,
                          &guider_ui.dashboard_Classic_del, setup_scr_settings,
                          LV_SCR_LOAD_ANIM_NONE, 200, 200, false, false);
}

static lv_obj_t *ref_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    s_screen = scr;
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    dash_set_bg_color(scr, REF_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);

    /* Title marker so the theme is unmistakable at a glance. */
    lv_obj_t *title = lv_label_create(scr);
    dash_label_set(title, "MINIMAL");
    dash_set_text_color(title, REF_DIM, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    /* Big centred speed. */
    s_speed = lv_label_create(scr);
    dash_label_set(s_speed, "0");
    dash_set_text_color(s_speed, REF_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_speed, &lv_font_Antonio_Regular_64, LV_PART_MAIN);
    lv_obj_align(s_speed, LV_ALIGN_CENTER, 0, -40);

    s_speed_unit = lv_label_create(scr);
    dash_label_set(s_speed_unit, settings_wrapper_speed_unit());
    dash_set_text_color(s_speed_unit, REF_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_speed_unit, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align_to(s_speed_unit, s_speed, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    /* Live current under the speed, small. */
    s_current = lv_label_create(scr);
    dash_label_set(s_current, "0.0 A");
    dash_set_text_color(s_current, REF_DIM, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_current, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(s_current, LV_ALIGN_CENTER, 0, 40);

    /* Battery bar + percent at the bottom. */
    s_batt_bar = lv_bar_create(scr);
    lv_obj_set_size(s_batt_bar, 600, 26);
    lv_obj_align(s_batt_bar, LV_ALIGN_BOTTOM_MID, 0, -64);
    lv_bar_set_range(s_batt_bar, 0, 100);
    lv_bar_set_value(s_batt_bar, 0, LV_ANIM_OFF);
    dash_set_bg_color(s_batt_bar, REF_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_batt_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s_batt_bar, 6, LV_PART_MAIN);
    dash_set_bg_color(s_batt_bar, REF_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_batt_bar, 6, LV_PART_INDICATOR);

    s_batt_lbl = lv_label_create(scr);
    dash_label_set(s_batt_lbl, "0%");
    dash_set_text_color(s_batt_lbl, REF_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_batt_lbl, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align_to(s_batt_lbl, s_batt_bar, LV_ALIGN_OUT_TOP_MID, 0, -4);

    /* Settings entry point (shared chrome). */
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 140, 44);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -12);
    dash_set_bg_color(btn, REF_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, ref_settings_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    dash_label_set(btn_lbl, "SETTINGS");
    dash_set_text_color(btn_lbl, REF_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(btn_lbl);

    return scr;
}

static void ref_destroy(void)
{
    /* Screen object freed by the switcher; just drop our references. */
    s_screen = NULL;
    s_speed = s_speed_unit = s_batt_bar = s_batt_lbl = s_current = NULL;
}

/* --- render ops (dedup is centralized in dash_label_set: at 10 Hz the
 *     label text is cheap, but under full_refresh a needless invalidation
 *     re-renders the whole screen, so skip writing unchanged text) --- */

static void ref_speed(float kmh)
{
    if (!s_speed) return;
    char t[12];
    snprintf(t, sizeof(t), "%d", (int)settings_wrapper_speed_to_display(kmh));
    dash_label_set(s_speed, t);
}

static void ref_current(float a)
{
    if (!s_current) return;
    char t[16];
    snprintf(t, sizeof(t), "%.1f A", a);
    dash_label_set(s_current, t);
}

static void ref_battery_proc(float pct)
{
    int v = (int)pct;
    if (v < 0) v = 0; else if (v > 100) v = 100;
    if (s_batt_bar) lv_bar_set_value(s_batt_bar, v, LV_ANIM_OFF);
    if (s_batt_lbl) {
        char t[8];
        snprintf(t, sizeof(t), "%d%%", v);
        dash_label_set(s_batt_lbl, t);
    }
}

static void ref_units_changed(void)
{
    if (s_speed_unit) dash_label_set(s_speed_unit, settings_wrapper_speed_unit());
}

static const dashboard_theme_ops_t ref_ops = {
    .speed         = ref_speed,
    .current       = ref_current,
    .battery_proc  = ref_battery_proc,
    .units_changed = ref_units_changed,
    /* Everything else NULL: this minimal theme intentionally shows nothing
     * more — the dispatcher skips unset fields. */
};

static const dashboard_theme_t ref_theme = {
    .id         = "ref",
    .name       = "Minimal (ref)",
    .create     = ref_create,
    .destroy    = ref_destroy,
    .music_tile = NULL,        /* no music tile in this theme */
    .ops        = &ref_ops,
};

void theme_ref_register(void)
{
    dashboard_theme_register(&ref_theme);
}
