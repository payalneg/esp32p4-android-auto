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



void setup_scr_dashboard_Classic_Max(lv_ui *ui)
{
    //Write codes dashboard_Classic_Max
    ui->dashboard_Classic_Max = lv_obj_create(NULL);
    lv_obj_set_size(ui->dashboard_Classic_Max, 480, 480);
    lv_obj_set_scrollbar_mode(ui->dashboard_Classic_Max, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Classic_Max, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max, lv_color_hex(0x07090A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_statusbar_sep
    ui->dashboard_Classic_Max_statusbar_sep = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_statusbar_sep, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_statusbar_sep, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_statusbar_sep, 0, 32);
    lv_obj_set_size(ui->dashboard_Classic_Max_statusbar_sep, 480, 1);

    //Write style for dashboard_Classic_Max_statusbar_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_statusbar_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_statusbar_sep, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_statusbar_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_statusbar_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_statusbar_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_statusbar_sep, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_statusbar_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_status_vesc
    ui->dashboard_Classic_Max_status_vesc = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_status_vesc, "VESC");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_status_vesc, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_status_vesc, 6, 5);
    lv_obj_set_size(ui->dashboard_Classic_Max_status_vesc, 55, 30);
    lv_obj_add_flag(ui->dashboard_Classic_Max_status_vesc, LV_OBJ_FLAG_CLICKABLE);

    //Write style for dashboard_Classic_Max_status_vesc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_status_vesc, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_status_vesc, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_status_vesc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_status_vesc, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_status_vesc, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_mode_text
    ui->dashboard_Classic_Max_mode_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_mode_text, "MODE ");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_mode_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_mode_text, 54, 5);
    lv_obj_set_size(ui->dashboard_Classic_Max_mode_text, 72, 30);

    //Write style for dashboard_Classic_Max_mode_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_mode_text, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_mode_text, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_mode_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_mode_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_uptime_text
    ui->dashboard_Classic_Max_uptime_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_uptime_text, "--:--:--");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_uptime_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_uptime_text, 123, 5);
    lv_obj_set_size(ui->dashboard_Classic_Max_uptime_text, 105, 30);

    //Write style for dashboard_Classic_Max_uptime_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_uptime_text, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_uptime_text, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_uptime_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_uptime_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_status_bt
    ui->dashboard_Classic_Max_status_bt = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_status_bt, "BT\n");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_status_bt, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_status_bt, 448, 5);
    lv_obj_set_size(ui->dashboard_Classic_Max_status_bt, 26, 30);

    //Write style for dashboard_Classic_Max_status_bt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_status_bt, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_status_bt, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_status_bt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_status_bt, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_battery_sep
    ui->dashboard_Classic_Max_battery_sep = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_battery_sep, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_battery_sep, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_battery_sep, 108, 32);
    lv_obj_set_size(ui->dashboard_Classic_Max_battery_sep, 1, 368);

    //Write style for dashboard_Classic_Max_battery_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_battery_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_battery_sep, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_battery_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_battery_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_battery_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_battery_sep, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_battery_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_battery_label
    ui->dashboard_Classic_Max_battery_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_battery_label, "BATTERY");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_battery_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_battery_label, 12, 40);
    lv_obj_set_size(ui->dashboard_Classic_Max_battery_label, 92, 35);

    //Write style for dashboard_Classic_Max_battery_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_battery_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_battery_label, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_battery_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_battery_label, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_battery_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_Battery_proc_text
    ui->dashboard_Classic_Max_Battery_proc_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_Battery_proc_text, "--");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_Battery_proc_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_Battery_proc_text, 12, 75);
    lv_obj_set_size(ui->dashboard_Classic_Max_Battery_proc_text, 70, 70);

    //Write style for dashboard_Classic_Max_Battery_proc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_Battery_proc_text, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_Battery_proc_text, &lv_font_Antonio_Regular_64, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_Battery_proc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_Battery_proc_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_battery_pct
    ui->dashboard_Classic_Max_battery_pct = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_battery_pct, "%");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_battery_pct, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_battery_pct, 53, 102);
    lv_obj_set_size(ui->dashboard_Classic_Max_battery_pct, 24, 24);

    //Write style for dashboard_Classic_Max_battery_pct, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_battery_pct, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_battery_pct, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_battery_pct, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_battery_pct, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_00
    ui->dashboard_Classic_Max_batt_seg_00 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_00, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_00, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_00, 12, 170);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_00, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_00, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_00, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_00, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_00, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_00, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_01
    ui->dashboard_Classic_Max_batt_seg_01 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_01, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_01, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_01, 12, 185);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_01, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_01, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_01, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_01, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_01, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_01, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_02
    ui->dashboard_Classic_Max_batt_seg_02 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_02, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_02, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_02, 12, 200);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_02, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_02, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_02, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_02, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_02, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_02, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_03
    ui->dashboard_Classic_Max_batt_seg_03 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_03, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_03, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_03, 12, 215);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_03, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_03, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_03, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_03, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_03, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_03, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_04
    ui->dashboard_Classic_Max_batt_seg_04 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_04, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_04, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_04, 12, 230);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_04, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_04, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_04, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_04, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_04, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_04, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_04, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_05
    ui->dashboard_Classic_Max_batt_seg_05 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_05, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_05, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_05, 12, 245);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_05, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_05, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_05, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_05, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_05, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_05, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_05, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_06
    ui->dashboard_Classic_Max_batt_seg_06 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_06, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_06, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_06, 12, 260);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_06, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_06, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_06, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_06, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_06, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_06, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_06, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_07
    ui->dashboard_Classic_Max_batt_seg_07 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_07, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_07, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_07, 12, 275);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_07, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_07, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_07, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_07, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_07, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_07, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_07, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_08
    ui->dashboard_Classic_Max_batt_seg_08 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_08, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_08, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_08, 12, 290);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_08, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_08, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_08, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_08, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_08, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_08, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_08, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_09
    ui->dashboard_Classic_Max_batt_seg_09 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_09, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_09, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_09, 12, 305);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_09, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_09, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_09, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_09, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_09, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_09, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_09, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_10
    ui->dashboard_Classic_Max_batt_seg_10 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_10, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_10, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_10, 12, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_10, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_10, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_10, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_10, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_10, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_10, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_11
    ui->dashboard_Classic_Max_batt_seg_11 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_11, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_11, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_11, 12, 335);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_11, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_11, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_11, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_11, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_11, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_12
    ui->dashboard_Classic_Max_batt_seg_12 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_12, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_12, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_12, 12, 350);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_12, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_12, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_12, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_12, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_12, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_12, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_12, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_batt_seg_13
    ui->dashboard_Classic_Max_batt_seg_13 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_batt_seg_13, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_batt_seg_13, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_batt_seg_13, 12, 365);
    lv_obj_set_size(ui->dashboard_Classic_Max_batt_seg_13, 86, 12);

    //Write style for dashboard_Classic_Max_batt_seg_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_batt_seg_13, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_batt_seg_13, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_batt_seg_13, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_batt_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_batt_seg_13, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_batt_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_batt_seg_13, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_batt_seg_13, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_battery_range_label
    ui->dashboard_Classic_Max_battery_range_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_battery_range_label, "RANGE");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_battery_range_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_battery_range_label, 6, 377);
    lv_obj_set_size(ui->dashboard_Classic_Max_battery_range_label, 60, 30);

    //Write style for dashboard_Classic_Max_battery_range_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_battery_range_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_battery_range_label, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_battery_range_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_battery_range_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_Range_text
    ui->dashboard_Classic_Max_Range_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_Range_text, "-- KM");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_Range_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_Range_text, 44, 377);
    lv_obj_set_size(ui->dashboard_Classic_Max_Range_text, 60, 30);

    //Write style for dashboard_Classic_Max_Range_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_Range_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_Range_text, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_Range_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_Range_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_label
    ui->dashboard_Classic_Max_speed_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_label, "SPEED · KM/H");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_label, 108, 42);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_label, 264, 14);

    //Write style for dashboard_Classic_Max_speed_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_label, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_Speed_text
    ui->dashboard_Classic_Max_Speed_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_Speed_text, "00");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_Speed_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_Speed_text, 108, 70);
    lv_obj_set_size(ui->dashboard_Classic_Max_Speed_text, 264, 224);

    //Write style for dashboard_Classic_Max_Speed_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_Speed_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_Speed_text, &lv_font_Antonio_Regular_200, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_Speed_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_Speed_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_00
    ui->dashboard_Classic_Max_speed_seg_00 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_00, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_00, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_00, 147, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_00, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_00, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_00, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_00, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_00, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_00, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_00, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_01
    ui->dashboard_Classic_Max_speed_seg_01 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_01, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_01, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_01, 163, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_01, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_01, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_01, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_01, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_01, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_01, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_01, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_02
    ui->dashboard_Classic_Max_speed_seg_02 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_02, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_02, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_02, 178, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_02, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_02, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_02, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_02, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_02, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_02, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_02, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_03
    ui->dashboard_Classic_Max_speed_seg_03 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_03, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_03, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_03, 194, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_03, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_03, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_03, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_03, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_03, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_03, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_03, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_04
    ui->dashboard_Classic_Max_speed_seg_04 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_04, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_04, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_04, 209, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_04, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_04, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_04, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_04, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_04, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_04, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_04, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_04, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_05
    ui->dashboard_Classic_Max_speed_seg_05 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_05, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_05, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_05, 225, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_05, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_05, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_05, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_05, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_05, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_05, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_05, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_05, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_06
    ui->dashboard_Classic_Max_speed_seg_06 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_06, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_06, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_06, 241, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_06, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_06, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_06, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_06, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_06, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_06, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_06, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_06, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_07
    ui->dashboard_Classic_Max_speed_seg_07 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_07, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_07, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_07, 256, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_07, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_07, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_07, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_07, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_07, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_07, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_07, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_07, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_08
    ui->dashboard_Classic_Max_speed_seg_08 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_08, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_08, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_08, 272, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_08, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_08, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_08, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_08, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_08, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_08, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_08, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_08, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_09
    ui->dashboard_Classic_Max_speed_seg_09 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_09, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_09, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_09, 287, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_09, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_09, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_09, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_09, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_09, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_09, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_09, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_09, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_10
    ui->dashboard_Classic_Max_speed_seg_10 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_10, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_10, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_10, 303, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_10, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_10, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_10, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_10, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_10, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_10, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_10, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_seg_11
    ui->dashboard_Classic_Max_speed_seg_11 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_seg_11, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_seg_11, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_seg_11, 319, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_seg_11, 14, 6);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_seg_11, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_seg_11, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_seg_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_seg_11, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_seg_11, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_speed_seg_11, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_speed_seg_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_min
    ui->dashboard_Classic_Max_speed_min = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_min, "0");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_min, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_min, 144, 332);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_min, 12, 20);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_min, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_min, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_min, lv_color_hex(0x4A5358), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_min, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_min, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_min, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_speed_max
    ui->dashboard_Classic_Max_speed_max = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_speed_max, "MAX 60");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_speed_max, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_speed_max, 277, 332);
    lv_obj_set_size(ui->dashboard_Classic_Max_speed_max, 66, 20);
    lv_obj_add_flag(ui->dashboard_Classic_Max_speed_max, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_speed_max, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_speed_max, lv_color_hex(0x4A5358), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_speed_max, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_speed_max, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_speed_max, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_sep
    ui->dashboard_Classic_Max_power_sep = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_sep, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_sep, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_sep, 372, 32);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_sep, 1, 368);

    //Write style for dashboard_Classic_Max_power_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_sep, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_sep, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_label
    ui->dashboard_Classic_Max_power_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_label, "POWER");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_label, 382, 40);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_label, 86, 29);

    //Write style for dashboard_Classic_Max_power_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_label, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_label, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_value
    ui->dashboard_Classic_Max_power_value = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_value, "-.-");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_value, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_value, 328, 75);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_value, 105, 70);

    //Write style for dashboard_Classic_Max_power_value, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_value, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_value, &lv_font_Antonio_Regular_64, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_value, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_unit
    ui->dashboard_Classic_Max_power_unit = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_unit, "kW");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_unit, 433, 102);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_unit, 36, 24);

    //Write style for dashboard_Classic_Max_power_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_unit, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_Current_text
    ui->dashboard_Classic_Max_Current_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_Current_text, "--.- A");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_Current_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_Current_text, 382, 138);
    lv_obj_set_size(ui->dashboard_Classic_Max_Current_text, 86, 30);

    //Write style for dashboard_Classic_Max_Current_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_Current_text, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_Current_text, &lv_font_montserratMedium_26, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_Current_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_Current_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_00
    ui->dashboard_Classic_Max_power_seg_00 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_00, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_00, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_00, 382, 170);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_00, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_00, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_00, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_00, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_00, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_00, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_01
    ui->dashboard_Classic_Max_power_seg_01 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_01, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_01, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_01, 382, 185);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_01, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_01, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_01, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_01, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_01, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_01, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_02
    ui->dashboard_Classic_Max_power_seg_02 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_02, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_02, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_02, 382, 200);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_02, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_02, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_02, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_02, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_02, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_02, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_03
    ui->dashboard_Classic_Max_power_seg_03 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_03, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_03, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_03, 382, 215);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_03, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_03, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_03, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_03, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_03, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_03, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_04
    ui->dashboard_Classic_Max_power_seg_04 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_04, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_04, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_04, 382, 230);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_04, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_04, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_04, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_04, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_04, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_04, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_04, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_05
    ui->dashboard_Classic_Max_power_seg_05 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_05, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_05, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_05, 382, 245);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_05, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_05, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_05, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_05, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_05, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_05, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_05, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_06
    ui->dashboard_Classic_Max_power_seg_06 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_06, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_06, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_06, 382, 260);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_06, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_06, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_06, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_06, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_06, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_06, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_06, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_07
    ui->dashboard_Classic_Max_power_seg_07 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_07, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_07, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_07, 382, 275);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_07, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_07, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_07, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_07, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_07, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_07, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_07, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_08
    ui->dashboard_Classic_Max_power_seg_08 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_08, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_08, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_08, 382, 290);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_08, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_08, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_08, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_08, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_08, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_08, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_08, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_09
    ui->dashboard_Classic_Max_power_seg_09 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_09, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_09, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_09, 382, 305);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_09, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_09, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_09, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_09, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_09, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_09, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_09, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_10
    ui->dashboard_Classic_Max_power_seg_10 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_10, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_10, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_10, 382, 320);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_10, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_10, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_10, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_10, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_10, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_10, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_11
    ui->dashboard_Classic_Max_power_seg_11 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_11, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_11, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_11, 382, 335);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_11, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_11, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_11, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_11, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_11, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_12
    ui->dashboard_Classic_Max_power_seg_12 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_12, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_12, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_12, 382, 350);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_12, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_12, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_12, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_12, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_12, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_12, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_12, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_seg_13
    ui->dashboard_Classic_Max_power_seg_13 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_seg_13, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_seg_13, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_seg_13, 382, 365);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_seg_13, 86, 12);

    //Write style for dashboard_Classic_Max_power_seg_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_seg_13, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_seg_13, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_seg_13, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_seg_13, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_power_seg_13, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_power_seg_13, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_max_label
    ui->dashboard_Classic_Max_power_max_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_max_label, "MAX");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_max_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_max_label, 376, 377);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_max_label, 36, 30);

    //Write style for dashboard_Classic_Max_power_max_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_max_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_max_label, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_max_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_max_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_power_max_val
    ui->dashboard_Classic_Max_power_max_val = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_power_max_val, "-.- KW");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_power_max_val, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_power_max_val, 402, 377);
    lv_obj_set_size(ui->dashboard_Classic_Max_power_max_val, 72, 30);

    //Write style for dashboard_Classic_Max_power_max_val, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_power_max_val, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_power_max_val, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_power_max_val, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_power_max_val, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_bottom_bg
    ui->dashboard_Classic_Max_bottom_bg = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_bottom_bg, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_bottom_bg, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_bottom_bg, 0, 400);
    lv_obj_set_size(ui->dashboard_Classic_Max_bottom_bg, 480, 80);

    //Write style for dashboard_Classic_Max_bottom_bg, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_bottom_bg, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_bottom_bg, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_bottom_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_bottom_bg, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_bottom_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_bottom_bg, lv_color_hex(0x0D1113), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_bottom_bg, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_bottom_top_sep
    ui->dashboard_Classic_Max_bottom_top_sep = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_bottom_top_sep, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_bottom_top_sep, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_bottom_top_sep, 0, 400);
    lv_obj_set_size(ui->dashboard_Classic_Max_bottom_top_sep, 480, 1);

    //Write style for dashboard_Classic_Max_bottom_top_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_bottom_top_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_bottom_top_sep, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_bottom_top_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_bottom_top_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_bottom_top_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_bottom_top_sep, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_bottom_top_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_bottom_col_sep_0
    ui->dashboard_Classic_Max_bottom_col_sep_0 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_bottom_col_sep_0, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_bottom_col_sep_0, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_bottom_col_sep_0, 96, 400);
    lv_obj_set_size(ui->dashboard_Classic_Max_bottom_col_sep_0, 1, 80);

    //Write style for dashboard_Classic_Max_bottom_col_sep_0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_bottom_col_sep_0, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_bottom_col_sep_0, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_bottom_col_sep_0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_bottom_col_sep_0, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_bottom_col_sep_0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_bottom_col_sep_0, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_bottom_col_sep_0, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_bottom_col_sep_1
    ui->dashboard_Classic_Max_bottom_col_sep_1 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_bottom_col_sep_1, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_bottom_col_sep_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_bottom_col_sep_1, 192, 400);
    lv_obj_set_size(ui->dashboard_Classic_Max_bottom_col_sep_1, 1, 80);

    //Write style for dashboard_Classic_Max_bottom_col_sep_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_bottom_col_sep_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_bottom_col_sep_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_bottom_col_sep_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_bottom_col_sep_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_bottom_col_sep_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_bottom_col_sep_1, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_bottom_col_sep_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_bottom_col_sep_2
    ui->dashboard_Classic_Max_bottom_col_sep_2 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_bottom_col_sep_2, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_bottom_col_sep_2, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_bottom_col_sep_2, 288, 400);
    lv_obj_set_size(ui->dashboard_Classic_Max_bottom_col_sep_2, 1, 80);

    //Write style for dashboard_Classic_Max_bottom_col_sep_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_bottom_col_sep_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_bottom_col_sep_2, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_bottom_col_sep_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_bottom_col_sep_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_bottom_col_sep_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_bottom_col_sep_2, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_bottom_col_sep_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_bottom_col_sep_3
    ui->dashboard_Classic_Max_bottom_col_sep_3 = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_bottom_col_sep_3, "");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_bottom_col_sep_3, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_bottom_col_sep_3, 384, 400);
    lv_obj_set_size(ui->dashboard_Classic_Max_bottom_col_sep_3, 1, 80);

    //Write style for dashboard_Classic_Max_bottom_col_sep_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_bottom_col_sep_3, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_bottom_col_sep_3, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_bottom_col_sep_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_bottom_col_sep_3, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_bottom_col_sep_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_bottom_col_sep_3, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_bottom_col_sep_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_trip_label
    ui->dashboard_Classic_Max_col_trip_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_trip_label, "TRIP");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_trip_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_trip_label, 96, 405);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_trip_label, 96, 18);

    //Write style for dashboard_Classic_Max_col_trip_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_trip_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_trip_label, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_trip_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_trip_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_trip_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_TRIP_text
    ui->dashboard_Classic_Max_TRIP_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_TRIP_text, "--.-");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_TRIP_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_TRIP_text, 67, 432);
    lv_obj_set_size(ui->dashboard_Classic_Max_TRIP_text, 88, 60);

    //Write style for dashboard_Classic_Max_TRIP_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_TRIP_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_TRIP_text, &lv_font_Antonio_Regular_40, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_TRIP_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_TRIP_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_trip_unit
    ui->dashboard_Classic_Max_col_trip_unit = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_trip_unit, "KM");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_trip_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_trip_unit, 157, 442);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_trip_unit, 34, 14);

    //Write style for dashboard_Classic_Max_col_trip_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_trip_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_trip_unit, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_trip_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_trip_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_odo_label
    ui->dashboard_Classic_Max_col_odo_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_odo_label, "ODO");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_odo_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_odo_label, 193, 405);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_odo_label, 96, 18);

    //Write style for dashboard_Classic_Max_col_odo_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_odo_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_odo_label, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_odo_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_odo_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_odo_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_odo_text
    ui->dashboard_Classic_Max_odo_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_odo_text, "----");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_odo_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_odo_text, 165, 432);
    lv_obj_set_size(ui->dashboard_Classic_Max_odo_text, 88, 60);

    //Write style for dashboard_Classic_Max_odo_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_odo_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_odo_text, &lv_font_Antonio_Regular_40, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_odo_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_odo_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_odo_unit
    ui->dashboard_Classic_Max_col_odo_unit = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_odo_unit, "KM");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_odo_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_odo_unit, 253, 442);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_odo_unit, 34, 14);

    //Write style for dashboard_Classic_Max_col_odo_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_odo_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_odo_unit, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_odo_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_odo_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_mtmp_label
    ui->dashboard_Classic_Max_col_mtmp_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_mtmp_label, "M·TEMP");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_mtmp_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_mtmp_label, 287, 405);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_mtmp_label, 96, 18);

    //Write style for dashboard_Classic_Max_col_mtmp_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_mtmp_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_mtmp_label, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_mtmp_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_mtmp_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_mtmp_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_temp_mot_text
    ui->dashboard_Classic_Max_temp_mot_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_temp_mot_text, "--");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_temp_mot_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_temp_mot_text, 287, 432);
    lv_obj_set_size(ui->dashboard_Classic_Max_temp_mot_text, 60, 60);

    //Write style for dashboard_Classic_Max_temp_mot_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_temp_mot_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_temp_mot_text, &lv_font_Antonio_Regular_40, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_temp_mot_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_temp_mot_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_mtmp_unit
    ui->dashboard_Classic_Max_col_mtmp_unit = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_mtmp_unit, "°C");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_mtmp_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_mtmp_unit, 349, 442);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_mtmp_unit, 34, 14);

    //Write style for dashboard_Classic_Max_col_mtmp_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_mtmp_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_mtmp_unit, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_mtmp_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_mtmp_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_ctmp_label
    ui->dashboard_Classic_Max_col_ctmp_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_ctmp_label, "C·TEMP");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_ctmp_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_ctmp_label, 379, 405);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_ctmp_label, 96, 16);

    //Write style for dashboard_Classic_Max_col_ctmp_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_ctmp_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_ctmp_label, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_ctmp_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_ctmp_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_ctmp_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_temp_esc_text
    ui->dashboard_Classic_Max_temp_esc_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_temp_esc_text, "--");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_temp_esc_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_temp_esc_text, 382, 432);
    lv_obj_set_size(ui->dashboard_Classic_Max_temp_esc_text, 60, 60);

    //Write style for dashboard_Classic_Max_temp_esc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_temp_esc_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_temp_esc_text, &lv_font_Antonio_Regular_40, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_temp_esc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_temp_esc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_ctmp_unit
    ui->dashboard_Classic_Max_col_ctmp_unit = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_ctmp_unit, "°C");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_ctmp_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_ctmp_unit, 445, 442);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_ctmp_unit, 34, 14);

    //Write style for dashboard_Classic_Max_col_ctmp_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_ctmp_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_ctmp_unit, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_ctmp_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_ctmp_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_avg_label
    ui->dashboard_Classic_Max_col_avg_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_avg_label, "AVG");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_avg_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_avg_label, 384, 414);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_avg_label, 96, 12);
    lv_obj_add_flag(ui->dashboard_Classic_Max_col_avg_label, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_col_avg_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_avg_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_avg_label, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_avg_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_avg_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_avg_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_avg_value
    ui->dashboard_Classic_Max_col_avg_value = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_avg_value, "24");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_avg_value, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_avg_value, 384, 432);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_avg_value, 60, 60);
    lv_obj_add_flag(ui->dashboard_Classic_Max_col_avg_value, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_col_avg_value, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_avg_value, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_avg_value, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_avg_value, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_avg_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_avg_unit
    ui->dashboard_Classic_Max_col_avg_unit = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_avg_unit, "KM/H");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_avg_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_avg_unit, 446, 442);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_avg_unit, 34, 14);
    lv_obj_add_flag(ui->dashboard_Classic_Max_col_avg_unit, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_col_avg_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_avg_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_avg_unit, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_avg_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_avg_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_cruise_control_img
    ui->dashboard_Classic_Max_cruise_control_img = lv_img_create(ui->dashboard_Classic_Max);
    lv_obj_add_flag(ui->dashboard_Classic_Max_cruise_control_img, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->dashboard_Classic_Max_cruise_control_img, &_cruise_control_alpha_38x38);
    lv_img_set_pivot(ui->dashboard_Classic_Max_cruise_control_img, 0,0);
    lv_img_set_angle(ui->dashboard_Classic_Max_cruise_control_img, 0);
    lv_obj_set_pos(ui->dashboard_Classic_Max_cruise_control_img, 331, 45);
    lv_obj_set_size(ui->dashboard_Classic_Max_cruise_control_img, 38, 38);
    lv_obj_add_flag(ui->dashboard_Classic_Max_cruise_control_img, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_cruise_control_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_Classic_Max_cruise_control_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_Classic_Max_cruise_control_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_cruise_control_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_Classic_Max_cruise_control_img, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_esc_not_connected_text
    ui->dashboard_Classic_Max_esc_not_connected_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_esc_not_connected_text, "ESC NOT CONNECTED");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_esc_not_connected_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_esc_not_connected_text, 138, 5);
    lv_obj_set_size(ui->dashboard_Classic_Max_esc_not_connected_text, 205, 32);
    lv_obj_add_flag(ui->dashboard_Classic_Max_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_esc_not_connected_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_esc_not_connected_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_esc_not_connected_text, lv_color_hex(0xe70023), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_esc_not_connected_text, &lv_font_Antonio_Regular_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_esc_not_connected_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_esc_not_connected_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_esc_not_connected_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_esc_not_connected_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_Ah_text
    ui->dashboard_Classic_Max_Ah_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_Ah_text, "--.- Ah");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_Ah_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_Ah_text, 12, 138);
    lv_obj_set_size(ui->dashboard_Classic_Max_Ah_text, 96, 27);

    //Write style for dashboard_Classic_Max_Ah_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_Ah_text, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_Ah_text, &lv_font_montserratMedium_25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_Ah_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_Ah_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_Speed_cc_text
    ui->dashboard_Classic_Max_Speed_cc_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_Speed_cc_text, "--");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_Speed_cc_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_Speed_cc_text, 311, 90);
    lv_obj_set_size(ui->dashboard_Classic_Max_Speed_cc_text, 55, 70);
    lv_obj_add_flag(ui->dashboard_Classic_Max_Speed_cc_text, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Classic_Max_Speed_cc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_Speed_cc_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_Speed_cc_text, &lv_font_Antonio_Regular_50, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_Speed_cc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_Speed_cc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_Settings_text
    ui->dashboard_Classic_Max_Settings_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_Settings_text, "SETTINGS");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_Settings_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_Settings_text, 366, 5);
    lv_obj_set_size(ui->dashboard_Classic_Max_Settings_text, 105, 29);
    lv_obj_add_flag(ui->dashboard_Classic_Max_Settings_text, LV_OBJ_FLAG_CLICKABLE);

    //Write style for dashboard_Classic_Max_Settings_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_Settings_text, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_Settings_text, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_Settings_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_Settings_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_Settings_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_v_label
    ui->dashboard_Classic_Max_col_v_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_v_label, "V");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_v_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_v_label, 62, 442);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_v_label, 34, 14);

    //Write style for dashboard_Classic_Max_col_v_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_v_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_v_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_v_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_v_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_Voltage_text
    ui->dashboard_Classic_Max_Voltage_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_Voltage_text, "--.-");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_Voltage_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_Voltage_text, 0, 432);
    lv_obj_set_size(ui->dashboard_Classic_Max_Voltage_text, 88, 60);

    //Write style for dashboard_Classic_Max_Voltage_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_Voltage_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_Voltage_text, &lv_font_Antonio_Regular_40, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_Voltage_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_Voltage_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_col_voltage_label
    ui->dashboard_Classic_Max_col_voltage_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_col_voltage_label, "VOLTAGE\n");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_col_voltage_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_col_voltage_label, 0, 405);
    lv_obj_set_size(ui->dashboard_Classic_Max_col_voltage_label, 96, 18);

    //Write style for dashboard_Classic_Max_col_voltage_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_col_voltage_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_col_voltage_label, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_col_voltage_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_col_voltage_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_col_voltage_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_cur_time_label
    ui->dashboard_Classic_Max_cur_time_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_cur_time_label, "--:--");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_cur_time_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_cur_time_label, 303, 5);
    lv_obj_set_size(ui->dashboard_Classic_Max_cur_time_label, 66, 30);

    //Write style for dashboard_Classic_Max_cur_time_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_cur_time_label, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_cur_time_label, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_cur_time_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_cur_time_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_brightness_slider
    ui->dashboard_Classic_Max_brightness_slider = lv_slider_create(ui->dashboard_Classic_Max);
    lv_slider_set_range(ui->dashboard_Classic_Max_brightness_slider, 0, 100);
    lv_slider_set_mode(ui->dashboard_Classic_Max_brightness_slider, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->dashboard_Classic_Max_brightness_slider, 50, LV_ANIM_OFF);
    lv_obj_set_pos(ui->dashboard_Classic_Max_brightness_slider, 310, 40);
    lv_obj_set_size(ui->dashboard_Classic_Max_brightness_slider, 170, 350);

    //Write style for dashboard_Classic_Max_brightness_slider, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_brightness_slider, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_brightness_slider, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->dashboard_Classic_Max_brightness_slider, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_brightness_slider, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Classic_Max_brightness_slider, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_brightness_slider, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_brightness_slider, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for dashboard_Classic_Max_brightness_slider, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_brightness_slider, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_brightness_slider, 13, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_statistics_button
    ui->dashboard_Classic_Max_statistics_button = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_statistics_button, "STATISTICS");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_statistics_button, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_statistics_button, 192, 5);
    lv_obj_set_size(ui->dashboard_Classic_Max_statistics_button, 132, 30);
    lv_obj_add_flag(ui->dashboard_Classic_Max_statistics_button, LV_OBJ_FLAG_CLICKABLE);

    //Write style for dashboard_Classic_Max_statistics_button, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_statistics_button, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_statistics_button, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_statistics_button, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_statistics_button, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_statistics_button, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_max_power_label
    ui->dashboard_Classic_Max_max_power_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_max_power_label, "P MAX");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_max_power_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_max_power_label, 110, 381);
    lv_obj_set_size(ui->dashboard_Classic_Max_max_power_label, 49, 22);

    //Write style for dashboard_Classic_Max_max_power_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_max_power_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_max_power_label, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_max_power_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_max_power_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_max_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_max_power_text
    ui->dashboard_Classic_Max_max_power_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_max_power_text, "-.- KW");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_max_power_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_max_power_text, 148, 377);
    lv_obj_set_size(ui->dashboard_Classic_Max_max_power_text, 72, 30);

    //Write style for dashboard_Classic_Max_max_power_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_max_power_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_max_power_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_max_power_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_max_power_text, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_max_power_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_max_power_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_max_power_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_max_power_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_max_power_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_max_power_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_max_power_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_max_power_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_max_power_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_max_power_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_max_speed_label
    ui->dashboard_Classic_Max_max_speed_label = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_max_speed_label, "V MAX");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_max_speed_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_max_speed_label, 211, 381);
    lv_obj_set_size(ui->dashboard_Classic_Max_max_speed_label, 49, 22);

    //Write style for dashboard_Classic_Max_max_speed_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_max_speed_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_max_speed_label, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_max_speed_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_max_speed_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_max_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_max_speed_text
    ui->dashboard_Classic_Max_max_speed_text = lv_label_create(ui->dashboard_Classic_Max);
    lv_label_set_text(ui->dashboard_Classic_Max_max_speed_text, "-- KM/H");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_max_speed_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Classic_Max_max_speed_text, 249, 377);
    lv_obj_set_size(ui->dashboard_Classic_Max_max_speed_text, 84, 30);

    //Write style for dashboard_Classic_Max_max_speed_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_max_speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_max_speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_max_speed_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_max_speed_text, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_max_speed_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Classic_Max_max_speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Classic_Max_max_speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_max_speed_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_max_speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Classic_Max_max_speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Classic_Max_max_speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Classic_Max_max_speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Classic_Max_max_speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_max_speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Classic_Max_max_reset_btn
    ui->dashboard_Classic_Max_max_reset_btn = lv_btn_create(ui->dashboard_Classic_Max);
    ui->dashboard_Classic_Max_max_reset_btn_label = lv_label_create(ui->dashboard_Classic_Max_max_reset_btn);
    lv_label_set_text(ui->dashboard_Classic_Max_max_reset_btn_label, "RESET");
    lv_label_set_long_mode(ui->dashboard_Classic_Max_max_reset_btn_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->dashboard_Classic_Max_max_reset_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->dashboard_Classic_Max_max_reset_btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->dashboard_Classic_Max_max_reset_btn_label, LV_PCT(100));
    lv_obj_set_pos(ui->dashboard_Classic_Max_max_reset_btn, 316, 372);
    lv_obj_set_size(ui->dashboard_Classic_Max_max_reset_btn, 54, 28);

    //Write style for dashboard_Classic_Max_max_reset_btn, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Classic_Max_max_reset_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Classic_Max_max_reset_btn, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Classic_Max_max_reset_btn, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Classic_Max_max_reset_btn, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->dashboard_Classic_Max_max_reset_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->dashboard_Classic_Max_max_reset_btn, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->dashboard_Classic_Max_max_reset_btn, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Classic_Max_max_reset_btn, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Classic_Max_max_reset_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Classic_Max_max_reset_btn, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Classic_Max_max_reset_btn, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Classic_Max_max_reset_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Classic_Max_max_reset_btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of dashboard_Classic_Max.


    //Update current screen layout.
    lv_obj_update_layout(ui->dashboard_Classic_Max);

    //Init events for screen.
    events_init_dashboard_Classic_Max(ui);
}
