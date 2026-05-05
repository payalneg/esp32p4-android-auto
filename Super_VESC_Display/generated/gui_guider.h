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
	lv_obj_t *dashboard_tileview_1;
	lv_obj_t *dashboard_tileview_1_tile;
	lv_obj_t *dashboard_Battery_meter;
	lv_meter_scale_t *dashboard_Battery_meter_scale_0;
	lv_meter_indicator_t *dashboard_Battery_meter_scale_0_ndline_0;
	lv_meter_indicator_t *dashboard_Battery_meter_scale_0_arc_0;
	lv_meter_indicator_t *dashboard_Battery_meter_scale_0_arc_1;
	lv_obj_t *dashboard_reset_trip_img;
	lv_obj_t *dashboard_img_3;
	lv_obj_t *dashboard_Current_meter;
	lv_meter_scale_t *dashboard_Current_meter_scale_0;
	lv_meter_indicator_t *dashboard_Current_meter_scale_0_ndline_0;
	lv_meter_indicator_t *dashboard_Current_meter_scale_0_arc_0;
	lv_meter_indicator_t *dashboard_Current_meter_scale_0_arc_1;
	lv_meter_indicator_t *dashboard_Current_meter_scale_0_arc_2;
	lv_obj_t *dashboard_Speed_meter;
	lv_meter_scale_t *dashboard_Speed_meter_scale_0;
	lv_meter_indicator_t *dashboard_Speed_meter_scale_0_ndline_0;
	lv_meter_indicator_t *dashboard_Speed_meter_scale_0_arc_0;
	lv_meter_scale_t *dashboard_Speed_meter_scale_1;
	lv_meter_indicator_t *dashboard_Speed_meter_scale_1_ndline_0;
	lv_meter_scale_t *dashboard_Speed_meter_scale_2;
	lv_meter_indicator_t *dashboard_Speed_meter_scale_2_ndline_0;
	lv_obj_t *dashboard_img_1;
	lv_obj_t *dashboard_Speed_text;
	lv_obj_t *dashboard_odo_text;
	lv_obj_t *dashboard_ta_1;
	lv_obj_t *dashboard_ta_2;
	lv_obj_t *dashboard_Battery_proc_text;
	lv_obj_t *dashboard_ta_4;
	lv_obj_t *dashboard_TRIP_text;
	lv_obj_t *dashboard_ta_5;
	lv_obj_t *dashboard_Voltage_text;
	lv_obj_t *dashboard_ta_6;
	lv_obj_t *dashboard_ta_7;
	lv_obj_t *dashboard_ta_8;
	lv_obj_t *dashboard_ta_9;
	lv_obj_t *dashboard_ta_10;
	lv_obj_t *dashboard_Current_text;
	lv_obj_t *dashboard_ta_14;
	lv_obj_t *dashboard_Range_text;
	lv_obj_t *dashboard_ta_12;
	lv_obj_t *dashboard_Ah_text;
	lv_obj_t *dashboard_Ah_const_text;
	lv_obj_t *dashboard_ta_17;
	lv_obj_t *dashboard_ta_18;
	lv_obj_t *dashboard_ta_20;
	lv_obj_t *dashboard_temp_mot_text;
	lv_obj_t *dashboard_temp_esc_text;
	lv_obj_t *dashboard_ta_22;
	lv_obj_t *dashboard_ta_23;
	lv_obj_t *dashboard_temp_bat_text;
	lv_obj_t *dashboard_ta_25;
	lv_obj_t *dashboard_ta_26;
	lv_obj_t *dashboard_uptime_text;
	lv_obj_t *dashboard_ble_connected_img;
	lv_obj_t *dashboard_esc_not_connected_text;
	lv_obj_t *dashboard_cruise_control_img;
	lv_obj_t *dashboard_mode_text;
	lv_obj_t *dashboard_slider_1;
	lv_obj_t *dashboard_slider_2;
	lv_obj_t *dashboard_slider_3;
	lv_obj_t *dashboard_fps_text;
	lv_obj_t *dashboard_settings_button;
	lv_obj_t *dashboard_navigation_icon;
	lv_obj_t *dashboard_navigation_text;
	lv_obj_t *dashboard_music_text;
	lv_obj_t *dashboard_img_4;
	lv_obj_t *dashboard_symbols;
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

LV_IMG_DECLARE(_grid_480_800x480);
LV_IMG_DECLARE(_trip_alpha_20x24);
LV_IMG_DECLARE(_battery_alpha_30x35);
LV_IMG_DECLARE(_speed_background_alpha_162x162);
LV_IMG_DECLARE(_ble_con_alpha_43x30);
LV_IMG_DECLARE(_cruise_control_alpha_38x38);
LV_IMG_DECLARE(_settings2_alpha_30x30);

LV_FONT_DECLARE(lv_font_montserratMedium_13)
LV_FONT_DECLARE(lv_font_Montserrat_I_61)
LV_FONT_DECLARE(lv_font_Montserrat_I_16)
LV_FONT_DECLARE(lv_font_Montserrat_I_18)
LV_FONT_DECLARE(lv_font_Montserrat_I_20)
LV_FONT_DECLARE(lv_font_Montserrat_I_26)
LV_FONT_DECLARE(lv_font_Montserrat_I_15)
LV_FONT_DECLARE(lv_font_Antonio_Regular_22)
LV_FONT_DECLARE(lv_font_Montserrat_I_24)
LV_FONT_DECLARE(lv_font_montserratMedium_12)
LV_FONT_DECLARE(lv_font_montserratMedium_20)
LV_FONT_DECLARE(lv_font_montserratMedium_24)


#ifdef __cplusplus
}
#endif
#endif
