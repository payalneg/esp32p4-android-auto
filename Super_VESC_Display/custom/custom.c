/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdatomic.h>
#include "lvgl.h"
#include "custom.h"
#include "settings_wrapper.h"
#include "dashboard_theme.h"
#include "theme_ref.h"
#include "theme_dashboard_amber.h"
#include "theme_generic.h"

#ifdef LV_REALDEVICE
#include "log_capture.h"
#endif

/* Cached display values exposed to readers outside the LVGL thread (e.g. the
 * AA video overlay running on the H.264 decoder task). Mirror what the user
 * sees on the cockpit so AA HUD and dashboard agree, including demo-mode
 * values. Atomic so the decoder can read without taking the LVGL lock. */
static _Atomic int s_cockpit_speed_value;
static _Atomic int s_cockpit_battery_proc_value;

int cockpit_get_speed_value(void)        { return atomic_load(&s_cockpit_speed_value); }
int cockpit_get_battery_proc_value(void) { return atomic_load(&s_cockpit_battery_proc_value); }

/* Bumped by dashboard_units_changed() whenever the km/miles toggle flips.
 * The update_* setters dedup on (value, epoch) so a units change forces a
 * re-format on the next push even though the canonical value is unchanged.
 * Single-threaded with the setters (all run on the LVGL task), plain int. */
static int s_units_epoch = 0;

#ifdef LV_REALDEVICE
#include "vesc_limits.h"
#include "vesc_battery_calc.h"
#include "vesc_head2.h"
#include "app_fs.h"
#endif

int cruise_active = 0;

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

extern lv_ui guider_ui;

/* Numeric field: a plain centred label paired with +/- buttons. Replaces the
 * built-in lv_spinbox, which had an editor-style border/cursor we didn't want
 * (the value never gets typed in — it's always driven by the buttons). */
typedef struct num_field {
    int value;
    int min, max, step;
    int decimals;          /* 0 → "%d", 1 → "%d.%d" */
    lv_obj_t *label;
    void (*on_change)(struct num_field *f);
} num_field_t;

static num_field_t s_target_id_field;
static num_field_t s_second_head_id_field;
static num_field_t s_battery_capacity_field;
static num_field_t s_power_max_field;
static num_field_t s_clock_hour_field;
static num_field_t s_clock_min_field;

static void num_field_refresh(num_field_t *f) {
    if (!f->label) return;
    char buf[16];
    if (f->decimals == 1) {
        int v = f->value;
        int sign = v < 0 ? -1 : 1;
        v *= sign;
        snprintf(buf, sizeof buf, "%s%d.%d", sign < 0 ? "-" : "", v / 10, v % 10);
    } else {
        snprintf(buf, sizeof buf, "%d", f->value);
    }
    lv_label_set_text(f->label, buf);
}

static void num_field_set(num_field_t *f, int v) {
    if (v < f->min) v = f->min;
    if (v > f->max) v = f->max;
    if (v == f->value) return;
    f->value = v;
    num_field_refresh(f);
    if (f->on_change) f->on_change(f);
}

static void num_field_inc(num_field_t *f) { num_field_set(f, f->value + f->step); }
static void num_field_dec(num_field_t *f) { num_field_set(f, f->value - f->step); }

/* Compact settings layout: one row per setting, heading on the left,
 * controls clustered on the right edge. */
#define SETTINGS_ROW_H        60
#define SETTINGS_BTN_W        60
#define SETTINGS_VAL_W        100
#define SETTINGS_PLUS_X       730
#define SETTINGS_VAL_X        620
#define SETTINGS_MINUS_X      550

static lv_obj_t *settings_step_btn_create(lv_obj_t *parent, int x, int y,
                                          const char *glyph, uint32_t bg_color,
                                          lv_event_cb_t cb) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, glyph);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, SETTINGS_BTN_W, 50);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), 0);
    lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

