/*
    Copyright 2026 Super VESC Display

    "Cockpit (Amber)" dashboard theme — the cyberpunk-amber reskin of the
    cockpit (design_handoff_vesc_dashboard/, Variant A). Same information layout
    as the green cockpit, recoloured to the amber palette; the numeric readouts
    use the real 7-segment DSEG7 font (subset built at compile time).

    Authored as its own GUI Guider screen `dashboard_amber`
    (Super_VESC_Display.guiguider) → generated/setup_scr_dashboard_amber.c +
    guider_ui.dashboard_amber_*. This module is the C side of the recipe in
    dashboard_theme.h: a create() that builds the screen and wires shared
    chrome, a render-ops table, and a register() hook.

    It does NOT touch the cockpit code — it is a self-contained sibling of
    theme_ref.c, reading only guider_ui.dashboard_amber_* widgets and the
    public settings_wrapper_* API.
*/

#include "theme_dashboard_amber.h"
#include "dashboard_theme.h"
#include "settings_wrapper.h"
#include "gui_guider.h"   /* guider_ui, setup_scr_dashboard_amber, setup_scr_settings, ui_load_scr_animation */

#include <stdio.h>
#include <math.h>

/* Amber palette — mirrors design_handoff_vesc_dashboard/styles.css, blended
 * over the near-black background. Distinct from the cockpit's green. */
#define AMBER_SEG_OFF  lv_color_hex(0x241405)  /* --amber-dim : unlit bar cell */
#define AMBER_ACCENT   lv_color_hex(0xFF7A1A)  /* --amber     : lit bar / drive */
#define AMBER_BRIGHT   lv_color_hex(0xFFB35C)  /* --amber-bright : battery ok    */
#define AMBER_WARN     lv_color_hex(0xFFB02E)  /* caution 20–50 %                */
#define AMBER_DANGER   lv_color_hex(0xFF3B2F)  /* --red : critical / over-temp   */
#define AMBER_REGEN    lv_color_hex(0x2EB6FF)  /* cyan : regen / negative power  */
#define AMBER_DIM      lv_color_hex(0x6B4A2C)  /* idle label                     */

/* ---- private per-field dedup epoch (independent of the cockpit's) -------- */
static int  s_amber_units_epoch;
static int  s_amber_cruise_active;
static float s_amber_last_current_a;
static float s_amber_last_voltage_v;

/* ---- bar painters (local copies; segs are passed in) -------------------- */
static lv_color_t amber_battery_color(int pct)
{
    if (pct > 50) return AMBER_BRIGHT;
    if (pct > 20) return AMBER_WARN;
    return AMBER_DANGER;
}

static void amber_paint_v_bar(lv_obj_t **segs, int count, int filled, lv_color_t color)
{
    if (filled < 0) filled = 0;
    if (filled > count) filled = count;
    for (int i = 0; i < count; i++) {
        bool on = (count - 1 - i) < filled;   /* fill bottom→top; segs[0] is top */
        lv_obj_set_style_bg_color(segs[i], on ? color : AMBER_SEG_OFF, LV_PART_MAIN);
    }
}

static void amber_paint_battery_bar(int pct)
{
    lv_obj_t *segs[14] = {
        guider_ui.dashboard_amber_batt_seg_00, guider_ui.dashboard_amber_batt_seg_01,
        guider_ui.dashboard_amber_batt_seg_02, guider_ui.dashboard_amber_batt_seg_03,
        guider_ui.dashboard_amber_batt_seg_04, guider_ui.dashboard_amber_batt_seg_05,
        guider_ui.dashboard_amber_batt_seg_06, guider_ui.dashboard_amber_batt_seg_07,
        guider_ui.dashboard_amber_batt_seg_08, guider_ui.dashboard_amber_batt_seg_09,
        guider_ui.dashboard_amber_batt_seg_10, guider_ui.dashboard_amber_batt_seg_11,
        guider_ui.dashboard_amber_batt_seg_12, guider_ui.dashboard_amber_batt_seg_13,
    };
    int filled = (pct * 14 + 50) / 100;
    amber_paint_v_bar(segs, 14, filled, amber_battery_color(pct));
}

