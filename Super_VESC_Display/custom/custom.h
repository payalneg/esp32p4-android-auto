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
void settings_ui_init(lv_ui *ui);

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
void update_fps(int fps);
void update_uptime(uint32_t uptime);
void update_ble_status(bool connected);
void update_esc_connection_status(bool connected);
void update_navigation_icon(const uint8_t *img_data, uint32_t data_size, uint16_t width, uint16_t height, lv_img_cf_t color_format);
void update_navigation_text(const char *text);
void update_music_text(const char *text);
void update_cruise_control_status(bool active);
void update_cruise_speed(float speed);
void update_mode_text(uint8_t mode);
void reset_icon_pressed(void);

#ifdef __cplusplus
}
#endif
#endif /* EVENT_CB_H_ */
