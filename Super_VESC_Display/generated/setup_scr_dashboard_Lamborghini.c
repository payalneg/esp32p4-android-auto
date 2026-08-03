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



void setup_scr_dashboard_Lamborghini(lv_ui *ui)
{
    //Write codes dashboard_Lamborghini
    ui->dashboard_Lamborghini = lv_obj_create(NULL);
    lv_obj_set_size(ui->dashboard_Lamborghini, 800, 480);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini, lv_color_hex(0x14161B), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Lamborghini, lv_color_hex(0x050506), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Lamborghini, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Lamborghini, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_frame_left
    ui->dashboard_Lamborghini_frame_left = lv_line_create(ui->dashboard_Lamborghini);
    static lv_point_t dashboard_Lamborghini_frame_left[] = {{1, 1},{96, 1},{129, 49},{129, 428},{1, 428},};
    lv_line_set_points(ui->dashboard_Lamborghini_frame_left, dashboard_Lamborghini_frame_left, 5);
    lv_obj_set_pos(ui->dashboard_Lamborghini_frame_left, 0, 26);
    lv_obj_set_size(ui->dashboard_Lamborghini_frame_left, 132, 430);

    //Write style for dashboard_Lamborghini_frame_left, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(ui->dashboard_Lamborghini_frame_left, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->dashboard_Lamborghini_frame_left, lv_color_hex(0x1F2228), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->dashboard_Lamborghini_frame_left, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_frame_right
    ui->dashboard_Lamborghini_frame_right = lv_line_create(ui->dashboard_Lamborghini);
    static lv_point_t dashboard_Lamborghini_frame_right[] = {{131, 1},{36, 1},{3, 49},{3, 428},{131, 428},};
    lv_line_set_points(ui->dashboard_Lamborghini_frame_right, dashboard_Lamborghini_frame_right, 5);
    lv_obj_set_pos(ui->dashboard_Lamborghini_frame_right, 668, 26);
    lv_obj_set_size(ui->dashboard_Lamborghini_frame_right, 132, 430);

    //Write style for dashboard_Lamborghini_frame_right, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(ui->dashboard_Lamborghini_frame_right, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->dashboard_Lamborghini_frame_right, lv_color_hex(0x1F2228), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->dashboard_Lamborghini_frame_right, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_accent_left
    ui->dashboard_Lamborghini_accent_left = lv_line_create(ui->dashboard_Lamborghini);
    static lv_point_t dashboard_Lamborghini_accent_left[] = {{0, 3},{68, 3},};
    lv_line_set_points(ui->dashboard_Lamborghini_accent_left, dashboard_Lamborghini_accent_left, 2);
    lv_obj_set_pos(ui->dashboard_Lamborghini_accent_left, 132, 27);
    lv_obj_set_size(ui->dashboard_Lamborghini_accent_left, 70, 6);

    //Write style for dashboard_Lamborghini_accent_left, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(ui->dashboard_Lamborghini_accent_left, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->dashboard_Lamborghini_accent_left, lv_color_hex(0xC8102E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->dashboard_Lamborghini_accent_left, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_accent_right
    ui->dashboard_Lamborghini_accent_right = lv_line_create(ui->dashboard_Lamborghini);
    static lv_point_t dashboard_Lamborghini_accent_right[] = {{2, 3},{70, 3},};
    lv_line_set_points(ui->dashboard_Lamborghini_accent_right, dashboard_Lamborghini_accent_right, 2);
    lv_obj_set_pos(ui->dashboard_Lamborghini_accent_right, 598, 27);
    lv_obj_set_size(ui->dashboard_Lamborghini_accent_right, 70, 6);

    //Write style for dashboard_Lamborghini_accent_right, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(ui->dashboard_Lamborghini_accent_right, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->dashboard_Lamborghini_accent_right, lv_color_hex(0xC8102E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->dashboard_Lamborghini_accent_right, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_00
    ui->dashboard_Lamborghini_speed_seg_00 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_00, 91, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_00, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_00, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_00, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_01
    ui->dashboard_Lamborghini_speed_seg_01 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_01, 143, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_01, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_01, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_01, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_02
    ui->dashboard_Lamborghini_speed_seg_02 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_02, 195, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_02, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_02, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_02, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_03
    ui->dashboard_Lamborghini_speed_seg_03 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_03, 247, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_03, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_03, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_03, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_04
    ui->dashboard_Lamborghini_speed_seg_04 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_04, 299, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_04, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_04, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_04, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_04, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_05
    ui->dashboard_Lamborghini_speed_seg_05 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_05, 351, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_05, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_05, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_05, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_05, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_06
    ui->dashboard_Lamborghini_speed_seg_06 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_06, 403, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_06, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_06, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_06, lv_color_hex(0xF5A623), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_06, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_07
    ui->dashboard_Lamborghini_speed_seg_07 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_07, 455, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_07, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_07, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_07, lv_color_hex(0x1A1C21), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_07, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_08
    ui->dashboard_Lamborghini_speed_seg_08 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_08, 507, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_08, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_08, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_08, lv_color_hex(0x1A1C21), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_08, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_09
    ui->dashboard_Lamborghini_speed_seg_09 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_09, 559, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_09, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_09, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_09, lv_color_hex(0x1A1C21), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_09, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_10
    ui->dashboard_Lamborghini_speed_seg_10 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_10, 611, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_10, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_10, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_10, lv_color_hex(0x1A1C21), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_10, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_seg_11
    ui->dashboard_Lamborghini_speed_seg_11 = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_seg_11, 663, 7);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_seg_11, 48, 5);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_speed_seg_11, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_speed_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_speed_seg_11, lv_color_hex(0x1A1C21), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_speed_seg_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_card_batt
    ui->dashboard_Lamborghini_card_batt = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_card_batt, 10, 74);
    lv_obj_set_size(ui->dashboard_Lamborghini_card_batt, 112, 78);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_card_batt, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_card_batt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_card_batt, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Lamborghini_card_batt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Lamborghini_card_batt, lv_color_hex(0xF5A623), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Lamborghini_card_batt, LV_BORDER_SIDE_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_card_batt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_card_batt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_card_batt, lv_color_hex(0x111318), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_card_batt, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_card_batt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_card_batt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_card_batt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_card_batt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_card_batt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_card_volt
    ui->dashboard_Lamborghini_card_volt = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_card_volt, 10, 163);
    lv_obj_set_size(ui->dashboard_Lamborghini_card_volt, 112, 64);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_card_volt, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_card_volt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_card_volt, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Lamborghini_card_volt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Lamborghini_card_volt, lv_color_hex(0x3A3F48), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Lamborghini_card_volt, LV_BORDER_SIDE_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_card_volt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_card_volt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_card_volt, lv_color_hex(0x111318), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_card_volt, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_card_volt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_card_volt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_card_volt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_card_volt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_card_volt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_card_amp
    ui->dashboard_Lamborghini_card_amp = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_card_amp, 10, 238);
    lv_obj_set_size(ui->dashboard_Lamborghini_card_amp, 112, 64);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_card_amp, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_card_amp, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_card_amp, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Lamborghini_card_amp, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Lamborghini_card_amp, lv_color_hex(0x3A3F48), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Lamborghini_card_amp, LV_BORDER_SIDE_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_card_amp, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_card_amp, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_card_amp, lv_color_hex(0x111318), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_card_amp, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_card_amp, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_card_amp, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_card_amp, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_card_amp, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_card_amp, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_batt_lbl
    ui->dashboard_Lamborghini_batt_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_batt_lbl, "BATT");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_batt_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_batt_lbl, 23, 84);
    lv_obj_set_size(ui->dashboard_Lamborghini_batt_lbl, 90, 14);

    //Write style for dashboard_Lamborghini_batt_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_batt_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_batt_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_batt_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_batt_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_batt_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_batt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_Battery_proc_text
    ui->dashboard_Lamborghini_Battery_proc_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_Battery_proc_text, "87");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_Battery_proc_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_Battery_proc_text, 23, 98);
    lv_obj_set_size(ui->dashboard_Lamborghini_Battery_proc_text, 90, 38);

    //Write style for dashboard_Lamborghini_Battery_proc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_Battery_proc_text, lv_color_hex(0xF5A623), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_Battery_proc_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_Battery_proc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_Battery_proc_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_batt_bar
    ui->dashboard_Lamborghini_batt_bar = lv_bar_create(ui->dashboard_Lamborghini);
    lv_obj_set_style_anim_time(ui->dashboard_Lamborghini_batt_bar, 1000, 0);
    lv_bar_set_mode(ui->dashboard_Lamborghini_batt_bar, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->dashboard_Lamborghini_batt_bar, 0, 100);
    lv_bar_set_value(ui->dashboard_Lamborghini_batt_bar, 87, LV_ANIM_OFF);
    lv_obj_set_pos(ui->dashboard_Lamborghini_batt_bar, 23, 138);
    lv_obj_set_size(ui->dashboard_Lamborghini_batt_bar, 88, 3);

    //Write style for dashboard_Lamborghini_batt_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_batt_bar, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_batt_bar, lv_color_hex(0x24272E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_batt_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_batt_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_batt_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Lamborghini_batt_bar, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_batt_bar, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_batt_bar, lv_color_hex(0xF5A623), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_batt_bar, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_batt_bar, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_volt_lbl
    ui->dashboard_Lamborghini_volt_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_volt_lbl, "VOLTS");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_volt_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_volt_lbl, 23, 173);
    lv_obj_set_size(ui->dashboard_Lamborghini_volt_lbl, 90, 14);

    //Write style for dashboard_Lamborghini_volt_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_volt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_volt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_volt_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_volt_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_volt_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_volt_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_volt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_volt_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_volt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_volt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_volt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_volt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_volt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_volt_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_Voltage_text
    ui->dashboard_Lamborghini_Voltage_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_Voltage_text, "42.1");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_Voltage_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_Voltage_text, 23, 187);
    lv_obj_set_size(ui->dashboard_Lamborghini_Voltage_text, 90, 36);

    //Write style for dashboard_Lamborghini_Voltage_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_Voltage_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_Voltage_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_Voltage_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_Voltage_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_amp_lbl
    ui->dashboard_Lamborghini_amp_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_amp_lbl, "AMPS");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_amp_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_amp_lbl, 23, 248);
    lv_obj_set_size(ui->dashboard_Lamborghini_amp_lbl, 90, 14);

    //Write style for dashboard_Lamborghini_amp_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_amp_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_amp_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_amp_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_amp_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_amp_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_amp_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_amp_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_amp_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_amp_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_amp_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_amp_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_amp_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_amp_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_amp_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_Current_text
    ui->dashboard_Lamborghini_Current_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_Current_text, "31.4 A");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_Current_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_Current_text, 23, 262);
    lv_obj_set_size(ui->dashboard_Lamborghini_Current_text, 95, 36);

    //Write style for dashboard_Lamborghini_Current_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_Current_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_Current_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_Current_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_Current_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_card_tmot
    ui->dashboard_Lamborghini_card_tmot = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_card_tmot, 678, 74);
    lv_obj_set_size(ui->dashboard_Lamborghini_card_tmot, 112, 66);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_card_tmot, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_card_tmot, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_card_tmot, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Lamborghini_card_tmot, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Lamborghini_card_tmot, lv_color_hex(0xE8613C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Lamborghini_card_tmot, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_card_tmot, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_card_tmot, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_card_tmot, lv_color_hex(0x111318), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_card_tmot, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_card_tmot, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_card_tmot, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_card_tmot, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_card_tmot, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_card_tmot, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_card_tfet
    ui->dashboard_Lamborghini_card_tfet = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_card_tfet, 678, 151);
    lv_obj_set_size(ui->dashboard_Lamborghini_card_tfet, 112, 66);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_card_tfet, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_card_tfet, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_card_tfet, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Lamborghini_card_tfet, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Lamborghini_card_tfet, lv_color_hex(0x3A3F48), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Lamborghini_card_tfet, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_card_tfet, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_card_tfet, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_card_tfet, lv_color_hex(0x111318), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_card_tfet, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_card_tfet, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_card_tfet, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_card_tfet, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_card_tfet, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_card_tfet, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_card_range
    ui->dashboard_Lamborghini_card_range = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_card_range, 678, 228);
    lv_obj_set_size(ui->dashboard_Lamborghini_card_range, 112, 66);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_card_range, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_card_range, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_card_range, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Lamborghini_card_range, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Lamborghini_card_range, lv_color_hex(0x3A3F48), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Lamborghini_card_range, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_card_range, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_card_range, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_card_range, lv_color_hex(0x111318), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_card_range, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_card_range, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_card_range, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_card_range, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_card_range, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_card_range, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_tmot_lbl
    ui->dashboard_Lamborghini_tmot_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_tmot_lbl, "MOTOR");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_tmot_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_tmot_lbl, 689, 84);
    lv_obj_set_size(ui->dashboard_Lamborghini_tmot_lbl, 90, 14);

    //Write style for dashboard_Lamborghini_tmot_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_tmot_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_tmot_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_tmot_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_tmot_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_tmot_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_tmot_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_tmot_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_tmot_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_tmot_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_tmot_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_tmot_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_tmot_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_tmot_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_tmot_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_temp_mot_text
    ui->dashboard_Lamborghini_temp_mot_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_temp_mot_text, "62");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_temp_mot_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_temp_mot_text, 679, 98);
    lv_obj_set_size(ui->dashboard_Lamborghini_temp_mot_text, 84, 36);

    //Write style for dashboard_Lamborghini_temp_mot_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_temp_mot_text, lv_color_hex(0xE8613C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_temp_mot_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_temp_mot_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_temp_mot_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_temp_mot_unit
    ui->dashboard_Lamborghini_temp_mot_unit = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_temp_mot_unit, "°");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_temp_mot_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_temp_mot_unit, 765, 98);
    lv_obj_set_size(ui->dashboard_Lamborghini_temp_mot_unit, 20, 36);

    //Write style for dashboard_Lamborghini_temp_mot_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_temp_mot_unit, lv_color_hex(0xE8613C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_temp_mot_unit, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_temp_mot_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_temp_mot_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_temp_mot_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_tfet_lbl
    ui->dashboard_Lamborghini_tfet_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_tfet_lbl, "FET");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_tfet_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_tfet_lbl, 689, 161);
    lv_obj_set_size(ui->dashboard_Lamborghini_tfet_lbl, 90, 14);

    //Write style for dashboard_Lamborghini_tfet_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_tfet_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_tfet_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_tfet_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_tfet_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_tfet_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_tfet_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_tfet_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_tfet_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_tfet_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_tfet_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_tfet_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_tfet_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_tfet_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_tfet_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_temp_esc_text
    ui->dashboard_Lamborghini_temp_esc_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_temp_esc_text, "48");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_temp_esc_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_temp_esc_text, 679, 175);
    lv_obj_set_size(ui->dashboard_Lamborghini_temp_esc_text, 84, 36);

    //Write style for dashboard_Lamborghini_temp_esc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_temp_esc_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_temp_esc_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_temp_esc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_temp_esc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_temp_esc_unit
    ui->dashboard_Lamborghini_temp_esc_unit = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_temp_esc_unit, "°");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_temp_esc_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_temp_esc_unit, 765, 175);
    lv_obj_set_size(ui->dashboard_Lamborghini_temp_esc_unit, 20, 36);

    //Write style for dashboard_Lamborghini_temp_esc_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_temp_esc_unit, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_temp_esc_unit, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_temp_esc_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_temp_esc_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_temp_esc_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_range_lbl
    ui->dashboard_Lamborghini_range_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_range_lbl, "RANGE");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_range_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_range_lbl, 689, 238);
    lv_obj_set_size(ui->dashboard_Lamborghini_range_lbl, 90, 14);

    //Write style for dashboard_Lamborghini_range_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_range_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_range_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_range_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_range_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_range_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_range_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_range_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_range_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_range_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_range_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_range_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_range_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_range_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_range_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_Range_text
    ui->dashboard_Lamborghini_Range_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_Range_text, "34.0");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_Range_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_Range_text, 668, 252);
    lv_obj_set_size(ui->dashboard_Lamborghini_Range_text, 84, 36);

    //Write style for dashboard_Lamborghini_Range_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_Range_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_Range_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_Range_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_Range_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_range_unit
    ui->dashboard_Lamborghini_range_unit = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_range_unit, "km");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_range_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_range_unit, 754, 266);
    lv_obj_set_size(ui->dashboard_Lamborghini_range_unit, 26, 16);

    //Write style for dashboard_Lamborghini_range_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_range_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_range_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_range_unit, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_range_unit, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_range_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_range_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_range_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_range_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_range_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_range_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_range_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_range_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_range_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_range_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_card_vesc
    ui->dashboard_Lamborghini_card_vesc = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_card_vesc, 10, 318);
    lv_obj_set_size(ui->dashboard_Lamborghini_card_vesc, 112, 46);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_card_vesc, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_card_vesc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_card_vesc, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Lamborghini_card_vesc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Lamborghini_card_vesc, lv_color_hex(0x3A3F48), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Lamborghini_card_vesc, LV_BORDER_SIDE_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_card_vesc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_card_vesc, lv_color_hex(0x111318), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_card_vesc, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_card_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_status_vesc
    ui->dashboard_Lamborghini_status_vesc = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_status_vesc, "VESC");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_status_vesc, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_status_vesc, 10, 331);
    lv_obj_set_size(ui->dashboard_Lamborghini_status_vesc, 112, 22);
    lv_obj_add_flag(ui->dashboard_Lamborghini_status_vesc, LV_OBJ_FLAG_CLICKABLE);

    //Write style for dashboard_Lamborghini_status_vesc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_status_vesc, lv_color_hex(0xF5A623), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_status_vesc, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_status_vesc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_status_vesc, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_status_vesc, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_card_settings
    ui->dashboard_Lamborghini_card_settings = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_card_settings, 678, 318);
    lv_obj_set_size(ui->dashboard_Lamborghini_card_settings, 112, 46);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_card_settings, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_card_settings, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_card_settings, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Lamborghini_card_settings, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Lamborghini_card_settings, lv_color_hex(0x3A3F48), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Lamborghini_card_settings, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_card_settings, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_card_settings, lv_color_hex(0x111318), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_card_settings, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_card_settings, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_Settings_text
    ui->dashboard_Lamborghini_Settings_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_Settings_text, "SETTINGS");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_Settings_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_Settings_text, 678, 331);
    lv_obj_set_size(ui->dashboard_Lamborghini_Settings_text, 112, 22);
    lv_obj_add_flag(ui->dashboard_Lamborghini_Settings_text, LV_OBJ_FLAG_CLICKABLE);

    //Write style for dashboard_Lamborghini_Settings_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_Settings_text, lv_color_hex(0xF5A623), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_Settings_text, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_Settings_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_Settings_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_meter
    ui->dashboard_Lamborghini_speed_meter = lv_meter_create(ui->dashboard_Lamborghini);
    // add scale ui->dashboard_Lamborghini_speed_meter_scale_0
    ui->dashboard_Lamborghini_speed_meter_scale_0 = lv_meter_add_scale(ui->dashboard_Lamborghini_speed_meter);
    lv_meter_set_scale_ticks(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0, 41, 1, 7, lv_color_hex(0x282C33));
    lv_meter_set_scale_major_ticks(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0, 4, 2, 13, lv_color_hex(0x4A4F58), 4);
    lv_meter_set_scale_range(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0, 0, 50, 270, 135);

    // add arc for ui->dashboard_Lamborghini_speed_meter_scale_0
    ui->dashboard_Lamborghini_speed_meter_scale_0_arc_0 = lv_meter_add_arc(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0, 13, lv_color_hex(0x1B1E24), 11);
    lv_meter_set_indicator_start_value(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0_arc_0, 0);
    lv_meter_set_indicator_end_value(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0_arc_0, 50);

    // add arc for ui->dashboard_Lamborghini_speed_meter_scale_0
    ui->dashboard_Lamborghini_speed_meter_scale_0_arc_1 = lv_meter_add_arc(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0, 13, lv_color_hex(0xC8102E), 11);
    lv_meter_set_indicator_start_value(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0_arc_1, 45);
    lv_meter_set_indicator_end_value(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0_arc_1, 50);

    // add arc for ui->dashboard_Lamborghini_speed_meter_scale_0
    ui->dashboard_Lamborghini_speed_meter_scale_0_arc_2 = lv_meter_add_arc(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0, 13, lv_color_hex(0xF5A623), 11);
    lv_meter_set_indicator_start_value(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0_arc_2, 0);
    lv_meter_set_indicator_end_value(ui->dashboard_Lamborghini_speed_meter, ui->dashboard_Lamborghini_speed_meter_scale_0_arc_2, 28);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_meter, 234, 80);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_meter, 332, 332);

    //Write style for dashboard_Lamborghini_speed_meter, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_meter, 166, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_meter, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_meter, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_meter, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_meter, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_meter, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Lamborghini_speed_meter, Part: LV_PART_TICKS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_speed_meter, lv_color_hex(0x050506), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_speed_meter, &lv_font_montserratMedium_11, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_speed_meter, 255, LV_PART_TICKS|LV_STATE_DEFAULT);

    //Write style for dashboard_Lamborghini_speed_meter, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_meter, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_Speed_text
    ui->dashboard_Lamborghini_Speed_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_Speed_text, "28");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_Speed_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_Speed_text, 250, 161);
    lv_obj_set_size(ui->dashboard_Lamborghini_Speed_text, 300, 145);

    //Write style for dashboard_Lamborghini_Speed_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_Speed_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_Speed_text, &lv_font_Antonio_Regular_100, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_Speed_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_Speed_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_speed_unit
    ui->dashboard_Lamborghini_speed_unit = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_speed_unit, "KM/H");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_speed_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_speed_unit, 250, 288);
    lv_obj_set_size(ui->dashboard_Lamborghini_speed_unit, 300, 18);

    //Write style for dashboard_Lamborghini_speed_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_speed_unit, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_speed_unit, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_speed_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_speed_unit, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_speed_unit, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_speed_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_power_value
    ui->dashboard_Lamborghini_power_value = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_power_value, "1.3");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_power_value, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_power_value, 284, 336);
    lv_obj_set_size(ui->dashboard_Lamborghini_power_value, 112, 18);

    //Write style for dashboard_Lamborghini_power_value, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_power_value, lv_color_hex(0x8B909A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_power_value, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_power_value, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_power_value, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_power_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_power_unit
    ui->dashboard_Lamborghini_power_unit = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_power_unit, "kW");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_power_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_power_unit, 400, 336);
    lv_obj_set_size(ui->dashboard_Lamborghini_power_unit, 60, 18);

    //Write style for dashboard_Lamborghini_power_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_power_unit, lv_color_hex(0x8B909A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_power_unit, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_power_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_power_unit, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_power_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_flow_regen_lbl
    ui->dashboard_Lamborghini_flow_regen_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_flow_regen_lbl, "REGEN");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_flow_regen_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_flow_regen_lbl, 190, 396);
    lv_obj_set_size(ui->dashboard_Lamborghini_flow_regen_lbl, 120, 14);

    //Write style for dashboard_Lamborghini_flow_regen_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_flow_regen_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_flow_regen_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_flow_regen_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_flow_regen_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_flow_regen_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_flow_regen_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_flow_power_lbl
    ui->dashboard_Lamborghini_flow_power_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_flow_power_lbl, "POWER");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_flow_power_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_flow_power_lbl, 490, 396);
    lv_obj_set_size(ui->dashboard_Lamborghini_flow_power_lbl, 120, 14);

    //Write style for dashboard_Lamborghini_flow_power_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_flow_power_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_flow_power_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_flow_power_lbl, lv_color_hex(0xF5A623), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_flow_power_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_flow_power_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_flow_power_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_flow_power_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_flow_power_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_flow_power_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_flow_power_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_flow_power_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_flow_power_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_flow_power_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_flow_power_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_power_bar
    ui->dashboard_Lamborghini_power_bar = lv_bar_create(ui->dashboard_Lamborghini);
    lv_obj_set_style_anim_time(ui->dashboard_Lamborghini_power_bar, 1000, 0);
    lv_bar_set_mode(ui->dashboard_Lamborghini_power_bar, LV_BAR_MODE_SYMMETRICAL);
    lv_bar_set_range(ui->dashboard_Lamborghini_power_bar, -100, 100);
    lv_bar_set_value(ui->dashboard_Lamborghini_power_bar, 26, LV_ANIM_OFF);
    lv_obj_set_pos(ui->dashboard_Lamborghini_power_bar, 190, 415);
    lv_obj_set_size(ui->dashboard_Lamborghini_power_bar, 420, 9);

    //Write style for dashboard_Lamborghini_power_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_power_bar, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_power_bar, lv_color_hex(0x15171C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_power_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_power_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_power_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Lamborghini_power_bar, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_power_bar, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_power_bar, lv_color_hex(0xF5A623), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_power_bar, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_power_bar, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_flow_center
    ui->dashboard_Lamborghini_flow_center = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_flow_center, 400, 413);
    lv_obj_set_size(ui->dashboard_Lamborghini_flow_center, 1, 13);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_flow_center, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_flow_center, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_flow_center, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_flow_center, lv_color_hex(0x4A4F58), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_flow_center, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_flow_center, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_bottom_bar
    ui->dashboard_Lamborghini_bottom_bar = lv_obj_create(ui->dashboard_Lamborghini);
    lv_obj_set_pos(ui->dashboard_Lamborghini_bottom_bar, 0, 436);
    lv_obj_set_size(ui->dashboard_Lamborghini_bottom_bar, 800, 44);
    lv_obj_set_scrollbar_mode(ui->dashboard_Lamborghini_bottom_bar, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Lamborghini_bottom_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_bottom_bar, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Lamborghini_bottom_bar, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Lamborghini_bottom_bar, lv_color_hex(0x1F2228), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Lamborghini_bottom_bar, LV_BORDER_SIDE_TOP, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_bottom_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_bottom_bar, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Lamborghini_bottom_bar, lv_color_hex(0x0A0B0D), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Lamborghini_bottom_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_bottom_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_bottom_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_bottom_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_bottom_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_bottom_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_led_tt_cruise_lbl
    ui->dashboard_Lamborghini_led_tt_cruise_lbl = lv_led_create(ui->dashboard_Lamborghini);
    lv_led_set_brightness(ui->dashboard_Lamborghini_led_tt_cruise_lbl, 255);
    lv_led_set_color(ui->dashboard_Lamborghini_led_tt_cruise_lbl, lv_color_hex(0x4ADE80));
    lv_obj_set_pos(ui->dashboard_Lamborghini_led_tt_cruise_lbl, 22, 451);
    lv_obj_set_size(ui->dashboard_Lamborghini_led_tt_cruise_lbl, 14, 14);

    //Write codes dashboard_Lamborghini_tt_cruise_lbl
    ui->dashboard_Lamborghini_tt_cruise_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_tt_cruise_lbl, "CRZ");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_tt_cruise_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_tt_cruise_lbl, 40, 452);
    lv_obj_set_size(ui->dashboard_Lamborghini_tt_cruise_lbl, 36, 13);

    //Write style for dashboard_Lamborghini_tt_cruise_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_tt_cruise_lbl, lv_color_hex(0x4ADE80), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_tt_cruise_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_tt_cruise_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_tt_cruise_lbl, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_tt_cruise_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_tt_cruise_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_led_tt_fault_lbl
    ui->dashboard_Lamborghini_led_tt_fault_lbl = lv_led_create(ui->dashboard_Lamborghini);
    lv_led_set_brightness(ui->dashboard_Lamborghini_led_tt_fault_lbl, 40);
    lv_led_set_color(ui->dashboard_Lamborghini_led_tt_fault_lbl, lv_color_hex(0xF5A623));
    lv_obj_set_pos(ui->dashboard_Lamborghini_led_tt_fault_lbl, 84, 451);
    lv_obj_set_size(ui->dashboard_Lamborghini_led_tt_fault_lbl, 14, 14);

    //Write codes dashboard_Lamborghini_tt_fault_lbl
    ui->dashboard_Lamborghini_tt_fault_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_tt_fault_lbl, "FLT");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_tt_fault_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_tt_fault_lbl, 102, 452);
    lv_obj_set_size(ui->dashboard_Lamborghini_tt_fault_lbl, 36, 13);

    //Write style for dashboard_Lamborghini_tt_fault_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_tt_fault_lbl, lv_color_hex(0x3A3F48), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_tt_fault_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_tt_fault_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_tt_fault_lbl, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_tt_fault_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_tt_fault_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_led_status_bt
    ui->dashboard_Lamborghini_led_status_bt = lv_led_create(ui->dashboard_Lamborghini);
    lv_led_set_brightness(ui->dashboard_Lamborghini_led_status_bt, 255);
    lv_led_set_color(ui->dashboard_Lamborghini_led_status_bt, lv_color_hex(0x60A5FA));
    lv_obj_set_pos(ui->dashboard_Lamborghini_led_status_bt, 146, 451);
    lv_obj_set_size(ui->dashboard_Lamborghini_led_status_bt, 14, 14);

    //Write codes dashboard_Lamborghini_status_bt
    ui->dashboard_Lamborghini_status_bt = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_status_bt, "BT");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_status_bt, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_status_bt, 164, 452);
    lv_obj_set_size(ui->dashboard_Lamborghini_status_bt, 36, 13);

    //Write style for dashboard_Lamborghini_status_bt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_status_bt, lv_color_hex(0x60A5FA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_status_bt, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_status_bt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_status_bt, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_status_bt, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_trip_lbl
    ui->dashboard_Lamborghini_trip_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_trip_lbl, "TRIP");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_trip_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_trip_lbl, 380, 444);
    lv_obj_set_size(ui->dashboard_Lamborghini_trip_lbl, 60, 13);

    //Write style for dashboard_Lamborghini_trip_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_trip_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_trip_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_trip_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_trip_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_trip_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_trip_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_TRIP_text
    ui->dashboard_Lamborghini_TRIP_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_TRIP_text, "4.2");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_TRIP_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_TRIP_text, 380, 457);
    lv_obj_set_size(ui->dashboard_Lamborghini_TRIP_text, 34, 18);

    //Write style for dashboard_Lamborghini_TRIP_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_TRIP_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_TRIP_text, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_TRIP_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_TRIP_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_trip_unit
    ui->dashboard_Lamborghini_trip_unit = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_trip_unit, "KM");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_trip_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_trip_unit, 416, 457);
    lv_obj_set_size(ui->dashboard_Lamborghini_trip_unit, 30, 18);

    //Write style for dashboard_Lamborghini_trip_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_trip_unit, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_trip_unit, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_trip_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_trip_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_odo_lbl
    ui->dashboard_Lamborghini_odo_lbl = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_odo_lbl, "ODO");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_odo_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_odo_lbl, 476, 444);
    lv_obj_set_size(ui->dashboard_Lamborghini_odo_lbl, 60, 13);

    //Write style for dashboard_Lamborghini_odo_lbl, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_odo_lbl, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_odo_lbl, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_odo_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_odo_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_odo_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_odo_lbl, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_odo_text
    ui->dashboard_Lamborghini_odo_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_odo_text, "00312");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_odo_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_odo_text, 476, 457);
    lv_obj_set_size(ui->dashboard_Lamborghini_odo_text, 48, 18);

    //Write style for dashboard_Lamborghini_odo_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_odo_text, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_odo_text, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_odo_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_odo_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_odo_unit
    ui->dashboard_Lamborghini_odo_unit = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_odo_unit, "KM");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_odo_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_odo_unit, 526, 457);
    lv_obj_set_size(ui->dashboard_Lamborghini_odo_unit, 30, 18);

    //Write style for dashboard_Lamborghini_odo_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_odo_unit, lv_color_hex(0x6A6F78), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_odo_unit, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_odo_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_odo_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_mode_text
    ui->dashboard_Lamborghini_mode_text = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_mode_text, "MODE 1");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_mode_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_mode_text, 578, 450);
    lv_obj_set_size(ui->dashboard_Lamborghini_mode_text, 100, 18);

    //Write style for dashboard_Lamborghini_mode_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_mode_text, lv_color_hex(0xF5A623), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_mode_text, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_mode_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_mode_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_mode_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Lamborghini_cur_time_label
    ui->dashboard_Lamborghini_cur_time_label = lv_label_create(ui->dashboard_Lamborghini);
    lv_label_set_text(ui->dashboard_Lamborghini_cur_time_label, "14:32");
    lv_label_set_long_mode(ui->dashboard_Lamborghini_cur_time_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Lamborghini_cur_time_label, 688, 447);
    lv_obj_set_size(ui->dashboard_Lamborghini_cur_time_label, 90, 22);

    //Write style for dashboard_Lamborghini_cur_time_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Lamborghini_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Lamborghini_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Lamborghini_cur_time_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Lamborghini_cur_time_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Lamborghini_cur_time_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Lamborghini_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Lamborghini_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Lamborghini_cur_time_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Lamborghini_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Lamborghini_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Lamborghini_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Lamborghini_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Lamborghini_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Lamborghini_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of dashboard_Lamborghini.


    //Update current screen layout.
    lv_obj_update_layout(ui->dashboard_Lamborghini);

    //Init events for screen.
    events_init_dashboard_Lamborghini(ui);
}