static void amber_paint_power_bar(float power_kw, float power_max_kw)
{
    lv_obj_t *segs[14] = {
        guider_ui.dashboard_amber_power_seg_00, guider_ui.dashboard_amber_power_seg_01,
        guider_ui.dashboard_amber_power_seg_02, guider_ui.dashboard_amber_power_seg_03,
        guider_ui.dashboard_amber_power_seg_04, guider_ui.dashboard_amber_power_seg_05,
        guider_ui.dashboard_amber_power_seg_06, guider_ui.dashboard_amber_power_seg_07,
        guider_ui.dashboard_amber_power_seg_08, guider_ui.dashboard_amber_power_seg_09,
        guider_ui.dashboard_amber_power_seg_10, guider_ui.dashboard_amber_power_seg_11,
        guider_ui.dashboard_amber_power_seg_12, guider_ui.dashboard_amber_power_seg_13,
    };
    if (power_max_kw <= 0.0f) power_max_kw = 4.5f;
    float ratio = fabsf(power_kw) / power_max_kw;
    if (ratio > 1.0f) ratio = 1.0f;
    int filled = (int)(ratio * 14.0f + 0.5f);
    lv_color_t color = (power_kw < 0.0f) ? AMBER_REGEN : AMBER_ACCENT;
    amber_paint_v_bar(segs, 14, filled, color);
}

static void amber_paint_speed_bar(int speed_kmh, int speed_max_kmh)
{
    lv_obj_t *segs[12] = {
        guider_ui.dashboard_amber_speed_seg_00, guider_ui.dashboard_amber_speed_seg_01,
        guider_ui.dashboard_amber_speed_seg_02, guider_ui.dashboard_amber_speed_seg_03,
        guider_ui.dashboard_amber_speed_seg_04, guider_ui.dashboard_amber_speed_seg_05,
        guider_ui.dashboard_amber_speed_seg_06, guider_ui.dashboard_amber_speed_seg_07,
        guider_ui.dashboard_amber_speed_seg_08, guider_ui.dashboard_amber_speed_seg_09,
        guider_ui.dashboard_amber_speed_seg_10, guider_ui.dashboard_amber_speed_seg_11,
    };
    if (speed_max_kmh <= 0) speed_max_kmh = 60;
    int filled = (speed_kmh * 12 + speed_max_kmh / 2) / speed_max_kmh;
    if (filled < 0) filled = 0;
    if (filled > 12) filled = 12;
    for (int i = 0; i < 12; i++)
        lv_obj_set_style_bg_color(segs[i], (i < filled) ? AMBER_ACCENT : AMBER_SEG_OFF, LV_PART_MAIN);
}

static void amber_refresh_power_max_label(void)
{
    if (!guider_ui.dashboard_amber_power_max_val) return;
    char text[16];
    snprintf(text, sizeof(text), "%.1f KW", settings_wrapper_get_power_max_kw());
    lv_label_set_text(guider_ui.dashboard_amber_power_max_val, text);
}

static void amber_update_power(void)
{
    float power_kw = s_amber_last_current_a * s_amber_last_voltage_v / 1000.0f;
    if (guider_ui.dashboard_amber_power_value) {
        char text[16];
        snprintf(text, sizeof(text), "%.1f", power_kw);
        lv_label_set_text(guider_ui.dashboard_amber_power_value, text);
        lv_obj_set_style_text_color(guider_ui.dashboard_amber_power_value,
                                    (power_kw < 0.0f) ? AMBER_REGEN : AMBER_ACCENT,
                                    LV_PART_MAIN);
    }
    amber_paint_power_bar(power_kw, settings_wrapper_get_power_max_kw());
}

/* ---- render ops --------------------------------------------------------- */

static void amber_speed(float speed)
{
    static float old = -999.0f; static int oe = -1;
    if (speed == old && oe == s_amber_units_epoch) return;
    old = speed; oe = s_amber_units_epoch;
    int disp = (int)settings_wrapper_speed_to_display(speed);
    if (disp < 0) disp = 0; else if (disp > 999) disp = 999;
    char text[10];
    snprintf(text, sizeof(text), "%02d", disp);
    lv_label_set_text(guider_ui.dashboard_amber_Speed_text, text);
    amber_paint_speed_bar((int)speed, 60);
}