static void settings_value_label_init(lv_obj_t *parent, num_field_t *f,
                                      int y, uint32_t text_color) {
    f->label = lv_label_create(parent);
    lv_obj_set_pos(f->label, SETTINGS_VAL_X, y);
    lv_obj_set_size(f->label, SETTINGS_VAL_W, 50);
    lv_obj_set_style_text_color(f->label, lv_color_hex(text_color), 0);
    lv_obj_set_style_text_font(f->label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(f->label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(f->label, 11, 0);
    num_field_refresh(f);
}

static lv_obj_t *settings_heading_create(lv_obj_t *parent, int y, const char *text) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_pos(lbl, 20, y + 15);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    return lbl;
}

// Settings UI objects (dynamically created)
static lv_obj_t *settings_target_id_label = NULL;
static lv_obj_t *settings_target_id_plus_btn = NULL;
static lv_obj_t *settings_target_id_minus_btn = NULL;
static lv_obj_t *settings_second_head_label = NULL;
static lv_obj_t *settings_second_head_switch = NULL;
static lv_obj_t *settings_second_head_id_label = NULL;
static lv_obj_t *settings_second_head_id_plus_btn = NULL;
static lv_obj_t *settings_second_head_id_minus_btn = NULL;
static lv_obj_t *settings_can_speed_dropdown = NULL;
static lv_obj_t *settings_can_speed_label = NULL;
static lv_obj_t *settings_theme_dropdown = NULL;
static lv_obj_t *settings_theme_label = NULL;
static lv_obj_t *settings_splash_loops_dropdown = NULL;
static lv_obj_t *settings_splash_loops_label = NULL;
static lv_obj_t *settings_brightness_slider = NULL;
static lv_obj_t *settings_brightness_label = NULL;
static lv_obj_t *settings_controller_id_slider = NULL;
static lv_obj_t *settings_controller_id_label = NULL;
static lv_obj_t *settings_battery_capacity_label = NULL;
static lv_obj_t *settings_battery_capacity_plus_btn = NULL;
static lv_obj_t *settings_battery_capacity_minus_btn = NULL;
static lv_obj_t *settings_battery_calc_mode_dropdown = NULL;
static lv_obj_t *settings_battery_calc_mode_label = NULL;
static lv_obj_t *settings_show_fps_switch = NULL;
static lv_obj_t *settings_show_fps_label = NULL;
static lv_obj_t *settings_demo_mode_switch = NULL;
static lv_obj_t *settings_demo_mode_label = NULL;
static lv_obj_t *settings_vesc_emulator_switch = NULL;
static lv_obj_t *settings_vesc_emulator_label = NULL;
static lv_obj_t *settings_vesc_emulator_hint  = NULL;
static lv_obj_t *settings_aa_autoconnect_switch = NULL;
static lv_obj_t *settings_aa_autoconnect_label  = NULL;
static lv_obj_t *settings_aa_autoconnect_hint   = NULL;
static lv_obj_t *settings_units_switch = NULL;
static lv_obj_t *settings_units_label  = NULL;
static lv_obj_t *settings_units_hint   = NULL;
static lv_obj_t *settings_temp_unit_switch = NULL;
static lv_obj_t *settings_temp_unit_label  = NULL;
static lv_obj_t *settings_temp_unit_hint   = NULL;
static lv_obj_t *settings_wheel_diameter_spinbox = NULL;
static lv_obj_t *settings_wheel_diameter_label = NULL;
static lv_obj_t *settings_wheel_diameter_plus_btn = NULL;
static lv_obj_t *settings_wheel_diameter_minus_btn = NULL;
static lv_obj_t *settings_motor_poles_spinbox = NULL;
static lv_obj_t *settings_motor_poles_label = NULL;
static lv_obj_t *settings_motor_poles_plus_btn = NULL;
static lv_obj_t *settings_motor_poles_minus_btn = NULL;
static lv_obj_t *settings_power_max_label = NULL;
static lv_obj_t *settings_power_max_plus_btn = NULL;
static lv_obj_t *settings_power_max_minus_btn = NULL;
static lv_obj_t *settings_clock_heading_label = NULL;
static lv_obj_t *settings_clock_h_plus_btn = NULL;
static lv_obj_t *settings_clock_h_minus_btn = NULL;
static lv_obj_t *settings_clock_m_plus_btn = NULL;
static lv_obj_t *settings_clock_m_minus_btn = NULL;
static lv_obj_t *settings_clock_colon_label = NULL;
static lv_obj_t *settings_reset_button = NULL;
static lv_obj_t *settings_reset_trip_button = NULL;
static lv_obj_t *settings_info_label = NULL;

// VESC Limits UI objects
static lv_obj_t *settings_limits_title_label = NULL;
static lv_obj_t *settings_read_limits_btn = NULL;
static lv_obj_t *settings_motor_current_spinbox = NULL;
static lv_obj_t *settings_motor_current_label = NULL;
static lv_obj_t *settings_motor_current_plus_btn = NULL;
static lv_obj_t *settings_motor_current_minus_btn = NULL;
static lv_obj_t *settings_battery_current_spinbox = NULL;
static lv_obj_t *settings_battery_current_label = NULL;
static lv_obj_t *settings_battery_current_plus_btn = NULL;
static lv_obj_t *settings_battery_current_minus_btn = NULL;
static lv_obj_t *settings_erpm_max_spinbox = NULL;
static lv_obj_t *settings_erpm_max_label = NULL;
static lv_obj_t *settings_erpm_max_plus_btn = NULL;
static lv_obj_t *settings_erpm_max_minus_btn = NULL;
static lv_obj_t *settings_apply_limits_btn = NULL;
static lv_obj_t *settings_limits_status_label = NULL;
/**
 * Create a demo application
 */

/* ============ Cockpit bars ============================================ */
/* Repaints segments of a vertical bar (battery / power).
 * segs[0] is the topmost segment, segs[count-1] is the bottommost;
 * fill goes from bottom to top. `filled` is the number of lit segments. */
#define COCKPIT_BG_2     lv_color_hex(0x161B1E)
#define COCKPIT_ACCENT   lv_color_hex(0xB6FF2E)
#define COCKPIT_WARN     lv_color_hex(0xFFB02E)
#define COCKPIT_DANGER   lv_color_hex(0xFF3B30)
#define COCKPIT_REGEN    lv_color_hex(0x2EB6FF)  /* cyan, regen / negative power */

static lv_color_t cockpit_battery_color(int pct)
{
    if (pct > 50) return COCKPIT_ACCENT;
    if (pct > 20) return COCKPIT_WARN;
    return COCKPIT_DANGER;
}

static void cockpit_paint_v_bar(lv_obj_t **segs, int count, int filled, lv_color_t color)
{
    if (filled < 0) filled = 0;
    if (filled > count) filled = count;
    for (int i = 0; i < count; i++) {
        bool on = (count - 1 - i) < filled;
        lv_obj_set_style_bg_color(segs[i], on ? color : COCKPIT_BG_2, LV_PART_MAIN);
    }
}

static void cockpit_paint_battery_bar(int pct)
{
    lv_obj_t *segs[14] = {
        guider_ui.dashboard_Classic_batt_seg_00, guider_ui.dashboard_Classic_batt_seg_01,
        guider_ui.dashboard_Classic_batt_seg_02, guider_ui.dashboard_Classic_batt_seg_03,
        guider_ui.dashboard_Classic_batt_seg_04, guider_ui.dashboard_Classic_batt_seg_05,
        guider_ui.dashboard_Classic_batt_seg_06, guider_ui.dashboard_Classic_batt_seg_07,
        guider_ui.dashboard_Classic_batt_seg_08, guider_ui.dashboard_Classic_batt_seg_09,
        guider_ui.dashboard_Classic_batt_seg_10, guider_ui.dashboard_Classic_batt_seg_11,
        guider_ui.dashboard_Classic_batt_seg_12, guider_ui.dashboard_Classic_batt_seg_13,
    };
    int filled = (pct * 14 + 50) / 100;
    cockpit_paint_v_bar(segs, 14, filled, cockpit_battery_color(pct));
}

static void cockpit_paint_power_bar(float power_kw, float power_max_kw)
{
    lv_obj_t *segs[14] = {
        guider_ui.dashboard_Classic_power_seg_00, guider_ui.dashboard_Classic_power_seg_01,
        guider_ui.dashboard_Classic_power_seg_02, guider_ui.dashboard_Classic_power_seg_03,
        guider_ui.dashboard_Classic_power_seg_04, guider_ui.dashboard_Classic_power_seg_05,
        guider_ui.dashboard_Classic_power_seg_06, guider_ui.dashboard_Classic_power_seg_07,
        guider_ui.dashboard_Classic_power_seg_08, guider_ui.dashboard_Classic_power_seg_09,
        guider_ui.dashboard_Classic_power_seg_10, guider_ui.dashboard_Classic_power_seg_11,
        guider_ui.dashboard_Classic_power_seg_12, guider_ui.dashboard_Classic_power_seg_13,
    };
    if (power_max_kw <= 0.0f) power_max_kw = 4.5f;
    /* Use the absolute value to size the fill, and pick the colour by sign:
     * positive power (drive) → accent green, negative (regen) → cyan. */
    float ratio = fabsf(power_kw) / power_max_kw;
    if (ratio > 1.0f) ratio = 1.0f;
    int filled = (int)(ratio * 14.0f + 0.5f);
    lv_color_t color = (power_kw < 0.0f) ? COCKPIT_REGEN : COCKPIT_ACCENT;
    cockpit_paint_v_bar(segs, 14, filled, color);
}

/* Speed bar is horizontal, filled left to right. */
static void cockpit_paint_speed_bar(int speed_kmh, int speed_max_kmh)
{
    lv_obj_t *segs[12] = {
        guider_ui.dashboard_Classic_speed_seg_00, guider_ui.dashboard_Classic_speed_seg_01,
        guider_ui.dashboard_Classic_speed_seg_02, guider_ui.dashboard_Classic_speed_seg_03,
        guider_ui.dashboard_Classic_speed_seg_04, guider_ui.dashboard_Classic_speed_seg_05,
        guider_ui.dashboard_Classic_speed_seg_06, guider_ui.dashboard_Classic_speed_seg_07,
        guider_ui.dashboard_Classic_speed_seg_08, guider_ui.dashboard_Classic_speed_seg_09,
        guider_ui.dashboard_Classic_speed_seg_10, guider_ui.dashboard_Classic_speed_seg_11,
    };
    if (speed_max_kmh <= 0) speed_max_kmh = 60;
    int filled = (speed_kmh * 12 + speed_max_kmh / 2) / speed_max_kmh;
    if (filled < 0) filled = 0;
    if (filled > 12) filled = 12;
    for (int i = 0; i < 12; i++) {
        bool on = i < filled;
        lv_obj_set_style_bg_color(segs[i], on ? COCKPIT_ACCENT : COCKPIT_BG_2, LV_PART_MAIN);
    }
}

/* Current power_kW is current * voltage / 1000.
 * update_current() and update_battery_voltage() cache the latest values and
 * call this helper to refresh the Power panel digit and bar. */
static float s_cockpit_last_current_a = 0.0f;
static float s_cockpit_last_voltage_v = 0.0f;

/* Refresh the "X.X KW" label next to the power bar from the current setting.
 * Called on init and from the settings event handlers so the scale label
 * always matches the bar's full-scale value. */
static void cockpit_refresh_power_max_label(void)
{
    if (!guider_ui.dashboard_Classic_power_max_val) return;
    float pmax = settings_wrapper_get_power_max_kw();
    char text[16];
    snprintf(text, sizeof(text), "%.1f KW", pmax);
    lv_label_set_text(guider_ui.dashboard_Classic_power_max_val, text);
}

static void cockpit_update_power(void)
{
    float power_kw = s_cockpit_last_current_a * s_cockpit_last_voltage_v / 1000.0f;
    if (guider_ui.dashboard_Classic_power_value) {
        char text[16];
        snprintf(text, sizeof(text), "%.1f", power_kw);
        lv_label_set_text(guider_ui.dashboard_Classic_power_value, text);
        /* Match the digit colour to the bar — cyan during regen. */
        lv_obj_set_style_text_color(
            guider_ui.dashboard_Classic_power_value,
            (power_kw < 0.0f) ? COCKPIT_REGEN : COCKPIT_ACCENT,
            LV_PART_MAIN);
    }
    cockpit_paint_power_bar(power_kw, settings_wrapper_get_power_max_kw());
}
/* ====================================================================== */

/* set the digital label and steering lamp image style. */
static void set_position_x(void * gui, int32_t temp)
{

}

static void set_position_y(void * gui, int32_t temp)
{
  
}

#ifdef LV_REALDEVICE
/* Global "Formatting backup storage" notice. The trip/backup LittleFS volume is
 * formatted once on first boot (background, on the app_fs task); with flash
 * AUTO_SUSPEND off that stalls rendering, so we draw a full-screen notice on the
 * top layer (above any screen) and flush it BEFORE the freeze deepens. A light
 * lv_timer polls app_fs_state and tears the notice down once mounted. */
static lv_obj_t   *s_fmt_overlay;
static lv_timer_t *s_fmt_overlay_tmr;

static void fmt_overlay_show(void)
{
    if (s_fmt_overlay) return;
    s_fmt_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_fmt_overlay, 800, 480);
    lv_obj_set_pos(s_fmt_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_fmt_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_fmt_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_fmt_overlay, 0, 0);
    lv_obj_add_flag(s_fmt_overlay, LV_OBJ_FLAG_CLICKABLE);   /* absorb touches */
    lv_obj_clear_flag(s_fmt_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *l = lv_label_create(s_fmt_overlay);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(l, 720);
    lv_label_set_text(l, "Formatting backup storage...\n"
                         "One-time setup, please wait (up to ~1 min).\n"
                         "Do not power off.");
    lv_obj_set_style_text_color(l, lv_color_hex(0xFFB02E), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l);
    lv_refr_now(NULL);   /* draw + flush before the format freezes rendering */
}

static void fmt_overlay_timer_cb(lv_timer_t *t)
{
    int st = app_fs_state();
    if (st == APP_FS_FORMATTING) {
        fmt_overlay_show();
    } else if (st == APP_FS_READY || st == APP_FS_FAIL) {
        if (s_fmt_overlay) { lv_obj_del(s_fmt_overlay); s_fmt_overlay = NULL; }
        lv_timer_del(t);
        s_fmt_overlay_tmr = NULL;
    }
}
#endif /* LV_REALDEVICE */

/* Defined further down (cockpit render op); cockpit_screen_init() calls it to
 * set the static unit captions + bump the units epoch at (re)build time. */
static void cockpit_units_changed(void);

/* Per-screen chrome — applied every time the cockpit screen is (re)built (boot,
 * and again when the user switches back to the cockpit theme). The one-time
 * bits (settings_wrapper_init, the format-notice timer, theme registration)
 * live in custom_init_once() at the bottom of this file. */
static void cockpit_screen_init(lv_ui *ui)
{
    /* Disable screen panning — dashboard is a static layout. */
    lv_obj_clear_flag(ui->dashboard_Classic, LV_OBJ_FLAG_SCROLLABLE);

    // BLE status is shown via dashboard_status_bt text — the
    // ble_connected_img icon has been removed from the project.
    // Setup_scr_dashboard paints "BT" in bright accent by default;
    // dim it on init since no peer is connected yet. update_ble_status()
    // will repaint when a peer joins (its internal old_state == false
    // dedup matches the inactive look we set here).
    if (ui->dashboard_Classic_status_bt) {
        lv_obj_set_style_text_color(ui->dashboard_Classic_status_bt,
                                    lv_color_hex(0x4A5358), LV_PART_MAIN);
        lv_obj_set_style_text_opa(ui->dashboard_Classic_status_bt,
                                  LV_OPA_50, LV_PART_MAIN);
    }

    // Initialize ESC not connected text as hidden (will be shown and blink if ESC disconnects)
    if (ui->dashboard_Classic_esc_not_connected_text != NULL) {
        lv_obj_add_flag(ui->dashboard_Classic_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
    }

    // Initialize cruise-active widgets as hidden (shown when CC engages).
    if (ui->dashboard_Classic_cruise_control_img != NULL) {
        lv_obj_add_flag(ui->dashboard_Classic_cruise_control_img, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui->dashboard_Classic_Speed_cc_text != NULL) {
        lv_obj_add_flag(ui->dashboard_Classic_Speed_cc_text, LV_OBJ_FLAG_HIDDEN);
    }

    // Initialize cruise-rpm label as hidden (will be shown when cruise-rpm is true)
    // Cockpit: Speed_meter removed — cruise/RPM needles are no longer needed.
    // if (ui->dashboard_Classic_Speed_meter_scale_1_ndline_0 != NULL) {
    //     lv_meter_set_indicator_value(ui->dashboard_Classic_Speed_meter, ui->dashboard_Classic_Speed_meter_scale_1_ndline_0, -1);
    //     lv_meter_set_indicator_value(ui->dashboard_Classic_Speed_meter, ui->dashboard_Classic_Speed_meter_scale_2_ndline_0, -1);
    // }

    /* Demo loop is no longer auto-started. It is toggled at runtime from
     * the Settings screen via dashboard_demo_set_active() — see the
     * "Demo mode" switch wired up in settings_ui_init(). */

    /* Paint the "X.X KW" scale label from the saved power-max setting so the
     * bar fill ratio and the printed full-scale agree at boot. settings_wrapper
     * is already initialised in custom_init_once(). */
    cockpit_refresh_power_max_label();

    /* Flip the static speed/distance unit captions to match the saved
     * km/miles setting so they're correct from the first frame (the value
     * setters convert on their own as data arrives). Call the cockpit impl
     * directly: at (re)build time the theme registry's active pointer may not
     * be us yet, so the dashboard_units_changed() dispatcher could no-op. */
    cockpit_units_changed();

    /* Sync the invisible dashboard brightness drag slider with the saved
     * value so a touch on the slider starts from the real brightness,
     * not the GUI Guider default of 50.
     *
     * Also override default styles so phantom touches from vibration on
     * the road don't paint LVGL's default-theme blue PRESSED-state
     * highlight across the entire 300×480 slider area. The slider stays
     * touch-active (still drags brightness when intentionally swiped)
     * but with transparent overlay in every state. */
    if (ui->dashboard_Classic_brightness_slider) {
        lv_slider_set_value(ui->dashboard_Classic_brightness_slider,
                            settings_wrapper_get_brightness(), LV_ANIM_OFF);
        lv_obj_t *sl = ui->dashboard_Classic_brightness_slider;
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_MAIN      | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_INDICATOR | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_KNOB      | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_MAIN      | LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_INDICATOR | LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_KNOB      | LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(sl, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_outline_width(sl, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_width(sl, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(sl, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    }

#if defined(ENABLE_WALL_CLOCK) && !ENABLE_WALL_CLOCK
    /* Wall clock disabled at compile time (see main/config.h, mirrored
     * in components/vesc_ui/CMakeLists.txt) — hide cur_time_label so
     * the placeholder "00:42:18" doesn't sit on the dashboard. */
    if (ui->dashboard_Classic_cur_time_label) {
        lv_obj_add_flag(ui->dashboard_Classic_cur_time_label, LV_OBJ_FLAG_HIDDEN);
    }
#endif

    /* GUI Guider's music-info tileview ships with a dark fill + light
     * border which flashes on the dashboard for the second or two
     * between setup_scr_dashboard and the first media-frame. Style
     * overrides on the various LV_PART_* don't reliably win against
     * the default theme, so we just hide the whole tileview until
     * music_info_view shows its first track. The view itself flips the
     * HIDDEN flag back off through music_info_view_set_visible(). */
    if (ui->dashboard_Classic_music_info) {
        lv_obj_add_flag(ui->dashboard_Classic_music_info, LV_OBJ_FLAG_HIDDEN);
    }
}

/* 4 Hz tick (~250 ms). sinf() drives smooth value sweeps; every change goes
 * through update_speed() and friends, the same path used on real hardware. */
static void cockpit_demo_tick(lv_timer_t * t)
{
    (void)t;
    static uint32_t tick = 0;
    tick++;
    float ts = tick * 0.25f;  /* seconds */

    /* Speed: smooth sweep 0..60 km/h (full cycle ~30 s). */
    float speed = (sinf(ts * 0.21f) * 0.5f + 0.5f) * 60.0f;
    update_speed(speed);

    /* Battery: slow discharge 100..0 (full cycle ~5 min), then loops. */
    int batt = 100 - ((int)(tick / 6)) % 101;
    update_battery_proc((float)batt);

    /* Voltage swings around 52 V depending on current load. */
    float current = sinf(ts * 0.5f) * 30.0f;          /* -30..+30 A   */
    float voltage = 52.0f - current * 0.05f;           /* sag/recovery */
    update_battery_voltage(voltage);
    update_current(current);
    /* power_kW is refreshed automatically by cockpit_update_power(),
     * called from update_battery_voltage/update_current.
     * P = U * I / 1000. */

    /* Temperatures: 35..70 °C motor, 30..60 °C controller. */
    update_temp_motor(50.0f + sinf(ts * 0.13f) * 18.0f);
    update_temp_fet  (45.0f + sinf(ts * 0.17f) * 15.0f);

    /* Trip and odo grow monotonically. */
    static float trip_km = 0.0f;
    trip_km += 0.01f;
    update_trip(trip_km);
    update_odometer(4128.0f + trip_km);

    /* Amp-hours = integral of |I| over time (Ah = A·h). 250 ms = 1/14400 h. */
    static float ah_consumed = 0.0f;
    ah_consumed += fabsf(current) * (0.25f / 3600.0f);
    update_amp_hours(ah_consumed);

    /* Range estimate ~ (batt% * 0.6) km. */
    update_range((float)batt * 0.6f);

    /* Uptime. */
    update_uptime(tick * 250U);

    /* Mode flips every ~30 s. */
    update_mode_text((tick / 120) % 3);

    /* BLE status is intentionally NOT touched here. vesc_ui_updater pushes
     * the real ble_host_is_connected() at 10 Hz outside the demo gate, so
     * if we toggled it from the demo the icon would flicker between the
     * fake value and the real one. The icon stays truthful in demo. */

    /* ESC link: 20 s connected, 5 s disconnected (25 s cycle). When
     * disconnected, esc_not_connected_text appears and (per existing logic)
     * starts blinking. */
    update_esc_connection_status((tick % 100) < 80);

    /* Cruise control: 15 s engaged, 15 s idle. When engaged, target speed
     * follows a slow sine around 45 km/h. */
    bool cc_on = (tick / 60) % 2;
    update_cruise_control_status(cc_on);
    if (cc_on) {
        update_cruise_speed(45.0f + sinf(ts * 0.3f) * 5.0f);
    }
}

static lv_timer_t *s_demo_timer = NULL;

bool dashboard_demo_is_active(void) {
    return s_demo_timer != NULL;
}

void dashboard_demo_set_active(bool on) {
    if (on && !s_demo_timer) {
        s_demo_timer = lv_timer_create(cockpit_demo_tick, 250, NULL);
    } else if (!on && s_demo_timer) {
        lv_timer_del(s_demo_timer);
        s_demo_timer = NULL;
    }
}

void speed_meter_timer_cb(lv_timer_t * t)
{
    
}

void home_label_digit_animation(lv_ui *ui)
{

}

void digital_cluster_chart_timer_cb(lv_timer_t * t)
{
    
}

void play_music(lv_ui *ui)
{
   
}

static const void * lv_demo_music_get_list_img(uint32_t track_id)
{
    (void)track_id;
    return NULL;
}

void music_album_next(bool next)
{
   
}

void reset_icon_pressed(void)
{
    #ifdef LV_REALDEVICE
    battery_calc_reset_trip_and_ah();
    #endif
}

static void cockpit_current(float current)
{
    static float old_value = -999.0f;
    if (current == old_value) {
        return;
    }
    old_value = current;
    
    int value = current;
    /* Cockpit: Current_meter removed, needle/arcs no longer needed. */
    // int abs_value = abs(value);
    // lv_meter_set_indicator_value(guider_ui.dashboard_Classic_Current_meter, guider_ui.dashboard_Classic_Current_meter_scale_0_ndline_0, abs_value);
    // if (value>0) {
    //     lv_meter_set_indicator_end_value(guider_ui.dashboard_Classic_Current_meter, guider_ui.dashboard_Classic_Current_meter_scale_0_arc_1, abs_value);
    //     lv_meter_set_indicator_end_value(guider_ui.dashboard_Classic_Current_meter, guider_ui.dashboard_Classic_Current_meter_scale_0_arc_2, 0);
    // } else {
    //     lv_meter_set_indicator_end_value(guider_ui.dashboard_Classic_Current_meter, guider_ui.dashboard_Classic_Current_meter_scale_0_arc_1, 0);
    //     lv_meter_set_indicator_end_value(guider_ui.dashboard_Classic_Current_meter, guider_ui.dashboard_Classic_Current_meter_scale_0_arc_2, abs_value);
    // }
    (void)value;
    char text[10];
    sprintf(text,"%.1f A", current);
    lv_label_set_text(guider_ui.dashboard_Classic_Current_text,text);

    /* Cockpit: recompute power using cached voltage. */
    s_cockpit_last_current_a = current;
    cockpit_update_power();
}

static void cockpit_speed(float speed)
{
    static float old_value = -999.0f;
    static int   old_epoch = -1;
    if (speed == old_value && old_epoch == s_units_epoch) {
        return;
    }
    old_value = speed;
    old_epoch = s_units_epoch;

    int value = speed;  /* canonical km/h — drives the bar + atomic snapshot */

    /* Cockpit: Speed_meter removed — needle/arc no longer needed. */
    // lv_meter_set_indicator_value(guider_ui.dashboard_Classic_Speed_meter, guider_ui.dashboard_Classic_Speed_meter_scale_0_ndline_0, value);
    // lv_meter_set_indicator_end_value(guider_ui.dashboard_Classic_Speed_meter, guider_ui.dashboard_Classic_Speed_meter_scale_0_arc_0, value);

    /* Cockpit: zero-padded 2-digit format (32, 05) and horizontal bar fill.
     * The printed digits honour the km/miles toggle; the bar + atomic
     * snapshot stay in canonical km/h (bar scale is fixed 0..60 km/h and the
     * AA HUD reader converts on its own). */
    int disp = (int)settings_wrapper_speed_to_display(speed);
    char text[10];
    int v_clamped    = value < 0 ? 0 : (value > 999 ? 999 : value);
    int disp_clamped = disp  < 0 ? 0 : (disp  > 999 ? 999 : disp);
    snprintf(text, sizeof(text), "%02d", disp_clamped);
    lv_label_set_text(guider_ui.dashboard_Classic_Speed_text, text);
    cockpit_paint_speed_bar(value, 60);
    atomic_store(&s_cockpit_speed_value, v_clamped);
}

static void cockpit_cruise_speed(float speed)
{
    if (cruise_active == 0) {
        return;
    }
    static float old_value = -999.0f;
    static int   old_epoch = -1;
    if (speed == old_value && old_epoch == s_units_epoch) {
        return;
    }
    if (speed < 0) {
        return;
    }
    old_value = speed;
    old_epoch = s_units_epoch;

    int value = (int)settings_wrapper_speed_to_display(speed);
    /* Cockpit: write cruise target speed into Speed_cc_text. The widget is
     * shown/hidden by update_cruise_control_status(); here we only update
     * the value so it is ready when CC engages. */
    if (guider_ui.dashboard_Classic_Speed_cc_text) {
        char text[8];
        snprintf(text, sizeof(text), "%d", value);
        lv_label_set_text(guider_ui.dashboard_Classic_Speed_cc_text, text);
    }
}


static void cockpit_battery_proc(float battery_proc)
{
    static float old_value = -999.0f;
    if (battery_proc == old_value) {
        return;
    }
    old_value = battery_proc;
    
    int value = battery_proc;

    /* Cockpit: Battery_meter removed — arc/needle no longer used. */
    // lv_meter_set_indicator_value(guider_ui.dashboard_Classic_Battery_meter, guider_ui.dashboard_Classic_Battery_meter_scale_0_ndline_0, 100-value);
    // lv_meter_set_indicator_start_value(guider_ui.dashboard_Classic_Battery_meter, guider_ui.dashboard_Classic_Battery_meter_scale_0_arc_1, 100-value);

    int v_clamped = value > 99 ? 99 : (value < 0 ? 0 : value);
    char text[10];
    snprintf(text, sizeof(text), "%d", v_clamped);
    lv_label_set_text(guider_ui.dashboard_Classic_Battery_proc_text,text);

    /* Cockpit: digit color + vertical bar fill per the 50/20 rule. */
    lv_color_t bcol = cockpit_battery_color(v_clamped);
    lv_obj_set_style_text_color(guider_ui.dashboard_Classic_Battery_proc_text, bcol, LV_PART_MAIN);
    cockpit_paint_battery_bar(v_clamped);
    atomic_store(&s_cockpit_battery_proc_value, v_clamped);
}

static void cockpit_trip(float trip_distance)
{
    static float old_value = -999.0f;
    static int   old_epoch = -1;
    if (trip_distance == old_value && old_epoch == s_units_epoch) {
        return;
    }
    old_value = trip_distance;
    old_epoch = s_units_epoch;

    char text[10];
    sprintf(text,"%0.1f", settings_wrapper_dist_to_display(trip_distance));
    lv_label_set_text(guider_ui.dashboard_Classic_TRIP_text,text);
}

static void cockpit_range(float range_distance)
{
    static float old_value = -999.0f;
    static int   old_epoch = -1;
    if (range_distance == old_value && old_epoch == s_units_epoch) {
        return;
    }
    old_value = range_distance;
    old_epoch = s_units_epoch;

    char text[10];
    sprintf(text,"%.1f", settings_wrapper_dist_to_display(range_distance));
    lv_label_set_text(guider_ui.dashboard_Classic_Range_text,text);
}

/* Dual-head temperatures. When a second VESC head is configured AND its passive
 * CAN STATUS is fresh, each temperature reads "h1/h2" (e.g. "34/37"). The string
 * widens, so dual mode grows the value labels leftward (right edge fixed) and
 * nudges the °C units right to make room; single-head layout is the GUI-Guider
 * default. vesc_head2_get_temps() is device-only — the simulator stays single. */
static bool dashboard_head2_temps(float *fet, float *motor)
{
#ifdef LV_REALDEVICE
    return vesc_head2_get_temps(fet, motor);
#else
    (void)fet; (void)motor;
    return false;
#endif
}

static void dashboard_temps_apply_layout(bool dual)
{
    /* Derive "already applied?" from the live widget width rather than a static
     * flag — the dashboard screen can be torn down and rebuilt (widgets reset to
     * the GUI-Guider single-head coords), and a static would desync from that. */
    int target_w = dual ? 160 : 100;
    if (lv_obj_get_width(guider_ui.dashboard_Classic_temp_mot_text) == target_w) return;

    if (dual) {
        lv_obj_set_pos (guider_ui.dashboard_Classic_temp_mot_text, 419, 432);
        lv_obj_set_size(guider_ui.dashboard_Classic_temp_mot_text, 160, 60);
        lv_obj_set_pos (guider_ui.dashboard_Classic_col_mtmp_unit, 588, 442);
        lv_obj_set_pos (guider_ui.dashboard_Classic_temp_esc_text, 577, 432);
        lv_obj_set_size(guider_ui.dashboard_Classic_temp_esc_text, 160, 60);
        lv_obj_set_pos (guider_ui.dashboard_Classic_col_ctmp_unit, 748, 442);
    } else {
        lv_obj_set_pos (guider_ui.dashboard_Classic_temp_mot_text, 479, 432);
        lv_obj_set_size(guider_ui.dashboard_Classic_temp_mot_text, 100, 60);
        lv_obj_set_pos (guider_ui.dashboard_Classic_col_mtmp_unit, 582, 442);
        lv_obj_set_pos (guider_ui.dashboard_Classic_temp_esc_text, 637, 432);
        lv_obj_set_size(guider_ui.dashboard_Classic_temp_esc_text, 100, 60);
        lv_obj_set_pos (guider_ui.dashboard_Classic_col_ctmp_unit, 742, 442);
    }
}

static void cockpit_temp_fet(float temp_fet)
{
    static int old_v1 = -9999, old_v2 = -9999, old_dual = -1, old_epoch = -1;

    float fet2 = 0.0f, mot2 = 0.0f;
    bool  dual = dashboard_head2_temps(&fet2, &mot2);
    dashboard_temps_apply_layout(dual);

    int v1 = (int)settings_wrapper_temp_to_display(temp_fet);
    int v2 = dual ? (int)settings_wrapper_temp_to_display(fet2) : 0;
    if (v1 == old_v1 && v2 == old_v2 &&
        old_dual == (dual ? 1 : 0) && old_epoch == s_units_epoch) {
        return;
    }
    old_v1 = v1; old_v2 = v2; old_dual = dual ? 1 : 0; old_epoch = s_units_epoch;

    char text[16];
    if (dual) sprintf(text, "%d/%d", v1, v2);
    else      sprintf(text, "%d", v1);
    lv_label_set_text(guider_ui.dashboard_Classic_temp_esc_text, text);
}

static void cockpit_temp_motor(float temp_motor)
{
    static int old_v1 = -9999, old_v2 = -9999, old_dual = -1, old_epoch = -1;

    float fet2 = 0.0f, mot2 = 0.0f;
    bool  dual = dashboard_head2_temps(&fet2, &mot2);
    dashboard_temps_apply_layout(dual);

    int v1 = (int)settings_wrapper_temp_to_display(temp_motor);
    int v2 = dual ? (int)settings_wrapper_temp_to_display(mot2) : 0;
    if (v1 == old_v1 && v2 == old_v2 &&
        old_dual == (dual ? 1 : 0) && old_epoch == s_units_epoch) {
        return;
    }
    old_v1 = v1; old_v2 = v2; old_dual = dual ? 1 : 0; old_epoch = s_units_epoch;

    char text[16];
    if (dual) sprintf(text, "%d/%d", v1, v2);
    else      sprintf(text, "%d", v1);
    lv_label_set_text(guider_ui.dashboard_Classic_temp_mot_text, text);
}

static void cockpit_amp_hours(float amp_hours)
{
    static float old_value = -999.0f;
    if (amp_hours == old_value) {
        return;
    }
    old_value = amp_hours;
    
    char text[16];
    sprintf(text, "%.1f Ah", amp_hours);
    lv_label_set_text(guider_ui.dashboard_Classic_Ah_text, text);
}

static void cockpit_battery_temp(float battery_temp)
{
    static float old_value = -999.0f;
    if (battery_temp == old_value) {
        return;
    }
    old_value = battery_temp;
    
    int value = battery_temp;
    /* Cockpit: temp_bat_text removed — battery temperature is not displayed. */
    (void)value;
    // char text[10];
    // sprintf(text,"%d", value);
    // lv_label_set_text(guider_ui.dashboard_Classic_temp_bat_text, text);
}

static void cockpit_battery_voltage(float battery_voltage)
{
    static float old_value = -999.0f;
    if (battery_voltage == old_value) {
        return;
    }
    old_value = battery_voltage;
    
    char text[10];
    sprintf(text,"%.1f", battery_voltage);

    lv_label_set_text(guider_ui.dashboard_Classic_Voltage_text,text);

    /* Cockpit: recompute power using cached current. */
    s_cockpit_last_voltage_v = battery_voltage;
    cockpit_update_power();
}


static void cockpit_odometer(float odometer)
{
    static float old_value = -999.0f;
    static int   old_epoch = -1;
    if (odometer == old_value && old_epoch == s_units_epoch) {
        return;
    }
    old_value = odometer;
    old_epoch = s_units_epoch;

    int value = (int)settings_wrapper_dist_to_display(odometer);
    char text[10];
    sprintf(text,"%05d", value);

    lv_label_set_text(guider_ui.dashboard_Classic_odo_text,text);
}

static void cockpit_units_changed(void)
{
    /* Force the value setters to re-format on their next push even though the
     * canonical km/km-h numbers haven't changed. */
    s_units_epoch++;

    /* Static unit captions are GUI-Guider-generated; we only retext them at
     * runtime so a regen of setup_scr_dashboard.c never has to be touched.
     * AVG column is hidden, Range shows a bare number after first update —
     * neither needs a caption flip. */
    bool imperial = settings_wrapper_get_use_imperial();
    if (guider_ui.dashboard_Classic_speed_label)
        lv_label_set_text(guider_ui.dashboard_Classic_speed_label,
                          imperial ? "SPEED · MPH" : "SPEED · KM/H");
    if (guider_ui.dashboard_Classic_col_trip_unit)
        lv_label_set_text(guider_ui.dashboard_Classic_col_trip_unit, settings_wrapper_dist_unit());
    if (guider_ui.dashboard_Classic_col_odo_unit)
        lv_label_set_text(guider_ui.dashboard_Classic_col_odo_unit, settings_wrapper_dist_unit());
    /* Temperature captions (motor / controller). */
    if (guider_ui.dashboard_Classic_col_mtmp_unit)
        lv_label_set_text(guider_ui.dashboard_Classic_col_mtmp_unit, settings_wrapper_temp_unit());
    if (guider_ui.dashboard_Classic_col_ctmp_unit)
        lv_label_set_text(guider_ui.dashboard_Classic_col_ctmp_unit, settings_wrapper_temp_unit());
}

static void cockpit_fps(int fps)
{
    /* Cockpit: fps_text removed — FPS counter is not displayed anymore. */
    (void)fps;
}

static void cockpit_uptime(uint32_t uptime)
{
    int value = uptime/1000;
    static uint32_t old_value = -999;
    if (value == old_value) {
        return;
    }
    old_value = value;
    
    char text[20];
    sprintf(text,"%02d:%02d:%02d", value/3600, (value%3600)/60, value%60);
    lv_label_set_text(guider_ui.dashboard_Classic_uptime_text,text);
}

static void cockpit_cur_time(int hour, int minute, int second)
{
    if (!guider_ui.dashboard_Classic_cur_time_label) return;
    static int old_h = -1, old_m = -1, old_s = -1;
    if (hour == old_h && minute == old_m && second == old_s) return;
    old_h = hour; old_m = minute; old_s = second;
    char text[12];
    snprintf(text, sizeof(text), "%02d:%02d:%02d", hour, minute, second);
    lv_label_set_text(guider_ui.dashboard_Classic_cur_time_label, text);
}

/* Phone-pushed clock (no on-device RTC): show "HH:MM" and reveal the
 * label, which boots hidden when the wall clock is compiled out. The
 * caller (vesc_ui_updater) polls notif_bridge and hides the label again
 * via hide_cur_time() once the phone stops sending updates. */
static void cockpit_cur_time_hm(int hour, int minute)
{
    lv_obj_t *lbl = guider_ui.dashboard_Classic_cur_time_label;
    if (!lbl) return;
    static int old_h = -1, old_m = -1;
    if (hour != old_h || minute != old_m) {
        old_h = hour; old_m = minute;
        char text[8];
        snprintf(text, sizeof(text), "%02d:%02d", hour, minute);
        lv_label_set_text(lbl, text);
    }
    if (lv_obj_has_flag(lbl, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

static void cockpit_hide_cur_time(void)
{
    lv_obj_t *lbl = guider_ui.dashboard_Classic_cur_time_label;
    if (lbl && !lv_obj_has_flag(lbl, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

static void cockpit_hide_mode_text(void)
{
    lv_obj_t *lbl = guider_ui.dashboard_Classic_mode_text;
    if (lbl && !lv_obj_has_flag(lbl, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

static void cockpit_mode_text(uint8_t mode)
{
    /* Re-show in case it was hidden while Lisp data was absent (no script). */
    lv_obj_t *lbl = guider_ui.dashboard_Classic_mode_text;
    if (lbl && lv_obj_has_flag(lbl, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    }

    static uint8_t old_mode = -1;
    if (mode == old_mode) {
        return;
    }
    old_mode = mode;

    char text[20];
    sprintf(text,"MODE %d", mode+1);
    lv_label_set_text(guider_ui.dashboard_Classic_mode_text,text);
}

static void cockpit_ble_status(bool connected)
{
    static bool old_state = false;
    if (connected == old_state) {
        return;
    }
    old_state = connected;
    
    /* Cockpit: status bar "BT" text — accent when connected, dim when not. */
    if (guider_ui.dashboard_Classic_status_bt) {
        lv_obj_set_style_text_color(
            guider_ui.dashboard_Classic_status_bt,
            connected ? COCKPIT_ACCENT : lv_color_hex(0x4A5358) /* TEXT_FAINT */,
            LV_PART_MAIN);
        lv_obj_set_style_text_opa(
            guider_ui.dashboard_Classic_status_bt,
            connected ? LV_OPA_COVER : LV_OPA_50,
            LV_PART_MAIN);
    }
}

static void cockpit_cruise_control_status(bool active)
{
    static bool old_state = false;
    if (active == old_state) {
        return;
    }
    old_state = active;
    
    // Show/hide cruise icon and target-speed text together.
    if (active) {
        if (guider_ui.dashboard_Classic_cruise_control_img)
            lv_obj_clear_flag(guider_ui.dashboard_Classic_cruise_control_img, LV_OBJ_FLAG_HIDDEN);
        if (guider_ui.dashboard_Classic_Speed_cc_text)
            lv_obj_clear_flag(guider_ui.dashboard_Classic_Speed_cc_text, LV_OBJ_FLAG_HIDDEN);
        cruise_active = 1;
    } else {
        if (guider_ui.dashboard_Classic_cruise_control_img)
            lv_obj_add_flag(guider_ui.dashboard_Classic_cruise_control_img, LV_OBJ_FLAG_HIDDEN);
        if (guider_ui.dashboard_Classic_Speed_cc_text)
            lv_obj_add_flag(guider_ui.dashboard_Classic_Speed_cc_text, LV_OBJ_FLAG_HIDDEN);
        cruise_active = 0;
    }
}

static void cockpit_esc_connection_status(bool connected)
{
    static bool old_state = true;
    static uint32_t last_blink_time = 0;
    static bool blink_state = false;
    
    // Handle connection state change
    if (connected != old_state) {
        old_state = connected;
        if (connected) {
            // ESC connected - hide warning text
            lv_obj_add_flag(guider_ui.dashboard_Classic_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_clear_flag(guider_ui.dashboard_Classic_Ah_const_text, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_clear_flag(guider_ui.dashboard_Classic_Ah_text, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(guider_ui.dashboard_Classic_mode_text, LV_OBJ_FLAG_HIDDEN);
            /* Re-show the STATISTICS entry point on reconnect — the blink loop
             * (which only runs while disconnected) may have left it hidden. */
            lv_obj_clear_flag(guider_ui.dashboard_Classic_statistics_button, LV_OBJ_FLAG_HIDDEN);
        } else {
            // ESC disconnected - show warning text (will start blinking)
            lv_obj_clear_flag(guider_ui.dashboard_Classic_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_add_flag(guider_ui.dashboard_Classic_Ah_text, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_add_flag(guider_ui.dashboard_Classic_Ah_const_text, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(guider_ui.dashboard_Classic_mode_text, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(guider_ui.dashboard_Classic_statistics_button, LV_OBJ_FLAG_HIDDEN);
            blink_state = true;
        }
    }
    
    // Blink warning text if ESC is not connected
    if (!connected) {
        uint32_t now = lv_tick_get();
        
        // Toggle visibility every 500ms
        if (now - last_blink_time >= 500) {
            last_blink_time = now;
            blink_state = !blink_state;
            
            if (blink_state) {
                //lv_obj_add_flag(guider_ui.dashboard_Classic_Ah_text, LV_OBJ_FLAG_HIDDEN);
                //lv_obj_add_flag(guider_ui.dashboard_Classic_Ah_const_text, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(guider_ui.dashboard_Classic_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(guider_ui.dashboard_Classic_statistics_button, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(guider_ui.dashboard_Classic_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
                //lv_obj_clear_flag(guider_ui.dashboard_Classic_Ah_const_text, LV_OBJ_FLAG_HIDDEN);
                //lv_obj_clear_flag(guider_ui.dashboard_Classic_Ah_text, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(guider_ui.dashboard_Classic_statistics_button, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void cockpit_navigation_icon(const uint8_t *img_data, uint32_t data_size, uint16_t width, uint16_t height, lv_img_cf_t color_format)
{
    /* Cockpit: navigation_icon removed — there is nowhere to render the
     * navigation icon, so buffer allocations would be pointless. */
    (void)img_data; (void)data_size; (void)width; (void)height; (void)color_format;
}

static void cockpit_navigation_text(const char *text)
{
    /* Cockpit: navigation_text removed from the project. */
    (void)text;
}

static void cockpit_music_text(const char *text)
{
    /* Cockpit: music_text removed from the project. */
    (void)text;
}

// ============================================================================
// SETTINGS UI IMPLEMENTATION
// ============================================================================

// Debounced NVS commit. Each spinbox tick / slider drag step during a rapid
// burst would otherwise issue an nvs_commit on the LVGL thread (~100ms of
// flash write) and starve touch_input's lock attempt, producing
// "esp_lv_adapter_lock: Failed to acquire LVGL lock" and a visible freeze.
// We update the in-memory cache live via the *_volatile setters (so the
// UI/hot-apply react immediately) and only commit to NVS once the user
// stops adjusting (debounce period below).
#define DEBOUNCED_COMMIT_PERIOD_MS 800

typedef struct {
    lv_timer_t *timer;
    void (*persist)(void);
} debounced_commit_t;

static void debounced_commit_timer_cb(lv_timer_t *t) {
    debounced_commit_t *d = (debounced_commit_t *)t->user_data;
    if (d && d->persist) d->persist();
    lv_timer_del(t);
    if (d) d->timer = NULL;
}

static void debounced_commit_schedule(debounced_commit_t *d,
                                      void (*persist)(void)) {
    d->persist = persist;
    if (d->timer) {
        lv_timer_reset(d->timer);
    } else {
        d->timer = lv_timer_create(debounced_commit_timer_cb,
                                   DEBOUNCED_COMMIT_PERIOD_MS, d);
        lv_timer_set_repeat_count(d->timer, 1);
    }
}

static debounced_commit_t s_target_id_commit;
static debounced_commit_t s_second_head_id_commit;
static debounced_commit_t s_brightness_commit;
static debounced_commit_t s_controller_id_commit;
static debounced_commit_t s_battery_capacity_commit;
static debounced_commit_t s_power_max_commit;
static debounced_commit_t s_theme_commit;

// Target VESC ID — value-change callback (volatile + debounced NVS commit)
static void target_id_on_change(num_field_t *f) {
    settings_wrapper_set_target_vesc_id_volatile((uint8_t)f->value);
    debounced_commit_schedule(&s_target_id_commit,
                              settings_wrapper_persist_target_vesc_id);
}

static void target_id_plus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) num_field_inc(&s_target_id_field);
}

static void target_id_minus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) num_field_dec(&s_target_id_field);
}

// Second head enable toggle — single click, persist immediately (like AA).
static void second_head_switch_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    bool checked = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    settings_wrapper_set_second_head_enabled(checked);
}

// Second head CAN ID — value-change callback (volatile + debounced NVS commit)
static void second_head_id_on_change(num_field_t *f) {
    settings_wrapper_set_second_head_id_volatile((uint8_t)f->value);
    debounced_commit_schedule(&s_second_head_id_commit,
                              settings_wrapper_persist_second_head_id);
}

static void second_head_id_plus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) num_field_inc(&s_second_head_id_field);
}

static void second_head_id_minus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) num_field_dec(&s_second_head_id_field);
}

// Event handler for CAN speed dropdown
static void can_speed_dropdown_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *dropdown = lv_event_get_target(e);
        uint16_t selected = lv_dropdown_get_selected(dropdown);
        
        // Save to settings
        settings_wrapper_set_can_speed_index((uint8_t)selected);
        
        // Update info label
        lv_label_set_text(settings_info_label, "CAN speed requires restart!");
    }
}

/* Dashboard theme dropdown. Live-switches the dashboard (no restart) and
 * debounces the NVS write — same volatile-cache + deferred-persist pattern as
 * the other settings so the flash commit never stalls the LVGL task. The
 * dropdown index maps 1:1 to the theme registry order. */
static void theme_dropdown_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    uint16_t sel = lv_dropdown_get_selected(lv_event_get_target(e));
    /* Runs on the LVGL thread (input handling) — safe to rebuild screens. The
     * Settings screen is up, so dashboard_theme_set rebuilds the dashboard
     * offscreen; it shows when the user taps "exit". */
    dashboard_theme_set((int)sel);
    settings_wrapper_set_dashboard_theme_volatile((uint8_t)sel);
    debounced_commit_schedule(&s_theme_commit, settings_wrapper_persist_dashboard_theme);
}

/* Boot-splash repeats dropdown. Index → repeat count; 0 = off. Applies on the
 * next boot (read by main/splash_screen.c at startup). */
static const uint8_t s_splash_loop_opts[] = {0, 1, 2, 3, 5};
#define SPLASH_LOOP_OPT_COUNT (sizeof(s_splash_loop_opts) / sizeof(s_splash_loop_opts[0]))

static uint16_t splash_loops_to_index(uint8_t loops) {
    for (uint16_t i = 0; i < SPLASH_LOOP_OPT_COUNT; i++) {
        if (s_splash_loop_opts[i] == loops) return i;
    }
    return 1;   /* default: "1" */
}

static void splash_loops_dropdown_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    uint16_t sel = lv_dropdown_get_selected(lv_event_get_target(e));
    if (sel >= SPLASH_LOOP_OPT_COUNT) sel = 1;
    settings_wrapper_set_splash_loops(s_splash_loop_opts[sel]);
}

/* Public entry from the dashboard's invisible full-screen brightness drag
 * slider (events_init_dashboard). Shares the s_brightness_commit debounce
 * timer with the settings-page slider so a fast drag doesn't queue two
 * NVS commits. Volatile setter fires the brightness callback right away,
 * the backlight tracks the drag live. */
void dashboard_brightness_slider_changed(int32_t value) {
    if (value < 0)   value = 0;
    if (value > 100) value = 100;
    settings_wrapper_set_brightness_volatile((uint8_t)value);
    debounced_commit_schedule(&s_brightness_commit,
                              settings_wrapper_persist_brightness);
}

// Event handler for brightness slider
static void brightness_slider_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    char buf[32];
    sprintf(buf, "Brightness: %d%%", (int)value);
    lv_label_set_text(settings_brightness_label, buf);

    /* Volatile setter fires the brightness callback immediately so the
     * backlight tracks the drag live; NVS commit is debounced. */
    settings_wrapper_set_brightness_volatile((uint8_t)value);
    debounced_commit_schedule(&s_brightness_commit,
                              settings_wrapper_persist_brightness);
}

// Event handler for Controller ID slider
static void controller_id_slider_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    char buf[32];
    sprintf(buf, "Controller ID: %d", (int)value);
    lv_label_set_text(settings_controller_id_label, buf);

    settings_wrapper_set_controller_id_volatile((uint8_t)value);
    debounced_commit_schedule(&s_controller_id_commit,
                              settings_wrapper_persist_controller_id);

    lv_label_set_text(settings_info_label, "Controller ID requires restart!");
}

// Battery Capacity — stored as int with 0.1 Ah step (e.g., 150 → 15.0 Ah)
static void battery_capacity_on_change(num_field_t *f) {
    settings_wrapper_set_battery_capacity_volatile((float)f->value / 10.0f);
    debounced_commit_schedule(&s_battery_capacity_commit,
                              settings_wrapper_persist_battery_capacity);
    if (settings_info_label) {
        lv_label_set_text(settings_info_label,
                          "Battery capacity changed - will recalibrate!");
    }
}

static void battery_capacity_plus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) num_field_inc(&s_battery_capacity_field);
}

static void battery_capacity_minus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) num_field_dec(&s_battery_capacity_field);
}

// Event handler for Battery Calculation Mode dropdown
static void battery_calc_mode_dropdown_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *dropdown = lv_event_get_target(e);
        uint16_t selected = lv_dropdown_get_selected(dropdown);
        
        // Save to settings
        settings_wrapper_set_battery_calc_mode((uint8_t)selected);
        
        // Update info label
        if (selected == 1) { // Smart Calculation
            lv_label_set_text(settings_info_label, "Smart calc enabled - will calibrate on next data!");
        } else {
            lv_label_set_text(settings_info_label, "Direct mode - using controller battery level");
        }
    }
}

// Power Max — stored as int with 0.1 kW step (e.g. 45 → 4.5 kW)
static void power_max_on_change(num_field_t *f) {
    settings_wrapper_set_power_max_kw_volatile((float)f->value / 10.0f);
    cockpit_refresh_power_max_label();
    debounced_commit_schedule(&s_power_max_commit,
                              settings_wrapper_persist_power_max_kw);
    if (settings_info_label) {
        lv_label_set_text(settings_info_label, "Power scale max updated");
    }
}

static void power_max_plus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) num_field_inc(&s_power_max_field);
}

static void power_max_minus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) num_field_dec(&s_power_max_field);
}

static void clock_apply_from_fields(void) {
    uint32_t cur_secs_of_day = settings_wrapper_get_clock_secs_of_day();
    int      cur_s = (int)(cur_secs_of_day % 60u);
    uint32_t new_secs_of_day =
        (uint32_t)s_clock_hour_field.value * 3600u +
        (uint32_t)s_clock_min_field.value  * 60u   +
        (uint32_t)cur_s;
    settings_wrapper_set_clock_secs_of_day(new_secs_of_day);
}

static void clock_hour_on_change(num_field_t *f) {
    (void)f;
    clock_apply_from_fields();
}

static void clock_min_on_change(num_field_t *f) {
    (void)f;
    clock_apply_from_fields();
}

static void clock_h_plus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int v = s_clock_hour_field.value + 1;
    if (v > 23) v = 0;       /* wrap 23→00 */
    num_field_set(&s_clock_hour_field, v);
}

static void clock_h_minus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int v = s_clock_hour_field.value - 1;
    if (v < 0) v = 23;       /* wrap 00→23 */
    num_field_set(&s_clock_hour_field, v);
}

static void clock_m_plus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int v = s_clock_min_field.value + 1;
    if (v > 59) v = 0;
    num_field_set(&s_clock_min_field, v);
}

