/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_dashboard_Supermoto(lv_ui *ui)
{
    //Write codes dashboard_Supermoto
    ui->dashboard_Supermoto = lv_obj_create(NULL);
    lv_obj_set_size(ui->dashboard_Supermoto, 800, 480);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto, lv_color_hex(0x0A0B0C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_rail
    ui->dashboard_Supermoto_rail = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_rail, 0, 0);
    lv_obj_set_size(ui->dashboard_Supermoto_rail, 800, 38);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_rail, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_rail, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_rail, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_rail, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_rail, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_rail, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_rail, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_rail, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_rail, lv_color_hex(0x111315), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_rail, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_rail, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_rail, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_rail, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_rail, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_rail, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_led_tt_cruise_lbl
    ui->dashboard_Supermoto_led_tt_cruise_lbl = lv_led_create(ui->dashboard_Supermoto);
    lv_led_set_brightness(ui->dashboard_Supermoto_led_tt_cruise_lbl, 255);
    lv_led_set_color(ui->dashboard_Supermoto_led_tt_cruise_lbl, lv_color_hex(0x4ADE80));
    lv_obj_set_pos(ui->dashboard_Supermoto_led_tt_cruise_lbl, 16, 12);
    lv_obj_set_size(ui->dashboard_Supermoto_led_tt_cruise_lbl, 14, 14);

    //Write codes dashboard_Supermoto_tt_cruise_lbl
    ui->dashboard_Supermoto_tt_cruise_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_tt_cruise_lbl, "CRZ");
    lv_label_set_long_mode(ui->dashboard_Supermoto_tt_cruise_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_tt_cruise_lbl, 34, 13);
    lv_obj_set_size(ui->dashboard_Supermoto_tt_cruise_lbl, 36, 13);

    //Write style for dashboard_Supermoto_tt_cruise_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_tt_cruise_lbl, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_tt_cruise_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_tt_cruise_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_tt_cruise_lbl, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_tt_cruise_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_led_status_bt
    ui->dashboard_Supermoto_led_status_bt = lv_led_create(ui->dashboard_Supermoto);
    lv_led_set_brightness(ui->dashboard_Supermoto_led_status_bt, 255);
    lv_led_set_color(ui->dashboard_Supermoto_led_status_bt, lv_color_hex(0x38BDF8));
    lv_obj_set_pos(ui->dashboard_Supermoto_led_status_bt, 74, 12);
    lv_obj_set_size(ui->dashboard_Supermoto_led_status_bt, 14, 14);

    //Write codes dashboard_Supermoto_status_bt
    ui->dashboard_Supermoto_status_bt = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_status_bt, "BT");
    lv_label_set_long_mode(ui->dashboard_Supermoto_status_bt, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_status_bt, 92, 13);
    lv_obj_set_size(ui->dashboard_Supermoto_status_bt, 36, 13);

    //Write style for dashboard_Supermoto_status_bt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_status_bt, lv_color_hex(0x38BDF8), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_status_bt, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_status_bt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_status_bt, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_status_bt, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_led_tt_fault_lbl
    ui->dashboard_Supermoto_led_tt_fault_lbl = lv_led_create(ui->dashboard_Supermoto);
    lv_led_set_brightness(ui->dashboard_Supermoto_led_tt_fault_lbl, 40);
    lv_led_set_color(ui->dashboard_Supermoto_led_tt_fault_lbl, lv_color_hex(0xEF4444));
    lv_obj_set_pos(ui->dashboard_Supermoto_led_tt_fault_lbl, 132, 12);
    lv_obj_set_size(ui->dashboard_Supermoto_led_tt_fault_lbl, 14, 14);

    //Write codes dashboard_Supermoto_tt_fault_lbl
    ui->dashboard_Supermoto_tt_fault_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_tt_fault_lbl, "FLT");
    lv_label_set_long_mode(ui->dashboard_Supermoto_tt_fault_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_tt_fault_lbl, 150, 13);
    lv_obj_set_size(ui->dashboard_Supermoto_tt_fault_lbl, 36, 13);

    //Write style for dashboard_Supermoto_tt_fault_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_tt_fault_lbl, lv_color_hex(0x4A5158), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_tt_fault_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_tt_fault_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_tt_fault_lbl, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_tt_fault_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_led_tt_cfg_lbl
    ui->dashboard_Supermoto_led_tt_cfg_lbl = lv_led_create(ui->dashboard_Supermoto);
    lv_led_set_brightness(ui->dashboard_Supermoto_led_tt_cfg_lbl, 40);
    lv_led_set_color(ui->dashboard_Supermoto_led_tt_cfg_lbl, lv_color_hex(0xF5A623));
    lv_obj_set_pos(ui->dashboard_Supermoto_led_tt_cfg_lbl, 190, 12);
    lv_obj_set_size(ui->dashboard_Supermoto_led_tt_cfg_lbl, 14, 14);

    //Write codes dashboard_Supermoto_tt_cfg_lbl
    ui->dashboard_Supermoto_tt_cfg_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_tt_cfg_lbl, "CFG");
    lv_label_set_long_mode(ui->dashboard_Supermoto_tt_cfg_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_tt_cfg_lbl, 208, 13);
    lv_obj_set_size(ui->dashboard_Supermoto_tt_cfg_lbl, 36, 13);

    //Write style for dashboard_Supermoto_tt_cfg_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_tt_cfg_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_tt_cfg_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_tt_cfg_lbl, lv_color_hex(0x4A5158), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_tt_cfg_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_tt_cfg_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_tt_cfg_lbl, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_tt_cfg_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_tt_cfg_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_tt_cfg_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_tt_cfg_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_tt_cfg_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_tt_cfg_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_tt_cfg_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_tt_cfg_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_rail_info
    ui->dashboard_Supermoto_rail_info = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_rail_info, "VESC / CAN 500K / ID 2");
    lv_label_set_long_mode(ui->dashboard_Supermoto_rail_info, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_rail_info, 280, 12);
    lv_obj_set_size(ui->dashboard_Supermoto_rail_info, 300, 16);

    //Write style for dashboard_Supermoto_rail_info, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_rail_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_rail_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_rail_info, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_rail_info, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_rail_info, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_rail_info, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_rail_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_rail_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_rail_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_rail_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_rail_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_rail_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_rail_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_rail_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_cur_time_label
    ui->dashboard_Supermoto_cur_time_label = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_cur_time_label, "14:32");
    lv_label_set_long_mode(ui->dashboard_Supermoto_cur_time_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_cur_time_label, 690, 9);
    lv_obj_set_size(ui->dashboard_Supermoto_cur_time_label, 94, 22);

    //Write style for dashboard_Supermoto_cur_time_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_cur_time_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_cur_time_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_cur_time_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_cur_time_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_speed_meter
    ui->dashboard_Supermoto_speed_meter = lv_meter_create(ui->dashboard_Supermoto);
    // add scale ui->dashboard_Supermoto_speed_meter_scale_0
    ui->dashboard_Supermoto_speed_meter_scale_0 = lv_meter_add_scale(ui->dashboard_Supermoto_speed_meter);
    lv_meter_set_scale_ticks(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0, 51, 1, 10, lv_color_hex(0x23262B));
    lv_meter_set_scale_major_ticks(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0, 5, 2, 12, lv_color_hex(0x4A4F58), 4);
    lv_meter_set_scale_range(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0, 0, 50, 270, 135);

    // add arc for ui->dashboard_Supermoto_speed_meter_scale_0
    ui->dashboard_Supermoto_speed_meter_scale_0_arc_0 = lv_meter_add_arc(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0, 20, lv_color_hex(0x17191C), -20);
    lv_meter_set_indicator_start_value(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0_arc_0, 0);
    lv_meter_set_indicator_end_value(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0_arc_0, 50);

    // add arc for ui->dashboard_Supermoto_speed_meter_scale_0
    ui->dashboard_Supermoto_speed_meter_scale_0_arc_1 = lv_meter_add_arc(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0, 3, lv_color_hex(0xEF4444), -4);
    lv_meter_set_indicator_start_value(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0_arc_1, 43);
    lv_meter_set_indicator_end_value(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0_arc_1, 50);

    // add arc for ui->dashboard_Supermoto_speed_meter_scale_0
    ui->dashboard_Supermoto_speed_meter_scale_0_arc_2 = lv_meter_add_arc(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0, 20, lv_color_hex(0x4ADE80), -20);
    lv_meter_set_indicator_start_value(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0_arc_2, 0);
    lv_meter_set_indicator_end_value(ui->dashboard_Supermoto_speed_meter, ui->dashboard_Supermoto_speed_meter_scale_0_arc_2, 28);
    lv_obj_set_pos(ui->dashboard_Supermoto_speed_meter, 27, 65);
    lv_obj_set_size(ui->dashboard_Supermoto_speed_meter, 308, 308);

    //Write style for dashboard_Supermoto_speed_meter, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_speed_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_speed_meter, 154, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_speed_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_speed_meter, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_speed_meter, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_speed_meter, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_speed_meter, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_speed_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Supermoto_speed_meter, Part: LV_PART_TICKS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_speed_meter, lv_color_hex(0x0A0B0C), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_speed_meter, &lv_font_montserratMedium_11, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_speed_meter, 255, LV_PART_TICKS|LV_STATE_DEFAULT);

    //Write style for dashboard_Supermoto_speed_meter, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_speed_meter, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_tach_face
    ui->dashboard_Supermoto_tach_face = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_tach_face, 73, 111);
    lv_obj_set_size(ui->dashboard_Supermoto_tach_face, 216, 216);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_tach_face, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_tach_face, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_tach_face, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_tach_face, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_tach_face, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_tach_face, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_tach_face, 108, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_tach_face, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_tach_face, lv_color_hex(0x0D0F11), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_tach_face, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_tach_face, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_tach_face, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_tach_face, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_tach_face, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_tach_face, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_tach_max
    ui->dashboard_Supermoto_tach_max = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_tach_max, "MAX 46");
    lv_label_set_long_mode(ui->dashboard_Supermoto_tach_max, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_tach_max, 31, 152);
    lv_obj_set_size(ui->dashboard_Supermoto_tach_max, 300, 14);

    //Write style for dashboard_Supermoto_tach_max, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_tach_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_tach_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_tach_max, lv_color_hex(0x5F646C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_tach_max, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_tach_max, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_tach_max, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_tach_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_tach_max, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_tach_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_tach_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_tach_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_tach_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_tach_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_tach_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_Speed_text
    ui->dashboard_Supermoto_Speed_text = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_Speed_text, "28");
    lv_label_set_long_mode(ui->dashboard_Supermoto_Speed_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_Speed_text, 31, 147);
    lv_obj_set_size(ui->dashboard_Supermoto_Speed_text, 300, 145);

    //Write style for dashboard_Supermoto_Speed_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_Speed_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_Speed_text, &lv_font_Antonio_Regular_100, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_Speed_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_Speed_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_speed_unit
    ui->dashboard_Supermoto_speed_unit = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_speed_unit, "KM/H");
    lv_label_set_long_mode(ui->dashboard_Supermoto_speed_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_speed_unit, 31, 268);
    lv_obj_set_size(ui->dashboard_Supermoto_speed_unit, 300, 22);

    //Write style for dashboard_Supermoto_speed_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_speed_unit, lv_color_hex(0xF5A623), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_speed_unit, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_speed_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_speed_unit, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_speed_unit, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_card
    ui->dashboard_Supermoto_batt_card = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_card, 364, 54);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_card, 420, 112);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_card, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_card, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_card, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_batt_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_batt_card, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_batt_card, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_card, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_card, lv_color_hex(0x111315), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_card, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_lbl
    ui->dashboard_Supermoto_batt_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_batt_lbl, "BATTERY");
    lv_label_set_long_mode(ui->dashboard_Supermoto_batt_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_lbl, 379, 68);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_lbl, 160, 14);

    //Write style for dashboard_Supermoto_batt_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_batt_lbl, lv_color_hex(0x8B909A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_batt_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_batt_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_batt_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_batt_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_Battery_proc_text
    ui->dashboard_Supermoto_Battery_proc_text = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_Battery_proc_text, "87");
    lv_label_set_long_mode(ui->dashboard_Supermoto_Battery_proc_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_Battery_proc_text, 604, 53);
    lv_obj_set_size(ui->dashboard_Supermoto_Battery_proc_text, 130, 58);

    //Write style for dashboard_Supermoto_Battery_proc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_Battery_proc_text, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_Battery_proc_text, &lv_font_Antonio_Regular_40, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_Battery_proc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_Battery_proc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_pct_unit
    ui->dashboard_Supermoto_batt_pct_unit = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_batt_pct_unit, "%");
    lv_label_set_long_mode(ui->dashboard_Supermoto_batt_pct_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_pct_unit, 738, 68);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_pct_unit, 32, 26);

    //Write style for dashboard_Supermoto_batt_pct_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_pct_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_pct_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_batt_pct_unit, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_batt_pct_unit, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_batt_pct_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_batt_pct_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_batt_pct_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_batt_pct_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_pct_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_pct_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_pct_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_pct_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_pct_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_pct_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_13
    ui->dashboard_Supermoto_batt_seg_13 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_13, 379, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_13, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_13, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_13, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_13, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_13, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_12
    ui->dashboard_Supermoto_batt_seg_12 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_12, 407, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_12, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_12, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_12, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_12, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_12, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_11
    ui->dashboard_Supermoto_batt_seg_11 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_11, 435, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_11, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_11, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_11, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_11, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_10
    ui->dashboard_Supermoto_batt_seg_10 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_10, 463, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_10, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_10, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_10, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_10, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_10, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_09
    ui->dashboard_Supermoto_batt_seg_09 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_09, 491, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_09, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_09, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_09, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_09, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_09, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_08
    ui->dashboard_Supermoto_batt_seg_08 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_08, 519, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_08, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_08, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_08, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_08, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_08, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_07
    ui->dashboard_Supermoto_batt_seg_07 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_07, 547, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_07, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_07, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_07, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_07, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_07, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_06
    ui->dashboard_Supermoto_batt_seg_06 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_06, 576, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_06, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_06, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_06, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_06, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_06, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_05
    ui->dashboard_Supermoto_batt_seg_05 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_05, 604, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_05, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_05, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_05, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_05, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_05, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_04
    ui->dashboard_Supermoto_batt_seg_04 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_04, 632, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_04, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_04, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_04, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_04, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_04, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_03
    ui->dashboard_Supermoto_batt_seg_03 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_03, 660, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_03, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_03, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_03, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_03, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_02
    ui->dashboard_Supermoto_batt_seg_02 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_02, 688, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_02, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_02, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_02, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_02, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_01
    ui->dashboard_Supermoto_batt_seg_01 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_01, 716, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_01, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_01, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_01, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_01, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_batt_seg_00
    ui->dashboard_Supermoto_batt_seg_00 = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_batt_seg_00, 744, 102);
    lv_obj_set_size(ui->dashboard_Supermoto_batt_seg_00, 25, 22);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_batt_seg_00, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_batt_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_batt_seg_00, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_batt_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_batt_seg_00, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_batt_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_Voltage_text
    ui->dashboard_Supermoto_Voltage_text = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_Voltage_text, "42.1");
    lv_label_set_long_mode(ui->dashboard_Supermoto_Voltage_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_Voltage_text, 379, 134);
    lv_obj_set_size(ui->dashboard_Supermoto_Voltage_text, 58, 16);

    //Write style for dashboard_Supermoto_Voltage_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_Voltage_text, lv_color_hex(0x8B909A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_Voltage_text, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_Voltage_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_Voltage_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_volt_unit
    ui->dashboard_Supermoto_volt_unit = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_volt_unit, "V");
    lv_label_set_long_mode(ui->dashboard_Supermoto_volt_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_volt_unit, 411, 134);
    lv_obj_set_size(ui->dashboard_Supermoto_volt_unit, 20, 16);

    //Write style for dashboard_Supermoto_volt_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_volt_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_volt_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_volt_unit, lv_color_hex(0x8B909A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_volt_unit, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_volt_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_volt_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_volt_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_volt_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_volt_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_volt_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_volt_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_volt_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_volt_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_volt_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_Current_text
    ui->dashboard_Supermoto_Current_text = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_Current_text, "31.4 A");
    lv_label_set_long_mode(ui->dashboard_Supermoto_Current_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_Current_text, 519, 134);
    lv_obj_set_size(ui->dashboard_Supermoto_Current_text, 110, 16);

    //Write style for dashboard_Supermoto_Current_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_Current_text, lv_color_hex(0x8B909A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_Current_text, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_Current_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_Current_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_power_value
    ui->dashboard_Supermoto_power_value = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_power_value, "1.3");
    lv_label_set_long_mode(ui->dashboard_Supermoto_power_value, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_power_value, 659, 134);
    lv_obj_set_size(ui->dashboard_Supermoto_power_value, 76, 16);

    //Write style for dashboard_Supermoto_power_value, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_power_value, lv_color_hex(0x8B909A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_power_value, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_power_value, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_power_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_power_unit
    ui->dashboard_Supermoto_power_unit = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_power_unit, "kW");
    lv_label_set_long_mode(ui->dashboard_Supermoto_power_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_power_unit, 739, 134);
    lv_obj_set_size(ui->dashboard_Supermoto_power_unit, 30, 16);

    //Write style for dashboard_Supermoto_power_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_power_unit, lv_color_hex(0x8B909A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_power_unit, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_power_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_power_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_temp_m_card
    ui->dashboard_Supermoto_temp_m_card = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_temp_m_card, 364, 178);
    lv_obj_set_size(ui->dashboard_Supermoto_temp_m_card, 204, 88);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_temp_m_card, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_temp_m_card, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_temp_m_card, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_temp_m_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_temp_m_card, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_temp_m_card, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_m_card, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_m_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_temp_m_card, lv_color_hex(0x111315), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_temp_m_card, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_temp_m_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_temp_m_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_temp_m_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_temp_m_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_temp_m_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_temp_f_card
    ui->dashboard_Supermoto_temp_f_card = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_temp_f_card, 580, 178);
    lv_obj_set_size(ui->dashboard_Supermoto_temp_f_card, 204, 88);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_temp_f_card, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_temp_f_card, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_temp_f_card, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_temp_f_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_temp_f_card, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_temp_f_card, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_f_card, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_f_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_temp_f_card, lv_color_hex(0x111315), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_temp_f_card, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_temp_f_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_temp_f_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_temp_f_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_temp_f_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_temp_f_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_temp_m_lbl
    ui->dashboard_Supermoto_temp_m_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_temp_m_lbl, "MOTOR");
    lv_label_set_long_mode(ui->dashboard_Supermoto_temp_m_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_temp_m_lbl, 377, 189);
    lv_obj_set_size(ui->dashboard_Supermoto_temp_m_lbl, 150, 14);

    //Write style for dashboard_Supermoto_temp_m_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_temp_m_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_m_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_temp_m_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_temp_m_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_temp_m_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_temp_m_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_temp_m_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_temp_m_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_m_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_temp_m_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_temp_m_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_temp_m_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_temp_m_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_temp_m_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_temp_mot_text
    ui->dashboard_Supermoto_temp_mot_text = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_temp_mot_text, "62");
    lv_label_set_long_mode(ui->dashboard_Supermoto_temp_mot_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_temp_mot_text, 377, 204);
    lv_obj_set_size(ui->dashboard_Supermoto_temp_mot_text, 44, 40);

    //Write style for dashboard_Supermoto_temp_mot_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_temp_mot_text, lv_color_hex(0xF5A623), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_temp_mot_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_temp_mot_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_temp_mot_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_temp_mot_unit
    ui->dashboard_Supermoto_temp_mot_unit = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_temp_mot_unit, "°");
    lv_label_set_long_mode(ui->dashboard_Supermoto_temp_mot_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_temp_mot_unit, 407, 204);
    lv_obj_set_size(ui->dashboard_Supermoto_temp_mot_unit, 24, 40);

    //Write style for dashboard_Supermoto_temp_mot_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_temp_mot_unit, lv_color_hex(0xF5A623), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_temp_mot_unit, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_temp_mot_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_temp_mot_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_temp_mot_bar
    ui->dashboard_Supermoto_temp_mot_bar = lv_bar_create(ui->dashboard_Supermoto);
    lv_obj_set_style_anim_time(ui->dashboard_Supermoto_temp_mot_bar, 1000, 0);
    lv_bar_set_mode(ui->dashboard_Supermoto_temp_mot_bar, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->dashboard_Supermoto_temp_mot_bar, 0, 100);
    lv_bar_set_value(ui->dashboard_Supermoto_temp_mot_bar, 62, LV_ANIM_OFF);
    lv_obj_set_pos(ui->dashboard_Supermoto_temp_mot_bar, 377, 248);
    lv_obj_set_size(ui->dashboard_Supermoto_temp_mot_bar, 178, 5);

    //Write style for dashboard_Supermoto_temp_mot_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_mot_bar, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_temp_mot_bar, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_temp_mot_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_mot_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_temp_mot_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Supermoto_temp_mot_bar, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_mot_bar, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_temp_mot_bar, lv_color_hex(0xF5A623), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_temp_mot_bar, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_mot_bar, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_temp_f_lbl
    ui->dashboard_Supermoto_temp_f_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_temp_f_lbl, "FET");
    lv_label_set_long_mode(ui->dashboard_Supermoto_temp_f_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_temp_f_lbl, 593, 189);
    lv_obj_set_size(ui->dashboard_Supermoto_temp_f_lbl, 150, 14);

    //Write style for dashboard_Supermoto_temp_f_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_temp_f_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_f_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_temp_f_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_temp_f_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_temp_f_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_temp_f_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_temp_f_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_temp_f_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_f_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_temp_f_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_temp_f_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_temp_f_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_temp_f_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_temp_f_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_temp_esc_text
    ui->dashboard_Supermoto_temp_esc_text = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_temp_esc_text, "48");
    lv_label_set_long_mode(ui->dashboard_Supermoto_temp_esc_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_temp_esc_text, 593, 204);
    lv_obj_set_size(ui->dashboard_Supermoto_temp_esc_text, 44, 40);

    //Write style for dashboard_Supermoto_temp_esc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_temp_esc_text, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_temp_esc_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_temp_esc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_temp_esc_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_temp_esc_unit
    ui->dashboard_Supermoto_temp_esc_unit = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_temp_esc_unit, "°");
    lv_label_set_long_mode(ui->dashboard_Supermoto_temp_esc_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_temp_esc_unit, 623, 204);
    lv_obj_set_size(ui->dashboard_Supermoto_temp_esc_unit, 24, 40);

    //Write style for dashboard_Supermoto_temp_esc_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_temp_esc_unit, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_temp_esc_unit, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_temp_esc_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_temp_esc_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_temp_esc_bar
    ui->dashboard_Supermoto_temp_esc_bar = lv_bar_create(ui->dashboard_Supermoto);
    lv_obj_set_style_anim_time(ui->dashboard_Supermoto_temp_esc_bar, 1000, 0);
    lv_bar_set_mode(ui->dashboard_Supermoto_temp_esc_bar, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->dashboard_Supermoto_temp_esc_bar, 0, 100);
    lv_bar_set_value(ui->dashboard_Supermoto_temp_esc_bar, 48, LV_ANIM_OFF);
    lv_obj_set_pos(ui->dashboard_Supermoto_temp_esc_bar, 593, 248);
    lv_obj_set_size(ui->dashboard_Supermoto_temp_esc_bar, 178, 5);

    //Write style for dashboard_Supermoto_temp_esc_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_esc_bar, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_temp_esc_bar, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_temp_esc_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_esc_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_temp_esc_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Supermoto_temp_esc_bar, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_temp_esc_bar, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_temp_esc_bar, lv_color_hex(0x4ADE80), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_temp_esc_bar, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_temp_esc_bar, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_flow_card
    ui->dashboard_Supermoto_flow_card = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_flow_card, 364, 278);
    lv_obj_set_size(ui->dashboard_Supermoto_flow_card, 420, 76);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_flow_card, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_flow_card, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_flow_card, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_flow_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_flow_card, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_flow_card, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_flow_card, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_flow_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_flow_card, lv_color_hex(0x111315), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_flow_card, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_flow_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_flow_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_flow_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_flow_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_flow_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_flow_regen_lbl
    ui->dashboard_Supermoto_flow_regen_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_flow_regen_lbl, "REGEN");
    lv_label_set_long_mode(ui->dashboard_Supermoto_flow_regen_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_flow_regen_lbl, 377, 289);
    lv_obj_set_size(ui->dashboard_Supermoto_flow_regen_lbl, 120, 14);

    //Write style for dashboard_Supermoto_flow_regen_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_flow_regen_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_flow_regen_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_flow_regen_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_flow_regen_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_flow_regen_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_flow_drive_lbl
    ui->dashboard_Supermoto_flow_drive_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_flow_drive_lbl, "DRIVE");
    lv_label_set_long_mode(ui->dashboard_Supermoto_flow_drive_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_flow_drive_lbl, 651, 289);
    lv_obj_set_size(ui->dashboard_Supermoto_flow_drive_lbl, 120, 14);

    //Write style for dashboard_Supermoto_flow_drive_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_flow_drive_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_flow_drive_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_flow_drive_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_flow_drive_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_flow_drive_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_flow_drive_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_flow_drive_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_flow_drive_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_flow_drive_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_flow_drive_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_flow_drive_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_flow_drive_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_flow_drive_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_flow_drive_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_flow_track
    ui->dashboard_Supermoto_flow_track = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_flow_track, 376, 310);
    lv_obj_set_size(ui->dashboard_Supermoto_flow_track, 396, 20);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_flow_track, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_flow_track, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_flow_track, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_flow_track, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_flow_track, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_flow_track, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_flow_track, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_flow_track, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_flow_track, lv_color_hex(0x0A0B0C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_flow_track, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_flow_track, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_flow_track, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_flow_track, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_flow_track, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_flow_track, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_power_bar
    ui->dashboard_Supermoto_power_bar = lv_bar_create(ui->dashboard_Supermoto);
    lv_obj_set_style_anim_time(ui->dashboard_Supermoto_power_bar, 1000, 0);
    lv_bar_set_mode(ui->dashboard_Supermoto_power_bar, LV_BAR_MODE_SYMMETRICAL);
    lv_bar_set_range(ui->dashboard_Supermoto_power_bar, -100, 100);
    lv_bar_set_value(ui->dashboard_Supermoto_power_bar, 28, LV_ANIM_OFF);
    lv_obj_set_pos(ui->dashboard_Supermoto_power_bar, 378, 312);
    lv_obj_set_size(ui->dashboard_Supermoto_power_bar, 392, 16);

    //Write style for dashboard_Supermoto_power_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_power_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_power_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_power_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Supermoto_power_bar, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_power_bar, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_power_bar, lv_color_hex(0xF5A623), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_power_bar, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_power_bar, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_flow_center
    ui->dashboard_Supermoto_flow_center = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_flow_center, 573, 312);
    lv_obj_set_size(ui->dashboard_Supermoto_flow_center, 2, 16);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_flow_center, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_flow_center, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_flow_center, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_flow_center, lv_color_hex(0x3A3F48), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_flow_center, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_card_vesc
    ui->dashboard_Supermoto_card_vesc = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_card_vesc, 28, 366);
    lv_obj_set_size(ui->dashboard_Supermoto_card_vesc, 150, 66);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_card_vesc, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_card_vesc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_card_vesc, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_card_vesc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_card_vesc, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_card_vesc, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_card_vesc, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_card_vesc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_card_vesc, lv_color_hex(0x111315), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_card_vesc, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_status_vesc
    ui->dashboard_Supermoto_status_vesc = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_status_vesc, "VESC");
    lv_label_set_long_mode(ui->dashboard_Supermoto_status_vesc, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_status_vesc, 28, 388);
    lv_obj_set_size(ui->dashboard_Supermoto_status_vesc, 150, 24);
    lv_obj_add_flag(ui->dashboard_Supermoto_status_vesc, LV_OBJ_FLAG_CLICKABLE);

    //Write style for dashboard_Supermoto_status_vesc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_status_vesc, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_status_vesc, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_status_vesc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_status_vesc, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_status_vesc, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_card_settings
    ui->dashboard_Supermoto_card_settings = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_card_settings, 194, 366);
    lv_obj_set_size(ui->dashboard_Supermoto_card_settings, 150, 66);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_card_settings, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_card_settings, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_card_settings, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_card_settings, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_card_settings, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_card_settings, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_card_settings, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_card_settings, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_card_settings, lv_color_hex(0x111315), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_card_settings, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_Settings_text
    ui->dashboard_Supermoto_Settings_text = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_Settings_text, "SETTINGS");
    lv_label_set_long_mode(ui->dashboard_Supermoto_Settings_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_Settings_text, 194, 388);
    lv_obj_set_size(ui->dashboard_Supermoto_Settings_text, 150, 24);
    lv_obj_add_flag(ui->dashboard_Supermoto_Settings_text, LV_OBJ_FLAG_CLICKABLE);

    //Write style for dashboard_Supermoto_Settings_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_Settings_text, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_Settings_text, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_Settings_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_Settings_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_Settings_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_trip_card
    ui->dashboard_Supermoto_trip_card = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_trip_card, 364, 366);
    lv_obj_set_size(ui->dashboard_Supermoto_trip_card, 132, 66);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_trip_card, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_trip_card, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_trip_card, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_trip_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_trip_card, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_trip_card, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_trip_card, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_trip_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_trip_card, lv_color_hex(0x111315), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_trip_card, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_trip_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_trip_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_trip_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_trip_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_trip_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_trip_lbl
    ui->dashboard_Supermoto_trip_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_trip_lbl, "TRIP");
    lv_label_set_long_mode(ui->dashboard_Supermoto_trip_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_trip_lbl, 376, 377);
    lv_obj_set_size(ui->dashboard_Supermoto_trip_lbl, 110, 14);

    //Write style for dashboard_Supermoto_trip_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_trip_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_trip_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_trip_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_trip_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_trip_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_TRIP_text
    ui->dashboard_Supermoto_TRIP_text = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_TRIP_text, "4.2");
    lv_label_set_long_mode(ui->dashboard_Supermoto_TRIP_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_TRIP_text, 376, 394);
    lv_obj_set_size(ui->dashboard_Supermoto_TRIP_text, 48, 30);

    //Write style for dashboard_Supermoto_TRIP_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_TRIP_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_TRIP_text, &lv_font_Antonio_Regular_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_TRIP_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_TRIP_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_trip_unit
    ui->dashboard_Supermoto_trip_unit = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_trip_unit, "KM");
    lv_label_set_long_mode(ui->dashboard_Supermoto_trip_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_trip_unit, 426, 402);
    lv_obj_set_size(ui->dashboard_Supermoto_trip_unit, 30, 18);

    //Write style for dashboard_Supermoto_trip_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_trip_unit, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_trip_unit, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_trip_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_trip_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_odo_card
    ui->dashboard_Supermoto_odo_card = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_odo_card, 508, 366);
    lv_obj_set_size(ui->dashboard_Supermoto_odo_card, 132, 66);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_odo_card, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_odo_card, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_odo_card, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_odo_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_odo_card, lv_color_hex(0x1E2124), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_odo_card, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_odo_card, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_odo_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_odo_card, lv_color_hex(0x111315), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_odo_card, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_odo_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_odo_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_odo_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_odo_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_odo_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_odo_lbl
    ui->dashboard_Supermoto_odo_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_odo_lbl, "ODO");
    lv_label_set_long_mode(ui->dashboard_Supermoto_odo_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_odo_lbl, 520, 377);
    lv_obj_set_size(ui->dashboard_Supermoto_odo_lbl, 110, 14);

    //Write style for dashboard_Supermoto_odo_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_odo_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_odo_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_odo_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_odo_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_odo_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_odo_text
    ui->dashboard_Supermoto_odo_text = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_odo_text, "00312");
    lv_label_set_long_mode(ui->dashboard_Supermoto_odo_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_odo_text, 520, 394);
    lv_obj_set_size(ui->dashboard_Supermoto_odo_text, 60, 30);

    //Write style for dashboard_Supermoto_odo_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_odo_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_odo_text, &lv_font_Antonio_Regular_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_odo_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_odo_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_odo_unit
    ui->dashboard_Supermoto_odo_unit = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_odo_unit, "KM");
    lv_label_set_long_mode(ui->dashboard_Supermoto_odo_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_odo_unit, 582, 402);
    lv_obj_set_size(ui->dashboard_Supermoto_odo_unit, 30, 18);

    //Write style for dashboard_Supermoto_odo_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_odo_unit, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_odo_unit, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_odo_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_odo_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_mode_card
    ui->dashboard_Supermoto_mode_card = lv_obj_create(ui->dashboard_Supermoto);
    lv_obj_set_pos(ui->dashboard_Supermoto_mode_card, 652, 366);
    lv_obj_set_size(ui->dashboard_Supermoto_mode_card, 132, 66);
    lv_obj_set_scrollbar_mode(ui->dashboard_Supermoto_mode_card, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Supermoto_mode_card, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_mode_card, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Supermoto_mode_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Supermoto_mode_card, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Supermoto_mode_card, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_mode_card, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_mode_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Supermoto_mode_card, lv_color_hex(0x111315), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Supermoto_mode_card, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_mode_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_mode_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_mode_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_mode_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_mode_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_mode_lbl
    ui->dashboard_Supermoto_mode_lbl = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_mode_lbl, "DRIVE");
    lv_label_set_long_mode(ui->dashboard_Supermoto_mode_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_mode_lbl, 664, 377);
    lv_obj_set_size(ui->dashboard_Supermoto_mode_lbl, 110, 14);

    //Write style for dashboard_Supermoto_mode_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_mode_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_mode_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_mode_lbl, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_mode_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_mode_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_mode_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_mode_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_mode_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_mode_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_mode_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_mode_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_mode_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_mode_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_mode_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Supermoto_mode_text
    ui->dashboard_Supermoto_mode_text = lv_label_create(ui->dashboard_Supermoto);
    lv_label_set_text(ui->dashboard_Supermoto_mode_text, "MODE 1");
    lv_label_set_long_mode(ui->dashboard_Supermoto_mode_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Supermoto_mode_text, 664, 394);
    lv_obj_set_size(ui->dashboard_Supermoto_mode_text, 110, 30);

    //Write style for dashboard_Supermoto_mode_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Supermoto_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Supermoto_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Supermoto_mode_text, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Supermoto_mode_text, &lv_font_Antonio_Regular_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Supermoto_mode_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Supermoto_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Supermoto_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Supermoto_mode_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Supermoto_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Supermoto_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Supermoto_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Supermoto_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Supermoto_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Supermoto_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of dashboard_Supermoto.


    //Update current screen layout.
    lv_obj_update_layout(ui->dashboard_Supermoto);

    //Init events for screen.
    events_init_dashboard_Supermoto(ui);
}