static void amber_current(float current)
{
    static float old = -999.0f;
    if (current == old) return;
    old = current;
    char text[12];
    snprintf(text, sizeof(text), "%.1f A", current);
    lv_label_set_text(guider_ui.dashboard_amber_Current_text, text);
    s_amber_last_current_a = current;
    amber_update_power();
}

static void amber_battery_proc(float pct)
{
    static float old = -999.0f;
    if (pct == old) return;
    old = pct;
    int v = (int)pct; if (v > 99) v = 99; else if (v < 0) v = 0;
    char text[8];
    snprintf(text, sizeof(text), "%d", v);
    lv_label_set_text(guider_ui.dashboard_amber_Battery_proc_text, text);
    lv_obj_set_style_text_color(guider_ui.dashboard_amber_Battery_proc_text,
                                amber_battery_color(v), LV_PART_MAIN);
    amber_paint_battery_bar(v);
}

static void amber_battery_voltage(float v)
{
    static float old = -999.0f;
    if (v == old) return;
    old = v;
    char text[10];
    snprintf(text, sizeof(text), "%.1f", v);
    lv_label_set_text(guider_ui.dashboard_amber_Voltage_text, text);
    s_amber_last_voltage_v = v;
    amber_update_power();
}

static void amber_temp_fet(float c)
{
    static int old = -9999, oe = -1;
    int v = (int)settings_wrapper_temp_to_display(c);
    if (v == old && oe == s_amber_units_epoch) return;
    old = v; oe = s_amber_units_epoch;
    char text[8];
    snprintf(text, sizeof(text), "%d", v);
    lv_label_set_text(guider_ui.dashboard_amber_temp_esc_text, text);
}

static void amber_temp_motor(float c)
{
    static int old = -9999, oe = -1;
    int v = (int)settings_wrapper_temp_to_display(c);
    if (v == old && oe == s_amber_units_epoch) return;
    old = v; oe = s_amber_units_epoch;
    char text[8];
    snprintf(text, sizeof(text), "%d", v);
    lv_label_set_text(guider_ui.dashboard_amber_temp_mot_text, text);
}

static void amber_trip(float km)
{
    static float old = -999.0f; static int oe = -1;
    if (km == old && oe == s_amber_units_epoch) return;
    old = km; oe = s_amber_units_epoch;
    char text[10];
    snprintf(text, sizeof(text), "%.1f", settings_wrapper_dist_to_display(km));
    lv_label_set_text(guider_ui.dashboard_amber_TRIP_text, text);
}

static void amber_range(float km)
{
    static float old = -999.0f; static int oe = -1;
    if (km == old && oe == s_amber_units_epoch) return;
    old = km; oe = s_amber_units_epoch;
    char text[10];
    snprintf(text, sizeof(text), "%.1f", settings_wrapper_dist_to_display(km));
    lv_label_set_text(guider_ui.dashboard_amber_Range_text, text);
}

static void amber_odometer(float km)
{
    static float old = -999.0f; static int oe = -1;
    if (km == old && oe == s_amber_units_epoch) return;
    old = km; oe = s_amber_units_epoch;
    char text[10];
    snprintf(text, sizeof(text), "%05d", (int)settings_wrapper_dist_to_display(km));
    lv_label_set_text(guider_ui.dashboard_amber_odo_text, text);
}

static void amber_amp_hours(float ah)
{
    static float old = -999.0f;
    if (ah == old) return;
    old = ah;
    char text[16];
    snprintf(text, sizeof(text), "%.1f Ah", ah);
    lv_label_set_text(guider_ui.dashboard_amber_Ah_text, text);
}

static void amber_uptime(uint32_t ms)
{
    int v = ms / 1000;
    static int old = -1;
    if (v == old) return;
    old = v;
    char text[20];
    snprintf(text, sizeof(text), "%02d:%02d:%02d", v / 3600, (v % 3600) / 60, v % 60);
    lv_label_set_text(guider_ui.dashboard_amber_uptime_text, text);
}

static void amber_mode_text(uint8_t mode)
{
    static int old = -1;
    if (mode == old) return;
    old = mode;
    char text[16];
    snprintf(text, sizeof(text), "MODE %d", mode + 1);
    lv_label_set_text(guider_ui.dashboard_amber_mode_text, text);
}

