/*
    Copyright 2026 Super VESC Display

    Generic dashboard renderer — see theme_generic.h for the widget naming
    convention and the build-time discovery mechanism.

    The ops here are screen-agnostic: they read from whatever dashboard_widgets_t
    the active generated theme installed via dashboard_generic_set_active(), and
    skip any NULL widget. Rendering mirrors the cockpit/amber themes (formats,
    unit conversion, bar fill direction) so a screen cloned from one of those in
    GUI Guider behaves the same with zero hand-written C. Colours fall back to a
    neutral accent — a screen that wants a bespoke palette/segment-gradient
    treatment writes its own theme module instead.

    No dependency on guider_ui or any specific screen, so it compiles unchanged
    in the desktop simulator alongside the firmware.
*/

#include "theme_generic.h"
#include "settings_wrapper.h"

#include <stdio.h>
#include <math.h>

/* Neutral palette (amber-leaning default). */
#define GEN_SEG_OFF   lv_color_hex(0x202020)
#define GEN_ACCENT    lv_color_hex(0xFF7A1A)
#define GEN_REGEN     lv_color_hex(0x2EB6FF)
#define GEN_BATT_OK   lv_color_hex(0xFFB35C)
#define GEN_BATT_WARN lv_color_hex(0xFFB02E)
#define GEN_BATT_CRIT lv_color_hex(0xFF3B2F)
#define GEN_BT_ON     lv_color_hex(0xFFB35C)
#define GEN_BT_OFF    lv_color_hex(0x6B4A2C)

static const dashboard_widgets_t *s_w;
static float s_last_current_a;
static float s_last_voltage_v;

void dashboard_generic_set_active(const dashboard_widgets_t *w)
{
    s_w = w;
    s_last_current_a = 0.0f;
    s_last_voltage_v = 0.0f;
}

/* ---- helpers ---- */

static lv_color_t batt_color(int pct)
{
    if (pct > 50) return GEN_BATT_OK;
    if (pct > 20) return GEN_BATT_WARN;
    return GEN_BATT_CRIT;
}

/* Vertical bar: segs[0] is the top cell, fill bottom→top (battery / power). */
static void paint_v_bar(lv_obj_t *const *segs, int count, int filled, lv_color_t on)
{
    if (filled < 0) filled = 0;
    if (filled > count) filled = count;
    for (int i = 0; i < count; i++) {
        if (!segs[i]) continue;
        bool lit = (count - 1 - i) < filled;
        dash_set_bg_color(segs[i], lit ? on : GEN_SEG_OFF, LV_PART_MAIN);
    }
}

/* Horizontal bar: seg_00 lights first (speed). */
static void paint_h_bar(lv_obj_t *const *segs, int count, int filled, lv_color_t on)
{
    if (filled < 0) filled = 0;
    if (filled > count) filled = count;
    for (int i = 0; i < count; i++) {
        if (!segs[i]) continue;
        dash_set_bg_color(segs[i], (i < filled) ? on : GEN_SEG_OFF, LV_PART_MAIN);
    }
}

static void render_power(void)
{
    if (!s_w) return;
    float power_kw = s_last_current_a * s_last_voltage_v / 1000.0f;
    lv_color_t color = (power_kw < 0.0f) ? GEN_REGEN : GEN_ACCENT;
    if (s_w->power_value) {
        char text[16];
        snprintf(text, sizeof(text), "%.1f", power_kw);
        dash_label_set(s_w->power_value, text);
        dash_set_text_color(s_w->power_value, color, LV_PART_MAIN);
    }
    if (s_w->power_seg_n > 0) {
        float pmax = settings_wrapper_get_power_max_kw();
        if (pmax <= 0.0f) pmax = 4.5f;
        float ratio = fabsf(power_kw) / pmax;
        if (ratio > 1.0f) ratio = 1.0f;
        paint_v_bar(s_w->power_seg, s_w->power_seg_n,
                    (int)(ratio * s_w->power_seg_n + 0.5f), color);
    }
}

/* ---- ops ---- */

static void g_speed(float speed)
{
    if (!s_w) return;
    int disp = (int)settings_wrapper_speed_to_display(speed);
    if (disp < 0) disp = 0; else if (disp > 999) disp = 999;
    if (s_w->speed_text) {
        char text[10];
        snprintf(text, sizeof(text), "%02d", disp);
        dash_label_set(s_w->speed_text, text);
    }
    if (s_w->speed_seg_n > 0) {
        int smax = 60;
        int filled = ((int)speed * s_w->speed_seg_n + smax / 2) / smax;
        paint_h_bar(s_w->speed_seg, s_w->speed_seg_n, filled, GEN_ACCENT);
    }
}

static void g_current(float current)
{
    if (!s_w) return;
    s_last_current_a = current;
    if (s_w->current_text) {
        char text[12];
        snprintf(text, sizeof(text), "%.1f A", current);
        dash_label_set(s_w->current_text, text);
    }
    render_power();
}

static void g_battery_voltage(float v)
{
    if (!s_w) return;
    s_last_voltage_v = v;
    if (s_w->voltage_text) {
        char text[10];
        snprintf(text, sizeof(text), "%.1f", v);
        dash_label_set(s_w->voltage_text, text);
    }
    render_power();
}