static void clock_m_minus_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int v = s_clock_min_field.value - 1;
    if (v < 0) v = 59;
    num_field_set(&s_clock_min_field, v);
}

// Timer callback for checking limits response
static void limits_response_timer_cb(lv_timer_t* timer) {
#ifdef LV_REALDEVICE
    static int timeout_count = 0;
    
    if (vesc_limits_is_valid()) {
        // Data received, update UI
        const vesc_motor_limits_t* limits = vesc_limits_get();
        
        // Update spinboxes with received values
        lv_spinbox_set_value(settings_motor_current_spinbox, (int32_t)(limits->l_current_max * 10));
        lv_spinbox_set_value(settings_battery_current_spinbox, (int32_t)(limits->l_in_current_max * 10));
        lv_spinbox_set_value(settings_erpm_max_spinbox, (int32_t)(limits->l_erpm_max / 1000));
        
        // Update status
        char buf[128];
        sprintf(buf, "✅ Limits loaded: Mot %.1fA, Bat %.1fA, ERPM %dk",
                limits->l_current_max, limits->l_in_current_max, (int)(limits->l_erpm_max / 1000));
        lv_label_set_text(settings_limits_status_label, buf);
        
        // Delete timer
        lv_timer_del(timer);
        timeout_count = 0;
    } else {
        // Check timeout (5 seconds)
        timeout_count++;
        if (timeout_count >= 50) { // 50 * 100ms = 5 seconds
            lv_label_set_text(settings_limits_status_label, "❌ Timeout: No response from VESC");
            lv_timer_del(timer);
            timeout_count = 0;
        }
    }
#else
    // Simulator mode - show placeholder values
    lv_spinbox_set_value(settings_motor_current_spinbox, 600);  // 60.0A
    lv_spinbox_set_value(settings_battery_current_spinbox, 450); // 45.0A
    lv_spinbox_set_value(settings_erpm_max_spinbox, 60);         // 60k ERPM
    lv_label_set_text(settings_limits_status_label, "✅ Simulator: Placeholder values loaded");
    lv_timer_del(timer);
#endif
}

