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
  
	lv_obj_t *dashboard_Classic;
	bool dashboard_Classic_del;
	lv_obj_t *dashboard_Classic_statusbar_sep;
	lv_obj_t *dashboard_Classic_status_vesc;
	lv_obj_t *dashboard_Classic_mode_text;
	lv_obj_t *dashboard_Classic_uptime_text;
	lv_obj_t *dashboard_Classic_status_bt;
	lv_obj_t *dashboard_Classic_battery_sep;
	lv_obj_t *dashboard_Classic_battery_label;
	lv_obj_t *dashboard_Classic_Battery_proc_text;
	lv_obj_t *dashboard_Classic_battery_pct;
	lv_obj_t *dashboard_Classic_batt_seg_00;
	lv_obj_t *dashboard_Classic_batt_seg_01;
	lv_obj_t *dashboard_Classic_batt_seg_02;
	lv_obj_t *dashboard_Classic_batt_seg_03;
	lv_obj_t *dashboard_Classic_batt_seg_04;
	lv_obj_t *dashboard_Classic_batt_seg_05;
	lv_obj_t *dashboard_Classic_batt_seg_06;
	lv_obj_t *dashboard_Classic_batt_seg_07;
	lv_obj_t *dashboard_Classic_batt_seg_08;
	lv_obj_t *dashboard_Classic_batt_seg_09;
	lv_obj_t *dashboard_Classic_batt_seg_10;
	lv_obj_t *dashboard_Classic_batt_seg_11;
	lv_obj_t *dashboard_Classic_batt_seg_12;
	lv_obj_t *dashboard_Classic_batt_seg_13;
	lv_obj_t *dashboard_Classic_battery_range_label;
	lv_obj_t *dashboard_Classic_Range_text;
	lv_obj_t *dashboard_Classic_speed_label;
	lv_obj_t *dashboard_Classic_Speed_text;
	lv_obj_t *dashboard_Classic_speed_seg_00;
	lv_obj_t *dashboard_Classic_speed_seg_01;
	lv_obj_t *dashboard_Classic_speed_seg_02;
	lv_obj_t *dashboard_Classic_speed_seg_03;
	lv_obj_t *dashboard_Classic_speed_seg_04;
	lv_obj_t *dashboard_Classic_speed_seg_05;
	lv_obj_t *dashboard_Classic_speed_seg_06;
	lv_obj_t *dashboard_Classic_speed_seg_07;
	lv_obj_t *dashboard_Classic_speed_seg_08;
	lv_obj_t *dashboard_Classic_speed_seg_09;
	lv_obj_t *dashboard_Classic_speed_seg_10;
	lv_obj_t *dashboard_Classic_speed_seg_11;
	lv_obj_t *dashboard_Classic_speed_min;
	lv_obj_t *dashboard_Classic_speed_max;
	lv_obj_t *dashboard_Classic_power_sep;
	lv_obj_t *dashboard_Classic_power_label;
	lv_obj_t *dashboard_Classic_power_value;
	lv_obj_t *dashboard_Classic_power_unit;
	lv_obj_t *dashboard_Classic_Current_text;
	lv_obj_t *dashboard_Classic_power_seg_00;
	lv_obj_t *dashboard_Classic_power_seg_01;
	lv_obj_t *dashboard_Classic_power_seg_02;
	lv_obj_t *dashboard_Classic_power_seg_03;
	lv_obj_t *dashboard_Classic_power_seg_04;
	lv_obj_t *dashboard_Classic_power_seg_05;
	lv_obj_t *dashboard_Classic_power_seg_06;
	lv_obj_t *dashboard_Classic_power_seg_07;
	lv_obj_t *dashboard_Classic_power_seg_08;
	lv_obj_t *dashboard_Classic_power_seg_09;
	lv_obj_t *dashboard_Classic_power_seg_10;
	lv_obj_t *dashboard_Classic_power_seg_11;
	lv_obj_t *dashboard_Classic_power_seg_12;
	lv_obj_t *dashboard_Classic_power_seg_13;
	lv_obj_t *dashboard_Classic_power_max_label;
	lv_obj_t *dashboard_Classic_power_max_val;
	lv_obj_t *dashboard_Classic_bottom_bg;
	lv_obj_t *dashboard_Classic_bottom_top_sep;
	lv_obj_t *dashboard_Classic_bottom_col_sep_0;
	lv_obj_t *dashboard_Classic_bottom_col_sep_1;
	lv_obj_t *dashboard_Classic_bottom_col_sep_2;
	lv_obj_t *dashboard_Classic_bottom_col_sep_3;
	lv_obj_t *dashboard_Classic_col_trip_label;
	lv_obj_t *dashboard_Classic_TRIP_text;
	lv_obj_t *dashboard_Classic_col_trip_unit;
	lv_obj_t *dashboard_Classic_col_odo_label;
	lv_obj_t *dashboard_Classic_odo_text;
	lv_obj_t *dashboard_Classic_col_odo_unit;
	lv_obj_t *dashboard_Classic_col_mtmp_label;
	lv_obj_t *dashboard_Classic_temp_mot_text;
	lv_obj_t *dashboard_Classic_col_mtmp_unit;
	lv_obj_t *dashboard_Classic_col_ctmp_label;
	lv_obj_t *dashboard_Classic_temp_esc_text;
	lv_obj_t *dashboard_Classic_col_ctmp_unit;
	lv_obj_t *dashboard_Classic_col_avg_label;
	lv_obj_t *dashboard_Classic_col_avg_value;
	lv_obj_t *dashboard_Classic_col_avg_unit;
	lv_obj_t *dashboard_Classic_cruise_control_img;
	lv_obj_t *dashboard_Classic_esc_not_connected_text;
	lv_obj_t *dashboard_Classic_Ah_text;
	lv_obj_t *dashboard_Classic_Speed_cc_text;
	lv_obj_t *dashboard_Classic_Settings_text;
	lv_obj_t *dashboard_Classic_song_title_label;
	lv_obj_t *dashboard_Classic_col_v_label;
	lv_obj_t *dashboard_Classic_Voltage_text;
	lv_obj_t *dashboard_Classic_col_voltage_label;
	lv_obj_t *dashboard_Classic_cur_time_label;
	lv_obj_t *dashboard_Classic_brightness_slider;
	lv_obj_t *dashboard_Classic_music_info;
	lv_obj_t *dashboard_Classic_music_info_tile;
	lv_obj_t *dashboard_Classic_statistics_button;
	lv_obj_t *settings;
	bool settings_del;
	lv_obj_t *settings_exit_button;
	lv_obj_t *settings_exit_button_label;
	lv_obj_t *reference;
	bool reference_del;
	lv_obj_t *reference_btn_1;
	lv_obj_t *reference_btn_1_label;
	lv_obj_t *reference_imgbtn_1;
	lv_obj_t *reference_imgbtn_1_label;
	lv_obj_t *reference_btnm_1;
	lv_obj_t *reference_sw_1;
	lv_obj_t *reference_bar_1;
	lv_obj_t *reference_slider_1;
	lv_obj_t *reference_img_1;
	lv_obj_t *reference_animimg_1;
	lv_obj_t *reference_image3D_1;
	lv_obj_t *reference_label_1;
	lv_obj_t *reference_spangroup_1;
	lv_span_t *reference_spangroup_1_span;
	lv_obj_t *reference_ddlist_1;
	lv_obj_t *reference_ta_1;
	lv_obj_t *reference_cb_1;
	lv_obj_t *reference_cont_1;
	lv_obj_t *reference_tabview_1;
	lv_obj_t *reference_tabview_1_tab_1;
	lv_obj_t *reference_tabview_1_tab_2;
	lv_obj_t *reference_tabview_1_tab_3;
	lv_obj_t *reference_win_1;
	lv_obj_t *reference_win_1_item0;
	lv_obj_t *reference_tileview_1;
	lv_obj_t *reference_tileview_1_tile;
	lv_obj_t *reference_menu_1;
	lv_obj_t *reference_menu_1_sidebar_page;
	lv_obj_t *reference_menu_1_subpage_1;
	lv_obj_t *reference_menu_1_cont_1;
	lv_obj_t *reference_menu_1_label_1;
	lv_obj_t *reference_menu_1_subpage_2;
	lv_obj_t *reference_menu_1_cont_2;
	lv_obj_t *reference_menu_1_label_2;
	lv_obj_t *reference_menu_1_subpage_3;
	lv_obj_t *reference_menu_1_cont_3;
	lv_obj_t *reference_menu_1_label_3;
	lv_obj_t *reference_roller_1;
	lv_obj_t *reference_arc_1;
	lv_obj_t *reference_line_1;
	lv_obj_t *reference_table_1;
	lv_obj_t *reference_msgbox_1;
	lv_obj_t *reference_calendar_1;
	lv_obj_t *reference_list_1;
	lv_obj_t *reference_list_1_item0;
	lv_obj_t *reference_spinbox_1;
	lv_obj_t *reference_spinbox_1_btn_plus;
	lv_obj_t *reference_spinbox_1_btn_minus;
	lv_obj_t *reference_meter_1;
	lv_meter_scale_t *reference_meter_1_scale_0;
	lv_meter_indicator_t *reference_meter_1_scale_0_ndline_0;
	lv_obj_t *reference_chart_1;
	lv_chart_series_t *reference_chart_1_0;
	lv_obj_t *reference_canvas_1;
	lv_obj_t *reference_led_1;
	lv_obj_t *reference_cpicker_1;
	lv_obj_t *reference_spinner_1;
	lv_obj_t *dashboard_Classic_Max;
	bool dashboard_Classic_Max_del;
	lv_obj_t *dashboard_Classic_Max_statusbar_sep;
	lv_obj_t *dashboard_Classic_Max_status_vesc;
	lv_obj_t *dashboard_Classic_Max_mode_text;
	lv_obj_t *dashboard_Classic_Max_uptime_text;
	lv_obj_t *dashboard_Classic_Max_status_bt;
	lv_obj_t *dashboard_Classic_Max_battery_sep;
	lv_obj_t *dashboard_Classic_Max_battery_label;
	lv_obj_t *dashboard_Classic_Max_Battery_proc_text;
	lv_obj_t *dashboard_Classic_Max_battery_pct;
	lv_obj_t *dashboard_Classic_Max_batt_seg_00;
	lv_obj_t *dashboard_Classic_Max_batt_seg_01;
	lv_obj_t *dashboard_Classic_Max_batt_seg_02;
	lv_obj_t *dashboard_Classic_Max_batt_seg_03;
	lv_obj_t *dashboard_Classic_Max_batt_seg_04;
	lv_obj_t *dashboard_Classic_Max_batt_seg_05;
	lv_obj_t *dashboard_Classic_Max_batt_seg_06;
	lv_obj_t *dashboard_Classic_Max_batt_seg_07;
	lv_obj_t *dashboard_Classic_Max_batt_seg_08;
	lv_obj_t *dashboard_Classic_Max_batt_seg_09;
	lv_obj_t *dashboard_Classic_Max_batt_seg_10;
	lv_obj_t *dashboard_Classic_Max_batt_seg_11;
	lv_obj_t *dashboard_Classic_Max_batt_seg_12;
	lv_obj_t *dashboard_Classic_Max_batt_seg_13;
	lv_obj_t *dashboard_Classic_Max_battery_range_label;
	lv_obj_t *dashboard_Classic_Max_Range_text;
	lv_obj_t *dashboard_Classic_Max_speed_label;
	lv_obj_t *dashboard_Classic_Max_Speed_text;
	lv_obj_t *dashboard_Classic_Max_speed_seg_00;
	lv_obj_t *dashboard_Classic_Max_speed_seg_01;
	lv_obj_t *dashboard_Classic_Max_speed_seg_02;
	lv_obj_t *dashboard_Classic_Max_speed_seg_03;
	lv_obj_t *dashboard_Classic_Max_speed_seg_04;
	lv_obj_t *dashboard_Classic_Max_speed_seg_05;
	lv_obj_t *dashboard_Classic_Max_speed_seg_06;
	lv_obj_t *dashboard_Classic_Max_speed_seg_07;
	lv_obj_t *dashboard_Classic_Max_speed_seg_08;
	lv_obj_t *dashboard_Classic_Max_speed_seg_09;
	lv_obj_t *dashboard_Classic_Max_speed_seg_10;
	lv_obj_t *dashboard_Classic_Max_speed_seg_11;
	lv_obj_t *dashboard_Classic_Max_speed_min;
	lv_obj_t *dashboard_Classic_Max_speed_max;
	lv_obj_t *dashboard_Classic_Max_power_sep;
	lv_obj_t *dashboard_Classic_Max_power_label;
	lv_obj_t *dashboard_Classic_Max_power_value;
	lv_obj_t *dashboard_Classic_Max_power_unit;
	lv_obj_t *dashboard_Classic_Max_Current_text;
	lv_obj_t *dashboard_Classic_Max_power_seg_00;
	lv_obj_t *dashboard_Classic_Max_power_seg_01;
	lv_obj_t *dashboard_Classic_Max_power_seg_02;
	lv_obj_t *dashboard_Classic_Max_power_seg_03;
	lv_obj_t *dashboard_Classic_Max_power_seg_04;
	lv_obj_t *dashboard_Classic_Max_power_seg_05;
	lv_obj_t *dashboard_Classic_Max_power_seg_06;
	lv_obj_t *dashboard_Classic_Max_power_seg_07;
	lv_obj_t *dashboard_Classic_Max_power_seg_08;
	lv_obj_t *dashboard_Classic_Max_power_seg_09;
	lv_obj_t *dashboard_Classic_Max_power_seg_10;
	lv_obj_t *dashboard_Classic_Max_power_seg_11;
	lv_obj_t *dashboard_Classic_Max_power_seg_12;
	lv_obj_t *dashboard_Classic_Max_power_seg_13;
	lv_obj_t *dashboard_Classic_Max_power_max_label;
	lv_obj_t *dashboard_Classic_Max_power_max_val;
	lv_obj_t *dashboard_Classic_Max_bottom_bg;
	lv_obj_t *dashboard_Classic_Max_bottom_top_sep;
	lv_obj_t *dashboard_Classic_Max_bottom_col_sep_0;
	lv_obj_t *dashboard_Classic_Max_bottom_col_sep_1;
	lv_obj_t *dashboard_Classic_Max_bottom_col_sep_2;
	lv_obj_t *dashboard_Classic_Max_bottom_col_sep_3;
	lv_obj_t *dashboard_Classic_Max_col_trip_label;
	lv_obj_t *dashboard_Classic_Max_TRIP_text;
	lv_obj_t *dashboard_Classic_Max_col_trip_unit;
	lv_obj_t *dashboard_Classic_Max_col_odo_label;
	lv_obj_t *dashboard_Classic_Max_odo_text;
	lv_obj_t *dashboard_Classic_Max_col_odo_unit;
	lv_obj_t *dashboard_Classic_Max_col_mtmp_label;
	lv_obj_t *dashboard_Classic_Max_temp_mot_text;
	lv_obj_t *dashboard_Classic_Max_col_mtmp_unit;
	lv_obj_t *dashboard_Classic_Max_col_ctmp_label;
	lv_obj_t *dashboard_Classic_Max_temp_esc_text;
	lv_obj_t *dashboard_Classic_Max_col_ctmp_unit;
	lv_obj_t *dashboard_Classic_Max_col_avg_label;
	lv_obj_t *dashboard_Classic_Max_col_avg_value;
	lv_obj_t *dashboard_Classic_Max_col_avg_unit;
	lv_obj_t *dashboard_Classic_Max_cruise_control_img;
	lv_obj_t *dashboard_Classic_Max_esc_not_connected_text;
	lv_obj_t *dashboard_Classic_Max_Ah_text;
	lv_obj_t *dashboard_Classic_Max_Speed_cc_text;
	lv_obj_t *dashboard_Classic_Max_Settings_text;
	lv_obj_t *dashboard_Classic_Max_col_v_label;
	lv_obj_t *dashboard_Classic_Max_Voltage_text;
	lv_obj_t *dashboard_Classic_Max_col_voltage_label;
	lv_obj_t *dashboard_Classic_Max_cur_time_label;
	lv_obj_t *dashboard_Classic_Max_brightness_slider;
	lv_obj_t *dashboard_Classic_Max_statistics_button;
	lv_obj_t *dashboard_Classic_Max_max_power_label;
	lv_obj_t *dashboard_Classic_Max_max_power_text;
	lv_obj_t *dashboard_Classic_Max_max_speed_label;
	lv_obj_t *dashboard_Classic_Max_max_speed_text;
	lv_obj_t *dashboard_Classic_Max_max_reset_btn;
	lv_obj_t *dashboard_Classic_Max_max_reset_btn_label;
	lv_obj_t *dashboard_Lamborghini;
	bool dashboard_Lamborghini_del;
	lv_obj_t *dashboard_Lamborghini_frame_left;
	lv_obj_t *dashboard_Lamborghini_frame_right;
	lv_obj_t *dashboard_Lamborghini_accent_left;
	lv_obj_t *dashboard_Lamborghini_accent_right;
	lv_obj_t *dashboard_Lamborghini_speed_seg_00;
	lv_obj_t *dashboard_Lamborghini_speed_seg_01;
	lv_obj_t *dashboard_Lamborghini_speed_seg_02;
	lv_obj_t *dashboard_Lamborghini_speed_seg_03;
	lv_obj_t *dashboard_Lamborghini_speed_seg_04;
	lv_obj_t *dashboard_Lamborghini_speed_seg_05;
	lv_obj_t *dashboard_Lamborghini_speed_seg_06;
	lv_obj_t *dashboard_Lamborghini_speed_seg_07;
	lv_obj_t *dashboard_Lamborghini_speed_seg_08;
	lv_obj_t *dashboard_Lamborghini_speed_seg_09;
	lv_obj_t *dashboard_Lamborghini_speed_seg_10;
	lv_obj_t *dashboard_Lamborghini_speed_seg_11;
	lv_obj_t *dashboard_Lamborghini_card_batt;
	lv_obj_t *dashboard_Lamborghini_card_volt;
	lv_obj_t *dashboard_Lamborghini_card_amp;
	lv_obj_t *dashboard_Lamborghini_batt_lbl;
	lv_obj_t *dashboard_Lamborghini_Battery_proc_text;
	lv_obj_t *dashboard_Lamborghini_batt_bar;
	lv_obj_t *dashboard_Lamborghini_volt_lbl;
	lv_obj_t *dashboard_Lamborghini_Voltage_text;
	lv_obj_t *dashboard_Lamborghini_amp_lbl;
	lv_obj_t *dashboard_Lamborghini_Current_text;
	lv_obj_t *dashboard_Lamborghini_card_tmot;
	lv_obj_t *dashboard_Lamborghini_card_tfet;
	lv_obj_t *dashboard_Lamborghini_card_range;
	lv_obj_t *dashboard_Lamborghini_tmot_lbl;
	lv_obj_t *dashboard_Lamborghini_temp_mot_text;
	lv_obj_t *dashboard_Lamborghini_temp_mot_unit;
	lv_obj_t *dashboard_Lamborghini_tfet_lbl;
	lv_obj_t *dashboard_Lamborghini_temp_esc_text;
	lv_obj_t *dashboard_Lamborghini_temp_esc_unit;
	lv_obj_t *dashboard_Lamborghini_range_lbl;
	lv_obj_t *dashboard_Lamborghini_Range_text;
	lv_obj_t *dashboard_Lamborghini_range_unit;
	lv_obj_t *dashboard_Lamborghini_card_vesc;
	lv_obj_t *dashboard_Lamborghini_status_vesc;
	lv_obj_t *dashboard_Lamborghini_card_settings;
	lv_obj_t *dashboard_Lamborghini_Settings_text;
	lv_obj_t *dashboard_Lamborghini_speed_meter;
	lv_meter_scale_t *dashboard_Lamborghini_speed_meter_scale_0;
	lv_meter_indicator_t *dashboard_Lamborghini_speed_meter_scale_0_arc_0;
	lv_meter_indicator_t *dashboard_Lamborghini_speed_meter_scale_0_arc_1;
	lv_meter_indicator_t *dashboard_Lamborghini_speed_meter_scale_0_arc_2;
	lv_obj_t *dashboard_Lamborghini_Speed_text;
	lv_obj_t *dashboard_Lamborghini_speed_unit;
	lv_obj_t *dashboard_Lamborghini_power_value;
	lv_obj_t *dashboard_Lamborghini_power_unit;
	lv_obj_t *dashboard_Lamborghini_flow_regen_lbl;
	lv_obj_t *dashboard_Lamborghini_flow_power_lbl;
	lv_obj_t *dashboard_Lamborghini_power_bar;
	lv_obj_t *dashboard_Lamborghini_flow_center;
	lv_obj_t *dashboard_Lamborghini_bottom_bar;
	lv_obj_t *dashboard_Lamborghini_led_tt_cruise_lbl;
	lv_obj_t *dashboard_Lamborghini_tt_cruise_lbl;
	lv_obj_t *dashboard_Lamborghini_led_tt_fault_lbl;
	lv_obj_t *dashboard_Lamborghini_tt_fault_lbl;
	lv_obj_t *dashboard_Lamborghini_led_status_bt;
	lv_obj_t *dashboard_Lamborghini_status_bt;
	lv_obj_t *dashboard_Lamborghini_trip_lbl;
	lv_obj_t *dashboard_Lamborghini_TRIP_text;
	lv_obj_t *dashboard_Lamborghini_trip_unit;
	lv_obj_t *dashboard_Lamborghini_odo_lbl;
	lv_obj_t *dashboard_Lamborghini_odo_text;
	lv_obj_t *dashboard_Lamborghini_odo_unit;
	lv_obj_t *dashboard_Lamborghini_mode_text;
	lv_obj_t *dashboard_Lamborghini_cur_time_label;
	lv_obj_t *dashboard_Supermoto;
	bool dashboard_Supermoto_del;
	lv_obj_t *dashboard_Supermoto_rail;
	lv_obj_t *dashboard_Supermoto_led_tt_cruise_lbl;
	lv_obj_t *dashboard_Supermoto_tt_cruise_lbl;
	lv_obj_t *dashboard_Supermoto_led_status_bt;
	lv_obj_t *dashboard_Supermoto_status_bt;
	lv_obj_t *dashboard_Supermoto_led_tt_fault_lbl;
	lv_obj_t *dashboard_Supermoto_tt_fault_lbl;
	lv_obj_t *dashboard_Supermoto_led_tt_cfg_lbl;
	lv_obj_t *dashboard_Supermoto_tt_cfg_lbl;
	lv_obj_t *dashboard_Supermoto_rail_info;
	lv_obj_t *dashboard_Supermoto_cur_time_label;
	lv_obj_t *dashboard_Supermoto_speed_meter;
	lv_meter_scale_t *dashboard_Supermoto_speed_meter_scale_0;
	lv_meter_indicator_t *dashboard_Supermoto_speed_meter_scale_0_arc_0;
	lv_meter_indicator_t *dashboard_Supermoto_speed_meter_scale_0_arc_1;
	lv_meter_indicator_t *dashboard_Supermoto_speed_meter_scale_0_arc_2;
	lv_obj_t *dashboard_Supermoto_tach_face;
	lv_obj_t *dashboard_Supermoto_tach_max;
	lv_obj_t *dashboard_Supermoto_Speed_text;
	lv_obj_t *dashboard_Supermoto_speed_unit;
	lv_obj_t *dashboard_Supermoto_batt_card;
	lv_obj_t *dashboard_Supermoto_batt_lbl;
	lv_obj_t *dashboard_Supermoto_Battery_proc_text;
	lv_obj_t *dashboard_Supermoto_batt_pct_unit;
	lv_obj_t *dashboard_Supermoto_batt_seg_13;
	lv_obj_t *dashboard_Supermoto_batt_seg_12;
	lv_obj_t *dashboard_Supermoto_batt_seg_11;
	lv_obj_t *dashboard_Supermoto_batt_seg_10;
	lv_obj_t *dashboard_Supermoto_batt_seg_09;
	lv_obj_t *dashboard_Supermoto_batt_seg_08;
	lv_obj_t *dashboard_Supermoto_batt_seg_07;
	lv_obj_t *dashboard_Supermoto_batt_seg_06;
	lv_obj_t *dashboard_Supermoto_batt_seg_05;
	lv_obj_t *dashboard_Supermoto_batt_seg_04;
	lv_obj_t *dashboard_Supermoto_batt_seg_03;
	lv_obj_t *dashboard_Supermoto_batt_seg_02;
	lv_obj_t *dashboard_Supermoto_batt_seg_01;
	lv_obj_t *dashboard_Supermoto_batt_seg_00;
	lv_obj_t *dashboard_Supermoto_Voltage_text;
	lv_obj_t *dashboard_Supermoto_volt_unit;
	lv_obj_t *dashboard_Supermoto_Current_text;
	lv_obj_t *dashboard_Supermoto_power_value;
	lv_obj_t *dashboard_Supermoto_power_unit;
	lv_obj_t *dashboard_Supermoto_temp_m_card;
	lv_obj_t *dashboard_Supermoto_temp_f_card;
	lv_obj_t *dashboard_Supermoto_temp_m_lbl;
	lv_obj_t *dashboard_Supermoto_temp_mot_text;
	lv_obj_t *dashboard_Supermoto_temp_mot_unit;
	lv_obj_t *dashboard_Supermoto_temp_mot_bar;
	lv_obj_t *dashboard_Supermoto_temp_f_lbl;
	lv_obj_t *dashboard_Supermoto_temp_esc_text;
	lv_obj_t *dashboard_Supermoto_temp_esc_unit;
	lv_obj_t *dashboard_Supermoto_temp_esc_bar;
	lv_obj_t *dashboard_Supermoto_flow_card;
	lv_obj_t *dashboard_Supermoto_flow_regen_lbl;
	lv_obj_t *dashboard_Supermoto_flow_drive_lbl;
	lv_obj_t *dashboard_Supermoto_flow_track;
	lv_obj_t *dashboard_Supermoto_power_bar;
	lv_obj_t *dashboard_Supermoto_flow_center;
	lv_obj_t *dashboard_Supermoto_card_vesc;
	lv_obj_t *dashboard_Supermoto_status_vesc;
	lv_obj_t *dashboard_Supermoto_card_settings;
	lv_obj_t *dashboard_Supermoto_Settings_text;
	lv_obj_t *dashboard_Supermoto_trip_card;
	lv_obj_t *dashboard_Supermoto_trip_lbl;
	lv_obj_t *dashboard_Supermoto_TRIP_text;
	lv_obj_t *dashboard_Supermoto_trip_unit;
	lv_obj_t *dashboard_Supermoto_odo_card;
	lv_obj_t *dashboard_Supermoto_odo_lbl;
	lv_obj_t *dashboard_Supermoto_odo_text;
	lv_obj_t *dashboard_Supermoto_odo_unit;
	lv_obj_t *dashboard_Supermoto_mode_card;
	lv_obj_t *dashboard_Supermoto_mode_lbl;
	lv_obj_t *dashboard_Supermoto_mode_text;
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


void setup_scr_dashboard_Classic(lv_ui *ui);
void setup_scr_settings(lv_ui *ui);
void setup_scr_reference(lv_ui *ui);
void setup_scr_dashboard_Classic_Max(lv_ui *ui);
void setup_scr_dashboard_Lamborghini(lv_ui *ui);
void setup_scr_dashboard_Supermoto(lv_ui *ui);
LV_IMG_DECLARE(_cruise_control_alpha_38x38);
#include "extra/widgets/animimg/lv_animimg.h"
#include "extra/widgets/animimg/lv_animimg.h"
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
LV_FONT_DECLARE(lv_font_Antonio_Regular_100)


#ifdef __cplusplus
}
#endif
#endif
