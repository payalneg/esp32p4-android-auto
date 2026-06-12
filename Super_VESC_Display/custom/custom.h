/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef __CUSTOM_H_
#define __CUSTOM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "gui_guider.h"

void custom_init(lv_ui *ui);
/* One-time, screen-independent init (NVS settings, theme registry, the
 * format-notice timer). The device path calls this directly from vesc_ui_init();
 * the simulator's custom_init() calls it too. Idempotent. */
void custom_init_once(void);
void settings_ui_init(lv_ui *ui);

/* VESC Tool controller-config menu. Opened from the dashboard VESC status
 * widget (events_init.c). Builds a self-contained screen that mirrors the
 * mobile VESC Tool config (Motor + App, all params grouped into tabs), backed
 * by either a real VESC over CAN or the in-RAM emulator. Defined in
 * custom/vesc_tool_menu.c. */
void run_vesc_tool_menu(void);

/* Trip statistics window. Opened from the dashboard STATISTICS button
 * (events_init.c). Reads the raw trip log (components/trip_log) and shows a
 * scrollable trip list + per-trip detail with charts. On the desktop simulator
 * (or when the VESC emulator is on / the log is empty) it falls back to
 * synthetic trips so the UI stays demoable. Defined in custom/trip_statistics.c. */
void show_trips_statistics(void);

/* Realtime data viewer. Opened from the VESC Tool config menu header.
 * Shows live telemetry (vesc_rt_data) plus decoded ADC and PPM inputs
 * (vesc_io_data, polled only while this screen is open). Defined in
 * custom/realtime_viewer.c. */
void show_realtime_viewer(void);

/* LISP script editor. Opened from the VESC Tool config menu header. Edits
 * scripts with an on-screen keyboard, saves/loads them to the local littlefs
 * (/vescfs/lisp), and reads/uploads/runs code on the VESC. Defined in
 * custom/lisp_editor.c. */
void show_lisp_editor(void);

/* LISP quick-action panel — a left-docked drawer whose controls (toggles /
 * buttons / numbers / read-only values) are described at runtime by the master
 * LISP script on the VESC over COMM_CUSTOM_APP_DATA (see
 * components/vesc_can/vesc_lisp_panel.h). Opened by a left-edge swipe on the
 * dashboard. Defined in custom/lisp_panel.c.
 *  - show_lisp_panel()      : open now (must run on the LVGL task).
 *  - lisp_panel_close()     : slide out + tear down (LVGL task).
 *  - lisp_panel_open_async(): thread-safe entry — marshals to the LVGL task and
 *                             opens only if the dashboard is the active screen.
 *                             Registered as the touch edge-swipe callback. */
void show_lisp_panel(void);
void lisp_panel_close(void);
void lisp_panel_open_async(void);

void update_current(float current);
void update_speed(float speed);
void update_battery_proc(float battery_proc);
void update_trip(float trip_distance);
void update_range(float range_distance);
void update_temp_fet(float temp_fet);
void update_temp_motor(float temp_motor);
void update_amp_hours(float amp_hours);
void update_battery_temp(float battery_temp);
void update_battery_voltage(float battery_voltage);
void update_odometer(float odometer);

/* Re-format all distance/speed/temperature readouts and flip the static unit
 * captions ("KM"↔"MI", "KM/H"↔"MPH", "°C"↔"°F") after either the km/miles or
 * the Celsius/Fahrenheit toggle changes (shared epoch). Call from the settings
 * switch handlers and once at UI init so captions match the stored settings.
 * Must run on the LVGL task. */
void dashboard_units_changed(void);

void update_fps(int fps);
void update_uptime(uint32_t uptime);
void update_cur_time(int hour, int minute, int second);
void update_cur_time_hm(int hour, int minute);
void hide_cur_time(void);
void hide_mode_text(void);

/* Called from events_init.c when the invisible dashboard brightness
 * slider (full-screen drag area) changes value. Applies the brightness
 * live and debounces the NVS commit so a continuous drag doesn't issue
 * one flash write per slider tick. */
void dashboard_brightness_slider_changed(int32_t value);
void update_ble_status(bool connected);
void update_esc_connection_status(bool connected);
void update_navigation_icon(const uint8_t *img_data, uint32_t data_size, uint16_t width, uint16_t height, lv_img_cf_t color_format);
void update_navigation_text(const char *text);
void update_music_text(const char *text);
void update_cruise_control_status(bool active);
void update_cruise_speed(float speed);
void update_mode_text(uint8_t mode);
void reset_icon_pressed(void);

/* Runtime demo loop: pumps sin-wave values through the same update_*
 * setters at 4 Hz. Toggled from the in-app Settings screen. The RT
 * updater (main/vesc_ui_updater.c) checks dashboard_demo_is_active()
 * and stays out of the way while demo is on. */
bool dashboard_demo_is_active(void);
void dashboard_demo_set_active(bool on);

/* Last clamped values fed into update_speed / update_battery_proc. Atomic
 * snapshots so non-LVGL threads (e.g. the AA video overlay) can read them
 * without holding the LVGL lock. Show the same numbers as the cockpit,
 * including demo-mode values. */
int cockpit_get_speed_value(void);
int cockpit_get_battery_proc_value(void);

#ifdef __cplusplus
}
#endif
#endif /* EVENT_CB_H_ */