static void amber_units_changed(void)
{
    s_amber_units_epoch++;
    bool imperial = settings_wrapper_get_use_imperial();
    if (guider_ui.dashboard_amber_speed_label)
        lv_label_set_text(guider_ui.dashboard_amber_speed_label,
                          imperial ? "SPEED · MPH" : "SPEED · KM/H");
    if (guider_ui.dashboard_amber_col_trip_unit)
        lv_label_set_text(guider_ui.dashboard_amber_col_trip_unit, settings_wrapper_dist_unit());
    if (guider_ui.dashboard_amber_col_odo_unit)
        lv_label_set_text(guider_ui.dashboard_amber_col_odo_unit, settings_wrapper_dist_unit());
    if (guider_ui.dashboard_amber_col_mtmp_unit)
        lv_label_set_text(guider_ui.dashboard_amber_col_mtmp_unit, settings_wrapper_temp_unit());
    if (guider_ui.dashboard_amber_col_ctmp_unit)
        lv_label_set_text(guider_ui.dashboard_amber_col_ctmp_unit, settings_wrapper_temp_unit());
}

static void amber_cur_time(int hour, int minute, int second)
{
    if (!guider_ui.dashboard_amber_cur_time_label) return;
    char text[12];
    snprintf(text, sizeof(text), "%02d:%02d:%02d", hour, minute, second);
    lv_label_set_text(guider_ui.dashboard_amber_cur_time_label, text);
}

static void amber_cur_time_hm(int hour, int minute)
{
    lv_obj_t *lbl = guider_ui.dashboard_amber_cur_time_label;
    if (!lbl) return;
    char text[8];
    snprintf(text, sizeof(text), "%02d:%02d", hour, minute);
    lv_label_set_text(lbl, text);
    if (lv_obj_has_flag(lbl, LV_OBJ_FLAG_HIDDEN))
        lv_obj_clear_flag(lbl, LV_OBJ_FLAG_HIDDEN);
}

static void amber_hide_cur_time(void)
{
    lv_obj_t *lbl = guider_ui.dashboard_amber_cur_time_label;
    if (lbl && !lv_obj_has_flag(lbl, LV_OBJ_FLAG_HIDDEN))
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
}

static void amber_ble_status(bool connected)
{
    static int old = -1;
    if ((int)connected == old) return;
    old = connected;
    if (!guider_ui.dashboard_amber_status_bt) return;
    lv_obj_set_style_text_color(guider_ui.dashboard_amber_status_bt,
                                connected ? AMBER_BRIGHT : AMBER_DIM, LV_PART_MAIN);
    lv_obj_set_style_text_opa(guider_ui.dashboard_amber_status_bt,
                              connected ? LV_OPA_COVER : LV_OPA_50, LV_PART_MAIN);
}

