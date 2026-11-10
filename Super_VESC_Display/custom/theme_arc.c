/*
    Copyright 2026 Super VESC Display

    "Twin Arc" dashboard — the modern HMI trend: two 270-degree rings frame the
    ride data (speed left, power right) with a compact battery/range column
    between them and the usual five-cell telemetry strip along the bottom.

    Everything is built from LVGL primitives, so the theme needs no GUI Guider
    screen and no new symbols beyond the DSEG7 faces the amber theme already
    compiles. 800x480 only; custom.c registers it only when SVD_WIDE_UI is set.
*/

#include "theme_arc.h"
#include "dashboard_theme.h"
#include "settings_wrapper.h"
#include "gui_guider.h"   /* guider_ui, setup_scr_settings, ui_load_scr_animation */
#include "custom.h"       /* run_vesc_tool_menu(), show_trips_statistics(), bms_screen_attach_percent() */

#include <math.h>
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(lv_font_DSEG7Classic_Regular_160)
LV_FONT_DECLARE(lv_font_DSEG7Classic_Regular_140)
LV_FONT_DECLARE(lv_font_DSEG7Classic_Regular_120)
LV_FONT_DECLARE(lv_font_DSEG7Classic_Regular_64)
LV_FONT_DECLARE(lv_font_DSEG7Classic_Regular_50)

/* Palette — cool HMI, deliberately different from the cockpit's green and the
 * amber theme's warm 7-seg. */
#define ARC_BG      lv_color_hex(0x05070A)
#define ARC_TRACK   lv_color_hex(0x14202A)
#define ARC_ORANGE  lv_color_hex(0xFF7A1A)
#define ARC_CYAN    lv_color_hex(0x2EC5FF)
#define ARC_REGEN   lv_color_hex(0x8A7BFF)
#define ARC_TEXT    lv_color_hex(0xE6EEF2)
#define ARC_DIM     lv_color_hex(0x6A767D)
#define ARC_WARN    lv_color_hex(0xFF3B2F)
#define ARC_AMBER   lv_color_hex(0xFFB02E)
#define ARC_GREEN   lv_color_hex(0x39D98A)

static struct {
    lv_obj_t *screen;
    lv_obj_t *speed_arc, *speed_lbl, *speed_unit, *speed_title, *mode_lbl;
    lv_obj_t *power_arc, *power_lbl, *power_unit, *power_title, *current_lbl, *power_max_lbl;
    lv_obj_t *batt_title, *batt_pct, *batt_pct_unit, *batt_bar, *ah_lbl;
    lv_obj_t *range_lbl, *range_unit;
    lv_obj_t *clock_lbl, *bt_lbl, *warn_lbl, *cruise_lbl;
    lv_obj_t *music_tile, *song_lbl;
    lv_obj_t *volt_val, *volt_unit, *trip_val, *trip_unit, *odo_val, *odo_unit;
    lv_obj_t *mtmp_val, *mtmp_unit, *ctmp_val, *ctmp_unit;
} s;

static float s_last_current_a;
static float s_last_voltage_v;
static bool  s_cruise_active;
static bool  s_warn_connected = true;
static bool  s_warn_shown;
static uint32_t s_warn_blink_ms;

/* ---- small builders ----------------------------------------------------- */

static lv_obj_t *mk_label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                          lv_color_t color, lv_coord_t x, lv_coord_t y,
                          lv_coord_t w, lv_text_align_t align)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, color, LV_PART_MAIN);
    lv_obj_set_pos(l, x, y);
    if (w > 0) lv_obj_set_width(l, w);
    lv_obj_set_style_text_align(l, align, LV_PART_MAIN);
    return l;
}

