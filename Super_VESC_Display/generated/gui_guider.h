/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct
{
  
	lv_obj_t *dashboard;
	bool dashboard_del;
	lv_obj_t *dashboard_statusbar_sep;
	lv_obj_t *dashboard_status_vesc;
	lv_obj_t *dashboard_mode_text;
	lv_obj_t *dashboard_uptime_text;
	lv_obj_t *dashboard_status_bt;
	lv_obj_t *dashboard_battery_sep;
	lv_obj_t *dashboard_battery_label;
	lv_obj_t *dashboard_Battery_proc_text;
	lv_obj_t *dashboard_battery_pct;
	lv_obj_t *dashboard_batt_seg_00;
	lv_obj_t *dashboard_batt_seg_01;
	lv_obj_t *dashboard_batt_seg_02;
	lv_obj_t *dashboard_batt_seg_03;
	lv_obj_t *dashboard_batt_seg_04;
	lv_obj_t *dashboard_batt_seg_05;
	lv_obj_t *dashboard_batt_seg_06;
	lv_obj_t *dashboard_batt_seg_07;
	lv_obj_t *dashboard_batt_seg_08;
	lv_obj_t *dashboard_batt_seg_09;
	lv_obj_t *dashboard_batt_seg_10;
	lv_obj_t *dashboard_batt_seg_11;
	lv_obj_t *dashboard_batt_seg_12;
	lv_obj_t *dashboard_batt_seg_13;
	lv_obj_t *dashboard_battery_range_label;
	lv_obj_t *dashboard_Range_text;
	lv_obj_t *dashboard_speed_label;
	lv_obj_t *dashboard_Speed_text;
	lv_obj_t *dashboard_speed_seg_00;
	lv_obj_t *dashboard_speed_seg_01;
	lv_obj_t *dashboard_speed_seg_02;
	lv_obj_t *dashboard_speed_seg_03;
	lv_obj_t *dashboard_speed_seg_04;
	lv_obj_t *dashboard_speed_seg_05;
	lv_obj_t *dashboard_speed_seg_06;
	lv_obj_t *dashboard_speed_seg_07;
	lv_obj_t *dashboard_speed_seg_08;
	lv_obj_t *dashboard_speed_seg_09;
	lv_obj_t *dashboard_speed_seg_10;
	lv_obj_t *dashboard_speed_seg_11;
	lv_obj_t *dashboard_speed_min;
	lv_obj_t *dashboard_speed_max;
	lv_obj_t *dashboard_power_sep;
	lv_obj_t *dashboard_power_label;
	lv_obj_t *dashboard_power_value;
	lv_obj_t *dashboard_power_unit;
	lv_obj_t *dashboard_Current_text;
	lv_obj_t *dashboard_power_seg_00;
	lv_obj_t *dashboard_power_seg_01;
	lv_obj_t *dashboard_power_seg_02;
	lv_obj_t *dashboard_power_seg_03;
	lv_obj_t *dashboard_power_seg_04;
	lv_obj_t *dashboard_power_seg_05;
	lv_obj_t *dashboard_power_seg_06;
	lv_obj_t *dashboard_power_seg_07;
	lv_obj_t *dashboard_power_seg_08;
	lv_obj_t *dashboard_power_seg_09;
	lv_obj_t *dashboard_power_seg_10;
	lv_obj_t *dashboard_power_seg_11;
	lv_obj_t *dashboard_power_seg_12;
	lv_obj_t *dashboard_power_seg_13;
	lv_obj_t *dashboard_power_max_label;
	lv_obj_t *dashboard_power_max_val;
	lv_obj_t *dashboard_bottom_bg;
	lv_obj_t *dashboard_bottom_top_sep;
	lv_obj_t *dashboard_bottom_col_sep_0;
	lv_obj_t *dashboard_bottom_col_sep_1;
	lv_obj_t *dashboard_bottom_col_sep_2;
	lv_obj_t *dashboard_bottom_col_sep_3;
	lv_obj_t *dashboard_col_trip_label;
	lv_obj_t *dashboard_TRIP_text;
	lv_obj_t *dashboard_col_trip_unit;
	lv_obj_t *dashboard_col_odo_label;
	lv_obj_t *dashboard_odo_text;
	lv_obj_t *dashboard_col_odo_unit;
	lv_obj_t *dashboard_col_mtmp_label;
	lv_obj_t *dashboard_temp_mot_text;
	lv_obj_t *dashboard_col_mtmp_unit;
	lv_obj_t *dashboard_col_ctmp_label;
	lv_obj_t *dashboard_temp_esc_text;
	lv_obj_t *dashboard_col_ctmp_unit;
	lv_obj_t *dashboard_col_avg_label;
	lv_obj_t *dashboard_col_avg_value;
	lv_obj_t *dashboard_col_avg_unit;
	lv_obj_t *dashboard_cruise_control_img;
	lv_obj_t *dashboard_esc_not_connected_text;
	lv_obj_t *dashboard_Ah_text;
	lv_obj_t *dashboard_Speed_cc_text;
	lv_obj_t *dashboard_Settings_text;
	lv_obj_t *dashboard_song_title_label;
	lv_obj_t *dashboard_col_v_label;
	lv_obj_t *dashboard_Voltage_text;
	lv_obj_t *dashboard_col_voltage_label;
	lv_obj_t *dashboard_cur_time_label;
	lv_obj_t *dashboard_brightness_slider;
	lv_obj_t *dashboard_music_info;
	lv_obj_t *dashboard_music_info_tile;
	lv_obj_t *settings;
	bool settings_del;
	lv_obj_t *settings_exit_button;
	lv_obj_t *settings_exit_button_label;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_animation(void * var, int32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                       uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                       lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);


void init_scr_del_flag(lv_ui *ui);

void setup_ui(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui guider_ui;


void setup_scr_dashboard(lv_ui *ui);
void setup_scr_settings(lv_ui *ui);
LV_IMG_DECLARE(_cruise_control_alpha_38x38);

LV_FONT_DECLARE(lv_font_montserratMedium_16)
LV_FONT_DECLARE(lv_font_montserratMedium_24)
LV_FONT_DECLARE(lv_font_Antonio_Regular_64)
LV_FONT_DECLARE(lv_font_montserratMedium_22)
LV_FONT_DECLARE(lv_font_montserratMedium_12)
LV_FONT_DECLARE(lv_font_Antonio_Regular_200)
LV_FONT_DECLARE(lv_font_montserratMedium_20)
LV_FONT_DECLARE(lv_font_montserratMedium_26)
LV_FONT_DECLARE(lv_font_montserratMedium_18)
LV_FONT_DECLARE(lv_font_Antonio_Regular_40)
LV_FONT_DECLARE(lv_font_montserratMedium_11)
LV_FONT_DECLARE(lv_font_Antonio_Regular_32)
LV_FONT_DECLARE(lv_font_Antonio_Regular_22)
LV_FONT_DECLARE(lv_font_montserratMedium_25)
LV_FONT_DECLARE(lv_font_Antonio_Regular_50)


#ifdef __cplusplus
}
#endif
#endif
