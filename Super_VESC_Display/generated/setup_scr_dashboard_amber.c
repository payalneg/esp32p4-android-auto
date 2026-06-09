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



void setup_scr_dashboard_Amber(lv_ui *ui)
{
    //Write codes dashboard_Amber
    ui->dashboard_Amber = lv_obj_create(NULL);
    lv_obj_set_size(ui->dashboard_Amber, 800, 480);
    lv_obj_set_scrollbar_mode(ui->dashboard_Amber, LV_SCROLLBAR_MODE_OFF);

    //Write style for dashboard_Amber, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Amber, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber, lv_color_hex(0x0A0806), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_statusbar_sep
    ui->dashboard_Amber_statusbar_sep = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_statusbar_sep, "");
    lv_label_set_long_mode(ui->dashboard_Amber_statusbar_sep, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_statusbar_sep, 0, 32);
    lv_obj_set_size(ui->dashboard_Amber_statusbar_sep, 800, 1);

    //Write style for dashboard_Amber_statusbar_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_statusbar_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_statusbar_sep, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_statusbar_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_statusbar_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_statusbar_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_statusbar_sep, lv_color_hex(0x2A1A0C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_statusbar_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_status_vesc
    ui->dashboard_Amber_status_vesc = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_status_vesc, "VESC");
    lv_label_set_long_mode(ui->dashboard_Amber_status_vesc, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_status_vesc, 10, 5);
    lv_obj_set_size(ui->dashboard_Amber_status_vesc, 92, 30);
    lv_obj_add_flag(ui->dashboard_Amber_status_vesc, LV_OBJ_FLAG_CLICKABLE);

    //Write style for dashboard_Amber_status_vesc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_status_vesc, lv_color_hex(0xFFB35C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_status_vesc, &lv_font_DSEG14Classic_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_status_vesc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_status_vesc, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_status_vesc, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_mode_text
    ui->dashboard_Amber_mode_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_mode_text, "MODE ");
    lv_label_set_long_mode(ui->dashboard_Amber_mode_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_mode_text, 88, 5);
    lv_obj_set_size(ui->dashboard_Amber_mode_text, 120, 30);

    //Write style for dashboard_Amber_mode_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_mode_text, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_mode_text, &lv_font_DSEG14Classic_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_mode_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_mode_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_uptime_text
    ui->dashboard_Amber_uptime_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_uptime_text, "0:48:12");
    lv_label_set_long_mode(ui->dashboard_Amber_uptime_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_uptime_text, 195, 5);
    lv_obj_set_size(ui->dashboard_Amber_uptime_text, 110, 30);

    //Write style for dashboard_Amber_uptime_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_uptime_text, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_uptime_text, &lv_font_DSEG14Classic_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_uptime_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_uptime_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_status_bt
    ui->dashboard_Amber_status_bt = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_status_bt, "BT\n");
    lv_label_set_long_mode(ui->dashboard_Amber_status_bt, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_status_bt, 750, 5);
    lv_obj_set_size(ui->dashboard_Amber_status_bt, 40, 30);

    //Write style for dashboard_Amber_status_bt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_status_bt, lv_color_hex(0xFFB35C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_status_bt, &lv_font_DSEG14Classic_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_status_bt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_status_bt, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_battery_sep
    ui->dashboard_Amber_battery_sep = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_battery_sep, "");
    lv_label_set_long_mode(ui->dashboard_Amber_battery_sep, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_battery_sep, 180, 32);
    lv_obj_set_size(ui->dashboard_Amber_battery_sep, 1, 378);

    //Write style for dashboard_Amber_battery_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_battery_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_battery_sep, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_battery_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_battery_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_battery_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_battery_sep, lv_color_hex(0x2A1A0C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_battery_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_battery_label
    ui->dashboard_Amber_battery_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_battery_label, "BATTERY");
    lv_label_set_long_mode(ui->dashboard_Amber_battery_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_battery_label, 20, 40);
    lv_obj_set_size(ui->dashboard_Amber_battery_label, 160, 35);

    //Write style for dashboard_Amber_battery_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_battery_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_battery_label, &lv_font_DSEG14Classic_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_battery_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_battery_label, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_battery_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_Battery_proc_text
    ui->dashboard_Amber_Battery_proc_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_Battery_proc_text, "73");
    lv_label_set_long_mode(ui->dashboard_Amber_Battery_proc_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_Battery_proc_text, 20, 82);
    lv_obj_set_size(ui->dashboard_Amber_Battery_proc_text, 116, 70);

    //Write style for dashboard_Amber_Battery_proc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_Battery_proc_text, lv_color_hex(0xFFB35C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_Battery_proc_text, &lv_font_DSEG14Classic_Regular_50, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_Battery_proc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_Battery_proc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_battery_pct
    ui->dashboard_Amber_battery_pct = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_battery_pct, "%");
    lv_label_set_long_mode(ui->dashboard_Amber_battery_pct, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_battery_pct, 140, 90);
    lv_obj_set_size(ui->dashboard_Amber_battery_pct, 40, 25);

    //Write style for dashboard_Amber_battery_pct, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_battery_pct, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_battery_pct, &lv_font_DSEG14Classic_Regular_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_battery_pct, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_battery_pct, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_00
    ui->dashboard_Amber_batt_seg_00 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_00, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_00, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_00, 20, 170);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_00, 144, 12);

    //Write style for dashboard_Amber_batt_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_00, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_00, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_00, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_00, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_00, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_01
    ui->dashboard_Amber_batt_seg_01 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_01, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_01, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_01, 20, 185);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_01, 144, 12);

    //Write style for dashboard_Amber_batt_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_01, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_01, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_01, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_01, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_01, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_02
    ui->dashboard_Amber_batt_seg_02 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_02, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_02, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_02, 20, 200);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_02, 144, 12);

    //Write style for dashboard_Amber_batt_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_02, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_02, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_02, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_02, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_02, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_03
    ui->dashboard_Amber_batt_seg_03 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_03, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_03, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_03, 20, 215);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_03, 144, 12);

    //Write style for dashboard_Amber_batt_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_03, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_03, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_03, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_03, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_03, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_04
    ui->dashboard_Amber_batt_seg_04 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_04, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_04, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_04, 20, 230);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_04, 144, 12);

    //Write style for dashboard_Amber_batt_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_04, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_04, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_04, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_04, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_04, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_04, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_batt_seg_04, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_batt_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_05
    ui->dashboard_Amber_batt_seg_05 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_05, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_05, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_05, 20, 245);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_05, 144, 12);

    //Write style for dashboard_Amber_batt_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_05, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_05, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_05, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_05, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_05, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_05, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_batt_seg_05, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_batt_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_06
    ui->dashboard_Amber_batt_seg_06 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_06, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_06, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_06, 20, 260);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_06, 144, 12);

    //Write style for dashboard_Amber_batt_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_06, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_06, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_06, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_06, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_06, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_06, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_batt_seg_06, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_batt_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_07
    ui->dashboard_Amber_batt_seg_07 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_07, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_07, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_07, 20, 275);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_07, 144, 12);

    //Write style for dashboard_Amber_batt_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_07, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_07, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_07, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_07, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_07, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_07, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_batt_seg_07, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_batt_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_08
    ui->dashboard_Amber_batt_seg_08 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_08, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_08, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_08, 20, 290);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_08, 144, 12);

    //Write style for dashboard_Amber_batt_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_08, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_08, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_08, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_08, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_08, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_08, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_batt_seg_08, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_batt_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_09
    ui->dashboard_Amber_batt_seg_09 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_09, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_09, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_09, 20, 305);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_09, 144, 12);

    //Write style for dashboard_Amber_batt_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_09, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_09, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_09, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_09, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_09, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_09, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_batt_seg_09, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_batt_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_10
    ui->dashboard_Amber_batt_seg_10 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_10, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_10, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_10, 20, 320);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_10, 144, 12);

    //Write style for dashboard_Amber_batt_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_10, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_10, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_10, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_10, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_10, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_batt_seg_10, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_batt_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_11
    ui->dashboard_Amber_batt_seg_11 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_11, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_11, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_11, 20, 335);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_11, 144, 12);

    //Write style for dashboard_Amber_batt_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_11, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_11, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_11, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_11, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_11, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_batt_seg_11, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_batt_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_12
    ui->dashboard_Amber_batt_seg_12 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_12, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_12, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_12, 20, 350);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_12, 144, 12);

    //Write style for dashboard_Amber_batt_seg_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_12, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_12, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_12, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_12, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_12, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_12, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_batt_seg_12, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_batt_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_batt_seg_13
    ui->dashboard_Amber_batt_seg_13 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_batt_seg_13, "");
    lv_label_set_long_mode(ui->dashboard_Amber_batt_seg_13, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_batt_seg_13, 20, 365);
    lv_obj_set_size(ui->dashboard_Amber_batt_seg_13, 144, 12);

    //Write style for dashboard_Amber_batt_seg_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_batt_seg_13, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_batt_seg_13, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_batt_seg_13, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_batt_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_batt_seg_13, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_batt_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_batt_seg_13, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_batt_seg_13, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_batt_seg_13, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_batt_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_battery_range_label
    ui->dashboard_Amber_battery_range_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_battery_range_label, "RANGE");
    lv_label_set_long_mode(ui->dashboard_Amber_battery_range_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_battery_range_label, 10, 385);
    lv_obj_set_size(ui->dashboard_Amber_battery_range_label, 84, 30);

    //Write style for dashboard_Amber_battery_range_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_battery_range_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_battery_range_label, &lv_font_DSEG14Classic_Regular_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_battery_range_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_battery_range_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_Range_text
    ui->dashboard_Amber_Range_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_Range_text, "38 KM");
    lv_label_set_long_mode(ui->dashboard_Amber_Range_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_Range_text, 88, 385);
    lv_obj_set_size(ui->dashboard_Amber_Range_text, 84, 30);

    //Write style for dashboard_Amber_Range_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_Range_text, lv_color_hex(0xFFE6C2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_Range_text, &lv_font_DSEG14Classic_Regular_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_Range_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_Range_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_label
    ui->dashboard_Amber_speed_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_label, "SPEED - KM/H");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_label, 180, 42);
    lv_obj_set_size(ui->dashboard_Amber_speed_label, 440, 14);

    //Write style for dashboard_Amber_speed_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_label, &lv_font_DSEG14Classic_Regular_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_Speed_text
    ui->dashboard_Amber_Speed_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_Speed_text, "42");
    lv_label_set_long_mode(ui->dashboard_Amber_Speed_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_Speed_text, 180, 80);
    lv_obj_set_size(ui->dashboard_Amber_Speed_text, 440, 180);

    //Write style for dashboard_Amber_Speed_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_Speed_text, lv_color_hex(0xFFE6C2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_Speed_text, &lv_font_DSEG14Classic_Regular_160, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_Speed_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_Speed_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_00
    ui->dashboard_Amber_speed_seg_00 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_00, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_00, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_00, 245, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_00, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_00, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_00, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_00, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_00, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_00, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_00, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_01
    ui->dashboard_Amber_speed_seg_01 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_01, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_01, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_01, 271, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_01, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_01, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_01, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_01, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_01, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_01, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_01, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_02
    ui->dashboard_Amber_speed_seg_02 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_02, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_02, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_02, 297, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_02, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_02, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_02, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_02, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_02, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_02, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_02, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_03
    ui->dashboard_Amber_speed_seg_03 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_03, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_03, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_03, 323, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_03, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_03, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_03, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_03, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_03, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_03, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_03, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_04
    ui->dashboard_Amber_speed_seg_04 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_04, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_04, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_04, 349, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_04, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_04, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_04, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_04, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_04, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_04, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_04, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_04, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_05
    ui->dashboard_Amber_speed_seg_05 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_05, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_05, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_05, 375, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_05, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_05, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_05, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_05, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_05, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_05, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_05, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_05, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_06
    ui->dashboard_Amber_speed_seg_06 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_06, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_06, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_06, 401, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_06, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_06, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_06, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_06, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_06, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_06, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_06, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_06, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_07
    ui->dashboard_Amber_speed_seg_07 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_07, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_07, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_07, 427, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_07, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_07, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_07, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_07, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_07, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_07, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_07, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_07, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_08
    ui->dashboard_Amber_speed_seg_08 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_08, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_08, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_08, 453, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_08, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_08, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_08, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_08, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_08, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_08, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_08, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_08, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_09
    ui->dashboard_Amber_speed_seg_09 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_09, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_09, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_09, 479, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_09, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_09, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_09, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_09, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_09, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_09, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_09, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_09, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_10
    ui->dashboard_Amber_speed_seg_10 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_10, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_10, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_10, 505, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_10, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_10, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_10, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_10, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_10, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_10, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_10, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_seg_11
    ui->dashboard_Amber_speed_seg_11 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_seg_11, "");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_seg_11, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_seg_11, 531, 320);
    lv_obj_set_size(ui->dashboard_Amber_speed_seg_11, 24, 6);
    lv_obj_add_flag(ui->dashboard_Amber_speed_seg_11, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_seg_11, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_seg_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_seg_11, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_seg_11, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_speed_seg_11, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_speed_seg_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_min
    ui->dashboard_Amber_speed_min = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_min, "0");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_min, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_min, 240, 332);
    lv_obj_set_size(ui->dashboard_Amber_speed_min, 20, 20);
    lv_obj_add_flag(ui->dashboard_Amber_speed_min, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_min, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_min, lv_color_hex(0x6B4A2C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_min, &lv_font_DSEG14Classic_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_min, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_min, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_speed_max
    ui->dashboard_Amber_speed_max = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_speed_max, "MAX 60");
    lv_label_set_long_mode(ui->dashboard_Amber_speed_max, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_speed_max, 491, 332);
    lv_obj_set_size(ui->dashboard_Amber_speed_max, 80, 20);
    lv_obj_add_flag(ui->dashboard_Amber_speed_max, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_speed_max, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_speed_max, lv_color_hex(0x6B4A2C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_speed_max, &lv_font_DSEG14Classic_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_speed_max, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_speed_max, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_sep
    ui->dashboard_Amber_power_sep = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_sep, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_sep, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_sep, 620, 32);
    lv_obj_set_size(ui->dashboard_Amber_power_sep, 1, 378);

    //Write style for dashboard_Amber_power_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_sep, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_sep, lv_color_hex(0x2A1A0C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_label
    ui->dashboard_Amber_power_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_label, "POWER");
    lv_label_set_long_mode(ui->dashboard_Amber_power_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_label, 636, 40);
    lv_obj_set_size(ui->dashboard_Amber_power_label, 144, 29);

    //Write style for dashboard_Amber_power_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_label, &lv_font_DSEG14Classic_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_label, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_value
    ui->dashboard_Amber_power_value = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_value, "-2.8");
    lv_label_set_long_mode(ui->dashboard_Amber_power_value, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_value, 620, 83);
    lv_obj_set_size(ui->dashboard_Amber_power_value, 126, 70);

    //Write style for dashboard_Amber_power_value, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_value, lv_color_hex(0xFFB35C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_value, &lv_font_DSEG14Classic_Regular_50, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_value, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_unit
    ui->dashboard_Amber_power_unit = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_unit, "kW");
    lv_label_set_long_mode(ui->dashboard_Amber_power_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_unit, 745, 90);
    lv_obj_set_size(ui->dashboard_Amber_power_unit, 60, 24);

    //Write style for dashboard_Amber_power_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_unit, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_unit, &lv_font_DSEG14Classic_Regular_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_Current_text
    ui->dashboard_Amber_Current_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_Current_text, "24.5 A");
    lv_label_set_long_mode(ui->dashboard_Amber_Current_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_Current_text, 636, 138);
    lv_obj_set_size(ui->dashboard_Amber_Current_text, 144, 30);

    //Write style for dashboard_Amber_Current_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_Current_text, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_Current_text, &lv_font_DSEG14Classic_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_Current_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_Current_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_00
    ui->dashboard_Amber_power_seg_00 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_00, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_00, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_00, 636, 170);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_00, 144, 12);

    //Write style for dashboard_Amber_power_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_00, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_00, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_00, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_00, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_00, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_01
    ui->dashboard_Amber_power_seg_01 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_01, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_01, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_01, 636, 185);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_01, 144, 12);

    //Write style for dashboard_Amber_power_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_01, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_01, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_01, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_01, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_01, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_02
    ui->dashboard_Amber_power_seg_02 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_02, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_02, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_02, 636, 200);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_02, 144, 12);

    //Write style for dashboard_Amber_power_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_02, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_02, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_02, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_02, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_02, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_03
    ui->dashboard_Amber_power_seg_03 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_03, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_03, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_03, 636, 215);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_03, 144, 12);

    //Write style for dashboard_Amber_power_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_03, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_03, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_03, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_03, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_03, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_04
    ui->dashboard_Amber_power_seg_04 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_04, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_04, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_04, 636, 230);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_04, 144, 12);

    //Write style for dashboard_Amber_power_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_04, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_04, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_04, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_04, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_04, lv_color_hex(0x241405), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_04, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_05
    ui->dashboard_Amber_power_seg_05 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_05, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_05, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_05, 636, 245);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_05, 144, 12);

    //Write style for dashboard_Amber_power_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_05, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_05, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_05, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_05, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_05, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_05, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_power_seg_05, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_power_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_06
    ui->dashboard_Amber_power_seg_06 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_06, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_06, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_06, 636, 260);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_06, 144, 12);

    //Write style for dashboard_Amber_power_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_06, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_06, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_06, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_06, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_06, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_06, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_power_seg_06, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_power_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_07
    ui->dashboard_Amber_power_seg_07 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_07, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_07, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_07, 636, 275);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_07, 144, 12);

    //Write style for dashboard_Amber_power_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_07, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_07, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_07, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_07, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_07, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_07, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_power_seg_07, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_power_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_08
    ui->dashboard_Amber_power_seg_08 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_08, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_08, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_08, 636, 290);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_08, 144, 12);

    //Write style for dashboard_Amber_power_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_08, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_08, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_08, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_08, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_08, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_08, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_power_seg_08, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_power_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_09
    ui->dashboard_Amber_power_seg_09 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_09, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_09, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_09, 636, 305);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_09, 144, 12);

    //Write style for dashboard_Amber_power_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_09, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_09, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_09, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_09, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_09, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_09, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_power_seg_09, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_power_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_10
    ui->dashboard_Amber_power_seg_10 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_10, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_10, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_10, 636, 320);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_10, 144, 12);

    //Write style for dashboard_Amber_power_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_10, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_10, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_10, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_10, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_10, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_power_seg_10, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_power_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_11
    ui->dashboard_Amber_power_seg_11 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_11, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_11, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_11, 636, 335);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_11, 144, 12);

    //Write style for dashboard_Amber_power_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_11, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_11, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_11, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_11, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_11, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_power_seg_11, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_power_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_12
    ui->dashboard_Amber_power_seg_12 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_12, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_12, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_12, 636, 350);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_12, 144, 12);

    //Write style for dashboard_Amber_power_seg_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_12, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_12, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_12, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_12, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_12, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_12, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_power_seg_12, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_power_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_seg_13
    ui->dashboard_Amber_power_seg_13 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_seg_13, "");
    lv_label_set_long_mode(ui->dashboard_Amber_power_seg_13, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_seg_13, 636, 365);
    lv_obj_set_size(ui->dashboard_Amber_power_seg_13, 144, 12);

    //Write style for dashboard_Amber_power_seg_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_seg_13, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_seg_13, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_seg_13, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_seg_13, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_power_seg_13, lv_color_hex(0xC8470A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_power_seg_13, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->dashboard_Amber_power_seg_13, lv_color_hex(0xFF7A1A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->dashboard_Amber_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->dashboard_Amber_power_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_max_label
    ui->dashboard_Amber_power_max_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_max_label, "MAX");
    lv_label_set_long_mode(ui->dashboard_Amber_power_max_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_max_label, 626, 385);
    lv_obj_set_size(ui->dashboard_Amber_power_max_label, 60, 30);

    //Write style for dashboard_Amber_power_max_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_max_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_max_label, &lv_font_DSEG14Classic_Regular_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_max_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_max_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_power_max_val
    ui->dashboard_Amber_power_max_val = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_power_max_val, "4.5 KW");
    lv_label_set_long_mode(ui->dashboard_Amber_power_max_val, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_power_max_val, 706, 385);
    lv_obj_set_size(ui->dashboard_Amber_power_max_val, 84, 30);

    //Write style for dashboard_Amber_power_max_val, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_power_max_val, lv_color_hex(0xFFE6C2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_power_max_val, &lv_font_DSEG14Classic_Regular_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_power_max_val, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_power_max_val, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_bottom_bg
    ui->dashboard_Amber_bottom_bg = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_bottom_bg, "");
    lv_label_set_long_mode(ui->dashboard_Amber_bottom_bg, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_bottom_bg, 0, 410);
    lv_obj_set_size(ui->dashboard_Amber_bottom_bg, 800, 70);

    //Write style for dashboard_Amber_bottom_bg, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_bottom_bg, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_bottom_bg, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_bottom_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_bottom_bg, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_bottom_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_bottom_bg, lv_color_hex(0x0A0806), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_bottom_bg, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_bottom_top_sep
    ui->dashboard_Amber_bottom_top_sep = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_bottom_top_sep, "");
    lv_label_set_long_mode(ui->dashboard_Amber_bottom_top_sep, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_bottom_top_sep, 0, 410);
    lv_obj_set_size(ui->dashboard_Amber_bottom_top_sep, 800, 1);

    //Write style for dashboard_Amber_bottom_top_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_bottom_top_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_bottom_top_sep, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_bottom_top_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_bottom_top_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_bottom_top_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_bottom_top_sep, lv_color_hex(0x2A1A0C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_bottom_top_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_bottom_col_sep_0
    ui->dashboard_Amber_bottom_col_sep_0 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_bottom_col_sep_0, "");
    lv_label_set_long_mode(ui->dashboard_Amber_bottom_col_sep_0, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_bottom_col_sep_0, 160, 410);
    lv_obj_set_size(ui->dashboard_Amber_bottom_col_sep_0, 1, 70);

    //Write style for dashboard_Amber_bottom_col_sep_0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_bottom_col_sep_0, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_bottom_col_sep_0, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_bottom_col_sep_0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_bottom_col_sep_0, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_bottom_col_sep_0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_bottom_col_sep_0, lv_color_hex(0x2A1A0C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_bottom_col_sep_0, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_bottom_col_sep_1
    ui->dashboard_Amber_bottom_col_sep_1 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_bottom_col_sep_1, "");
    lv_label_set_long_mode(ui->dashboard_Amber_bottom_col_sep_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_bottom_col_sep_1, 320, 410);
    lv_obj_set_size(ui->dashboard_Amber_bottom_col_sep_1, 1, 70);

    //Write style for dashboard_Amber_bottom_col_sep_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_bottom_col_sep_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_bottom_col_sep_1, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_bottom_col_sep_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_bottom_col_sep_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_bottom_col_sep_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_bottom_col_sep_1, lv_color_hex(0x2A1A0C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_bottom_col_sep_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_bottom_col_sep_2
    ui->dashboard_Amber_bottom_col_sep_2 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_bottom_col_sep_2, "");
    lv_label_set_long_mode(ui->dashboard_Amber_bottom_col_sep_2, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_bottom_col_sep_2, 480, 410);
    lv_obj_set_size(ui->dashboard_Amber_bottom_col_sep_2, 1, 70);

    //Write style for dashboard_Amber_bottom_col_sep_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_bottom_col_sep_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_bottom_col_sep_2, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_bottom_col_sep_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_bottom_col_sep_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_bottom_col_sep_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_bottom_col_sep_2, lv_color_hex(0x2A1A0C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_bottom_col_sep_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_bottom_col_sep_3
    ui->dashboard_Amber_bottom_col_sep_3 = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_bottom_col_sep_3, "");
    lv_label_set_long_mode(ui->dashboard_Amber_bottom_col_sep_3, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_bottom_col_sep_3, 640, 410);
    lv_obj_set_size(ui->dashboard_Amber_bottom_col_sep_3, 1, 70);

    //Write style for dashboard_Amber_bottom_col_sep_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_bottom_col_sep_3, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_bottom_col_sep_3, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_bottom_col_sep_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_bottom_col_sep_3, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_bottom_col_sep_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_bottom_col_sep_3, lv_color_hex(0x2A1A0C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_bottom_col_sep_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_trip_label
    ui->dashboard_Amber_col_trip_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_trip_label, "TRIP");
    lv_label_set_long_mode(ui->dashboard_Amber_col_trip_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_trip_label, 160, 415);
    lv_obj_set_size(ui->dashboard_Amber_col_trip_label, 160, 20);

    //Write style for dashboard_Amber_col_trip_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_trip_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_trip_label, &lv_font_DSEG14Classic_Regular_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_trip_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_trip_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_trip_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_TRIP_text
    ui->dashboard_Amber_TRIP_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_TRIP_text, "12.6");
    lv_label_set_long_mode(ui->dashboard_Amber_TRIP_text, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(ui->dashboard_Amber_TRIP_text, 158, 440);
    lv_obj_set_size(ui->dashboard_Amber_TRIP_text, 100, 60);

    //Write style for dashboard_Amber_TRIP_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_TRIP_text, lv_color_hex(0xFFE6C2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_TRIP_text, &lv_font_DSEG14Classic_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_TRIP_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_TRIP_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_trip_unit
    ui->dashboard_Amber_col_trip_unit = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_trip_unit, "KM");
    lv_label_set_long_mode(ui->dashboard_Amber_col_trip_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_trip_unit, 260, 440);
    lv_obj_set_size(ui->dashboard_Amber_col_trip_unit, 56, 20);

    //Write style for dashboard_Amber_col_trip_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_trip_unit, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_trip_unit, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_trip_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_trip_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_odo_label
    ui->dashboard_Amber_col_odo_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_odo_label, "ODO");
    lv_label_set_long_mode(ui->dashboard_Amber_col_odo_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_odo_label, 321, 415);
    lv_obj_set_size(ui->dashboard_Amber_col_odo_label, 160, 20);

    //Write style for dashboard_Amber_col_odo_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_odo_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_odo_label, &lv_font_DSEG14Classic_Regular_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_odo_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_odo_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_odo_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_odo_text
    ui->dashboard_Amber_odo_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_odo_text, "6436");
    lv_label_set_long_mode(ui->dashboard_Amber_odo_text, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(ui->dashboard_Amber_odo_text, 321, 440);
    lv_obj_set_size(ui->dashboard_Amber_odo_text, 100, 60);

    //Write style for dashboard_Amber_odo_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_odo_text, lv_color_hex(0xFFE6C2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_odo_text, &lv_font_DSEG14Classic_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_odo_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_odo_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_odo_unit
    ui->dashboard_Amber_col_odo_unit = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_odo_unit, "KM");
    lv_label_set_long_mode(ui->dashboard_Amber_col_odo_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_odo_unit, 422, 442);
    lv_obj_set_size(ui->dashboard_Amber_col_odo_unit, 56, 20);

    //Write style for dashboard_Amber_col_odo_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_odo_unit, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_odo_unit, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_odo_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_odo_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_mtmp_label
    ui->dashboard_Amber_col_mtmp_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_mtmp_label, "M-TEMP");
    lv_label_set_long_mode(ui->dashboard_Amber_col_mtmp_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_mtmp_label, 479, 415);
    lv_obj_set_size(ui->dashboard_Amber_col_mtmp_label, 160, 20);

    //Write style for dashboard_Amber_col_mtmp_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_mtmp_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_mtmp_label, &lv_font_DSEG14Classic_Regular_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_mtmp_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_mtmp_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_mtmp_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_temp_mot_text
    ui->dashboard_Amber_temp_mot_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_temp_mot_text, "47");
    lv_label_set_long_mode(ui->dashboard_Amber_temp_mot_text, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(ui->dashboard_Amber_temp_mot_text, 479, 440);
    lv_obj_set_size(ui->dashboard_Amber_temp_mot_text, 100, 60);

    //Write style for dashboard_Amber_temp_mot_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_temp_mot_text, lv_color_hex(0xFFE6C2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_temp_mot_text, &lv_font_DSEG14Classic_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_temp_mot_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_temp_mot_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_mtmp_unit
    ui->dashboard_Amber_col_mtmp_unit = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_mtmp_unit, "°C");
    lv_label_set_long_mode(ui->dashboard_Amber_col_mtmp_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_mtmp_unit, 582, 442);
    lv_obj_set_size(ui->dashboard_Amber_col_mtmp_unit, 56, 21);

    //Write style for dashboard_Amber_col_mtmp_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_mtmp_unit, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_mtmp_unit, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_mtmp_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_mtmp_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_ctmp_label
    ui->dashboard_Amber_col_ctmp_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_ctmp_label, "C-TEMP");
    lv_label_set_long_mode(ui->dashboard_Amber_col_ctmp_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_ctmp_label, 631, 415);
    lv_obj_set_size(ui->dashboard_Amber_col_ctmp_label, 160, 20);

    //Write style for dashboard_Amber_col_ctmp_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_ctmp_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_ctmp_label, &lv_font_DSEG14Classic_Regular_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_ctmp_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_ctmp_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_ctmp_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_temp_esc_text
    ui->dashboard_Amber_temp_esc_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_temp_esc_text, "39");
    lv_label_set_long_mode(ui->dashboard_Amber_temp_esc_text, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(ui->dashboard_Amber_temp_esc_text, 637, 440);
    lv_obj_set_size(ui->dashboard_Amber_temp_esc_text, 100, 60);

    //Write style for dashboard_Amber_temp_esc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_temp_esc_text, lv_color_hex(0xFFE6C2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_temp_esc_text, &lv_font_DSEG14Classic_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_temp_esc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_temp_esc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_ctmp_unit
    ui->dashboard_Amber_col_ctmp_unit = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_ctmp_unit, "°C");
    lv_label_set_long_mode(ui->dashboard_Amber_col_ctmp_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_ctmp_unit, 742, 442);
    lv_obj_set_size(ui->dashboard_Amber_col_ctmp_unit, 56, 20);

    //Write style for dashboard_Amber_col_ctmp_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_ctmp_unit, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_ctmp_unit, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_ctmp_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_ctmp_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_avg_label
    ui->dashboard_Amber_col_avg_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_avg_label, "AVG");
    lv_label_set_long_mode(ui->dashboard_Amber_col_avg_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_avg_label, 640, 414);
    lv_obj_set_size(ui->dashboard_Amber_col_avg_label, 160, 12);
    lv_obj_add_flag(ui->dashboard_Amber_col_avg_label, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_col_avg_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_avg_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_avg_label, &lv_font_DSEG14Classic_Regular_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_avg_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_avg_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_avg_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_avg_value
    ui->dashboard_Amber_col_avg_value = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_avg_value, "24");
    lv_label_set_long_mode(ui->dashboard_Amber_col_avg_value, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(ui->dashboard_Amber_col_avg_value, 640, 432);
    lv_obj_set_size(ui->dashboard_Amber_col_avg_value, 100, 60);
    lv_obj_add_flag(ui->dashboard_Amber_col_avg_value, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_col_avg_value, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_avg_value, lv_color_hex(0xFFE6C2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_avg_value, &lv_font_DSEG14Classic_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_avg_value, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_avg_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_avg_unit
    ui->dashboard_Amber_col_avg_unit = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_avg_unit, "KM/H");
    lv_label_set_long_mode(ui->dashboard_Amber_col_avg_unit, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_avg_unit, 744, 442);
    lv_obj_set_size(ui->dashboard_Amber_col_avg_unit, 56, 14);
    lv_obj_add_flag(ui->dashboard_Amber_col_avg_unit, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_col_avg_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_avg_unit, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_avg_unit, &lv_font_DSEG14Classic_Regular_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_avg_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_avg_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_cruise_control_img
    ui->dashboard_Amber_cruise_control_img = lv_img_create(ui->dashboard_Amber);
    lv_obj_add_flag(ui->dashboard_Amber_cruise_control_img, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->dashboard_Amber_cruise_control_img, &_cruise_control_alpha_38x38);
    lv_img_set_pivot(ui->dashboard_Amber_cruise_control_img, 0,0);
    lv_img_set_angle(ui->dashboard_Amber_cruise_control_img, 0);
    lv_obj_set_pos(ui->dashboard_Amber_cruise_control_img, 565, 45);
    lv_obj_set_size(ui->dashboard_Amber_cruise_control_img, 38, 38);
    lv_obj_add_flag(ui->dashboard_Amber_cruise_control_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->dashboard_Amber_cruise_control_img, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_cruise_control_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_Amber_cruise_control_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_Amber_cruise_control_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_cruise_control_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_Amber_cruise_control_img, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_esc_not_connected_text
    ui->dashboard_Amber_esc_not_connected_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_esc_not_connected_text, "ESC NOT CONNECTED");
    lv_label_set_long_mode(ui->dashboard_Amber_esc_not_connected_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_esc_not_connected_text, 280, 9);
    lv_obj_set_size(ui->dashboard_Amber_esc_not_connected_text, 240, 32);
    lv_obj_add_flag(ui->dashboard_Amber_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->dashboard_Amber_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_esc_not_connected_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_esc_not_connected_text, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_esc_not_connected_text, lv_color_hex(0xFF3B2F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_esc_not_connected_text, &lv_font_DSEG14Classic_Regular_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_esc_not_connected_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_esc_not_connected_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_esc_not_connected_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_esc_not_connected_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_esc_not_connected_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_Ah_text
    ui->dashboard_Amber_Ah_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_Ah_text, "6.4 Ah");
    lv_label_set_long_mode(ui->dashboard_Amber_Ah_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_Ah_text, 20, 138);
    lv_obj_set_size(ui->dashboard_Amber_Ah_text, 130, 27);

    //Write style for dashboard_Amber_Ah_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_Ah_text, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_Ah_text, &lv_font_DSEG14Classic_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_Ah_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_Ah_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_Speed_cc_text
    ui->dashboard_Amber_Speed_cc_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_Speed_cc_text, "--");
    lv_label_set_long_mode(ui->dashboard_Amber_Speed_cc_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_Speed_cc_text, 550, 90);
    lv_obj_set_size(ui->dashboard_Amber_Speed_cc_text, 60, 70);
    lv_obj_add_flag(ui->dashboard_Amber_Speed_cc_text, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->dashboard_Amber_Speed_cc_text, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_Speed_cc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_Speed_cc_text, lv_color_hex(0xFFE6C2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_Speed_cc_text, &lv_font_DSEG14Classic_Regular_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_Speed_cc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_Speed_cc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_Settings_text
    ui->dashboard_Amber_Settings_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_Settings_text, "SETTINGS");
    lv_label_set_long_mode(ui->dashboard_Amber_Settings_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_Settings_text, 600, 5);
    lv_obj_set_size(ui->dashboard_Amber_Settings_text, 149, 29);
    lv_obj_add_flag(ui->dashboard_Amber_Settings_text, LV_OBJ_FLAG_CLICKABLE);

    //Write style for dashboard_Amber_Settings_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_Settings_text, lv_color_hex(0xFFB35C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_Settings_text, &lv_font_DSEG14Classic_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_Settings_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_Settings_text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_Settings_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_Settings_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_song_title_label
    ui->dashboard_Amber_song_title_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_song_title_label, "");
    lv_label_set_long_mode(ui->dashboard_Amber_song_title_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_song_title_label, 180, 290);
    lv_obj_set_size(ui->dashboard_Amber_song_title_label, 440, 50);
    lv_obj_add_flag(ui->dashboard_Amber_song_title_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->dashboard_Amber_song_title_label, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_song_title_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_song_title_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_song_title_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_song_title_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_song_title_label, &lv_font_DSEG14Classic_Regular_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_song_title_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_song_title_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_song_title_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_song_title_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_song_title_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_song_title_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_song_title_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_song_title_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_song_title_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_song_title_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_v_label
    ui->dashboard_Amber_col_v_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_v_label, "V");
    lv_label_set_long_mode(ui->dashboard_Amber_col_v_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_v_label, 102, 440);
    lv_obj_set_size(ui->dashboard_Amber_col_v_label, 56, 20);

    //Write style for dashboard_Amber_col_v_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_v_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_v_label, &lv_font_DSEG14Classic_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_v_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_v_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_v_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_Voltage_text
    ui->dashboard_Amber_Voltage_text = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_Voltage_text, "58.4");
    lv_label_set_long_mode(ui->dashboard_Amber_Voltage_text, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(ui->dashboard_Amber_Voltage_text, 0, 440);
    lv_obj_set_size(ui->dashboard_Amber_Voltage_text, 100, 60);

    //Write style for dashboard_Amber_Voltage_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_Voltage_text, lv_color_hex(0xFFE6C2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_Voltage_text, &lv_font_DSEG14Classic_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_Voltage_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_Voltage_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_col_voltage_label
    ui->dashboard_Amber_col_voltage_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_col_voltage_label, "VOLTAGE\n");
    lv_label_set_long_mode(ui->dashboard_Amber_col_voltage_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_col_voltage_label, 0, 415);
    lv_obj_set_size(ui->dashboard_Amber_col_voltage_label, 160, 20);

    //Write style for dashboard_Amber_col_voltage_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_col_voltage_label, lv_color_hex(0xB0814F), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_col_voltage_label, &lv_font_DSEG14Classic_Regular_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_col_voltage_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_col_voltage_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_col_voltage_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_col_voltage_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_cur_time_label
    ui->dashboard_Amber_cur_time_label = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_cur_time_label, "14:32");
    lv_label_set_long_mode(ui->dashboard_Amber_cur_time_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_cur_time_label, 503, 5);
    lv_obj_set_size(ui->dashboard_Amber_cur_time_label, 110, 30);

    //Write style for dashboard_Amber_cur_time_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_cur_time_label, lv_color_hex(0xFFB35C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_cur_time_label, &lv_font_DSEG14Classic_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_cur_time_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_cur_time_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_cur_time_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_brightness_slider
    ui->dashboard_Amber_brightness_slider = lv_slider_create(ui->dashboard_Amber);
    lv_slider_set_range(ui->dashboard_Amber_brightness_slider, 0, 100);
    lv_slider_set_mode(ui->dashboard_Amber_brightness_slider, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->dashboard_Amber_brightness_slider, 50, LV_ANIM_OFF);
    lv_obj_set_pos(ui->dashboard_Amber_brightness_slider, 624, 40);
    lv_obj_set_size(ui->dashboard_Amber_brightness_slider, 170, 350);
    lv_obj_add_flag(ui->dashboard_Amber_brightness_slider, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->dashboard_Amber_brightness_slider, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_brightness_slider, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_brightness_slider, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_brightness_slider, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->dashboard_Amber_brightness_slider, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_brightness_slider, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Amber_brightness_slider, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_brightness_slider, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_brightness_slider, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for dashboard_Amber_brightness_slider, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_brightness_slider, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_brightness_slider, 13, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes dashboard_Amber_music_info
    ui->dashboard_Amber_music_info = lv_tileview_create(ui->dashboard_Amber);
    ui->dashboard_Amber_music_info_tile = lv_tileview_add_tile(ui->dashboard_Amber_music_info, 0, 0, LV_DIR_RIGHT);
    lv_obj_set_pos(ui->dashboard_Amber_music_info, 183, 261);
    lv_obj_set_size(ui->dashboard_Amber_music_info, 434, 136);
    lv_obj_set_scrollbar_mode(ui->dashboard_Amber_music_info, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->dashboard_Amber_music_info, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->dashboard_Amber_music_info, LV_OBJ_FLAG_HIDDEN);

    //Write style for dashboard_Amber_music_info, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_music_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_music_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_music_info, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Amber_music_info, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_music_info, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Amber_music_info, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Amber_music_info, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_music_info, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes dashboard_Amber_statistics_button
    ui->dashboard_Amber_statistics_button = lv_label_create(ui->dashboard_Amber);
    lv_label_set_text(ui->dashboard_Amber_statistics_button, "STATISTICS");
    lv_label_set_long_mode(ui->dashboard_Amber_statistics_button, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->dashboard_Amber_statistics_button, 310, 5);
    lv_obj_set_size(ui->dashboard_Amber_statistics_button, 185, 30);
    lv_obj_add_flag(ui->dashboard_Amber_statistics_button, LV_OBJ_FLAG_CLICKABLE);

    //Write style for dashboard_Amber_statistics_button, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->dashboard_Amber_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Amber_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->dashboard_Amber_statistics_button, lv_color_hex(0xFFB35C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Amber_statistics_button, &lv_font_DSEG14Classic_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Amber_statistics_button, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Amber_statistics_button, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->dashboard_Amber_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Amber_statistics_button, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Amber_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Amber_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Amber_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->dashboard_Amber_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Amber_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Amber_statistics_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of dashboard_Amber.


    //Update current screen layout.
    lv_obj_update_layout(ui->dashboard_Amber);

}