// Event handlers for VESC Limits buttons
static void read_limits_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
#ifdef LV_REALDEVICE
        // Real device - communicate with VESC
        uint8_t target_vesc_id = settings_wrapper_get_target_vesc_id();
        
        // Update status label
        lv_label_set_text(settings_limits_status_label, "Reading limits from VESC...");
        
        // Request limits from VESC
        if (vesc_limits_request(target_vesc_id)) {
            // Request sent successfully, wait for response
            lv_label_set_text(settings_limits_status_label, "Request sent, waiting for response...");
            
            // Start a timer to check for response
            lv_timer_create(limits_response_timer_cb, 100, NULL);
            
        } else {
            // Request failed
            lv_label_set_text(settings_limits_status_label, "❌ Failed to send request to VESC");
        }
#else
        // Simulator mode - load placeholder values immediately
        lv_label_set_text(settings_limits_status_label, "Simulator: Loading placeholder values...");
        
        // Start a timer to simulate loading delay
        lv_timer_create(limits_response_timer_cb, 100, NULL);
#endif
    }
}

static void apply_limits_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        // Get values from spinboxes
        int32_t motor_current_raw = lv_spinbox_get_value(settings_motor_current_spinbox);
        int32_t battery_current_raw = lv_spinbox_get_value(settings_battery_current_spinbox);
        int32_t erpm_max_raw = lv_spinbox_get_value(settings_erpm_max_spinbox);
        
        // Convert to actual values
        float motor_current = motor_current_raw / 10.0f;  // Convert from 0.1A units
        float battery_current = battery_current_raw / 10.0f;  // Convert from 0.1A units
        float erpm_max = erpm_max_raw * 1000.0f;  // Convert from kERPM to ERPM
        
#ifdef LV_REALDEVICE
        // Real device - apply to VESC
        uint8_t target_vesc_id = settings_wrapper_get_target_vesc_id();
        
        // Update status
        lv_label_set_text(settings_limits_status_label, "Applying limits to VESC...");
        
        // Apply limits using the vesc_limits module
        bool success = true;
        
        // Set current limits
        if (!vesc_limits_set_current_max(target_vesc_id, motor_current, battery_current)) {
            success = false;
        }
        
        // Set speed limit
        if (success && !vesc_limits_set_speed_max(target_vesc_id, erpm_max)) {
            success = false;
        }
        
        // Update status based on result
        if (success) {
            char buf[128];
            sprintf(buf, "✅ Applied: Mot %.1fA, Bat %.1fA, ERPM %dk",
                    motor_current, battery_current, (int)erpm_max_raw);
            lv_label_set_text(settings_limits_status_label, buf);
        } else {
            lv_label_set_text(settings_limits_status_label, "❌ Failed to apply limits to VESC");
        }
#else
        // Simulator mode - just show confirmation
        char buf[128];
        sprintf(buf, "✅ Simulator: Values set - Mot %.1fA, Bat %.1fA, ERPM %dk",
                motor_current, battery_current, (int)erpm_max_raw);
        lv_label_set_text(settings_limits_status_label, buf);
#endif
    }
}

static void motor_current_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_motor_current_spinbox);
        if (current_value < 2000) {
            lv_spinbox_increment(settings_motor_current_spinbox);
        }
    }
}

static void motor_current_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_motor_current_spinbox);
        if (current_value > 50) {
            lv_spinbox_decrement(settings_motor_current_spinbox);
        }
    }
}

static void battery_current_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_battery_current_spinbox);
        if (current_value < 2000) {
            lv_spinbox_increment(settings_battery_current_spinbox);
        }
    }
}

static void battery_current_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_battery_current_spinbox);
        if (current_value > 0) {
            lv_spinbox_decrement(settings_battery_current_spinbox);
        }
    }
}

static void erpm_max_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_erpm_max_spinbox);
        if (current_value < 2000) {
            lv_spinbox_increment(settings_erpm_max_spinbox);
        }
    }
}

static void erpm_max_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_erpm_max_spinbox);
        if (current_value > 0) {
            lv_spinbox_decrement(settings_erpm_max_spinbox);
        }
    }
}

/* Show FPS switch removed from the Settings UI — fps_text widget is no
 * longer rendered anywhere on the dashboard. Keeping the handler around
 * would just be dead code.
// Event handler for Show FPS switch
static void show_fps_switch_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *obj = lv_event_get_target(e);
        bool checked = lv_obj_has_state(obj, LV_STATE_CHECKED);
        settings_wrapper_set_show_fps(checked);
    }
}
*/

// Event handler for Demo mode switch
static void demo_mode_switch_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *obj = lv_event_get_target(e);
        bool checked = lv_obj_has_state(obj, LV_STATE_CHECKED);
        dashboard_demo_set_active(checked);
    }
}

// Event handler for VESC Emulator switch. Persists to NVS; takes effect on
// next boot (the CAN driver vs simulated source is wired once in main.c).
static void vesc_emulator_switch_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *obj = lv_event_get_target(e);
        bool checked = lv_obj_has_state(obj, LV_STATE_CHECKED);
        settings_wrapper_set_vesc_emulator(checked);
    }
}

// Event handler for AA Auto-Connect switch. The actual reconnect loop lives
// on the BT agent (D1 Mini); the setter fires a callback in main.c that
// pushes AUTO_RECONNECT|0|1 over UART, so the flag takes effect immediately.
static void aa_autoconnect_switch_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *obj = lv_event_get_target(e);
        bool checked = lv_obj_has_state(obj, LV_STATE_CHECKED);
        settings_wrapper_set_aa_autoconnect(checked);
    }
}

// Event handler for Units switch. OFF = metric (km, km/h), ON = imperial
// (miles, mph). Persists immediately and re-skins every speed/distance readout
// + the static unit captions via dashboard_units_changed().
static void units_switch_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *obj = lv_event_get_target(e);
        bool checked = lv_obj_has_state(obj, LV_STATE_CHECKED);
        settings_wrapper_set_use_imperial(checked);
        dashboard_units_changed();
        if (settings_info_label) {
            lv_label_set_text(settings_info_label,
                              checked ? "Units: miles / mph" : "Units: km / km/h");
        }
    }
}