static void amber_cruise_control_status(bool active)
{
    static int old = -1;
    if ((int)active == old) return;
    old = active;
    s_amber_cruise_active = active ? 1 : 0;
    lv_obj_t *img = guider_ui.dashboard_amber_cruise_control_img;
    lv_obj_t *txt = guider_ui.dashboard_amber_Speed_cc_text;
    if (active) {
        if (img) lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
        if (txt) lv_obj_clear_flag(txt, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (img) lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
        if (txt) lv_obj_add_flag(txt, LV_OBJ_FLAG_HIDDEN);
    }
}

static void amber_cruise_speed(float speed)
{
    if (!s_amber_cruise_active || speed < 0) return;
    if (!guider_ui.dashboard_amber_Speed_cc_text) return;
    char text[8];
    snprintf(text, sizeof(text), "%d", (int)settings_wrapper_speed_to_display(speed));
    lv_label_set_text(guider_ui.dashboard_amber_Speed_cc_text, text);
}

static void amber_esc_connection_status(bool connected)
{
    lv_obj_t *warn = guider_ui.dashboard_amber_esc_not_connected_text;
    lv_obj_t *mode = guider_ui.dashboard_amber_mode_text;
    if (connected) {
        if (warn) lv_obj_add_flag(warn, LV_OBJ_FLAG_HIDDEN);
        if (mode) lv_obj_clear_flag(mode, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (warn) lv_obj_clear_flag(warn, LV_OBJ_FLAG_HIDDEN);
        if (mode) lv_obj_add_flag(mode, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ---- shared chrome: open Settings (so the user can reach the theme dropdown
 * and switch back). Mirrors theme_ref's settings button. -------------------- */
static void amber_settings_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_load_scr_animation(&guider_ui, &guider_ui.settings, guider_ui.settings_del,
                          &guider_ui.dashboard_del, setup_scr_settings,
                          LV_SCR_LOAD_ANIM_NONE, 200, 200, false, false);
}

static void amber_screen_init(lv_ui *ui)
{
    lv_obj_clear_flag(ui->dashboard_amber, LV_OBJ_FLAG_SCROLLABLE);

    /* Dynamic overlays start hidden (the static design has none). */
    if (ui->dashboard_amber_esc_not_connected_text)
        lv_obj_add_flag(ui->dashboard_amber_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
    if (ui->dashboard_amber_cruise_control_img)
        lv_obj_add_flag(ui->dashboard_amber_cruise_control_img, LV_OBJ_FLAG_HIDDEN);
    if (ui->dashboard_amber_Speed_cc_text)
        lv_obj_add_flag(ui->dashboard_amber_Speed_cc_text, LV_OBJ_FLAG_HIDDEN);
    if (ui->dashboard_amber_music_info)
        lv_obj_add_flag(ui->dashboard_amber_music_info, LV_OBJ_FLAG_HIDDEN);
    if (ui->dashboard_amber_brightness_slider)
        lv_obj_add_flag(ui->dashboard_amber_brightness_slider, LV_OBJ_FLAG_HIDDEN);
    if (ui->dashboard_amber_song_title_label)
        lv_obj_add_flag(ui->dashboard_amber_song_title_label, LV_OBJ_FLAG_HIDDEN);

    /* "SETTINGS" → open the Settings screen (and the theme dropdown). */
    if (ui->dashboard_amber_Settings_text) {
        lv_obj_add_flag(ui->dashboard_amber_Settings_text, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui->dashboard_amber_Settings_text, amber_settings_cb,
                            LV_EVENT_CLICKED, NULL);
    }

    amber_refresh_power_max_label();
    amber_units_changed();
}

/* ---- theme lifecycle ---------------------------------------------------- */
static lv_obj_t *amber_create(void)
{
    setup_scr_dashboard_amber(&guider_ui);
    amber_screen_init(&guider_ui);
    return guider_ui.dashboard_amber;
}

static void amber_destroy(void)
{
    s_amber_last_current_a = 0.0f;
    s_amber_last_voltage_v = 0.0f;
    /* The Settings power-max handler may live-preview this label; NULL it so a
     * dangling pointer can't be touched while another theme is active.
     * setup_scr_dashboard_amber() repopulates it on rebuild. */
    guider_ui.dashboard_amber_power_max_val = NULL;
}

static lv_obj_t *amber_music_tile(void)
{
    return guider_ui.dashboard_amber_music_info_tile;
}

static const dashboard_theme_ops_t amber_ops = {
    .speed                 = amber_speed,
    .current               = amber_current,
    .battery_proc          = amber_battery_proc,
    .battery_voltage       = amber_battery_voltage,
    .temp_fet              = amber_temp_fet,
    .temp_motor            = amber_temp_motor,
    .trip                  = amber_trip,
    .range                 = amber_range,
    .odometer              = amber_odometer,
    .amp_hours             = amber_amp_hours,
    .uptime                = amber_uptime,
    .mode_text             = amber_mode_text,
    .units_changed         = amber_units_changed,
    .cur_time              = amber_cur_time,
    .cur_time_hm           = amber_cur_time_hm,
    .hide_cur_time         = amber_hide_cur_time,
    .ble_status            = amber_ble_status,
    .esc_connection_status = amber_esc_connection_status,
    .cruise_control_status = amber_cruise_control_status,
    .cruise_speed          = amber_cruise_speed,
    /* battery_temp / fps / navigation_* / music_text: not shown — left NULL. */
};

static const dashboard_theme_t amber_theme = {
    .id         = "amber",
    .name       = "Cockpit (Amber)",
    .create     = amber_create,
    .destroy    = amber_destroy,
    .music_tile = amber_music_tile,
    .ops        = &amber_ops,
};

void theme_dashboard_amber_register(void)
{
    dashboard_theme_register(&amber_theme);
}
