/*
    Copyright 2026 Super VESC Display

    Generic, convention-bound dashboard theme.

    Lets a brand-new GUI Guider screen named `dashboard_<something>` show up in
    the Settings theme dropdown AND get live VESC data — with no hand-written
    C — as long as its widgets follow the naming convention below. The build
    step scripts/gen_dashboard_themes.py scans generated/gui_guider.h, resolves
    each matching screen's widgets into a dashboard_widgets_t, and registers it
    against the shared dashboard_generic_ops here. Hand-written themes
    (cockpit, amber) are skipped by the generator and keep their bespoke ops.

    Widget naming convention (screen prefix is the GUI Guider screen name):
        <scr>_Speed_text         label   km/h or mph (unit-converted)
        <scr>_Current_text       label   "%.1f A"
        <scr>_Voltage_text       label   "%.1f"
        <scr>_Battery_proc_text  label   "%d" (coloured by charge)
        <scr>_temp_esc_text      label   ESC temp (unit-converted)
        <scr>_temp_mot_text      label   motor temp (unit-converted)
        <scr>_TRIP_text          label   trip distance (unit-converted)
        <scr>_Range_text         label   range (unit-converted)
        <scr>_odo_text           label   odometer (unit-converted)
        <scr>_Ah_text            label   "%.1f Ah"
        <scr>_uptime_text        label   "HH:MM:SS"
        <scr>_mode_text          label   "MODE N"
        <scr>_cur_time_label     label   "HH:MM[:SS]"
        <scr>_power_value        label   "%.1f" kW (current*voltage)
        <scr>_max_power_text     label   session peak power, "%.1f KW"
        <scr>_max_speed_text     label   session peak speed, "%d <unit>"
        <scr>_power_max_val      label   configured power ceiling, "%.1f KW"
        <scr>_status_bt          label   BLE link indicator (colour/opacity)
        <scr>_batt_seg_00..13    objects vertical battery bar (bottom→top fill)
        <scr>_power_seg_00..13   objects vertical power bar  (bottom→top fill)
        <scr>_speed_seg_00..11   objects horizontal speed bar (seg_00 first)

    Analogue gauges — the range always comes from the widget as authored in GUI
    Guider, so the design owns the scale and no value is hard-coded here:
        <scr>_speed_meter        lv_meter; its LAST arc indicator on scale_0
                                 tracks speed (put the value arc last — it also
                                 has to be drawn on top of track/redline)
        <scr>_batt_bar           lv_bar, charge %
        <scr>_temp_mot_bar       lv_bar, motor temp in display units
        <scr>_temp_esc_bar       lv_bar, ESC temp in display units
        <scr>_power_bar          lv_bar, power as a fraction of the configured
                                 ceiling; author it LV_BAR_MODE_SYMMETRICAL with
                                 a negative minimum to get a regen/drive flow bar

    Any field a screen omits is simply left NULL and skipped at render time.
*/
#pragma once

#include "lvgl.h"
#include "dashboard_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resolved widget pointers for one convention-named dashboard screen. Filled
 * by the generated create() (scripts/gen_dashboard_themes.py); NULL fields are
 * skipped by the renderer. */
typedef struct {
    lv_obj_t *speed_text;
    lv_obj_t *current_text;
    lv_obj_t *voltage_text;
    lv_obj_t *batt_pct_text;
    lv_obj_t *temp_fet_text;
    lv_obj_t *temp_motor_text;
    lv_obj_t *trip_text;
    lv_obj_t *range_text;
    lv_obj_t *odo_text;
    lv_obj_t *ah_text;
    lv_obj_t *uptime_text;
    lv_obj_t *mode_text;
    lv_obj_t *time_label;
    lv_obj_t *power_value;
    lv_obj_t *max_power_text;
    lv_obj_t *max_speed_text;
    lv_obj_t *power_max_val;
    lv_obj_t *status_bt;
    lv_obj_t *batt_seg[14];
    int       batt_seg_n;
    lv_obj_t *power_seg[14];
    int       power_seg_n;
    lv_obj_t *speed_seg[12];
    int       speed_seg_n;
    /* Analogue gauges. speed_scale is kept only to read its authored min/max —
     * lv_meter_scale_t is a plain public struct, there is no getter. */
    lv_obj_t             *speed_meter;
    lv_meter_scale_t     *speed_scale;
    lv_meter_indicator_t *speed_arc;
    lv_obj_t *batt_bar;
    lv_obj_t *temp_mot_bar;
    lv_obj_t *temp_esc_bar;
    lv_obj_t *power_bar;
    /* Invisible full-screen brightness drag slider, hidden when the user turns
     * the gesture off in Settings. Not a render target — see
     * dashboard_theme_ops_t.brightness_gesture. */
    lv_obj_t *brightness_slider;
} dashboard_widgets_t;

/* Shared ops table every auto-discovered theme points at. */
extern const dashboard_theme_ops_t dashboard_generic_ops;

/* Make [w] the target of subsequent dashboard_generic_ops calls. Called from
 * each generated theme's create() — only one theme is active at a time, so a
 * single active pointer is enough. [w] must outlive the active period (the
 * generator uses a file-static per screen). Pass NULL to detach. */
void dashboard_generic_set_active(const dashboard_widgets_t *w);

/* Implemented by the generated dashboard_themes_auto.c (firmware build only);
 * registers a generic theme for every discovered dashboard_* screen. Declared
 * here so custom_init_once() can call it under LV_REALDEVICE. */
void dashboard_themes_auto_register_all(void);

#ifdef __cplusplus
}
#endif
