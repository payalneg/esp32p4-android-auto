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



void setup_scr_dashboard(lv_ui *ui)
{
    //Write codes dashboard
    ui->dashboard = lv_obj_create(NULL);
    lv_obj_set_size(ui->dashboard, 800, 480);
    lv_obj_set_scrollbar_mode(ui->dashboard, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard, lv_color_hex(0x1f1f1f), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_src(ui->dashboard, &_grid_480_800x480, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_opa(ui->dashboard, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_tileview_1
    ui->dashboard_tileview_1 = lv_tileview_create(ui->dashboard);
    ui->dashboard_tileview_1_tile = lv_tileview_add_tile(ui->dashboard_tileview_1, 0, 0, LV_DIR_BOTTOM);
    lv_obj_set_pos(ui->dashboard_tileview_1, 0, 0);
    lv_obj_set_size(ui->dashboard_tileview_1, 800, 380);
    lv_obj_set_scrollbar_mode(ui->dashboard_tileview_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_tileview_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_tileview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_tileview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_tileview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_tileview_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_tileview_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_tileview_1, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_tileview_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_tileview_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes dashboard_Battery_meter
    ui->dashboard_Battery_meter = lv_meter_create(ui->dashboard_tileview_1_tile);
    // add scale ui->dashboard_Battery_meter_scale_0
    ui->dashboard_Battery_meter_scale_0 = lv_meter_add_scale(ui->dashboard_Battery_meter);
    lv_meter_set_scale_ticks(ui->dashboard_Battery_meter, ui->dashboard_Battery_meter_scale_0, 20, 0, 12, lv_color_hex(0x8e97a6));
    lv_meter_set_scale_range(ui->dashboard_Battery_meter, ui->dashboard_Battery_meter_scale_0, 0, 100, 100, 310);

    // add arc for ui->dashboard_Battery_meter_scale_0
    ui->dashboard_Battery_meter_scale_0_arc_0 = lv_meter_add_arc(ui->dashboard_Battery_meter, ui->dashboard_Battery_meter_scale_0, 10, lv_color_hex(0x2a3440), 0);
    lv_meter_set_indicator_start_value(ui->dashboard_Battery_meter, ui->dashboard_Battery_meter_scale_0_arc_0, 0);
    lv_meter_set_indicator_end_value(ui->dashboard_Battery_meter, ui->dashboard_Battery_meter_scale_0_arc_0, 100);

    // add arc for ui->dashboard_Battery_meter_scale_0
    ui->dashboard_Battery_meter_scale_0_arc_1 = lv_meter_add_arc(ui->dashboard_Battery_meter, ui->dashboard_Battery_meter_scale_0, 10, lv_color_hex(0x2acb48), 0);
    lv_meter_set_indicator_start_value(ui->dashboard_Battery_meter, ui->dashboard_Battery_meter_scale_0_arc_1, 33);
    lv_meter_set_indicator_end_value(ui->dashboard_Battery_meter, ui->dashboard_Battery_meter_scale_0_arc_1, 100);

    // add needle line for ui->dashboard_Battery_meter_scale_0.
    ui->dashboard_Battery_meter_scale_0_ndline_0 = lv_meter_add_needle_line(ui->dashboard_Battery_meter, ui->dashboard_Battery_meter_scale_0, 4, lv_color_hex(0xffffff), 0);
    lv_meter_set_indicator_value(ui->dashboard_Battery_meter, ui->dashboard_Battery_meter_scale_0_ndline_0, 33);
    lv_obj_set_pos(ui->dashboard_Battery_meter, 72, 5);
    lv_obj_set_size(ui->dashboard_Battery_meter, 336, 336);

    //Write style for dashboard_Battery_meter, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Battery_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Battery_meter, 175, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Battery_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Battery_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Battery_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Battery_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Battery_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Battery_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Battery_meter, Part: LV_PART_TICKS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Battery_meter, lv_color_hex(0xffffff), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Battery_meter, &lv_font_montserratMedium_13, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Battery_meter, 255, LV_PART_TICKS|LV_STATE_DEFAULT);

    //Write style for dashboard_Battery_meter, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Battery_meter, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Battery_meter, lv_color_hex(0x2a3440), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Battery_meter, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes dashboard_reset_trip_img
    ui->dashboard_reset_trip_img = lv_img_create(ui->dashboard_tileview_1_tile);
    lv_obj_add_flag(ui->dashboard_reset_trip_img, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->dashboard_reset_trip_img, &_trip_alpha_20x24);
    lv_img_set_pivot(ui->dashboard_reset_trip_img, 50,50);
    lv_img_set_angle(ui->dashboard_reset_trip_img, 0);
    lv_obj_set_pos(ui->dashboard_reset_trip_img, 444, 272);
    lv_obj_set_size(ui->dashboard_reset_trip_img, 20, 24);

    //Write style for dashboard_reset_trip_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_reset_trip_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_reset_trip_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_reset_trip_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_reset_trip_img, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_img_3
    ui->dashboard_img_3 = lv_img_create(ui->dashboard_tileview_1_tile);
    lv_obj_add_flag(ui->dashboard_img_3, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->dashboard_img_3, &_battery_alpha_30x35);
    lv_img_set_pivot(ui->dashboard_img_3, 50,50);
    lv_img_set_angle(ui->dashboard_img_3, 0);
    lv_obj_set_pos(ui->dashboard_img_3, 407, 157);
    lv_obj_set_size(ui->dashboard_img_3, 30, 35);

    //Write style for dashboard_img_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_img_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_img_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_img_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_img_3, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Current_meter
    ui->dashboard_Current_meter = lv_meter_create(ui->dashboard_tileview_1_tile);
    // add scale ui->dashboard_Current_meter_scale_0
    ui->dashboard_Current_meter_scale_0 = lv_meter_add_scale(ui->dashboard_Current_meter);
    lv_meter_set_scale_ticks(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0, 2, 0, 10, lv_color_hex(0x2a3440));
    lv_meter_set_scale_range(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0, 0, 60, 100, 130);

    // add arc for ui->dashboard_Current_meter_scale_0
    ui->dashboard_Current_meter_scale_0_arc_0 = lv_meter_add_arc(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0, 10, lv_color_hex(0x2a3440), 0);
    lv_meter_set_indicator_start_value(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0_arc_0, 0);
    lv_meter_set_indicator_end_value(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0_arc_0, 60);

    // add arc for ui->dashboard_Current_meter_scale_0
    ui->dashboard_Current_meter_scale_0_arc_1 = lv_meter_add_arc(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0, 10, lv_color_hex(0xff5700), 0);
    lv_meter_set_indicator_start_value(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0_arc_1, 0);
    lv_meter_set_indicator_end_value(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0_arc_1, 18);

    // add arc for ui->dashboard_Current_meter_scale_0
    ui->dashboard_Current_meter_scale_0_arc_2 = lv_meter_add_arc(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0, 10, lv_color_hex(0x00ffff), 0);
    lv_meter_set_indicator_start_value(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0_arc_2, 0);
    lv_meter_set_indicator_end_value(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0_arc_2, 0);

    // add needle line for ui->dashboard_Current_meter_scale_0.
    ui->dashboard_Current_meter_scale_0_ndline_0 = lv_meter_add_needle_line(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0, 4, lv_color_hex(0xffffff), 0);
    lv_meter_set_indicator_value(ui->dashboard_Current_meter, ui->dashboard_Current_meter_scale_0_ndline_0, 18);
    lv_obj_set_pos(ui->dashboard_Current_meter, 72, 5);
    lv_obj_set_size(ui->dashboard_Current_meter, 336, 336);

    //Write style for dashboard_Current_meter, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Current_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Current_meter, 175, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Current_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Current_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Current_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Current_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Current_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Current_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Current_meter, Part: LV_PART_TICKS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Current_meter, lv_color_hex(0xffffff), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Current_meter, &lv_font_montserratMedium_13, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Current_meter, 255, LV_PART_TICKS|LV_STATE_DEFAULT);

    //Write style for dashboard_Current_meter, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Current_meter, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Current_meter, lv_color_hex(0x2a3440), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Current_meter, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes dashboard_Speed_meter
    ui->dashboard_Speed_meter = lv_meter_create(ui->dashboard_tileview_1_tile);
    // add scale ui->dashboard_Speed_meter_scale_0
    ui->dashboard_Speed_meter_scale_0 = lv_meter_add_scale(ui->dashboard_Speed_meter);
    lv_meter_set_scale_ticks(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_0, 13, 3, 9, lv_color_hex(0x8e97a6));
    lv_meter_set_scale_major_ticks(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_0, 2, 4, 8, lv_color_hex(0xffffff), 11);
    lv_meter_set_scale_range(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_0, 0, 60, 282, 129);

    // add arc for ui->dashboard_Speed_meter_scale_0
    ui->dashboard_Speed_meter_scale_0_arc_0 = lv_meter_add_arc(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_0, 10, lv_color_hex(0x0ab8f7), 14);
    lv_meter_set_indicator_start_value(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_0_arc_0, 0);
    lv_meter_set_indicator_end_value(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_0_arc_0, 19);

    // add needle line for ui->dashboard_Speed_meter_scale_0.
    ui->dashboard_Speed_meter_scale_0_ndline_0 = lv_meter_add_needle_line(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_0, 4, lv_color_hex(0x3eb3df), 14);
    lv_meter_set_indicator_value(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_0_ndline_0, 19);
    // add scale ui->dashboard_Speed_meter_scale_1
    ui->dashboard_Speed_meter_scale_1 = lv_meter_add_scale(ui->dashboard_Speed_meter);
    lv_meter_set_scale_ticks(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_1, 41, 0, 0, lv_color_hex(0xff0000));
    lv_meter_set_scale_range(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_1, -6, 60, 310, 101);

    // add needle line for ui->dashboard_Speed_meter_scale_1.
    ui->dashboard_Speed_meter_scale_1_ndline_0 = lv_meter_add_needle_line(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_1, 3, lv_color_hex(0x00ff86), 2);
    lv_meter_set_indicator_value(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_1_ndline_0, 0);
    // add scale ui->dashboard_Speed_meter_scale_2
    ui->dashboard_Speed_meter_scale_2 = lv_meter_add_scale(ui->dashboard_Speed_meter);
    lv_meter_set_scale_ticks(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_2, 41, 0, 25, lv_color_hex(0x1f1f1f));
    lv_meter_set_scale_range(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_2, -6, 60, 310, 101);

    // add needle line for ui->dashboard_Speed_meter_scale_2.
    ui->dashboard_Speed_meter_scale_2_ndline_0 = lv_meter_add_needle_line(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_2, 10, lv_color_hex(0x1f1f1f), 6);
    lv_meter_set_indicator_value(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_2_ndline_0, 0);
    lv_obj_set_pos(ui->dashboard_Speed_meter, 103, 37);
    lv_obj_set_size(ui->dashboard_Speed_meter, 274, 274);

    //Write style for dashboard_Speed_meter, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Speed_meter, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Speed_meter, lv_color_hex(0x1f1f1f), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Speed_meter, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Speed_meter, 168, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Speed_meter, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Speed_meter, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Speed_meter, lv_color_hex(0x2a3440), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Speed_meter, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Speed_meter, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Speed_meter, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Speed_meter, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Speed_meter, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Speed_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Speed_meter, Part: LV_PART_TICKS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Speed_meter, lv_color_hex(0xffffff), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Speed_meter, &lv_font_montserratMedium_13, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Speed_meter, 255, LV_PART_TICKS|LV_STATE_DEFAULT);

    //Write style for dashboard_Speed_meter, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Speed_meter, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Speed_meter, lv_color_hex(0x2a3440), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Speed_meter, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes dashboard_img_1
    ui->dashboard_img_1 = lv_img_create(ui->dashboard_tileview_1_tile);
    lv_obj_add_flag(ui->dashboard_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->dashboard_img_1, &_speed_background_alpha_162x162);
    lv_img_set_pivot(ui->dashboard_img_1, 50,50);
    lv_img_set_angle(ui->dashboard_img_1, 0);
    lv_obj_set_pos(ui->dashboard_img_1, 159, 92);
    lv_obj_set_size(ui->dashboard_img_1, 162, 162);

    //Write style for dashboard_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Speed_text
    ui->dashboard_Speed_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_Speed_text, "38");
    lv_textarea_set_placeholder_text(ui->dashboard_Speed_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Speed_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Speed_text, false);
    lv_textarea_set_one_line(ui->dashboard_Speed_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Speed_text, "");
    lv_textarea_set_max_length(ui->dashboard_Speed_text, 2);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Speed_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Speed_text, 165, 137);
    lv_obj_set_size(ui->dashboard_Speed_text, 150, 110);

    //Write style for dashboard_Speed_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Speed_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Speed_text, &lv_font_Montserrat_I_61, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Speed_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Speed_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Speed_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Speed_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Speed_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Speed_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Speed_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Speed_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Speed_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Speed_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Speed_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Speed_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Speed_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Speed_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_odo_text
    ui->dashboard_odo_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_odo_text, "019018");
    lv_textarea_set_placeholder_text(ui->dashboard_odo_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_odo_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_odo_text, false);
    lv_textarea_set_one_line(ui->dashboard_odo_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_odo_text, "");
    lv_textarea_set_max_length(ui->dashboard_odo_text, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_odo_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_odo_text, 159, 267);
    lv_obj_set_size(ui->dashboard_odo_text, 100, 39);

    //Write style for dashboard_odo_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_odo_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_odo_text, &lv_font_Montserrat_I_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_odo_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_odo_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_odo_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_odo_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_odo_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_odo_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_odo_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_odo_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_odo_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_odo_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_odo_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_odo_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_1
    ui->dashboard_ta_1 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_1, "km");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_1, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_1, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_1, false);
    lv_textarea_set_one_line(ui->dashboard_ta_1, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_1, "");
    lv_textarea_set_max_length(ui->dashboard_ta_1, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_1, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_1, 193, 265);
    lv_obj_set_size(ui->dashboard_ta_1, 100, 39);

    //Write style for dashboard_ta_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_1, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_1, &lv_font_Montserrat_I_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_1, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_1, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_2
    ui->dashboard_ta_2 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_2, "km/h");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_2, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_2, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_2, false);
    lv_textarea_set_one_line(ui->dashboard_ta_2, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_2, "");
    lv_textarea_set_max_length(ui->dashboard_ta_2, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_2, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_2, 190, 204);
    lv_obj_set_size(ui->dashboard_ta_2, 100, 39);

    //Write style for dashboard_ta_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_2, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_2, &lv_font_Montserrat_I_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_2, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_2, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_2, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_2, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_2, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Battery_proc_text
    ui->dashboard_Battery_proc_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_Battery_proc_text, "38");
    lv_textarea_set_placeholder_text(ui->dashboard_Battery_proc_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Battery_proc_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Battery_proc_text, false);
    lv_textarea_set_one_line(ui->dashboard_Battery_proc_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Battery_proc_text, "");
    lv_textarea_set_max_length(ui->dashboard_Battery_proc_text, 3);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Battery_proc_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Battery_proc_text, 353, 166);
    lv_obj_set_size(ui->dashboard_Battery_proc_text, 107, 51);

    //Write style for dashboard_Battery_proc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Battery_proc_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Battery_proc_text, &lv_font_Montserrat_I_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Battery_proc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Battery_proc_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Battery_proc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Battery_proc_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Battery_proc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Battery_proc_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Battery_proc_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Battery_proc_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Battery_proc_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Battery_proc_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Battery_proc_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Battery_proc_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Battery_proc_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Battery_proc_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_4
    ui->dashboard_ta_4 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_4, "TRIP");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_4, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_4, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_4, false);
    lv_textarea_set_one_line(ui->dashboard_ta_4, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_4, "");
    lv_textarea_set_max_length(ui->dashboard_ta_4, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_4, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_4, 364, 271);
    lv_obj_set_size(ui->dashboard_ta_4, 73, 33);

    //Write style for dashboard_ta_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_4, lv_color_hex(0x2FCADA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_4, &lv_font_Montserrat_I_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_4, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_4, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_4, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_4, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_4, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_4, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_4, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_4, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_4, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_TRIP_text
    ui->dashboard_TRIP_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_TRIP_text, "180");
    lv_textarea_set_placeholder_text(ui->dashboard_TRIP_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_TRIP_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_TRIP_text, false);
    lv_textarea_set_one_line(ui->dashboard_TRIP_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_TRIP_text, "");
    lv_textarea_set_max_length(ui->dashboard_TRIP_text, 7);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_TRIP_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_TRIP_text, 337, 297);
    lv_obj_set_size(ui->dashboard_TRIP_text, 105, 40);

    //Write style for dashboard_TRIP_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_TRIP_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_TRIP_text, &lv_font_Montserrat_I_26, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_TRIP_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_TRIP_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_TRIP_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_TRIP_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_TRIP_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_TRIP_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_TRIP_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_TRIP_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_TRIP_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_TRIP_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_TRIP_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_TRIP_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_TRIP_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_5
    ui->dashboard_ta_5 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_5, "km");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_5, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_5, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_5, false);
    lv_textarea_set_one_line(ui->dashboard_ta_5, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_5, "");
    lv_textarea_set_max_length(ui->dashboard_ta_5, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_5, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_5, 372, 300);
    lv_obj_set_size(ui->dashboard_ta_5, 103, 35);

    //Write style for dashboard_ta_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_5, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_5, &lv_font_Montserrat_I_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_5, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_5, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_5, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_5, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_5, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_5, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_5, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_5, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_5, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_5, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Voltage_text
    ui->dashboard_Voltage_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_Voltage_text, "54.2");
    lv_textarea_set_placeholder_text(ui->dashboard_Voltage_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Voltage_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Voltage_text, false);
    lv_textarea_set_one_line(ui->dashboard_Voltage_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Voltage_text, "");
    lv_textarea_set_max_length(ui->dashboard_Voltage_text, 6);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Voltage_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Voltage_text, 347, 30);
    lv_obj_set_size(ui->dashboard_Voltage_text, 107, 51);

    //Write style for dashboard_Voltage_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Voltage_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Voltage_text, &lv_font_Montserrat_I_26, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Voltage_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Voltage_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Voltage_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Voltage_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Voltage_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Voltage_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Voltage_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Voltage_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Voltage_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Voltage_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Voltage_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Voltage_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Voltage_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Voltage_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_6
    ui->dashboard_ta_6 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_6, "VOLTAGE");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_6, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_6, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_6, false);
    lv_textarea_set_one_line(ui->dashboard_ta_6, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_6, "");
    lv_textarea_set_max_length(ui->dashboard_ta_6, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_6, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_6, 356, 6);
    lv_obj_set_size(ui->dashboard_ta_6, 119, 40);

    //Write style for dashboard_ta_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_6, lv_color_hex(0x2FCADA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_6, &lv_font_Montserrat_I_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_6, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_6, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_6, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_6, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_6, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_6, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_6, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_6, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_6, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_7
    ui->dashboard_ta_7 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_7, "%");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_7, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_7, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_7, false);
    lv_textarea_set_one_line(ui->dashboard_ta_7, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_7, "");
    lv_textarea_set_max_length(ui->dashboard_ta_7, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_7, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_7, 372, 165);
    lv_obj_set_size(ui->dashboard_ta_7, 102, 46);

    //Write style for dashboard_ta_7, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_7, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_7, &lv_font_Montserrat_I_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_7, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_7, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_7, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_7, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_7, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_7, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_7, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_7, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_7, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_7, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_8
    ui->dashboard_ta_8 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_8, "V");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_8, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_8, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_8, false);
    lv_textarea_set_one_line(ui->dashboard_ta_8, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_8, "");
    lv_textarea_set_max_length(ui->dashboard_ta_8, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_8, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_8, 373, 33);
    lv_obj_set_size(ui->dashboard_ta_8, 103, 47);

    //Write style for dashboard_ta_8, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_8, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_8, &lv_font_Montserrat_I_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_8, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_8, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_8, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_8, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_8, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_8, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_8, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_8, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_8, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_8, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_8, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_8, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_9
    ui->dashboard_ta_9 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_9, "CURRENT");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_9, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_9, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_9, false);
    lv_textarea_set_one_line(ui->dashboard_ta_9, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_9, "");
    lv_textarea_set_max_length(ui->dashboard_ta_9, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_9, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_9, 8, 6);
    lv_obj_set_size(ui->dashboard_ta_9, 202, 51);

    //Write style for dashboard_ta_9, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_9, lv_color_hex(0x2FCADA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_9, &lv_font_Montserrat_I_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_9, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_9, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_9, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_9, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_9, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_9, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_9, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_9, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_9, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_9, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_10
    ui->dashboard_ta_10 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_10, "A");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_10, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_10, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_10, false);
    lv_textarea_set_one_line(ui->dashboard_ta_10, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_10, "");
    lv_textarea_set_max_length(ui->dashboard_ta_10, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_10, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_10, -5, 33);
    lv_obj_set_size(ui->dashboard_ta_10, 103, 47);

    //Write style for dashboard_ta_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_10, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_10, &lv_font_Montserrat_I_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_10, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_10, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_10, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_10, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_10, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_10, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_10, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_10, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_10, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_10, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_10, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Current_text
    ui->dashboard_Current_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_Current_text, "54.2");
    lv_textarea_set_placeholder_text(ui->dashboard_Current_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Current_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Current_text, false);
    lv_textarea_set_one_line(ui->dashboard_Current_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Current_text, "");
    lv_textarea_set_max_length(ui->dashboard_Current_text, 5);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Current_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Current_text, -33, 30);
    lv_obj_set_size(ui->dashboard_Current_text, 107, 51);

    //Write style for dashboard_Current_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Current_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Current_text, &lv_font_Montserrat_I_26, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Current_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Current_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Current_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Current_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Current_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Current_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Current_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Current_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Current_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Current_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Current_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Current_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Current_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Current_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_14
    ui->dashboard_ta_14 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_14, "km");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_14, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_14, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_14, false);
    lv_textarea_set_one_line(ui->dashboard_ta_14, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_14, "");
    lv_textarea_set_max_length(ui->dashboard_ta_14, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_14, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_14, 16, 300);
    lv_obj_set_size(ui->dashboard_ta_14, 103, 35);

    //Write style for dashboard_ta_14, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_14, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_14, &lv_font_Montserrat_I_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_14, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_14, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_14, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_14, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_14, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_14, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_14, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_14, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_14, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_14, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_14, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_14, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Range_text
    ui->dashboard_Range_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_Range_text, "180");
    lv_textarea_set_placeholder_text(ui->dashboard_Range_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Range_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Range_text, false);
    lv_textarea_set_one_line(ui->dashboard_Range_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Range_text, "");
    lv_textarea_set_max_length(ui->dashboard_Range_text, 6);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Range_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Range_text, -22, 297);
    lv_obj_set_size(ui->dashboard_Range_text, 105, 40);

    //Write style for dashboard_Range_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Range_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Range_text, &lv_font_Montserrat_I_26, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Range_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Range_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Range_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Range_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Range_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Range_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Range_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Range_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Range_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Range_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Range_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Range_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Range_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_12
    ui->dashboard_ta_12 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_12, "RANGE");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_12, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_12, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_12, false);
    lv_textarea_set_one_line(ui->dashboard_ta_12, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_12, "");
    lv_textarea_set_max_length(ui->dashboard_ta_12, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_12, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_12, -4, 271);
    lv_obj_set_size(ui->dashboard_ta_12, 101, 34);

    //Write style for dashboard_ta_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_12, lv_color_hex(0x2FCADA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_12, &lv_font_Montserrat_I_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_12, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_12, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_12, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_12, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_12, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_12, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_12, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_12, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_12, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Ah_text
    ui->dashboard_Ah_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_Ah_text, "54.2");
    lv_textarea_set_placeholder_text(ui->dashboard_Ah_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Ah_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Ah_text, false);
    lv_textarea_set_one_line(ui->dashboard_Ah_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Ah_text, "");
    lv_textarea_set_max_length(ui->dashboard_Ah_text, 6);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Ah_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Ah_text, -44, 63);
    lv_obj_set_size(ui->dashboard_Ah_text, 107, 51);

    //Write style for dashboard_Ah_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Ah_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Ah_text, &lv_font_Montserrat_I_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Ah_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Ah_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Ah_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Ah_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Ah_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Ah_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Ah_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Ah_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Ah_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Ah_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Ah_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Ah_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Ah_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Ah_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Ah_const_text
    ui->dashboard_Ah_const_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_Ah_const_text, "Ah");
    lv_textarea_set_placeholder_text(ui->dashboard_Ah_const_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Ah_const_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Ah_const_text, false);
    lv_textarea_set_one_line(ui->dashboard_Ah_const_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Ah_const_text, "");
    lv_textarea_set_max_length(ui->dashboard_Ah_const_text, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Ah_const_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Ah_const_text, -15, 66);
    lv_obj_set_size(ui->dashboard_Ah_const_text, 103, 46);

    //Write style for dashboard_Ah_const_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Ah_const_text, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Ah_const_text, &lv_font_Montserrat_I_15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Ah_const_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Ah_const_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Ah_const_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Ah_const_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Ah_const_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Ah_const_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Ah_const_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Ah_const_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Ah_const_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Ah_const_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Ah_const_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Ah_const_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Ah_const_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Ah_const_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Ah_const_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_17
    ui->dashboard_ta_17 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_17, "t.MOT");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_17, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_17, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_17, false);
    lv_textarea_set_one_line(ui->dashboard_ta_17, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_17, "");
    lv_textarea_set_max_length(ui->dashboard_ta_17, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_17, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_17, 4, 126);
    lv_obj_set_size(ui->dashboard_ta_17, 97, 37);

    //Write style for dashboard_ta_17, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_17, lv_color_hex(0x2FCADA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_17, &lv_font_Montserrat_I_15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_17, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_17, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_17, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_17, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_17, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_17, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_17, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_17, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_17, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_17, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_18
    ui->dashboard_ta_18 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_18, "t.ESC");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_18, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_18, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_18, false);
    lv_textarea_set_one_line(ui->dashboard_ta_18, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_18, "");
    lv_textarea_set_max_length(ui->dashboard_ta_18, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_18, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_18, 4, 176);
    lv_obj_set_size(ui->dashboard_ta_18, 80, 40);

    //Write style for dashboard_ta_18, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_18, lv_color_hex(0x2FCADA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_18, &lv_font_Montserrat_I_15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_18, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_18, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_18, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_18, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_18, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_18, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_18, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_18, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_18, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_18, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_20
    ui->dashboard_ta_20 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_20, "C");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_20, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_20, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_20, false);
    lv_textarea_set_one_line(ui->dashboard_ta_20, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_20, "");
    lv_textarea_set_max_length(ui->dashboard_ta_20, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_20, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_20, -43, 142);
    lv_obj_set_size(ui->dashboard_ta_20, 103, 47);

    //Write style for dashboard_ta_20, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_20, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_20, &lv_font_Montserrat_I_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_20, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_20, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_20, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_20, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_20, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_20, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_20, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_20, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_20, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_20, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_20, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_20, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_temp_mot_text
    ui->dashboard_temp_mot_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_temp_mot_text, "18");
    lv_textarea_set_placeholder_text(ui->dashboard_temp_mot_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_temp_mot_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_temp_mot_text, false);
    lv_textarea_set_one_line(ui->dashboard_temp_mot_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_temp_mot_text, "");
    lv_textarea_set_max_length(ui->dashboard_temp_mot_text, 3);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_temp_mot_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_temp_mot_text, -63, 144);
    lv_obj_set_size(ui->dashboard_temp_mot_text, 107, 51);

    //Write style for dashboard_temp_mot_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_temp_mot_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_temp_mot_text, &lv_font_Montserrat_I_26, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_temp_mot_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_temp_mot_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_temp_mot_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_temp_mot_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_temp_mot_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_temp_mot_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_temp_mot_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_temp_mot_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_temp_mot_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_temp_mot_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_temp_mot_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_temp_mot_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_temp_mot_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_temp_mot_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_temp_esc_text
    ui->dashboard_temp_esc_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_temp_esc_text, "18");
    lv_textarea_set_placeholder_text(ui->dashboard_temp_esc_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_temp_esc_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_temp_esc_text, false);
    lv_textarea_set_one_line(ui->dashboard_temp_esc_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_temp_esc_text, "");
    lv_textarea_set_max_length(ui->dashboard_temp_esc_text, 3);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_temp_esc_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_temp_esc_text, -63, 194);
    lv_obj_set_size(ui->dashboard_temp_esc_text, 107, 51);

    //Write style for dashboard_temp_esc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_temp_esc_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_temp_esc_text, &lv_font_Montserrat_I_26, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_temp_esc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_temp_esc_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_temp_esc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_temp_esc_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_temp_esc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_temp_esc_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_temp_esc_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_temp_esc_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_temp_esc_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_temp_esc_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_temp_esc_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_temp_esc_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_temp_esc_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_temp_esc_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_22
    ui->dashboard_ta_22 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_22, "C");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_22, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_22, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_22, false);
    lv_textarea_set_one_line(ui->dashboard_ta_22, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_22, "");
    lv_textarea_set_max_length(ui->dashboard_ta_22, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_22, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_22, -43, 192);
    lv_obj_set_size(ui->dashboard_ta_22, 103, 47);

    //Write style for dashboard_ta_22, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_22, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_22, &lv_font_Montserrat_I_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_22, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_22, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_22, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_22, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_22, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_22, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_22, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_22, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_22, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_22, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_22, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_22, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_23
    ui->dashboard_ta_23 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_23, "C");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_23, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_23, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_23, false);
    lv_textarea_set_one_line(ui->dashboard_ta_23, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_23, "");
    lv_textarea_set_max_length(ui->dashboard_ta_23, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_23, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_23, 377, 152);
    lv_obj_set_size(ui->dashboard_ta_23, 103, 47);
    lv_obj_add_flag(ui->dashboard_ta_23, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_ta_23, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_23, lv_color_hex(0x8e97a6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_23, &lv_font_Montserrat_I_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_23, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_23, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_23, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_23, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_23, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_23, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_23, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_23, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_23, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_23, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_23, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_23, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_temp_bat_text
    ui->dashboard_temp_bat_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_temp_bat_text, "18");
    lv_textarea_set_placeholder_text(ui->dashboard_temp_bat_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_temp_bat_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_temp_bat_text, false);
    lv_textarea_set_one_line(ui->dashboard_temp_bat_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_temp_bat_text, "");
    lv_textarea_set_max_length(ui->dashboard_temp_bat_text, 3);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_temp_bat_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_temp_bat_text, 357, 152);
    lv_obj_set_size(ui->dashboard_temp_bat_text, 107, 51);
    lv_obj_add_flag(ui->dashboard_temp_bat_text, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_temp_bat_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_temp_bat_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_temp_bat_text, &lv_font_Montserrat_I_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_temp_bat_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_temp_bat_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_temp_bat_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_temp_bat_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_temp_bat_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_temp_bat_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_temp_bat_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_temp_bat_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_temp_bat_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_temp_bat_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_temp_bat_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_temp_bat_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_temp_bat_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_temp_bat_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_temp_bat_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_temp_bat_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_temp_bat_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_temp_bat_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_25
    ui->dashboard_ta_25 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_25, "t.BAT");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_25, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_25, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_25, false);
    lv_textarea_set_one_line(ui->dashboard_ta_25, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_25, "");
    lv_textarea_set_max_length(ui->dashboard_ta_25, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_25, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_25, 420, 136);
    lv_obj_set_size(ui->dashboard_ta_25, 57, 51);
    lv_obj_add_flag(ui->dashboard_ta_25, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_ta_25, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_25, lv_color_hex(0x2FCADA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_25, &lv_font_Montserrat_I_15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_25, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_25, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_25, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_25, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_25, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_25, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_25, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_25, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_25, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_25, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ta_26
    ui->dashboard_ta_26 = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_ta_26, "54.2");
    lv_textarea_set_placeholder_text(ui->dashboard_ta_26, "");
    lv_textarea_set_password_bullet(ui->dashboard_ta_26, "*");
    lv_textarea_set_password_mode(ui->dashboard_ta_26, false);
    lv_textarea_set_one_line(ui->dashboard_ta_26, true);
    lv_textarea_set_accepted_chars(ui->dashboard_ta_26, "");
    lv_textarea_set_max_length(ui->dashboard_ta_26, 6);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_ta_26, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_ta_26, 173, 25);
    lv_obj_set_size(ui->dashboard_ta_26, 107, 51);
    lv_obj_add_flag(ui->dashboard_ta_26, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_ta_26, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_ta_26, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_ta_26, &lv_font_Montserrat_I_26, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_ta_26, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_ta_26, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_ta_26, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_ta_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_ta_26, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_ta_26, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_ta_26, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_ta_26, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_ta_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_ta_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_ta_26, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_ta_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_26, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_ta_26, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_ta_26, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_ta_26, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_ta_26, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ta_26, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_uptime_text
    ui->dashboard_uptime_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_uptime_text, "12:34:56");
    lv_textarea_set_placeholder_text(ui->dashboard_uptime_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_uptime_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_uptime_text, false);
    lv_textarea_set_one_line(ui->dashboard_uptime_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_uptime_text, "");
    lv_textarea_set_max_length(ui->dashboard_uptime_text, 10);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_uptime_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_uptime_text, 191, 313);
    lv_obj_set_size(ui->dashboard_uptime_text, 107, 44);

    //Write style for dashboard_uptime_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_uptime_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_uptime_text, &lv_font_Montserrat_I_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_uptime_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_uptime_text, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_uptime_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_uptime_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_uptime_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_uptime_text, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_uptime_text, LV_BORDER_SIDE_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_uptime_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_uptime_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_uptime_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_uptime_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_uptime_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_uptime_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_uptime_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_ble_connected_img
    ui->dashboard_ble_connected_img = lv_img_create(ui->dashboard_tileview_1_tile);
    lv_obj_add_flag(ui->dashboard_ble_connected_img, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->dashboard_ble_connected_img, &_ble_con_alpha_43x30);
    lv_img_set_pivot(ui->dashboard_ble_connected_img, 0,0);
    lv_img_set_angle(ui->dashboard_ble_connected_img, 0);
    lv_obj_set_pos(ui->dashboard_ble_connected_img, 427, 81);
    lv_obj_set_size(ui->dashboard_ble_connected_img, 43, 30);

    //Write style for dashboard_ble_connected_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_ble_connected_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_ble_connected_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_ble_connected_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_ble_connected_img, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_esc_not_connected_text
    ui->dashboard_esc_not_connected_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_esc_not_connected_text, "ESC NOT CONNECTED");
    lv_textarea_set_placeholder_text(ui->dashboard_esc_not_connected_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_esc_not_connected_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_esc_not_connected_text, false);
    lv_textarea_set_one_line(ui->dashboard_esc_not_connected_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_esc_not_connected_text, "");
    lv_textarea_set_max_length(ui->dashboard_esc_not_connected_text, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_esc_not_connected_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_esc_not_connected_text, 126, 7);
    lv_obj_set_size(ui->dashboard_esc_not_connected_text, 237, 32);
    lv_obj_add_flag(ui->dashboard_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_esc_not_connected_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_esc_not_connected_text, lv_color_hex(0xe70023), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_esc_not_connected_text, &lv_font_Antonio_Regular_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_esc_not_connected_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_esc_not_connected_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_esc_not_connected_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_esc_not_connected_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_esc_not_connected_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_esc_not_connected_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_esc_not_connected_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_esc_not_connected_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_esc_not_connected_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_esc_not_connected_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_cruise_control_img
    ui->dashboard_cruise_control_img = lv_img_create(ui->dashboard_tileview_1_tile);
    lv_obj_add_flag(ui->dashboard_cruise_control_img, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->dashboard_cruise_control_img, &_cruise_control_alpha_38x38);
    lv_img_set_pivot(ui->dashboard_cruise_control_img, 0,0);
    lv_img_set_angle(ui->dashboard_cruise_control_img, 0);
    lv_obj_set_pos(ui->dashboard_cruise_control_img, 219, 98);
    lv_obj_set_size(ui->dashboard_cruise_control_img, 38, 38);

    //Write style for dashboard_cruise_control_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_cruise_control_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_cruise_control_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_cruise_control_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_cruise_control_img, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_mode_text
    ui->dashboard_mode_text = lv_textarea_create(ui->dashboard_tileview_1_tile);
    lv_textarea_set_text(ui->dashboard_mode_text, "");
    lv_textarea_set_placeholder_text(ui->dashboard_mode_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_mode_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_mode_text, false);
    lv_textarea_set_one_line(ui->dashboard_mode_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_mode_text, "");
    lv_textarea_set_max_length(ui->dashboard_mode_text, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_mode_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_mode_text, 139, 6);
    lv_obj_set_size(ui->dashboard_mode_text, 202, 51);

    //Write style for dashboard_mode_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_mode_text, lv_color_hex(0x2FCADA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_mode_text, &lv_font_Montserrat_I_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_mode_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_mode_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_mode_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_mode_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_mode_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_mode_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_mode_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_mode_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_mode_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_mode_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_slider_1
    ui->dashboard_slider_1 = lv_slider_create(ui->dashboard);
    lv_slider_set_range(ui->dashboard_slider_1, -60, 60);
    lv_slider_set_mode(ui->dashboard_slider_1, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->dashboard_slider_1, 50, LV_ANIM_OFF);
    lv_obj_set_pos(ui->dashboard_slider_1, 28, 355);
    lv_obj_set_size(ui->dashboard_slider_1, 418, 15);
    lv_obj_add_flag(ui->dashboard_slider_1, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_slider_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_slider_1, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_slider_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_slider_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_slider_1, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->dashboard_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_slider_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_slider_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_slider_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_slider_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_slider_1, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for dashboard_slider_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_slider_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_slider_1, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_slider_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_slider_1, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes dashboard_slider_2
    ui->dashboard_slider_2 = lv_slider_create(ui->dashboard);
    lv_slider_set_range(ui->dashboard_slider_2, 0, 60);
    lv_slider_set_mode(ui->dashboard_slider_2, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->dashboard_slider_2, 50, LV_ANIM_OFF);
    lv_obj_set_pos(ui->dashboard_slider_2, 28, 392);
    lv_obj_set_size(ui->dashboard_slider_2, 418, 15);
    lv_obj_add_flag(ui->dashboard_slider_2, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_slider_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_slider_2, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_slider_2, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_slider_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_slider_2, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->dashboard_slider_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_slider_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_slider_2, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_slider_2, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_slider_2, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_slider_2, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_slider_2, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for dashboard_slider_2, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_slider_2, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_slider_2, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_slider_2, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_slider_2, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes dashboard_slider_3
    ui->dashboard_slider_3 = lv_slider_create(ui->dashboard);
    lv_slider_set_range(ui->dashboard_slider_3, 0, 100);
    lv_slider_set_mode(ui->dashboard_slider_3, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->dashboard_slider_3, 50, LV_ANIM_OFF);
    lv_obj_set_pos(ui->dashboard_slider_3, 28, 430);
    lv_obj_set_size(ui->dashboard_slider_3, 418, 15);
    lv_obj_add_flag(ui->dashboard_slider_3, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_slider_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_slider_3, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_slider_3, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_slider_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_slider_3, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->dashboard_slider_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_slider_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_slider_3, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_slider_3, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_slider_3, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_slider_3, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_slider_3, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for dashboard_slider_3, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_slider_3, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_slider_3, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_slider_3, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_slider_3, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes dashboard_fps_text
    ui->dashboard_fps_text = lv_textarea_create(ui->dashboard);
    lv_textarea_set_text(ui->dashboard_fps_text, "FPS");
    lv_textarea_set_placeholder_text(ui->dashboard_fps_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_fps_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_fps_text, false);
    lv_textarea_set_one_line(ui->dashboard_fps_text, false);
    lv_textarea_set_accepted_chars(ui->dashboard_fps_text, "");
    lv_textarea_set_max_length(ui->dashboard_fps_text, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_fps_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_fps_text, 315, 445);
    lv_obj_set_size(ui->dashboard_fps_text, 158, 30);

    //Write style for dashboard_fps_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_fps_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_fps_text, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_fps_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_fps_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_fps_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_fps_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_fps_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_fps_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_fps_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_fps_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_fps_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_fps_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_fps_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_fps_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_fps_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_fps_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_fps_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_settings_button
    ui->dashboard_settings_button = lv_img_create(ui->dashboard);
    lv_obj_add_flag(ui->dashboard_settings_button, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->dashboard_settings_button, &_settings2_alpha_30x30);
    lv_img_set_pivot(ui->dashboard_settings_button, 0,0);
    lv_img_set_angle(ui->dashboard_settings_button, 0);
    lv_obj_set_pos(ui->dashboard_settings_button, 11, 440);
    lv_obj_set_size(ui->dashboard_settings_button, 30, 30);

    //Write style for dashboard_settings_button, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_settings_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_settings_button, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_settings_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_settings_button, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_navigation_icon
    ui->dashboard_navigation_icon = lv_img_create(ui->dashboard);
    lv_obj_add_flag(ui->dashboard_navigation_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_pivot(ui->dashboard_navigation_icon, 0,0);
    lv_img_set_angle(ui->dashboard_navigation_icon, 0);
    lv_obj_set_pos(ui->dashboard_navigation_icon, 0, 340);
    lv_obj_set_size(ui->dashboard_navigation_icon, 121, 121);
    lv_obj_add_flag(ui->dashboard_navigation_icon, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_navigation_icon, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_navigation_icon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_navigation_icon, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_navigation_icon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_navigation_icon, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_navigation_text
    ui->dashboard_navigation_text = lv_textarea_create(ui->dashboard);
    lv_textarea_set_text(ui->dashboard_navigation_text, "Navigation Off");
    lv_textarea_set_placeholder_text(ui->dashboard_navigation_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_navigation_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_navigation_text, false);
    lv_textarea_set_one_line(ui->dashboard_navigation_text, false);
    lv_textarea_set_accepted_chars(ui->dashboard_navigation_text, "");
    lv_textarea_set_max_length(ui->dashboard_navigation_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_navigation_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_navigation_text, 110, 350);
    lv_obj_set_size(ui->dashboard_navigation_text, 360, 60);
    lv_obj_add_flag(ui->dashboard_navigation_text, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_navigation_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_navigation_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_navigation_text, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_navigation_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_navigation_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_navigation_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_navigation_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_navigation_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_navigation_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_navigation_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_navigation_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_navigation_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_navigation_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_navigation_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_navigation_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_navigation_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_navigation_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_navigation_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_music_text
    ui->dashboard_music_text = lv_textarea_create(ui->dashboard);
    lv_textarea_set_text(ui->dashboard_music_text, "Music Off");
    lv_textarea_set_placeholder_text(ui->dashboard_music_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_music_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_music_text, false);
    lv_textarea_set_one_line(ui->dashboard_music_text, false);
    lv_textarea_set_accepted_chars(ui->dashboard_music_text, "");
    lv_textarea_set_max_length(ui->dashboard_music_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_music_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_music_text, 110, 416);
    lv_obj_set_size(ui->dashboard_music_text, 360, 60);

    //Write style for dashboard_music_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_music_text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_music_text, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_music_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_music_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_music_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_music_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_music_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_music_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_music_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_music_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_music_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_music_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_music_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_music_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_music_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_music_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_music_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_img_4
    ui->dashboard_img_4 = lv_img_create(ui->dashboard);
    lv_obj_add_flag(ui->dashboard_img_4, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_pivot(ui->dashboard_img_4, 0,0);
    lv_img_set_angle(ui->dashboard_img_4, 0);
    lv_obj_set_pos(ui->dashboard_img_4, 360, 320);
    lv_obj_set_size(ui->dashboard_img_4, 120, 120);
    lv_obj_add_flag(ui->dashboard_img_4, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_img_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_img_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_img_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_img_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_img_4, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_symbols
    ui->dashboard_symbols = lv_textarea_create(ui->dashboard);
    lv_textarea_set_text(ui->dashboard_symbols, "АаБбВвГгДдЕеЁёЖжЗзИиЙйКкЛлМмНнОоПпРрСсТтУуФфХхЦцЧчШшЩщЪъЫыЬьЭэЮюЯяAaĄąBbCcĆćDdEeĘęFfGgHhIiJjKkLlŁłMmNnŃńOoÓóPpRrSsŚśTtUuWwYyZzŹźŻż  ­\n’");
    lv_textarea_set_placeholder_text(ui->dashboard_symbols, "");
    lv_textarea_set_password_bullet(ui->dashboard_symbols, "*");
    lv_textarea_set_password_mode(ui->dashboard_symbols, false);
    lv_textarea_set_one_line(ui->dashboard_symbols, false);
    lv_textarea_set_accepted_chars(ui->dashboard_symbols, "");
    lv_textarea_set_max_length(ui->dashboard_symbols, 256);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_symbols, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_symbols, 0, -127);
    lv_obj_set_size(ui->dashboard_symbols, 800, 122);

    //Write style for dashboard_symbols, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_symbols, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_symbols, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_symbols, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_symbols, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_symbols, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_symbols, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_symbols, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_symbols, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_symbols, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_symbols, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_symbols, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_symbols, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_symbols, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_symbols, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_symbols, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_symbols, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_symbols, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_symbols, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_symbols, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_symbols, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_symbols, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_symbols, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //The custom code of dashboard.


    //Update current screen layout.
    lv_obj_update_layout(ui->dashboard);

    //Init events for screen.
    events_init_dashboard(ui);
}