// Event handler for Temperature units switch. OFF = Celsius, ON = Fahrenheit.
// Persists immediately and re-skins the temperature readouts + captions via
// dashboard_units_changed() (shared epoch with the km/miles toggle).
static void temp_unit_switch_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *obj = lv_event_get_target(e);
        bool checked = lv_obj_has_state(obj, LV_STATE_CHECKED);
        settings_wrapper_set_use_fahrenheit(checked);
        dashboard_units_changed();
        if (settings_info_label) {
            lv_label_set_text(settings_info_label,
                              checked ? "Temperature: Fahrenheit" : "Temperature: Celsius");
        }
    }
}

// Event handlers for Wheel Diameter spinbox
static void wheel_diameter_spinbox_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        int32_t value = lv_spinbox_get_value(settings_wheel_diameter_spinbox);
        settings_wrapper_set_wheel_diameter_mm((uint16_t)value);
    }
}

static void wheel_diameter_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_wheel_diameter_spinbox);
        if (current_value < 2000) {
            lv_spinbox_increment(settings_wheel_diameter_spinbox);
        }
    }
}

static void wheel_diameter_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_wheel_diameter_spinbox);
        if (current_value > 0) {
            lv_spinbox_decrement(settings_wheel_diameter_spinbox);
        }
    }
}

// Event handlers for Motor Poles spinbox
static void motor_poles_spinbox_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        int32_t value = lv_spinbox_get_value(settings_motor_poles_spinbox);
        settings_wrapper_set_motor_poles((uint8_t)value);
    }
}

static void motor_poles_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_motor_poles_spinbox);
        if (current_value < 50) {
            lv_spinbox_increment(settings_motor_poles_spinbox);
        }
    }
}

static void motor_poles_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_motor_poles_spinbox);
        if (current_value > 0) {
            lv_spinbox_decrement(settings_motor_poles_spinbox);
        }
    }
}

/* ====== Logs screen — created on-demand from Settings ============= */
/* A full-screen scrollable view backed by the log_capture ring buffer
 * in PSRAM. Allocated only when the user actually opens it; freed on
 * exit so the snapshot buffer (32 KB) doesn't sit around when nobody's
 * looking.
 *
 * A 500 ms lv_timer tails the ring buffer while the screen is up:
 * each tick we check whether the captured byte count grew, and if so
 * re-render the label and keep the scroll position pinned to the
 * bottom (unless the user has scrolled up to read older entries —
 * detected by being more than 40 px above the bottom). */
static lv_obj_t   *s_logs_screen     = NULL;
static lv_obj_t   *s_logs_label      = NULL;
static lv_obj_t   *s_logs_container  = NULL;
static char       *s_logs_text_buf   = NULL;
static lv_timer_t *s_logs_refresh_tmr = NULL;
static size_t      s_logs_last_total = 0;
/* 16 KB tail is ~200 lines at typical ESP_LOG density — enough to see
 * recent context, small enough that lv_label_set_text + recolor parse
 * stays responsive. Was 32 KB; the larger snapshot caused visible UI
 * hitching every 500 ms refresh because LVGL re-measured the entire
 * label on each set_text. */
#define LOGS_SNAPSHOT_BYTES (16u * 1024u)
#define LOGS_TAIL_SLACK_PX  40
/* Fixed label width — skips LV_SIZE_CONTENT's per-glyph width scan that
 * runs on every set_text. Wide enough that most log lines fit without
 * clipping; the container scrolls horizontally for the rare longer ones. */
#define LOGS_LABEL_WIDTH_PX 2000

static void logs_screen_destroy(void) {
    if (s_logs_refresh_tmr) {
        lv_timer_del(s_logs_refresh_tmr);
        s_logs_refresh_tmr = NULL;
    }
    s_logs_label = NULL;
    s_logs_container = NULL;
    if (s_logs_screen) {
        /* Async delete — safe to call from within an event callback
         * on the same object; LVGL queues the delete to run after the
         * current event dispatch returns. */
        lv_obj_del_async(s_logs_screen);
        s_logs_screen = NULL;
    }
    if (s_logs_text_buf) {
        free(s_logs_text_buf);
        s_logs_text_buf = NULL;
    }
}

static void logs_back_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    /* Switch to settings first; defer screen destruction until the
     * animation has finished so we don't yank widgets out from under
     * LVGL while it's still rendering them. */
    lv_scr_load_anim(guider_ui.settings, LV_SCR_LOAD_ANIM_MOVE_TOP,
                     200, 200, false);
}

static void logs_screen_loaded_event_cb(lv_event_t *e) {
    /* Auto-scroll to bottom so the newest log line is visible. Has to
     * run after the SCREEN_LOADED event because LVGL needs at least
     * one layout pass to know the label's actual height. */
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_LOADED) return;
    lv_obj_t *cont = (lv_obj_t *)lv_event_get_user_data(e);
    if (cont) {
        lv_obj_scroll_to_y(cont, lv_obj_get_scroll_bottom(cont), LV_ANIM_OFF);
    }
}

static void logs_screen_unloaded_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    /* User has gone back to settings (or somewhere else) — the screen
     * is off-screen now, safe to delete + free the snapshot. */
    logs_screen_destroy();
}

#ifdef LV_REALDEVICE
/* 500 ms refresh tick. Re-snapshots the ring buffer only when its byte
 * count has grown since the last tick, and only re-pins to the bottom
 * if the user wasn't actively scrolling up. */
static void logs_refresh_timer_cb(lv_timer_t *t) {
    (void)t;
    if (!s_logs_label || !s_logs_text_buf || !s_logs_container) return;

    size_t total = log_capture_total_bytes();
    if (total == s_logs_last_total) return;  /* nothing new */
    s_logs_last_total = total;

    bool was_at_bottom =
        lv_obj_get_scroll_bottom(s_logs_container) <= LOGS_TAIL_SLACK_PX;

    log_capture_snapshot_colorized(s_logs_text_buf, LOGS_SNAPSHOT_BYTES);
    lv_label_set_text(s_logs_label, s_logs_text_buf);

    if (was_at_bottom) {
        lv_obj_update_layout(s_logs_container);
        lv_coord_t down = lv_obj_get_scroll_bottom(s_logs_container);
        if (down > 0) {
            lv_obj_scroll_by(s_logs_container, 0, -down, LV_ANIM_OFF);
        }
    }
}
#endif

static void logs_open_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    /* Re-entrancy guard: if previous screen wasn't cleaned up, do it
     * now. Shouldn't normally happen but defensive. */
    logs_screen_destroy();

    s_logs_text_buf = malloc(LOGS_SNAPSHOT_BYTES);
    if (!s_logs_text_buf) return;
#ifdef LV_REALDEVICE
    log_capture_snapshot_colorized(s_logs_text_buf, LOGS_SNAPSHOT_BYTES);
    if (s_logs_text_buf[0] == '\0') {
        snprintf(s_logs_text_buf, LOGS_SNAPSHOT_BYTES,
                 "(no log entries captured yet)");
    }
#else
    snprintf(s_logs_text_buf, LOGS_SNAPSHOT_BYTES,
             "(log capture is disabled in the simulator build)");
#endif

    s_logs_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_logs_screen, 800, 480);
    lv_obj_set_style_bg_color(s_logs_screen, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(s_logs_screen, 255, 0);
    lv_obj_clear_flag(s_logs_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Back button — matches the "Back to dashboard" style on settings. */
    lv_obj_t *back_btn = lv_btn_create(s_logs_screen);
    lv_obj_set_pos(back_btn, 17, 14);
    lv_obj_set_size(back_btn, 770, 40);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_border_width(back_btn, 0, 0);
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back to settings");
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(back_lbl);
    lv_obj_add_event_cb(back_btn, logs_back_btn_event_cb,
                        LV_EVENT_CLICKED, NULL);

    /* Scrollable container holding the long log label. The label
     * sits inside its own clipping region so LVGL can scroll just
     * the log content, leaving the Back button pinned at the top. */
    lv_obj_t *cont = lv_obj_create(s_logs_screen);
    lv_obj_set_pos(cont, 10, 64);
    lv_obj_set_size(cont, 780, 410);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 8, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);

    /* Single lv_label with recolor — much lighter on RAM than one widget
     * per line. Long log lines aren't wrapped (LV_LABEL_LONG_CLIP +
     * fixed width); the container scrolls both vertically and horizontally
     * so the user can pan along long messages. */
    lv_obj_set_scroll_dir(cont, LV_DIR_ALL);

    lv_obj_t *log_lbl = lv_label_create(cont);
    lv_label_set_long_mode(log_lbl, LV_LABEL_LONG_CLIP);
    lv_label_set_recolor(log_lbl, true);
    lv_obj_set_width(log_lbl, LOGS_LABEL_WIDTH_PX);
    lv_obj_set_style_text_color(log_lbl, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(log_lbl, &lv_font_montserratMedium_16, 0);
    lv_obj_set_style_text_line_space(log_lbl, 2, 0);
    lv_label_set_text(log_lbl, s_logs_text_buf);

    s_logs_label     = log_lbl;
    s_logs_container = cont;

#ifdef LV_REALDEVICE
    s_logs_last_total = log_capture_total_bytes();
    /* 500 ms live-tail refresh while the screen is up. */
    s_logs_refresh_tmr = lv_timer_create(logs_refresh_timer_cb, 500, NULL);
#endif

    /* After the screen-load animation completes, jump scroll to the
     * bottom so the newest entries are on-screen. */
    lv_obj_add_event_cb(s_logs_screen, logs_screen_loaded_event_cb,
                        LV_EVENT_SCREEN_LOADED, cont);

    /* Hook destruction onto the SCREEN_UNLOADED event of the logs
     * screen — fires once the user has navigated away (back button)
     * and the move-top animation has completed. */
    lv_obj_add_event_cb(s_logs_screen, logs_screen_unloaded_event_cb,
                        LV_EVENT_SCREEN_UNLOADED, NULL);

    lv_scr_load_anim(s_logs_screen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM,
                     200, 0, false);
}

/* ===== QR codes screen — connection info & OTA URL ============= */

/* Connection info comes from the qr_info component (so we don't have
 * to depend on `main/` and create a circular link). */
#ifdef LV_REALDEVICE
#include "qr_info.h"
#endif

static lv_obj_t *s_qr_screen = NULL;

static void qr_screen_destroy(void) {
    if (s_qr_screen) {
        lv_obj_del_async(s_qr_screen);
        s_qr_screen = NULL;
    }
}

static void qr_back_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_scr_load_anim(guider_ui.settings, LV_SCR_LOAD_ANIM_MOVE_TOP,
                     200, 200, false);
}

static void qr_screen_unloaded_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    qr_screen_destroy();
}

/* Build one labelled QR tile: caption on top, QR in the middle, footer
 * with the plain-text value below. Returns nothing — caller positions
 * the returned container. The container is a plain lv_obj_t parented
 * to `parent`, positioned absolutely. */