static void mk_hline(lv_obj_t *parent, lv_coord_t y)
{
    lv_obj_t *l = lv_obj_create(parent);
    lv_obj_set_size(l, 800, 1);
    lv_obj_set_pos(l, 0, y);
    lv_obj_set_style_bg_color(l, ARC_TRACK, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(l, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(l, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(l, 0, LV_PART_MAIN);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *mk_arc(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_color_t color)
{
    lv_obj_t *a = lv_arc_create(parent);
    lv_obj_set_size(a, 300, 300);
    lv_obj_set_pos(a, x, y);
    lv_obj_set_style_bg_opa(a, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(a, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(a, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(a, true, LV_PART_MAIN);
    lv_obj_set_style_arc_color(a, ARC_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(a, true, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(a, color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(a, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(a, 0, LV_PART_KNOB);
    lv_obj_clear_flag(a, LV_OBJ_FLAG_CLICKABLE);
    /* 270° sweep: start at 135° and go clockwise through 45°. */
    lv_arc_set_bg_angles(a, 135, 45);
    lv_arc_set_range(a, 0, 100);
    lv_arc_set_value(a, 0);
    return a;
}

static void mk_cell(lv_obj_t *parent, lv_coord_t cx, const char *title,
                    lv_obj_t **val, lv_obj_t **unit)
{
    (void)mk_label(parent, title, &lv_font_montserrat_16, ARC_DIM,
                   cx - 70, 406, 140, LV_TEXT_ALIGN_CENTER);
    *val  = mk_label(parent, "--", &lv_font_montserrat_24, ARC_TEXT,
                     cx - 68, 425, 90, LV_TEXT_ALIGN_RIGHT);
    *unit = mk_label(parent, "", &lv_font_montserrat_16, ARC_DIM,
                     cx + 28, 433, 42, LV_TEXT_ALIGN_LEFT);
}

/* ---- value helpers ------------------------------------------------------ */

static void set_font_if(lv_obj_t *l, const lv_font_t *f)
{
    if (!l) return;
    if (lv_obj_get_style_text_font(l, LV_PART_MAIN) != f)
        lv_obj_set_style_text_font(l, f, LV_PART_MAIN);
}

static void set_arc_pct(lv_obj_t *arc, float pct)
{
    if (!arc) return;
    if (!isfinite(pct)) pct = 0.0f;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    int v = (int)(pct + 0.5f);
    if (lv_arc_get_value(arc) != v) lv_arc_set_value(arc, v);
}

static lv_color_t batt_color(int pct)
{
    if (pct > 50) return ARC_GREEN;
    if (pct > 20) return ARC_AMBER;
    return ARC_WARN;
}

/* ---- navigation --------------------------------------------------------- */

static void arc_settings_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_load_scr_animation(&guider_ui, &guider_ui.settings, guider_ui.settings_del,
                          &guider_ui.dashboard_Classic_del, setup_scr_settings,
                          LV_SCR_LOAD_ANIM_NONE, 200, 200, false, false);
}

static void arc_vesc_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) run_vesc_tool_menu();
}

static void arc_stats_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) show_trips_statistics();
}

/* ---- render ops --------------------------------------------------------- */

static void arc_speed(float kmh)
{
    if (!s.speed_lbl) return;
    if (!isfinite(kmh)) {
        set_font_if(s.speed_lbl, &lv_font_montserrat_48);
        dash_label_set(s.speed_lbl, "N/A");
        set_arc_pct(s.speed_arc, 0.0f);
        return;
    }
    set_font_if(s.speed_lbl, &lv_font_DSEG7Classic_Regular_120);
    int disp = (int)settings_wrapper_speed_to_display(kmh);
    if (disp < 0) disp = 0; else if (disp > 99) disp = 99;
    char text[8];
    snprintf(text, sizeof(text), "%02d", disp);
    dash_label_set(s.speed_lbl, text);
    set_arc_pct(s.speed_arc, kmh / 60.0f * 100.0f);
}

static void arc_update_power(void)
{
    float kw = s_last_current_a * s_last_voltage_v / 1000.0f;
    float max_kw = settings_wrapper_get_power_max_kw();
    if (max_kw <= 0.0f) max_kw = 4.5f;

    if (!isfinite(kw)) {
        set_font_if(s.power_lbl, &lv_font_montserrat_32);
        dash_label_set(s.power_lbl, "N/A");
        dash_set_text_color(s.power_lbl, ARC_DIM, LV_PART_MAIN);
        set_arc_pct(s.power_arc, 0.0f);
        return;
    }

    set_font_if(s.power_lbl, &lv_font_DSEG7Classic_Regular_64);
    char text[16];
    snprintf(text, sizeof(text), "%.1f", kw);
    dash_label_set(s.power_lbl, text);
    lv_color_t color = (kw < 0.0f) ? ARC_REGEN : ARC_CYAN;
    dash_set_text_color(s.power_lbl, color, LV_PART_MAIN);
    if (s.power_arc) {
        if (lv_obj_get_style_arc_color(s.power_arc, LV_PART_INDICATOR).full != color.full)
            lv_obj_set_style_arc_color(s.power_arc, color, LV_PART_INDICATOR);
    }
    set_arc_pct(s.power_arc, fabsf(kw) / max_kw * 100.0f);
}

static void arc_current(float a)
{
    s_last_current_a = isfinite(a) ? a : 0.0f;
    if (s.current_lbl) {
        if (isfinite(a)) {
            char text[16];
            snprintf(text, sizeof(text), "%.1f A", a);
            dash_label_set(s.current_lbl, text);
        } else {
            dash_label_set(s.current_lbl, "N/A");
        }
    }
    arc_update_power();
}

static void arc_battery_voltage(float v)
{
    s_last_voltage_v = isfinite(v) ? v : 0.0f;
    if (s.volt_val) {
        if (isfinite(v)) {
            char text[16];
            snprintf(text, sizeof(text), "%.1f", v);
            dash_label_set(s.volt_val, text);
        } else {
            dash_label_set(s.volt_val, "N/A");
        }
    }
    arc_update_power();
}

static void arc_battery_proc(float pct)
{
    if (!s.batt_pct) return;
    bool na = !isfinite(pct);
    int v = 0;
    if (!na) {
        v = (int)pct;
        if (v < 0) v = 0; else if (v > 99) v = 99;
    }

    set_font_if(s.batt_pct, na ? &lv_font_montserrat_24
                                : &lv_font_DSEG7Classic_Regular_50);
    if (na) {
        dash_label_set(s.batt_pct, "N/A");
        dash_set_text_color(s.batt_pct, ARC_DIM, LV_PART_MAIN);
    } else {
        char text[8];
        snprintf(text, sizeof(text), "%d", v);
        dash_label_set(s.batt_pct, text);
        dash_set_text_color(s.batt_pct, batt_color(v), LV_PART_MAIN);
    }

    if (s.batt_bar) {
        int32_t old = lv_bar_get_value(s.batt_bar);
        int32_t now = na ? 0 : v;
        if (old != now) lv_bar_set_value(s.batt_bar, now, LV_ANIM_OFF);
        dash_set_bg_color(s.batt_bar, na ? ARC_DIM : batt_color(v), LV_PART_INDICATOR);
    }
}

static void arc_amp_hours(float ah)
{
    if (!s.ah_lbl) return;
    char text[16];
    if (isfinite(ah)) snprintf(text, sizeof(text), "%.1f Ah", ah);
    else              snprintf(text, sizeof(text), "N/A Ah");
    dash_label_set(s.ah_lbl, text);
}

static void arc_range(float km)
{
    if (!s.range_lbl) return;
    char text[12];
    if (isfinite(km)) snprintf(text, sizeof(text), "%.1f", settings_wrapper_dist_to_display(km));
    else              snprintf(text, sizeof(text), "N/A");
    dash_label_set(s.range_lbl, text);
}

static void arc_trip(float km)
{
    if (!s.trip_val) return;
    char text[12];
    if (isfinite(km)) snprintf(text, sizeof(text), "%.1f", settings_wrapper_dist_to_display(km));
    else              snprintf(text, sizeof(text), "N/A");
    dash_label_set(s.trip_val, text);
}

static void arc_odometer(float km)
{
    if (!s.odo_val) return;
    char text[12];
    if (isfinite(km)) snprintf(text, sizeof(text), "%05d", (int)settings_wrapper_dist_to_display(km));
    else              snprintf(text, sizeof(text), "N/A");
    dash_label_set(s.odo_val, text);
}

static void arc_temp_fet(float c)
{
    if (!s.mtmp_val) return;
    if (!isfinite(c)) { dash_label_set(s.mtmp_val, "N/A"); return; }
    float f2 = 0.0f, m2 = 0.0f;
    bool dual = settings_wrapper_head2_temps(&f2, &m2);
    char text[16];
    int v = (int)settings_wrapper_temp_to_display(c);
    if (dual && isfinite(f2))
        snprintf(text, sizeof(text), "%d/%d", v, (int)settings_wrapper_temp_to_display(f2));
    else
        snprintf(text, sizeof(text), "%d", v);
    dash_label_set(s.mtmp_val, text);
}

static void arc_temp_motor(float c)
{
    if (!s.ctmp_val) return;
    if (!isfinite(c)) { dash_label_set(s.ctmp_val, "N/A"); return; }
    float f2 = 0.0f, m2 = 0.0f;
    bool dual = settings_wrapper_head2_temps(&f2, &m2);
    char text[16];
    int v = (int)settings_wrapper_temp_to_display(c);
    if (dual && isfinite(m2))
        snprintf(text, sizeof(text), "%d/%d", v, (int)settings_wrapper_temp_to_display(m2));
    else
        snprintf(text, sizeof(text), "%d", v);
    dash_label_set(s.ctmp_val, text);
}

static void arc_mode_text(uint8_t mode)
{
    if (!s.mode_lbl) return;
    if (lv_obj_has_flag(s.mode_lbl, LV_OBJ_FLAG_HIDDEN))
        lv_obj_clear_flag(s.mode_lbl, LV_OBJ_FLAG_HIDDEN);
    char text[16];
    snprintf(text, sizeof(text), "MODE %d", mode + 1);
    dash_label_set(s.mode_lbl, text);
}

static void arc_hide_mode_text(void)
{
    if (s.mode_lbl && !lv_obj_has_flag(s.mode_lbl, LV_OBJ_FLAG_HIDDEN))
        lv_obj_add_flag(s.mode_lbl, LV_OBJ_FLAG_HIDDEN);
}

static void arc_max_values(float max_speed_kmh, float max_power_kw)
{
    (void)max_speed_kmh;   /* the left ring shows live speed only */
    if (s.power_max_lbl) {
        char text[20];
        snprintf(text, sizeof(text), "MAX %.1f kW", max_power_kw);
        dash_label_set(s.power_max_lbl, text);
    }
}

static void arc_cur_time(int hour, int minute, int second)
{
    (void)second;
    if (!s.clock_lbl) return;
    char text[8];
    snprintf(text, sizeof(text), "%02d:%02d", hour, minute);
    dash_label_set(s.clock_lbl, text);
}

static void arc_cur_time_hm(int hour, int minute)
{
    if (!s.clock_lbl) return;
    char text[8];
    snprintf(text, sizeof(text), "%02d:%02d", hour, minute);
    dash_label_set(s.clock_lbl, text);
    if (lv_obj_has_flag(s.clock_lbl, LV_OBJ_FLAG_HIDDEN))
        lv_obj_clear_flag(s.clock_lbl, LV_OBJ_FLAG_HIDDEN);
}

static void arc_hide_cur_time(void)
{
    if (s.clock_lbl && !lv_obj_has_flag(s.clock_lbl, LV_OBJ_FLAG_HIDDEN))
        lv_obj_add_flag(s.clock_lbl, LV_OBJ_FLAG_HIDDEN);
}

static void arc_ble_status(bool connected)
{
    if (!s.bt_lbl) return;
    dash_set_text_color(s.bt_lbl, connected ? ARC_CYAN : ARC_DIM, LV_PART_MAIN);
    lv_obj_set_style_text_opa(s.bt_lbl, connected ? LV_OPA_COVER : LV_OPA_50,
                              LV_PART_MAIN);
}

static void arc_esc_connection_status(bool connected)
{
    if (!s.warn_lbl) return;
    uint32_t now = lv_tick_get();

    if (connected != s_warn_connected) {
        s_warn_connected = connected;
        s_warn_blink_ms = now;
        s_warn_shown = !connected;
        if (connected) lv_obj_add_flag(s.warn_lbl, LV_OBJ_FLAG_HIDDEN);
        else           lv_obj_clear_flag(s.warn_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (connected) return;
    if (now - s_warn_blink_ms >= 500) {
        s_warn_blink_ms = now;
        s_warn_shown = !s_warn_shown;
        if (s_warn_shown) lv_obj_clear_flag(s.warn_lbl, LV_OBJ_FLAG_HIDDEN);
        else              lv_obj_add_flag(s.warn_lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

static void arc_cruise_control_status(bool active)
{
    s_cruise_active = active;
    if (!s.cruise_lbl) return;
    if (active) {
        if (lv_obj_has_flag(s.cruise_lbl, LV_OBJ_FLAG_HIDDEN))
            lv_obj_clear_flag(s.cruise_lbl, LV_OBJ_FLAG_HIDDEN);
        dash_label_set(s.cruise_lbl, "CC");
    } else {
        lv_obj_add_flag(s.cruise_lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

static void arc_cruise_speed(float kmh)
{
    if (!s_cruise_active || !s.cruise_lbl || !isfinite(kmh)) return;
    char text[20];
    snprintf(text, sizeof(text), "CC %d",
             (int)settings_wrapper_speed_to_display(kmh));
    dash_label_set(s.cruise_lbl, text);
}

/* Demo/simulator path only: the real device drives the music tile through
 * music_info_view, which owns its own labels. This keeps a scripted screenshot
 * (or the generated GIF) able to show a track without notif_bridge. */
static void arc_music_text(const char *text)
{
    if (!s.song_lbl) return;
    if (!text || !*text) {
        lv_obj_add_flag(s.song_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    char buf[160];
    strncpy(buf, text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    for (char *p = buf; *p; p++) if (*p == '|') *p = '\n';
    lv_label_set_text(s.song_lbl, buf);
    lv_obj_clear_flag(s.song_lbl, LV_OBJ_FLAG_HIDDEN);
}

static void arc_units_changed(void)
{
    dash_label_set(s.speed_unit, settings_wrapper_speed_unit());
    dash_label_set(s.trip_unit, settings_wrapper_dist_unit());
    dash_label_set(s.odo_unit, settings_wrapper_dist_unit());
    dash_label_set(s.range_unit, settings_wrapper_dist_unit());
    dash_label_set(s.mtmp_unit, settings_wrapper_temp_unit());
    dash_label_set(s.ctmp_unit, settings_wrapper_temp_unit());
}

/* ---- lifecycle ---------------------------------------------------------- */

static lv_obj_t *arc_create(void)
{
    memset(&s, 0, sizeof(s));
    s_last_current_a = 0.0f;
    s_last_voltage_v = 0.0f;
    s_warn_connected = true;
    s_warn_shown = false;
    s_warn_blink_ms = 0;

    lv_obj_t *scr = lv_obj_create(NULL);
    s.screen = scr;
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, ARC_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);

    /* Top status bar. */
    lv_obj_t *vesc = mk_label(scr, "VESC", &lv_font_montserrat_20, ARC_TEXT,
                              20, 7, 100, LV_TEXT_ALIGN_LEFT);
    lv_obj_t *stats = mk_label(scr, "STATISTICS", &lv_font_montserrat_20, ARC_TEXT,
                               170, 7, 180, LV_TEXT_ALIGN_LEFT);
    s.clock_lbl = mk_label(scr, "14:32", &lv_font_montserrat_20, ARC_TEXT,
                           370, 7, 100, LV_TEXT_ALIGN_CENTER);
    lv_obj_t *settings = mk_label(scr, "SETTINGS", &lv_font_montserrat_20, ARC_TEXT,
                                  500, 7, 120, LV_TEXT_ALIGN_RIGHT);
    s.cruise_lbl = mk_label(scr, "CC", &lv_font_montserrat_16, ARC_GREEN,
                            630, 9, 62, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_style_bg_color(s.cruise_lbl, ARC_GREEN, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s.cruise_lbl, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_radius(s.cruise_lbl, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s.cruise_lbl, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(s.cruise_lbl, 2, LV_PART_MAIN);
    lv_obj_add_flag(s.cruise_lbl, LV_OBJ_FLAG_HIDDEN);
    s.bt_lbl = mk_label(scr, "BT", &lv_font_montserrat_20, ARC_DIM,
                        700, 7, 60, LV_TEXT_ALIGN_RIGHT);
    lv_obj_add_flag(vesc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(stats, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(settings, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(vesc, arc_vesc_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(stats, arc_stats_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(settings, arc_settings_cb, LV_EVENT_CLICKED, NULL);
    mk_hline(scr, 42);

    /* Two 270° rings. */
    s.speed_arc = mk_arc(scr, 20, 95, ARC_ORANGE);
    s.power_arc = mk_arc(scr, 480, 95, ARC_CYAN);

    s.speed_lbl = mk_label(scr, "00", &lv_font_DSEG7Classic_Regular_120, ARC_TEXT,
                           20, 0, 300, LV_TEXT_ALIGN_CENTER);
    lv_obj_align_to(s.speed_lbl, s.speed_arc, LV_ALIGN_CENTER, 0, -20);
    s.speed_unit = mk_label(scr, settings_wrapper_speed_unit(), &lv_font_montserrat_20,
                            ARC_ORANGE, 0, 0, 0, LV_TEXT_ALIGN_CENTER);
    lv_obj_align_to(s.speed_unit, s.speed_arc, LV_ALIGN_CENTER, 0, 72);
    s.speed_title = mk_label(scr, "SPEED", &lv_font_montserrat_16, ARC_DIM,
                             0, 0, 0, LV_TEXT_ALIGN_CENTER);
    lv_obj_align_to(s.speed_title, s.speed_arc, LV_ALIGN_CENTER, 0, 94);
    s.mode_lbl = mk_label(scr, "MODE 1", &lv_font_montserrat_16, ARC_ORANGE,
                          0, 0, 0, LV_TEXT_ALIGN_CENTER);
    lv_obj_align_to(s.mode_lbl, s.speed_arc, LV_ALIGN_CENTER, 0, 116);

    s.power_lbl = mk_label(scr, "0.0", &lv_font_DSEG7Classic_Regular_64, ARC_CYAN,
                           480, 0, 300, LV_TEXT_ALIGN_CENTER);
    lv_obj_align_to(s.power_lbl, s.power_arc, LV_ALIGN_CENTER, 0, -52);
    s.power_unit = mk_label(scr, "kW", &lv_font_montserrat_20, ARC_CYAN,
                            0, 0, 0, LV_TEXT_ALIGN_CENTER);
    lv_obj_align_to(s.power_unit, s.power_arc, LV_ALIGN_CENTER, 0, 12);
    s.power_title = mk_label(scr, "POWER", &lv_font_montserrat_16, ARC_DIM,
                             0, 0, 0, LV_TEXT_ALIGN_CENTER);
    lv_obj_align_to(s.power_title, s.power_arc, LV_ALIGN_CENTER, 0, 38);
    s.current_lbl = mk_label(scr, "0.0 A", &lv_font_montserrat_18, ARC_DIM,
                             0, 0, 0, LV_TEXT_ALIGN_CENTER);
    lv_obj_align_to(s.current_lbl, s.power_arc, LV_ALIGN_CENTER, 0, 66);
    s.power_max_lbl = mk_label(scr, "MAX 0.0 kW", &lv_font_montserrat_16, ARC_DIM,
                               0, 0, 0, LV_TEXT_ALIGN_CENTER);
    lv_obj_align_to(s.power_max_lbl, s.power_arc, LV_ALIGN_CENTER, 0, 96);

    /* Centre column: battery %, remaining Ah, range. */
    s.batt_title = mk_label(scr, "BATTERY", &lv_font_montserrat_16, ARC_DIM,
                            330, 108, 140, LV_TEXT_ALIGN_CENTER);
    s.batt_pct = mk_label(scr, "0", &lv_font_DSEG7Classic_Regular_50, ARC_GREEN,
                          330, 132, 92, LV_TEXT_ALIGN_CENTER);
    s.batt_pct_unit = mk_label(scr, "%", &lv_font_montserrat_20, ARC_DIM,
                               424, 152, 26, LV_TEXT_ALIGN_LEFT);
    s.batt_bar = lv_bar_create(scr);
    lv_obj_set_size(s.batt_bar, 130, 8);
    lv_obj_set_pos(s.batt_bar, 335, 198);
    lv_bar_set_range(s.batt_bar, 0, 100);
    lv_bar_set_value(s.batt_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s.batt_bar, ARC_TRACK, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s.batt_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s.batt_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s.batt_bar, ARC_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s.batt_bar, 4, LV_PART_INDICATOR);
    s.ah_lbl = mk_label(scr, "0.0 Ah", &lv_font_montserrat_18, ARC_DIM,
                        330, 214, 140, LV_TEXT_ALIGN_CENTER);
    lv_obj_t *range_title = mk_label(scr, "RANGE", &lv_font_montserrat_16, ARC_DIM,
                                     330, 252, 140, LV_TEXT_ALIGN_CENTER);
    (void)range_title;
    s.range_lbl = mk_label(scr, "0.0", &lv_font_montserrat_24, ARC_TEXT,
                           330, 272, 92, LV_TEXT_ALIGN_RIGHT);
    s.range_unit = mk_label(scr, settings_wrapper_dist_unit(), &lv_font_montserrat_16,
                            ARC_DIM, 424, 280, 40, LV_TEXT_ALIGN_LEFT);

    /* Now-playing card in the centre gap. On the device the app attaches
     * music_info_view here (album art + scrolling title/artist); the dark card
     * and demo label below are only what the headless simulator shows, because
     * it has no notif_bridge/media poller. */
    s.music_tile = lv_obj_create(scr);
    lv_obj_set_pos(s.music_tile, 324, 300);
    lv_obj_set_size(s.music_tile, 152, 92);
    lv_obj_clear_flag(s.music_tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s.music_tile, lv_color_hex(0x0B141B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s.music_tile, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s.music_tile, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(s.music_tile, ARC_TRACK, LV_PART_MAIN);
    lv_obj_set_style_radius(s.music_tile, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s.music_tile, 0, LV_PART_MAIN);

    s.song_lbl = mk_label(s.music_tile, "", &lv_font_montserrat_18, ARC_TEXT,
                          0, 0, 140, LV_TEXT_ALIGN_CENTER);
    lv_label_set_long_mode(s.song_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_align(s.song_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(s.song_lbl, LV_OBJ_FLAG_HIDDEN);

    /* Bottom five-cell telemetry strip. */
    mk_hline(scr, 399);
    mk_cell(scr, 80,  "VOLTAGE", &s.volt_val, &s.volt_unit);
    mk_cell(scr, 240, "TRIP",    &s.trip_val, &s.trip_unit);
    mk_cell(scr, 400, "ODO",     &s.odo_val,  &s.odo_unit);
    mk_cell(scr, 560, "M.TEMP",  &s.mtmp_val, &s.mtmp_unit);
    mk_cell(scr, 720, "C.TEMP",  &s.ctmp_val, &s.ctmp_unit);

    /* Dead-link banner (hidden until the ESC link drops). */
    s.warn_lbl = mk_label(scr, "ESC OFFLINE", &lv_font_montserrat_16, ARC_WARN,
                          0, 0, 0, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_style_bg_color(s.warn_lbl, ARC_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s.warn_lbl, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s.warn_lbl, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(s.warn_lbl, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(s.warn_lbl, 4, LV_PART_MAIN);
    lv_obj_align(s.warn_lbl, LV_ALIGN_TOP_MID, 0, 48);
    lv_obj_add_flag(s.warn_lbl, LV_OBJ_FLAG_HIDDEN);

    bms_screen_attach_percent(s.batt_pct);
    arc_units_changed();
    return scr;
}

static void arc_destroy(void)
{
    memset(&s, 0, sizeof(s));
    s_last_current_a = 0.0f;
    s_last_voltage_v = 0.0f;
    s_cruise_active = false;
}

static const dashboard_theme_ops_t arc_ops = {
    .speed                 = arc_speed,
    .current               = arc_current,
    .battery_proc          = arc_battery_proc,
    .battery_voltage       = arc_battery_voltage,
    .temp_fet              = arc_temp_fet,
    .temp_motor            = arc_temp_motor,
    .trip                  = arc_trip,
    .range                 = arc_range,
    .odometer              = arc_odometer,
    .amp_hours             = arc_amp_hours,
    .mode_text             = arc_mode_text,
    .hide_mode_text        = arc_hide_mode_text,
    .cur_time              = arc_cur_time,
    .cur_time_hm           = arc_cur_time_hm,
    .hide_cur_time         = arc_hide_cur_time,
    .ble_status            = arc_ble_status,
    .esc_connection_status = arc_esc_connection_status,
    .cruise_control_status = arc_cruise_control_status,
    .cruise_speed          = arc_cruise_speed,
    .music_text            = arc_music_text,
    .max_values            = arc_max_values,
    .units_changed         = arc_units_changed,
    /* uptime / fps / cruise_* / navigation_* / music_* / brightness: not shown */
};

static lv_obj_t *arc_music_tile(void)
{
    return s.music_tile;
}

static const dashboard_theme_t arc_theme = {
    .id         = "arc",
    .name       = "Twin Arc",
    .create     = arc_create,
    .destroy    = arc_destroy,
    .music_tile = arc_music_tile,
    .ops        = &arc_ops,
};

void theme_arc_register(void)
{
    dashboard_theme_register(&arc_theme);
}