static void g_battery_proc(float pct)
{
    if (!s_w) return;
    int v = (int)pct; if (v > 99) v = 99; else if (v < 0) v = 0;
    if (s_w->batt_pct_text) {
        char text[8];
        snprintf(text, sizeof(text), "%d", v);
        dash_label_set(s_w->batt_pct_text, text);
        dash_set_text_color(s_w->batt_pct_text, batt_color(v), LV_PART_MAIN);
    }
    if (s_w->batt_seg_n > 0) {
        int filled = (v * s_w->batt_seg_n + 50) / 100;
        paint_v_bar(s_w->batt_seg, s_w->batt_seg_n, filled, batt_color(v));
    }
}

static void g_temp_fet(float c)
{
    if (!s_w || !s_w->temp_fet_text) return;
    char text[8];
    snprintf(text, sizeof(text), "%d", (int)settings_wrapper_temp_to_display(c));
    dash_label_set(s_w->temp_fet_text, text);
}

static void g_temp_motor(float c)
{
    if (!s_w || !s_w->temp_motor_text) return;
    char text[8];
    snprintf(text, sizeof(text), "%d", (int)settings_wrapper_temp_to_display(c));
    dash_label_set(s_w->temp_motor_text, text);
}

static void g_trip(float km)
{
    if (!s_w || !s_w->trip_text) return;
    char text[10];
    snprintf(text, sizeof(text), "%.1f", settings_wrapper_dist_to_display(km));
    dash_label_set(s_w->trip_text, text);
}

static void g_range(float km)
{
    if (!s_w || !s_w->range_text) return;
    char text[10];
    snprintf(text, sizeof(text), "%.1f", settings_wrapper_dist_to_display(km));
    dash_label_set(s_w->range_text, text);
}

static void g_odometer(float km)
{
    if (!s_w || !s_w->odo_text) return;
    char text[10];
    snprintf(text, sizeof(text), "%05d", (int)settings_wrapper_dist_to_display(km));
    dash_label_set(s_w->odo_text, text);
}

static void g_amp_hours(float ah)
{
    if (!s_w || !s_w->ah_text) return;
    char text[16];
    snprintf(text, sizeof(text), "%.1f Ah", ah);
    dash_label_set(s_w->ah_text, text);
}

static void g_uptime(uint32_t ms)
{
    if (!s_w || !s_w->uptime_text) return;
    int v = ms / 1000;
    char text[20];
    snprintf(text, sizeof(text), "%02d:%02d:%02d", v / 3600, (v % 3600) / 60, v % 60);
    dash_label_set(s_w->uptime_text, text);
}

static void g_hide_mode_text(void)
{
    if (!s_w || !s_w->mode_text) return;
    if (!lv_obj_has_flag(s_w->mode_text, LV_OBJ_FLAG_HIDDEN))
        lv_obj_add_flag(s_w->mode_text, LV_OBJ_FLAG_HIDDEN);
}

static void g_mode_text(uint8_t mode)
{
    if (!s_w || !s_w->mode_text) return;
    if (lv_obj_has_flag(s_w->mode_text, LV_OBJ_FLAG_HIDDEN))
        lv_obj_clear_flag(s_w->mode_text, LV_OBJ_FLAG_HIDDEN);   /* re-show after no-Lisp */
    char text[16];
    snprintf(text, sizeof(text), "MODE %d", mode + 1);
    dash_label_set(s_w->mode_text, text);
}

static void g_cur_time(int hour, int minute, int second)
{
    if (!s_w || !s_w->time_label) return;
    char text[12];
    snprintf(text, sizeof(text), "%02d:%02d:%02d", hour, minute, second);
    dash_label_set(s_w->time_label, text);
}

static void g_cur_time_hm(int hour, int minute)
{
    if (!s_w || !s_w->time_label) return;
    char text[8];
    snprintf(text, sizeof(text), "%02d:%02d", hour, minute);
    dash_label_set(s_w->time_label, text);
    if (lv_obj_has_flag(s_w->time_label, LV_OBJ_FLAG_HIDDEN))
        lv_obj_clear_flag(s_w->time_label, LV_OBJ_FLAG_HIDDEN);
}

static void g_hide_cur_time(void)
{
    if (!s_w || !s_w->time_label) return;
    if (!lv_obj_has_flag(s_w->time_label, LV_OBJ_FLAG_HIDDEN))
        lv_obj_add_flag(s_w->time_label, LV_OBJ_FLAG_HIDDEN);
}

static void g_ble_status(bool connected)
{
    if (!s_w || !s_w->status_bt) return;
    dash_set_text_color(s_w->status_bt, connected ? GEN_BT_ON : GEN_BT_OFF, LV_PART_MAIN);
    lv_obj_set_style_text_opa(s_w->status_bt, connected ? LV_OPA_COVER : LV_OPA_50, LV_PART_MAIN);
}

const dashboard_theme_ops_t dashboard_generic_ops = {
    .speed           = g_speed,
    .current         = g_current,
    .battery_proc    = g_battery_proc,
    .battery_voltage = g_battery_voltage,
    .temp_fet        = g_temp_fet,
    .temp_motor      = g_temp_motor,
    .trip            = g_trip,
    .range           = g_range,
    .odometer        = g_odometer,
    .amp_hours       = g_amp_hours,
    .uptime          = g_uptime,
    .mode_text       = g_mode_text,
    .hide_mode_text  = g_hide_mode_text,
    .cur_time        = g_cur_time,
    .cur_time_hm     = g_cur_time_hm,
    .hide_cur_time   = g_hide_cur_time,
    .ble_status      = g_ble_status,
    /* battery_temp / fps / units_changed / cruise_* / navigation_* / music_*:
     * theme-specific or need extra widgets — left NULL (skipped). */
};