static void qr_tile_create(lv_obj_t *parent, int x, int y,
                           const char *caption,
                           const char *data, const char *footer)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, 340, 380);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x1a1f25), 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_set_style_pad_all(box, 16, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cap = lv_label_create(box);
    lv_label_set_text(cap, caption);
    lv_obj_set_style_text_color(cap, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(cap, &lv_font_montserrat_24, 0);
    lv_obj_align(cap, LV_ALIGN_TOP_MID, 0, 0);

    /* 260 px QR — fits the box with room for caption + footer. Dark on
     * light, the canonical scannable combination. */
    lv_obj_t *qr = lv_qrcode_create(box, 260,
                                    lv_color_hex(0x000000),
                                    lv_color_hex(0xFFFFFF));
    lv_qrcode_update(qr, data, strlen(data));
    lv_obj_align(qr, LV_ALIGN_CENTER, 0, 6);
    /* 4 px white quiet zone around the modules — scanners need it. */
    lv_obj_set_style_border_color(qr, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(qr, 4, 0);

    if (footer && footer[0]) {
        lv_obj_t *ft = lv_label_create(box);
        lv_label_set_text(ft, footer);
        lv_label_set_long_mode(ft, LV_LABEL_LONG_DOT);
        lv_obj_set_width(ft, 308);
        lv_obj_set_style_text_color(ft, lv_color_hex(0xa0a8b0), 0);
        lv_obj_set_style_text_font(ft, &lv_font_montserratMedium_16, 0);
        lv_obj_set_style_text_align(ft, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(ft, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
}

static void qr_open_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    qr_screen_destroy();

    s_qr_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_qr_screen, 800, 480);
    lv_obj_set_style_bg_color(s_qr_screen, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(s_qr_screen, 255, 0);
    lv_obj_clear_flag(s_qr_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Back button — same style as the logs screen. */
    lv_obj_t *back_btn = lv_btn_create(s_qr_screen);
    lv_obj_set_pos(back_btn, 17, 14);
    lv_obj_set_size(back_btn, 770, 40);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_border_width(back_btn, 0, 0);
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back to settings");
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(back_lbl);
    lv_obj_add_event_cb(back_btn, qr_back_btn_event_cb,
                        LV_EVENT_CLICKED, NULL);

    char wifi_qr[160]  = "(no AP)";
    char wifi_foot[80] = "Wi-Fi not in AP mode";
    char ota_url[64]   = "(unavailable)";
    char ota_foot[80]  = "OTA HTTP server is off";

#ifdef LV_REALDEVICE
    char tmp[160];
    if (qr_info_get_wifi_qr(tmp, sizeof(tmp))) {
        strlcpy(wifi_qr, tmp, sizeof(wifi_qr));
        /* For the footer caption, parse out S: and show the bare SSID
         * rather than the full WIFI: URI — friendlier to read. */
        const char *s = strstr(tmp, "S:");
        if (s) {
            s += 2;
            const char *end = strchr(s, ';');
            size_t n = end ? (size_t)(end - s) : strlen(s);
            if (n >= sizeof(wifi_foot)) n = sizeof(wifi_foot) - 1;
            memcpy(wifi_foot, s, n);
            wifi_foot[n] = '\0';
        }
    }
    if (qr_info_get_ota_url(tmp, sizeof(tmp))) {
        strlcpy(ota_url, tmp, sizeof(ota_url));
        strlcpy(ota_foot, tmp, sizeof(ota_foot));
    }
#endif

    /* Two 340-wide tiles, 20 px outer + 20 px gap. (340*2 + 20*3 = 740) */
    qr_tile_create(s_qr_screen,  30, 70, "Wi-Fi",        wifi_qr, wifi_foot);
    qr_tile_create(s_qr_screen, 430, 70, "Firmware OTA", ota_url, ota_foot);

    lv_obj_add_event_cb(s_qr_screen, qr_screen_unloaded_event_cb,
                        LV_EVENT_SCREEN_UNLOADED, NULL);

    lv_scr_load_anim(s_qr_screen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM,
                     200, 0, false);
}

// Event handler for "Reset trip" — zero trip / Ah without touching settings.
// Sometimes the trip doesn't auto-reset on battery swap, so a manual button.
static void reset_trip_button_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
#ifdef LV_REALDEVICE
    battery_calc_reset_trip_and_ah();
#endif
    if (settings_info_label) {
        lv_label_set_text(settings_info_label, "Trip reset");
    }
}

// Event handler for Reset button
static void reset_button_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        // Reset all settings to defaults
        settings_wrapper_set_target_vesc_id(10);
        settings_wrapper_set_second_head_enabled(false);
        settings_wrapper_set_second_head_id(11);
        settings_wrapper_set_can_speed_index(3); // 1000 kbps
        settings_wrapper_set_brightness(80);
        settings_wrapper_set_controller_id(2);
        settings_wrapper_set_battery_capacity(15.0f);
        settings_wrapper_set_battery_calc_mode(0); // Direct
        settings_wrapper_set_show_fps(true);
        settings_wrapper_set_wheel_diameter_mm(200); // 200mm
        settings_wrapper_set_motor_poles(7); // Standard for VESC
        settings_wrapper_set_power_max_kw(4.5f);
        settings_wrapper_set_dashboard_theme(0); // Cockpit
        settings_wrapper_set_splash_loops(1);     // play boot splash once

        // Update UI elements
        if (s_target_id_field.label)        num_field_set(&s_target_id_field, 10);
        if (s_second_head_id_field.label)   num_field_set(&s_second_head_id_field, 11);
        if (settings_second_head_switch) {
            lv_obj_clear_state(settings_second_head_switch, LV_STATE_CHECKED);
        }
        if (settings_can_speed_dropdown) {
            lv_dropdown_set_selected(settings_can_speed_dropdown, 3);
        }
        if (settings_theme_dropdown) {
            lv_dropdown_set_selected(settings_theme_dropdown, 0);
        }
        if (settings_splash_loops_dropdown) {
            lv_dropdown_set_selected(settings_splash_loops_dropdown, splash_loops_to_index(1));
        }
        dashboard_theme_set(0);   // live-switch back to cockpit
        if (settings_brightness_slider) {
            lv_slider_set_value(settings_brightness_slider, 80, LV_ANIM_ON);
        }
        if (settings_controller_id_slider) {
            lv_slider_set_value(settings_controller_id_slider, 2, LV_ANIM_ON);
        }
        if (s_battery_capacity_field.label) num_field_set(&s_battery_capacity_field, 150); // 15.0 Ah
        if (settings_battery_calc_mode_dropdown) {
            lv_dropdown_set_selected(settings_battery_calc_mode_dropdown, 0);
        }
        if (settings_show_fps_switch) {
            lv_obj_add_state(settings_show_fps_switch, LV_STATE_CHECKED);
        }
        if (settings_wheel_diameter_spinbox) {
            lv_spinbox_set_value(settings_wheel_diameter_spinbox, 200); // 200mm
        }
        if (settings_motor_poles_spinbox) {
            lv_spinbox_set_value(settings_motor_poles_spinbox, 7); // Standard for VESC
        }
        if (s_power_max_field.label)        num_field_set(&s_power_max_field, 45); // 4.5 kW
        cockpit_refresh_power_max_label();

        /* Cockpit: fps_text removed — nothing to toggle visibility on. */
        // if (guider_ui.dashboard_Classic_fps_text) {
        //     lv_obj_clear_flag(guider_ui.dashboard_Classic_fps_text, LV_OBJ_FLAG_HIDDEN);
        // }

        // Update info
        lv_label_set_text(settings_info_label, "Settings reset to defaults!");
    }
}

/* On-device file browser — strong impl lives in main/files_screen.c (pulled
 * in via files_screen_link_anchor referenced from main.c). The weak stub below
 * keeps vesc_ui linkable on its own and lets the desktop simulator build (it
 * just logs a tap). */
__attribute__((weak)) void files_screen_show(void)
{
    printf("files_screen_show (no impl linked)\n");
}

static void settings_files_button_event_cb(lv_event_t *e)
{
    (void)e;
    files_screen_show();
}

// Initialize settings UI - called from custom_init or when settings screen loads
void settings_ui_init(lv_ui *ui) {
    if (!ui || !ui->settings) {
        return;
    }
    
    /* Guard: skip re-init only if our widgets still belong to the current
     * ui->settings. setup_scr_settings re-creates ui->settings via
     * lv_obj_create(NULL) every time settings_del=true, and gui_guider's
     * ui_load_scr_animation never resets settings_del after calling
     * setup_scr. So if the user enters Settings, toggles AA/VESC mode (which
     * bypasses the "Back to dashboard" path that would have cleared the
     * flag), then re-enters Settings, ui->settings is fresh but our static
     * pointers still reference orphaned widgets on the old screen. Without
     * the parent check the early-return would leave only the generated
     * "Back to dashboard" button visible. */
    if (s_target_id_field.label != NULL &&
        lv_obj_get_parent(s_target_id_field.label) == ui->settings) {
        return;
    }
    
    // Initialize modules
    settings_wrapper_init();
    
#ifdef LV_REALDEVICE
    // Initialize vesc_limits only on real device
    vesc_limits_init();
#endif
    
    // Get current settings
    uint8_t target_id = settings_wrapper_get_target_vesc_id();
    uint8_t can_speed_idx = settings_wrapper_get_can_speed_index();
    uint8_t brightness = settings_wrapper_get_brightness();
    uint8_t controller_id = settings_wrapper_get_controller_id();
    float battery_capacity = settings_wrapper_get_battery_capacity();
    uint8_t battery_calc_mode = settings_wrapper_get_battery_calc_mode();
    
    int y_pos = 70; // Start below "Back to dashboard" button

    // ========== Target VESC ID ==========
    settings_target_id_label = settings_heading_create(ui->settings, y_pos, "Target VESC ID:");
    s_target_id_field = (num_field_t){
        .value = target_id, .min = 1, .max = 254, .step = 1, .decimals = 0,
        .on_change = target_id_on_change,
    };
    settings_target_id_minus_btn = settings_step_btn_create(ui->settings,
        SETTINGS_MINUS_X, y_pos + 5, "-", 0xff4444, target_id_minus_btn_event_cb);
    settings_value_label_init(ui->settings, &s_target_id_field, y_pos + 5, 0x00a9ff);
    settings_target_id_plus_btn = settings_step_btn_create(ui->settings,
        SETTINGS_PLUS_X, y_pos + 5, "+", 0x00a9ff, target_id_plus_btn_event_cb);
    y_pos += SETTINGS_ROW_H;

    // ========== Second head (dual-motor board) ==========
    // Enable when the controller is a dual (two VESC nodes on one CAN bus).
    // The second head's temps/liveness are read passively from its CAN STATUS,
    // so the user must enable "Send status over CAN" (1-5 @ 50 Hz) on it.
    settings_second_head_label = settings_heading_create(ui->settings, y_pos, "Second head:");
    settings_second_head_switch = lv_switch_create(ui->settings);
    lv_obj_set_pos(settings_second_head_switch, 730, y_pos + 15);
    lv_obj_set_size(settings_second_head_switch, 60, 30);
    if (settings_wrapper_get_second_head_enabled()) {
        lv_obj_add_state(settings_second_head_switch, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(settings_second_head_switch, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_second_head_switch, lv_color_hex(0x00a9ff),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(settings_second_head_switch, second_head_switch_event_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);
    y_pos += SETTINGS_ROW_H;

    // ========== Second head CAN ID ==========
    settings_second_head_id_label = settings_heading_create(ui->settings, y_pos, "Second head ID:");
    s_second_head_id_field = (num_field_t){
        .value = settings_wrapper_get_second_head_id(), .min = 1, .max = 254,
        .step = 1, .decimals = 0, .on_change = second_head_id_on_change,
    };
    settings_second_head_id_minus_btn = settings_step_btn_create(ui->settings,
        SETTINGS_MINUS_X, y_pos + 5, "-", 0xff4444, second_head_id_minus_btn_event_cb);
    settings_value_label_init(ui->settings, &s_second_head_id_field, y_pos + 5, 0x00a9ff);
    settings_second_head_id_plus_btn = settings_step_btn_create(ui->settings,
        SETTINGS_PLUS_X, y_pos + 5, "+", 0x00a9ff, second_head_id_plus_btn_event_cb);
    y_pos += SETTINGS_ROW_H;
    
    // ========== CAN Speed Dropdown ==========
    settings_can_speed_label = settings_heading_create(ui->settings, y_pos, "CAN Speed (kbps)");

    settings_can_speed_dropdown = lv_dropdown_create(ui->settings);
    lv_dropdown_set_options(settings_can_speed_dropdown, "125 kbps\n250 kbps\n500 kbps\n1000 kbps");
    lv_dropdown_set_selected(settings_can_speed_dropdown, can_speed_idx);
    lv_obj_set_pos(settings_can_speed_dropdown, 400, y_pos + 5);
    lv_obj_set_size(settings_can_speed_dropdown, 390, 50);
    lv_obj_set_style_bg_color(settings_can_speed_dropdown, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_text_color(settings_can_speed_dropdown, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_can_speed_dropdown, &lv_font_montserrat_24, 0);
    lv_obj_set_style_border_width(settings_can_speed_dropdown, 0, 0);
    lv_obj_set_style_radius(settings_can_speed_dropdown, 8, 0);
    lv_obj_add_event_cb(settings_can_speed_dropdown, can_speed_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    y_pos += SETTINGS_ROW_H;

    // ========== Dashboard Theme Dropdown ==========
    // Options are built from the dashboard-theme registry, so any theme added
    // (via theme_*_register in custom_init_once) shows up here automatically.
    settings_theme_label = settings_heading_create(ui->settings, y_pos, "Dashboard theme:");

    char theme_opts[192];
    size_t theme_off = 0;
    int theme_n = dashboard_theme_count();
    for (int i = 0; i < theme_n && theme_off < sizeof(theme_opts); i++) {
        const dashboard_theme_t *t = dashboard_theme_get(i);
        theme_off += snprintf(theme_opts + theme_off, sizeof(theme_opts) - theme_off,
                              "%s%s", i ? "\n" : "", (t && t->name) ? t->name : "?");
    }
    if (theme_n == 0) snprintf(theme_opts, sizeof(theme_opts), "Cockpit");

    settings_theme_dropdown = lv_dropdown_create(ui->settings);
    lv_dropdown_set_options(settings_theme_dropdown, theme_opts);
    int active_theme = dashboard_theme_active_index();
    lv_dropdown_set_selected(settings_theme_dropdown, active_theme < 0 ? 0 : active_theme);
    lv_obj_set_pos(settings_theme_dropdown, 400, y_pos + 5);
    lv_obj_set_size(settings_theme_dropdown, 390, 50);
    lv_obj_set_style_bg_color(settings_theme_dropdown, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_text_color(settings_theme_dropdown, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_theme_dropdown, &lv_font_montserrat_24, 0);
    lv_obj_set_style_border_width(settings_theme_dropdown, 0, 0);
    lv_obj_set_style_radius(settings_theme_dropdown, 8, 0);
    lv_obj_add_event_cb(settings_theme_dropdown, theme_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    y_pos += SETTINGS_ROW_H;

    // ========== Boot Splash Repeats ==========
    settings_splash_loops_label = settings_heading_create(ui->settings, y_pos, "Boot splash repeats:");

    settings_splash_loops_dropdown = lv_dropdown_create(ui->settings);
    lv_dropdown_set_options(settings_splash_loops_dropdown, "Off\n1\n2\n3\n5");
    lv_dropdown_set_selected(settings_splash_loops_dropdown,
                             splash_loops_to_index(settings_wrapper_get_splash_loops()));
    lv_obj_set_pos(settings_splash_loops_dropdown, 400, y_pos + 5);
    lv_obj_set_size(settings_splash_loops_dropdown, 390, 50);
    lv_obj_set_style_bg_color(settings_splash_loops_dropdown, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_text_color(settings_splash_loops_dropdown, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_splash_loops_dropdown, &lv_font_montserrat_24, 0);
    lv_obj_set_style_border_width(settings_splash_loops_dropdown, 0, 0);
    lv_obj_set_style_radius(settings_splash_loops_dropdown, 8, 0);
    lv_obj_add_event_cb(settings_splash_loops_dropdown, splash_loops_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    y_pos += SETTINGS_ROW_H;

    // ========== Battery Capacity ==========
    settings_battery_capacity_label = settings_heading_create(ui->settings, y_pos, "Battery Capacity (Ah):");
    s_battery_capacity_field = (num_field_t){
        .value = (int)(battery_capacity * 10.0f),
        .min = 10, .max = 2000, .step = 1, .decimals = 1,
        .on_change = battery_capacity_on_change,
    };
    settings_battery_capacity_minus_btn = settings_step_btn_create(ui->settings,
        SETTINGS_MINUS_X, y_pos + 5, "-", 0xff4444, battery_capacity_minus_btn_event_cb);
    settings_value_label_init(ui->settings, &s_battery_capacity_field, y_pos + 5, 0xffa500);
    settings_battery_capacity_plus_btn = settings_step_btn_create(ui->settings,
        SETTINGS_PLUS_X, y_pos + 5, "+", 0x00a9ff, battery_capacity_plus_btn_event_cb);
    y_pos += SETTINGS_ROW_H;
    
    // ========== Battery Calculation Mode Dropdown ==========
    settings_battery_calc_mode_label = settings_heading_create(ui->settings, y_pos, "Battery Calculation:");

    settings_battery_calc_mode_dropdown = lv_dropdown_create(ui->settings);
    lv_dropdown_set_options(settings_battery_calc_mode_dropdown, "Direct from Controller\nSmart Calculation");
    lv_dropdown_set_selected(settings_battery_calc_mode_dropdown, battery_calc_mode);
    lv_obj_set_pos(settings_battery_calc_mode_dropdown, 400, y_pos + 5);
    lv_obj_set_size(settings_battery_calc_mode_dropdown, 390, 50);
    lv_obj_set_style_bg_color(settings_battery_calc_mode_dropdown, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_text_color(settings_battery_calc_mode_dropdown, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_calc_mode_dropdown, &lv_font_montserrat_24, 0);
    lv_obj_set_style_border_width(settings_battery_calc_mode_dropdown, 0, 0);
    lv_obj_set_style_radius(settings_battery_calc_mode_dropdown, 8, 0);
    lv_obj_add_event_cb(settings_battery_calc_mode_dropdown, battery_calc_mode_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    y_pos += SETTINGS_ROW_H;

    // ========== Power Max (kW) ==========
    // Full-scale value for the cockpit power bar — drives both bar fill ratio
    // and the "X.X KW" label next to it.
    float power_max_kw = settings_wrapper_get_power_max_kw();

    settings_power_max_label = settings_heading_create(ui->settings, y_pos, "Power Max (kW):");
    s_power_max_field = (num_field_t){
        .value = (int)(power_max_kw * 10.0f + 0.5f),
        .min = 1, .max = 1000, .step = 1, .decimals = 1,
        .on_change = power_max_on_change,
    };
    settings_power_max_minus_btn = settings_step_btn_create(ui->settings,
        SETTINGS_MINUS_X, y_pos + 5, "-", 0xff4444, power_max_minus_btn_event_cb);
    settings_value_label_init(ui->settings, &s_power_max_field, y_pos + 5, 0xffa500);
    settings_power_max_plus_btn = settings_step_btn_create(ui->settings,
        SETTINGS_PLUS_X, y_pos + 5, "+", 0x00a9ff, power_max_plus_btn_event_cb);
    y_pos += SETTINGS_ROW_H;

#if !defined(ENABLE_WALL_CLOCK) || ENABLE_WALL_CLOCK
    // ========== Wall clock (Time of day) ==========
    uint32_t clock_now = settings_wrapper_get_clock_secs_of_day();
    int      clock_h_now = (int)(clock_now / 3600u);
    int      clock_m_now = (int)((clock_now / 60u) % 60u);
    /* Hours cluster — placed in the gap between heading and the minutes
     * cluster on the right. Mirrors the SETTINGS_*_X spacing pattern but
     * shifted ~250 px to the left so the row fits in 800 px. */
    enum { CLK_H_MINUS_X = 300, CLK_H_VAL_X = 370, CLK_H_PLUS_X = 470 };
    settings_clock_heading_label = settings_heading_create(ui->settings, y_pos, "Time:");
    s_clock_hour_field = (num_field_t){
        .value = clock_h_now, .min = 0, .max = 23, .step = 1, .decimals = 0,
        .on_change = clock_hour_on_change,
    };
    settings_clock_h_minus_btn = settings_step_btn_create(ui->settings,
        CLK_H_MINUS_X, y_pos + 5, "-", 0xff4444, clock_h_minus_btn_event_cb);
    s_clock_hour_field.label = lv_label_create(ui->settings);
    lv_obj_set_pos(s_clock_hour_field.label, CLK_H_VAL_X, y_pos + 5);
    lv_obj_set_size(s_clock_hour_field.label, SETTINGS_VAL_W, 50);
    lv_obj_set_style_text_color(s_clock_hour_field.label, lv_color_hex(0xB6FF2E), 0);
    lv_obj_set_style_text_font(s_clock_hour_field.label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(s_clock_hour_field.label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(s_clock_hour_field.label, 11, 0);
    num_field_refresh(&s_clock_hour_field);
    settings_clock_h_plus_btn = settings_step_btn_create(ui->settings,
        CLK_H_PLUS_X, y_pos + 5, "+", 0x00a9ff, clock_h_plus_btn_event_cb);

    /* ':' separator between H and M clusters. */
    settings_clock_colon_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_clock_colon_label, ":");
    lv_obj_set_pos(settings_clock_colon_label, 537, y_pos + 16);
    lv_obj_set_style_text_color(settings_clock_colon_label, lv_color_hex(0xB6FF2E), 0);
    lv_obj_set_style_text_font(settings_clock_colon_label, &lv_font_montserrat_24, 0);

    /* Minutes cluster — same right-edge anchors used by the other rows. */
    s_clock_min_field = (num_field_t){
        .value = clock_m_now, .min = 0, .max = 59, .step = 1, .decimals = 0,
        .on_change = clock_min_on_change,
    };
    settings_clock_m_minus_btn = settings_step_btn_create(ui->settings,
        SETTINGS_MINUS_X, y_pos + 5, "-", 0xff4444, clock_m_minus_btn_event_cb);
    settings_value_label_init(ui->settings, &s_clock_min_field, y_pos + 5, 0xB6FF2E);
    settings_clock_m_plus_btn = settings_step_btn_create(ui->settings,
        SETTINGS_PLUS_X, y_pos + 5, "+", 0x00a9ff, clock_m_plus_btn_event_cb);
    y_pos += SETTINGS_ROW_H;
#endif  /* ENABLE_WALL_CLOCK */

    /*
    // ========== VESC LIMITS SECTION ==========
    // Title
    settings_limits_title_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_limits_title_label, "=== VESC Motor Limits ===");
    lv_obj_set_pos(settings_limits_title_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_limits_title_label, lv_color_hex(0xffa500), 0);
    lv_obj_set_style_text_font(settings_limits_title_label, &lv_font_montserrat_24, 0);
    
    y_pos += 35;
    
    // Read Limits Button
    settings_read_limits_btn = lv_btn_create(ui->settings);
    lv_obj_t *read_limits_label = lv_label_create(settings_read_limits_btn);
    lv_label_set_text(read_limits_label, "Read Limits from VESC");
    lv_obj_align(read_limits_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_read_limits_btn, 20, y_pos);
    lv_obj_set_size(settings_read_limits_btn, 770, 50);
    lv_obj_set_style_bg_color(settings_read_limits_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_read_limits_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_read_limits_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_read_limits_btn, 8, 0);
    lv_obj_set_style_border_width(settings_read_limits_btn, 0, 0);
    lv_obj_add_event_cb(settings_read_limits_btn, read_limits_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += 70;
    
    // Motor Current Max Spinbox
    settings_motor_current_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_motor_current_label, "Motor Current Max (A):");
    lv_obj_set_pos(settings_motor_current_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_motor_current_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_current_label, &lv_font_montserrat_24, 0);
    
    // Minus button
    settings_motor_current_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *motor_curr_minus_label = lv_label_create(settings_motor_current_minus_btn);
    lv_label_set_text(motor_curr_minus_label, "-");
    lv_obj_align(motor_curr_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_motor_current_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_motor_current_minus_btn, 130, 50);
    lv_obj_set_style_bg_color(settings_motor_current_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_motor_current_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_current_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_motor_current_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_motor_current_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_motor_current_minus_btn, motor_current_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Spinbox
    settings_motor_current_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_motor_current_spinbox, 50, 2000); // 5.0 to 200.0 A
    lv_spinbox_set_digit_format(settings_motor_current_spinbox, 4, 3); // Format: "60.0"
    lv_spinbox_set_value(settings_motor_current_spinbox, 600); // Default 60.0A
    lv_spinbox_set_step(settings_motor_current_spinbox, 10); // 1.0 A steps
    lv_obj_set_pos(settings_motor_current_spinbox, 335, y_pos + 30);
    lv_obj_set_size(settings_motor_current_spinbox, 130, 50);
    lv_obj_set_style_bg_color(settings_motor_current_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_motor_current_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_motor_current_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_motor_current_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_motor_current_spinbox, lv_color_hex(0xff6600), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_motor_current_spinbox, 8, LV_PART_MAIN);
    
    // Plus button
    settings_motor_current_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *motor_curr_plus_label = lv_label_create(settings_motor_current_plus_btn);
    lv_label_set_text(motor_curr_plus_label, "+");
    lv_obj_align(motor_curr_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_motor_current_plus_btn, 660, y_pos + 30);
    lv_obj_set_size(settings_motor_current_plus_btn, 130, 50);
    lv_obj_set_style_bg_color(settings_motor_current_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_motor_current_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_current_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_motor_current_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_motor_current_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_motor_current_plus_btn, motor_current_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += spacing;
    
    // Battery Current Max Spinbox
    settings_battery_current_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_battery_current_label, "Battery Current Max (A):");
    lv_obj_set_pos(settings_battery_current_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_battery_current_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_current_label, &lv_font_montserrat_24, 0);
    
    // Minus button
    settings_battery_current_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *batt_curr_minus_label = lv_label_create(settings_battery_current_minus_btn);
    lv_label_set_text(batt_curr_minus_label, "-");
    lv_obj_align(batt_curr_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_battery_current_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_battery_current_minus_btn, 130, 50);
    lv_obj_set_style_bg_color(settings_battery_current_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_battery_current_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_current_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_battery_current_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_battery_current_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_battery_current_minus_btn, battery_current_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Spinbox
    settings_battery_current_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_battery_current_spinbox, 50, 2000); // 5.0 to 200.0 A
    lv_spinbox_set_digit_format(settings_battery_current_spinbox, 4, 3); // Format: "45.0"
    lv_spinbox_set_value(settings_battery_current_spinbox, 450); // Default 45.0A
    lv_spinbox_set_step(settings_battery_current_spinbox, 10); // 1.0 A steps
    lv_obj_set_pos(settings_battery_current_spinbox, 335, y_pos + 30);
    lv_obj_set_size(settings_battery_current_spinbox, 130, 50);
    lv_obj_set_style_bg_color(settings_battery_current_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_battery_current_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_battery_current_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_battery_current_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_battery_current_spinbox, lv_color_hex(0xffaa00), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_battery_current_spinbox, 8, LV_PART_MAIN);
    
    // Plus button
    settings_battery_current_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *batt_curr_plus_label = lv_label_create(settings_battery_current_plus_btn);
    lv_label_set_text(batt_curr_plus_label, "+");
    lv_obj_align(batt_curr_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_battery_current_plus_btn, 660, y_pos + 30);
    lv_obj_set_size(settings_battery_current_plus_btn, 130, 50);
    lv_obj_set_style_bg_color(settings_battery_current_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_battery_current_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_current_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_battery_current_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_battery_current_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_battery_current_plus_btn, battery_current_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += spacing;
    
    // ERPM Max Spinbox
    settings_erpm_max_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_erpm_max_label, "ERPM Max (x1000):");
    lv_obj_set_pos(settings_erpm_max_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_erpm_max_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_erpm_max_label, &lv_font_montserrat_24, 0);
    
    // Minus button
    settings_erpm_max_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *erpm_minus_label = lv_label_create(settings_erpm_max_minus_btn);
    lv_label_set_text(erpm_minus_label, "-");
    lv_obj_align(erpm_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_erpm_max_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_erpm_max_minus_btn, 130, 50);
    lv_obj_set_style_bg_color(settings_erpm_max_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_erpm_max_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_erpm_max_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_erpm_max_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_erpm_max_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_erpm_max_minus_btn, erpm_max_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Spinbox  
    settings_erpm_max_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_erpm_max_spinbox, 10, 200); // 10k to 200k ERPM
    lv_spinbox_set_digit_format(settings_erpm_max_spinbox, 3, 0); // Format: "60" (means 60k)
    lv_spinbox_set_value(settings_erpm_max_spinbox, 60); // Default 60k ERPM
    lv_spinbox_set_step(settings_erpm_max_spinbox, 5); // 5k ERPM steps
    lv_obj_set_pos(settings_erpm_max_spinbox, 335, y_pos + 30);
    lv_obj_set_size(settings_erpm_max_spinbox, 130, 50);
    lv_obj_set_style_bg_color(settings_erpm_max_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_erpm_max_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_erpm_max_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_erpm_max_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_erpm_max_spinbox, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_erpm_max_spinbox, 8, LV_PART_MAIN);
    
    // Plus button
    settings_erpm_max_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *erpm_plus_label = lv_label_create(settings_erpm_max_plus_btn);
    lv_label_set_text(erpm_plus_label, "+");
    lv_obj_align(erpm_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_erpm_max_plus_btn, 660, y_pos + 30);
    lv_obj_set_size(settings_erpm_max_plus_btn, 130, 50);
    lv_obj_set_style_bg_color(settings_erpm_max_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_erpm_max_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_erpm_max_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_erpm_max_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_erpm_max_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_erpm_max_plus_btn, erpm_max_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += spacing;
    
    // Apply Limits Button
    settings_apply_limits_btn = lv_btn_create(ui->settings);
    lv_obj_t *apply_limits_label = lv_label_create(settings_apply_limits_btn);
    lv_label_set_text(apply_limits_label, "Apply Limits to VESC");
    lv_obj_align(apply_limits_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_apply_limits_btn, 20, y_pos);
    lv_obj_set_size(settings_apply_limits_btn, 770, 50);
    lv_obj_set_style_bg_color(settings_apply_limits_btn, lv_color_hex(0xff6600), 0);
    lv_obj_set_style_text_color(settings_apply_limits_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_apply_limits_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_apply_limits_btn, 8, 0);
    lv_obj_set_style_border_width(settings_apply_limits_btn, 0, 0);
    lv_obj_add_event_cb(settings_apply_limits_btn, apply_limits_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += 70;
    
    // Limits Status Label
    settings_limits_status_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_limits_status_label, "Press 'Read Limits' to load VESC values");
    lv_obj_set_pos(settings_limits_status_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_limits_status_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(settings_limits_status_label, &lv_font_montserrat_24, 0);
    
    y_pos += spacing;
    */
    /*
    // ========== Brightness Slider ==========
    settings_brightness_label = lv_label_create(ui->settings);
    sprintf(buf, "Brightness: %d%%", brightness);
    lv_label_set_text(settings_brightness_label, buf);
    lv_obj_set_pos(settings_brightness_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_brightness_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_brightness_label, &lv_font_montserrat_24, 0);
    
    settings_brightness_slider = lv_slider_create(ui->settings);
    lv_slider_set_range(settings_brightness_slider, 10, 100);
    lv_slider_set_value(settings_brightness_slider, brightness, LV_ANIM_OFF);
    lv_obj_set_pos(settings_brightness_slider, 20, y_pos + 35);
    lv_obj_set_size(settings_brightness_slider, 770, 20); // Slightly thicker for better touch
    lv_obj_set_style_bg_color(settings_brightness_slider, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_brightness_slider, lv_color_hex(0xffa500), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(settings_brightness_slider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_set_style_radius(settings_brightness_slider, 10, LV_PART_MAIN); // Rounded slider
    lv_obj_set_style_radius(settings_brightness_slider, 10, LV_PART_INDICATOR);
    lv_obj_add_event_cb(settings_brightness_slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    y_pos += spacing;
    */
    /*
    // ========== Controller ID Slider ==========
    settings_controller_id_label = lv_label_create(ui->settings);
    sprintf(buf, "Controller ID: %d", controller_id);
    lv_label_set_text(settings_controller_id_label, buf);
    lv_obj_set_pos(settings_controller_id_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_controller_id_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_controller_id_label, &lv_font_montserratMedium_16, 0);
    
    settings_controller_id_slider = lv_slider_create(ui->settings);
    lv_slider_set_range(settings_controller_id_slider, 1, 254);
    lv_slider_set_value(settings_controller_id_slider, controller_id, LV_ANIM_OFF);
    lv_obj_set_pos(settings_controller_id_slider, 20, y_pos + 30);
    lv_obj_set_size(settings_controller_id_slider, 770, 15);
    lv_obj_set_style_bg_color(settings_controller_id_slider, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_controller_id_slider, lv_color_hex(0x00ff00), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(settings_controller_id_slider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_add_event_cb(settings_controller_id_slider, controller_id_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    y_pos += spacing;
    */
    
    /*
    // ========== Show FPS Switch ==========
    bool show_fps = settings_wrapper_get_show_fps();

    settings_show_fps_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_show_fps_label, "Show FPS Counter");
    lv_obj_set_pos(settings_show_fps_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_show_fps_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_show_fps_label, &lv_font_montserrat_24, 0);

    settings_show_fps_switch = lv_switch_create(ui->settings);
    lv_obj_set_pos(settings_show_fps_switch, 380, y_pos - 5);
    lv_obj_set_size(settings_show_fps_switch, 60, 30);

    // Set switch state based on current setting
    if (show_fps) {
        lv_obj_add_state(settings_show_fps_switch, LV_STATE_CHECKED);
    }

    // Style the switch
    lv_obj_set_style_bg_color(settings_show_fps_switch, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_show_fps_switch, lv_color_hex(0x00a9ff), LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_add_event_cb(settings_show_fps_switch, show_fps_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    y_pos += spacing;
    */

    // ========== Demo Mode Switch ==========
    settings_demo_mode_label = settings_heading_create(ui->settings, y_pos, "Demo mode");

    settings_demo_mode_switch = lv_switch_create(ui->settings);
    lv_obj_set_pos(settings_demo_mode_switch, 730, y_pos + 15);
    lv_obj_set_size(settings_demo_mode_switch, 60, 30);
    if (dashboard_demo_is_active()) {
        lv_obj_add_state(settings_demo_mode_switch, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(settings_demo_mode_switch, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_demo_mode_switch, lv_color_hex(0x00a9ff), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(settings_demo_mode_switch, demo_mode_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    y_pos += SETTINGS_ROW_H;

    // ========== VESC Emulator Switch ==========
    settings_vesc_emulator_label = settings_heading_create(ui->settings, y_pos, "VESC Emulator");

    settings_vesc_emulator_switch = lv_switch_create(ui->settings);
    lv_obj_set_pos(settings_vesc_emulator_switch, 730, y_pos + 15);
    lv_obj_set_size(settings_vesc_emulator_switch, 60, 30);
    if (settings_wrapper_get_vesc_emulator()) {
        lv_obj_add_state(settings_vesc_emulator_switch, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(settings_vesc_emulator_switch, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_vesc_emulator_switch, lv_color_hex(0x00a9ff),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(settings_vesc_emulator_switch, vesc_emulator_switch_event_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    settings_vesc_emulator_hint = lv_label_create(ui->settings);
    lv_label_set_text(settings_vesc_emulator_hint, "Restart required");
    lv_obj_set_pos(settings_vesc_emulator_hint, 220, y_pos + 22);
    lv_obj_set_style_text_color(settings_vesc_emulator_hint, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(settings_vesc_emulator_hint, &lv_font_montserrat_14, 0);

    y_pos += SETTINGS_ROW_H;

    // ========== AA Auto-Connect Switch ==========
    // Controls the BT agent's auto-reconnect-on-boot loop (it HFP-pages the
    // last paired phone until ACL comes up). When off, the agent stays quiet
    // and the user must initiate BT from the phone side.
    settings_aa_autoconnect_label =
        settings_heading_create(ui->settings, y_pos, "AA Auto-Connect");

    settings_aa_autoconnect_switch = lv_switch_create(ui->settings);
    lv_obj_set_pos(settings_aa_autoconnect_switch, 730, y_pos + 15);
    lv_obj_set_size(settings_aa_autoconnect_switch, 60, 30);
    if (settings_wrapper_get_aa_autoconnect()) {
        lv_obj_add_state(settings_aa_autoconnect_switch, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(settings_aa_autoconnect_switch, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_aa_autoconnect_switch, lv_color_hex(0x00a9ff),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(settings_aa_autoconnect_switch, aa_autoconnect_switch_event_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    settings_aa_autoconnect_hint = lv_label_create(ui->settings);
    lv_label_set_text(settings_aa_autoconnect_hint, "Reconnect to last phone on power-on");
    lv_obj_set_pos(settings_aa_autoconnect_hint, 250, y_pos + 22);
    lv_obj_set_style_text_color(settings_aa_autoconnect_hint, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(settings_aa_autoconnect_hint, &lv_font_montserrat_14, 0);

    y_pos += SETTINGS_ROW_H;

    // ========== Units Switch ==========
    // OFF = metric (km, km/h), ON = imperial (miles, mph). Applies live across
    // the dashboard, AA HUD overlay and trip statistics.
    settings_units_label = settings_heading_create(ui->settings, y_pos, "Units (miles)");

    settings_units_switch = lv_switch_create(ui->settings);
    lv_obj_set_pos(settings_units_switch, 730, y_pos + 15);
    lv_obj_set_size(settings_units_switch, 60, 30);
    if (settings_wrapper_get_use_imperial()) {
        lv_obj_add_state(settings_units_switch, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(settings_units_switch, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_units_switch, lv_color_hex(0x00a9ff),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(settings_units_switch, units_switch_event_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    settings_units_hint = lv_label_create(ui->settings);
    lv_label_set_text(settings_units_hint, "Off = km / km/h, On = miles / mph");
    lv_obj_set_pos(settings_units_hint, 230, y_pos + 22);
    lv_obj_set_style_text_color(settings_units_hint, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(settings_units_hint, &lv_font_montserrat_14, 0);

    y_pos += SETTINGS_ROW_H;

    // ========== Temperature Units Switch ==========
    // OFF = Celsius, ON = Fahrenheit. Applies live to the dashboard motor /
    // controller temperatures and the trip-statistics temperature chart.
    settings_temp_unit_label = settings_heading_create(ui->settings, y_pos, "Temperature (F)");

    settings_temp_unit_switch = lv_switch_create(ui->settings);
    lv_obj_set_pos(settings_temp_unit_switch, 730, y_pos + 15);
    lv_obj_set_size(settings_temp_unit_switch, 60, 30);
    if (settings_wrapper_get_use_fahrenheit()) {
        lv_obj_add_state(settings_temp_unit_switch, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(settings_temp_unit_switch, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_temp_unit_switch, lv_color_hex(0x00a9ff),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(settings_temp_unit_switch, temp_unit_switch_event_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    settings_temp_unit_hint = lv_label_create(ui->settings);
    lv_label_set_text(settings_temp_unit_hint, "Off = Celsius, On = Fahrenheit");
    lv_obj_set_pos(settings_temp_unit_hint, 230, y_pos + 22);
    lv_obj_set_style_text_color(settings_temp_unit_hint, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(settings_temp_unit_hint, &lv_font_montserrat_14, 0);

    y_pos += SETTINGS_ROW_H;
    /*
    // ========== Wheel Diameter Spinbox ==========
    uint16_t wheel_diameter = settings_wrapper_get_wheel_diameter_mm();
    
    settings_wheel_diameter_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_wheel_diameter_label, "Wheel Diameter (mm):");
    lv_obj_set_pos(settings_wheel_diameter_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_wheel_diameter_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_wheel_diameter_label, &lv_font_montserrat_24, 0);
    
    // Minus button
    settings_wheel_diameter_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *wheel_diam_minus_label = lv_label_create(settings_wheel_diameter_minus_btn);
    lv_label_set_text(wheel_diam_minus_label, "-");
    lv_obj_align(wheel_diam_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_wheel_diameter_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_wheel_diameter_minus_btn, 130, 50);
    lv_obj_set_style_bg_color(settings_wheel_diameter_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_wheel_diameter_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_wheel_diameter_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_wheel_diameter_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_wheel_diameter_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_wheel_diameter_minus_btn, wheel_diameter_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Spinbox
    settings_wheel_diameter_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_wheel_diameter_spinbox, 0, 2000);
    lv_spinbox_set_digit_format(settings_wheel_diameter_spinbox, 3, 0); // 3 digits, 0 decimal places
    lv_spinbox_set_value(settings_wheel_diameter_spinbox, wheel_diameter);
    lv_spinbox_set_step(settings_wheel_diameter_spinbox, 5); // 5mm increments
    lv_obj_set_pos(settings_wheel_diameter_spinbox, 335, y_pos + 30);
    lv_obj_set_size(settings_wheel_diameter_spinbox, 130, 50);
    
    // Style the spinbox
    lv_obj_set_style_bg_color(settings_wheel_diameter_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_wheel_diameter_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_wheel_diameter_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_wheel_diameter_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_wheel_diameter_spinbox, lv_color_hex(0x00a9ff), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_wheel_diameter_spinbox, 8, LV_PART_MAIN);
    
    lv_obj_add_event_cb(settings_wheel_diameter_spinbox, wheel_diameter_spinbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Plus button
    settings_wheel_diameter_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *wheel_diam_plus_label = lv_label_create(settings_wheel_diameter_plus_btn);
    lv_label_set_text(wheel_diam_plus_label, "+");
    lv_obj_align(wheel_diam_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_wheel_diameter_plus_btn, 660, y_pos + 30);
    lv_obj_set_size(settings_wheel_diameter_plus_btn, 130, 50);
    lv_obj_set_style_bg_color(settings_wheel_diameter_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_wheel_diameter_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_wheel_diameter_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_wheel_diameter_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_wheel_diameter_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_wheel_diameter_plus_btn, wheel_diameter_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);

    y_pos += spacing;

    // ========== Motor Poles Spinbox ==========
    uint8_t motor_poles = settings_wrapper_get_motor_poles();

    settings_motor_poles_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_motor_poles_label, "Motor Poles:");
    lv_obj_set_pos(settings_motor_poles_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_motor_poles_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_poles_label, &lv_font_montserrat_24, 0);

    // Minus button
    settings_motor_poles_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *motor_poles_minus_label = lv_label_create(settings_motor_poles_minus_btn);
    lv_label_set_text(motor_poles_minus_label, "-");
    lv_obj_align(motor_poles_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_motor_poles_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_motor_poles_minus_btn, 130, 50);
    lv_obj_set_style_bg_color(settings_motor_poles_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_motor_poles_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_poles_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_motor_poles_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_motor_poles_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_motor_poles_minus_btn, motor_poles_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Spinbox
    settings_motor_poles_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_motor_poles_spinbox, 0, 50);
    lv_spinbox_set_digit_format(settings_motor_poles_spinbox, 2, 0); // 2 digits, 0 decimal places
    lv_spinbox_set_value(settings_motor_poles_spinbox, motor_poles);
    lv_spinbox_set_step(settings_motor_poles_spinbox, 1);
    lv_obj_set_pos(settings_motor_poles_spinbox, 335, y_pos + 30);
    lv_obj_set_size(settings_motor_poles_spinbox, 130, 50);

    // Style the spinbox
    lv_obj_set_style_bg_color(settings_motor_poles_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_motor_poles_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_motor_poles_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_motor_poles_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_motor_poles_spinbox, lv_color_hex(0x00a9ff), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_motor_poles_spinbox, 8, LV_PART_MAIN);

    lv_obj_add_event_cb(settings_motor_poles_spinbox, motor_poles_spinbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Plus button
    settings_motor_poles_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *motor_poles_plus_label = lv_label_create(settings_motor_poles_plus_btn);
    lv_label_set_text(motor_poles_plus_label, "+");
    lv_obj_align(motor_poles_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_motor_poles_plus_btn, 660, y_pos + 30);
    lv_obj_set_size(settings_motor_poles_plus_btn, 130, 50);
    lv_obj_set_style_bg_color(settings_motor_poles_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_motor_poles_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_poles_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_motor_poles_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_motor_poles_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_motor_poles_plus_btn, motor_poles_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);

    y_pos += spacing;
*/
    // ========== Reset trip (own row) ===================================
    // Sometimes the trip doesn't auto-reset on battery swap — manual button.
    // Orange (caution) and physically separated from the destructive "Reset"
    // (all settings) below.
    y_pos += 10;

    settings_reset_trip_button = lv_btn_create(ui->settings);
    lv_obj_t *reset_trip_label = lv_label_create(settings_reset_trip_button);
    lv_label_set_text(reset_trip_label, "Reset trip");
    lv_obj_align(reset_trip_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_reset_trip_button, 20, y_pos);
    lv_obj_set_size(settings_reset_trip_button, 245, 50);
    lv_obj_set_style_bg_color(settings_reset_trip_button, lv_color_hex(0xff8800), 0);
    lv_obj_set_style_text_color(settings_reset_trip_button, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_reset_trip_button, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_reset_trip_button, 8, 0);
    lv_obj_set_style_border_width(settings_reset_trip_button, 0, 0);
    lv_obj_add_event_cb(settings_reset_trip_button, reset_trip_button_event_cb,
                        LV_EVENT_CLICKED, NULL);

    // ========== Files browser (same row as Reset trip, second slot) =====
    lv_obj_t *files_btn = lv_btn_create(ui->settings);
    lv_obj_t *files_lbl = lv_label_create(files_btn);
    lv_label_set_text(files_lbl, "Files");
    lv_obj_align(files_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(files_btn, 281, y_pos);
    lv_obj_set_size(files_btn, 245, 50);
    lv_obj_set_style_bg_color(files_btn, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_text_color(files_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(files_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(files_btn, 8, 0);
    lv_obj_set_style_border_width(files_btn, 0, 0);
    lv_obj_add_event_cb(files_btn, settings_files_button_event_cb,
                        LV_EVENT_CLICKED, NULL);

    y_pos += SETTINGS_ROW_H;

    // ========== Logs + QR codes + Reset (three buttons in one row) =====
    y_pos += 10; // small visual break before the destructive action

    lv_obj_t *logs_btn = lv_btn_create(ui->settings);
    lv_obj_t *logs_lbl = lv_label_create(logs_btn);
    lv_label_set_text(logs_lbl, "Logs");
    lv_obj_align(logs_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(logs_btn, 20, y_pos);
    lv_obj_set_size(logs_btn, 245, 50);
    lv_obj_set_style_bg_color(logs_btn, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_text_color(logs_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(logs_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(logs_btn, 8, 0);
    lv_obj_set_style_border_width(logs_btn, 0, 0);
    lv_obj_add_event_cb(logs_btn, logs_open_btn_event_cb,
                        LV_EVENT_CLICKED, NULL);

    lv_obj_t *qr_btn = lv_btn_create(ui->settings);
    lv_obj_t *qr_lbl = lv_label_create(qr_btn);
    lv_label_set_text(qr_lbl, "QR codes");
    lv_obj_align(qr_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(qr_btn, 281, y_pos);
    lv_obj_set_size(qr_btn, 245, 50);
    lv_obj_set_style_bg_color(qr_btn, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_text_color(qr_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(qr_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(qr_btn, 8, 0);
    lv_obj_set_style_border_width(qr_btn, 0, 0);
    lv_obj_add_event_cb(qr_btn, qr_open_btn_event_cb,
                        LV_EVENT_CLICKED, NULL);

    settings_reset_button = lv_btn_create(ui->settings);
    lv_obj_t *reset_label = lv_label_create(settings_reset_button);
    lv_label_set_text(reset_label, "Reset");
    lv_obj_align(reset_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_reset_button, 542, y_pos);
    lv_obj_set_size(settings_reset_button, 245, 50);
    lv_obj_set_style_bg_color(settings_reset_button, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_reset_button, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_reset_button, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_reset_button, 8, 0);
    lv_obj_set_style_border_width(settings_reset_button, 0, 0);
    lv_obj_add_event_cb(settings_reset_button, reset_button_event_cb, LV_EVENT_CLICKED, NULL);

    y_pos += SETTINGS_ROW_H;

    // ========== Info Label ==========
    settings_info_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_info_label, "Settings saved automatically");
    lv_obj_set_pos(settings_info_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_info_label, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_font(settings_info_label, &lv_font_montserrat_24, 0);

    y_pos += 40;

    /* Firmware-version block.
     *
     * Reads dev_settings RAM cache that main.c populates as each subsystem
     * comes up:
     *   P4 — esp_app_get_description()->version (set near start of main)
     *   BT — UART line "BT-VER:<v>" from the D1 Mini agent (set after
     *        bt_agent_ota_check_and_update)
     *   C6 — slave version captured by c6_ota_check_and_update
     *
     * If a subsystem isn't up yet when Settings is opened for the very first
     * time, the fw_info_get_* returns "" and the row shows "(unknown)". */
#ifdef LV_REALDEVICE
    const char *p4_v = fw_info_get_p4();
    const char *bt_v = fw_info_get_bt();
    const char *c6_v = fw_info_get_c6();
    if (!*p4_v) p4_v = "(unknown)";
    if (!*bt_v) bt_v = "(unknown)";
    if (!*c6_v) c6_v = "(unknown)";
    char fw_buf[160];
    snprintf(fw_buf, sizeof(fw_buf),
             "Firmware versions:\n"
             "  P4 host  : %s\n"
             "  BT agent : %s\n"
             "  C6 slave : %s",
             p4_v, bt_v, c6_v);
    lv_obj_t *fw_label = lv_label_create(ui->settings);
    lv_label_set_text(fw_label, fw_buf);
    lv_obj_set_pos(fw_label, 20, y_pos);
    lv_obj_set_style_text_color(fw_label, lv_color_hex(0xB6FF2E), 0);
    lv_obj_set_style_text_font(fw_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_line_space(fw_label, 4, 0);
#endif
}

/* ======================================================================== *
 *  Cockpit theme (theme 0) — the default GUI Guider dashboard, wrapped in
 *  the dashboard-theme interface. The static cockpit_*() render functions
 *  above are the implementation; the table below binds them to the registry.
 *  The public update_*() entry points are the dispatchers in
 *  dashboard_theme.c, which forward to whichever theme's ops are active.
 * ======================================================================== */

static lv_obj_t *cockpit_create(void)
{
    /* (Re)build the GUI Guider dashboard screen and apply per-screen chrome.
     * setup_scr_dashboard_Classic() points guider_ui.dashboard_Classic at the fresh screen. */
    setup_scr_dashboard_Classic(&guider_ui);
    cockpit_screen_init(&guider_ui);
    return guider_ui.dashboard_Classic;
}

static void cockpit_destroy(void)
{
    /* The screen object itself is freed by the switcher. Reset the cached power
     * inputs so a rebuilt cockpit recomputes from scratch; the per-setter dedup
     * caches re-sync via the units-epoch bump in cockpit_screen_init(). */
    s_cockpit_last_current_a = 0.0f;
    s_cockpit_last_voltage_v = 0.0f;
    atomic_store(&s_cockpit_speed_value, 0);
    atomic_store(&s_cockpit_battery_proc_value, 0);

    /* The Settings power-max handler calls cockpit_refresh_power_max_label() to
     * live-preview the "X.X KW" caption; once our screen is freed that widget
     * pointer dangles. NULL it so the function's own guard short-circuits while
     * another theme is active. setup_scr_dashboard_Classic() repopulates it on rebuild. */
    guider_ui.dashboard_Classic_power_max_val = NULL;
}

static lv_obj_t *cockpit_music_tile(void)
{
    return guider_ui.dashboard_Classic_music_info_tile;
}

static const dashboard_theme_ops_t cockpit_ops = {
    .speed                 = cockpit_speed,
    .current               = cockpit_current,
    .battery_proc          = cockpit_battery_proc,
    .battery_voltage       = cockpit_battery_voltage,
    .battery_temp          = cockpit_battery_temp,
    .temp_fet              = cockpit_temp_fet,
    .temp_motor            = cockpit_temp_motor,
    .trip                  = cockpit_trip,
    .range                 = cockpit_range,
    .odometer              = cockpit_odometer,
    .amp_hours             = cockpit_amp_hours,
    .uptime                = cockpit_uptime,
    .fps                   = cockpit_fps,
    .ble_status            = cockpit_ble_status,
    .esc_connection_status = cockpit_esc_connection_status,
    .cruise_control_status = cockpit_cruise_control_status,
    .cruise_speed          = cockpit_cruise_speed,
    .mode_text             = cockpit_mode_text,
    .units_changed         = cockpit_units_changed,
    .cur_time              = cockpit_cur_time,
    .cur_time_hm           = cockpit_cur_time_hm,
    .hide_cur_time         = cockpit_hide_cur_time,
    .hide_mode_text        = cockpit_hide_mode_text,
    .navigation_icon       = cockpit_navigation_icon,
    .navigation_text       = cockpit_navigation_text,
    .music_text            = cockpit_music_text,
};

static const dashboard_theme_t cockpit_theme = {
    .id         = "cockpit",
    .name       = "Cockpit",
    .create     = cockpit_create,
    .destroy    = cockpit_destroy,
    .music_tile = cockpit_music_tile,
    .ops        = &cockpit_ops,
};

/* One-time init: NVS-backed settings, the format-notice timer, and the theme
 * registry. Idempotent. Theme registration order == dropdown order; index 0
 * (cockpit) is the default stored in dev_settings ("dash_theme"). */
void custom_init_once(void)
{
    static bool done;
    if (done) return;
    done = true;

    settings_wrapper_init();

#ifdef LV_REALDEVICE
    /* Watch for the one-time backup-FS format and show a notice while it runs. */
    if (!s_fmt_overlay_tmr) {
        s_fmt_overlay_tmr = lv_timer_create(fmt_overlay_timer_cb, 200, NULL);
    }
#endif

    dashboard_theme_register(&cockpit_theme);   /* idx 0 (default) */
    /* theme_ref ("Minimal (ref)") is a scaffold/reference theme — not exposed
     * in the Settings dropdown. Re-enable by calling theme_ref_register() here
     * (it would take the next free index). See theme_ref.c. */
    /* Append new themes at the END so saved NVS theme indices stay stable. */
    theme_dashboard_amber_register();   /* idx 1 — "Cockpit (Amber)", see theme_dashboard_amber.c */

#ifdef LV_REALDEVICE
    /* Auto-register every convention-named dashboard_* GUI Guider screen
     * (firmware build only — the registrar is code-generated from
     * gui_guider.h; see scripts/gen_dashboard_themes.py + theme_generic.h).
     * Appended last, so these take the indices after the hand-written themes. */
    dashboard_themes_auto_register_all();
#endif
}

/* Simulator / desktop-port entry point. setup_ui() already built and loaded the
 * cockpit screen before this runs, so register themes, apply the per-screen
 * chrome, and adopt the cockpit as active WITHOUT rebuilding it. The device path
 * (components/vesc_ui/vesc_ui.c) instead calls custom_init_once() +
 * dashboard_theme_build(saved_index). */
void custom_init(lv_ui *ui)
{
    custom_init_once();
    cockpit_screen_init(ui);
    dashboard_theme_adopt(0);
}

