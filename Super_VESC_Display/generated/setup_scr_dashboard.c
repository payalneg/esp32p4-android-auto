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
    lv_obj_set_style_bg_color(ui->dashboard, lv_color_hex(0x07090A), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_statusbar_sep
    ui->dashboard_statusbar_sep = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_statusbar_sep, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_statusbar_sep, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_statusbar_sep, "");
    lv_textarea_set_placeholder_text(ui->dashboard_statusbar_sep, "");
    lv_textarea_set_password_bullet(ui->dashboard_statusbar_sep, "*");
    lv_textarea_set_password_mode(ui->dashboard_statusbar_sep, false);
    lv_textarea_set_one_line(ui->dashboard_statusbar_sep, true);
    lv_textarea_set_accepted_chars(ui->dashboard_statusbar_sep, "");
    lv_textarea_set_max_length(ui->dashboard_statusbar_sep, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_statusbar_sep, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_statusbar_sep, 0, 32);
    lv_obj_set_size(ui->dashboard_statusbar_sep, 800, 1);

    //Write style for dashboard_statusbar_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_statusbar_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_statusbar_sep, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_statusbar_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_statusbar_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_statusbar_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_statusbar_sep, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_statusbar_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_statusbar_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_statusbar_sep, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_statusbar_sep, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_statusbar_sep, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_statusbar_sep, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_statusbar_sep, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_status_vesc
    ui->dashboard_status_vesc = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_status_vesc, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_status_vesc, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_status_vesc, "VESC");
    lv_textarea_set_placeholder_text(ui->dashboard_status_vesc, "");
    lv_textarea_set_password_bullet(ui->dashboard_status_vesc, "*");
    lv_textarea_set_password_mode(ui->dashboard_status_vesc, false);
    lv_textarea_set_one_line(ui->dashboard_status_vesc, true);
    lv_textarea_set_accepted_chars(ui->dashboard_status_vesc, "");
    lv_textarea_set_max_length(ui->dashboard_status_vesc, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_status_vesc, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_status_vesc, 16, 10);
    lv_obj_set_size(ui->dashboard_status_vesc, 50, 30);

    //Write style for dashboard_status_vesc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_status_vesc, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_status_vesc, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_status_vesc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_status_vesc, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_status_vesc, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_status_vesc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_status_vesc, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_status_vesc, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_status_vesc, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_status_vesc, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_status_vesc, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_mode_text
    ui->dashboard_mode_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_mode_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_mode_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_mode_text, "MODE 1");
    lv_textarea_set_placeholder_text(ui->dashboard_mode_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_mode_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_mode_text, false);
    lv_textarea_set_one_line(ui->dashboard_mode_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_mode_text, "");
    lv_textarea_set_max_length(ui->dashboard_mode_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_mode_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_mode_text, 76, 10);
    lv_obj_set_size(ui->dashboard_mode_text, 80, 30);

    //Write style for dashboard_mode_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_mode_text, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_mode_text, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_mode_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_mode_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_mode_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_mode_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_mode_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_mode_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_mode_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_mode_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_uptime_text
    ui->dashboard_uptime_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_uptime_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_uptime_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_uptime_text, "00:42:18");
    lv_textarea_set_placeholder_text(ui->dashboard_uptime_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_uptime_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_uptime_text, false);
    lv_textarea_set_one_line(ui->dashboard_uptime_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_uptime_text, "");
    lv_textarea_set_max_length(ui->dashboard_uptime_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_uptime_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_uptime_text, 170, 10);
    lv_obj_set_size(ui->dashboard_uptime_text, 80, 30);

    //Write style for dashboard_uptime_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_uptime_text, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_uptime_text, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_uptime_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_uptime_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_uptime_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_uptime_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_uptime_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_uptime_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_uptime_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_uptime_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_status_bt
    ui->dashboard_status_bt = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_status_bt, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_status_bt, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_status_bt, "BT");
    lv_textarea_set_placeholder_text(ui->dashboard_status_bt, "");
    lv_textarea_set_password_bullet(ui->dashboard_status_bt, "*");
    lv_textarea_set_password_mode(ui->dashboard_status_bt, false);
    lv_textarea_set_one_line(ui->dashboard_status_bt, true);
    lv_textarea_set_accepted_chars(ui->dashboard_status_bt, "");
    lv_textarea_set_max_length(ui->dashboard_status_bt, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_status_bt, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_status_bt, 740, 10);
    lv_obj_set_size(ui->dashboard_status_bt, 40, 30);

    //Write style for dashboard_status_bt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_status_bt, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_status_bt, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_status_bt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_status_bt, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_status_bt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_status_bt, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_status_bt, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_status_bt, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_status_bt, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_status_bt, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_battery_sep
    ui->dashboard_battery_sep = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_battery_sep, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_battery_sep, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_battery_sep, "");
    lv_textarea_set_placeholder_text(ui->dashboard_battery_sep, "");
    lv_textarea_set_password_bullet(ui->dashboard_battery_sep, "*");
    lv_textarea_set_password_mode(ui->dashboard_battery_sep, false);
    lv_textarea_set_one_line(ui->dashboard_battery_sep, true);
    lv_textarea_set_accepted_chars(ui->dashboard_battery_sep, "");
    lv_textarea_set_max_length(ui->dashboard_battery_sep, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_battery_sep, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_battery_sep, 180, 32);
    lv_obj_set_size(ui->dashboard_battery_sep, 1, 368);

    //Write style for dashboard_battery_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_battery_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_battery_sep, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_battery_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_battery_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_battery_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_battery_sep, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_battery_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_battery_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_battery_sep, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_battery_sep, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_battery_sep, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_battery_sep, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_battery_sep, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_battery_label
    ui->dashboard_battery_label = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_battery_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_battery_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_battery_label, "BATTERY");
    lv_textarea_set_placeholder_text(ui->dashboard_battery_label, "");
    lv_textarea_set_password_bullet(ui->dashboard_battery_label, "*");
    lv_textarea_set_password_mode(ui->dashboard_battery_label, false);
    lv_textarea_set_one_line(ui->dashboard_battery_label, true);
    lv_textarea_set_accepted_chars(ui->dashboard_battery_label, "");
    lv_textarea_set_max_length(ui->dashboard_battery_label, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_battery_label, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_battery_label, 20, 50);
    lv_obj_set_size(ui->dashboard_battery_label, 120, 16);

    //Write style for dashboard_battery_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_battery_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_battery_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_battery_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_battery_label, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_battery_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_battery_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_battery_label, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_battery_label, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_battery_label, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_battery_label, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_battery_label, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Battery_proc_text
    ui->dashboard_Battery_proc_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_Battery_proc_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_Battery_proc_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_Battery_proc_text, "68");
    lv_textarea_set_placeholder_text(ui->dashboard_Battery_proc_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Battery_proc_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Battery_proc_text, false);
    lv_textarea_set_one_line(ui->dashboard_Battery_proc_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Battery_proc_text, "");
    lv_textarea_set_max_length(ui->dashboard_Battery_proc_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Battery_proc_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Battery_proc_text, 20, 73);
    lv_obj_set_size(ui->dashboard_Battery_proc_text, 90, 70);

    //Write style for dashboard_Battery_proc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Battery_proc_text, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Battery_proc_text, &lv_font_Antonio_Regular_64, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Battery_proc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Battery_proc_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Battery_proc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Battery_proc_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Battery_proc_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Battery_proc_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Battery_proc_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Battery_proc_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_battery_pct
    ui->dashboard_battery_pct = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_battery_pct, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_battery_pct, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_battery_pct, "%");
    lv_textarea_set_placeholder_text(ui->dashboard_battery_pct, "");
    lv_textarea_set_password_bullet(ui->dashboard_battery_pct, "*");
    lv_textarea_set_password_mode(ui->dashboard_battery_pct, false);
    lv_textarea_set_one_line(ui->dashboard_battery_pct, true);
    lv_textarea_set_accepted_chars(ui->dashboard_battery_pct, "");
    lv_textarea_set_max_length(ui->dashboard_battery_pct, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_battery_pct, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_battery_pct, 88, 96);
    lv_obj_set_size(ui->dashboard_battery_pct, 40, 24);

    //Write style for dashboard_battery_pct, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_battery_pct, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_battery_pct, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_battery_pct, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_battery_pct, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_battery_pct, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_battery_pct, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_battery_pct, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_battery_pct, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_battery_pct, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_battery_pct, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Voltage_text
    ui->dashboard_Voltage_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_Voltage_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_Voltage_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_Voltage_text, "54.2 V");
    lv_textarea_set_placeholder_text(ui->dashboard_Voltage_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Voltage_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Voltage_text, false);
    lv_textarea_set_one_line(ui->dashboard_Voltage_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Voltage_text, "");
    lv_textarea_set_max_length(ui->dashboard_Voltage_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Voltage_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Voltage_text, 20, 140);
    lv_obj_set_size(ui->dashboard_Voltage_text, 130, 30);

    //Write style for dashboard_Voltage_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Voltage_text, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Voltage_text, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Voltage_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Voltage_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Voltage_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Voltage_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Voltage_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Voltage_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Voltage_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Voltage_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_00
    ui->dashboard_batt_seg_00 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_00, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_00, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_00, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_00, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_00, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_00, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_00, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_00, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_00, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_00, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_00, 20, 170);
    lv_obj_set_size(ui->dashboard_batt_seg_00, 144, 12);

    //Write style for dashboard_batt_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_00, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_00, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_00, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_00, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_00, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_00, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_00, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_00, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_00, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_00, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_01
    ui->dashboard_batt_seg_01 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_01, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_01, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_01, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_01, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_01, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_01, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_01, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_01, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_01, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_01, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_01, 20, 185);
    lv_obj_set_size(ui->dashboard_batt_seg_01, 144, 12);

    //Write style for dashboard_batt_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_01, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_01, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_01, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_01, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_01, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_01, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_01, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_01, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_01, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_01, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_02
    ui->dashboard_batt_seg_02 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_02, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_02, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_02, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_02, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_02, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_02, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_02, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_02, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_02, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_02, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_02, 20, 200);
    lv_obj_set_size(ui->dashboard_batt_seg_02, 144, 12);

    //Write style for dashboard_batt_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_02, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_02, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_02, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_02, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_02, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_02, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_02, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_02, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_02, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_02, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_03
    ui->dashboard_batt_seg_03 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_03, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_03, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_03, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_03, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_03, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_03, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_03, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_03, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_03, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_03, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_03, 20, 215);
    lv_obj_set_size(ui->dashboard_batt_seg_03, 144, 12);

    //Write style for dashboard_batt_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_03, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_03, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_03, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_03, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_03, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_03, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_03, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_03, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_03, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_03, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_04
    ui->dashboard_batt_seg_04 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_04, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_04, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_04, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_04, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_04, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_04, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_04, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_04, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_04, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_04, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_04, 20, 230);
    lv_obj_set_size(ui->dashboard_batt_seg_04, 144, 12);

    //Write style for dashboard_batt_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_04, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_04, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_04, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_04, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_04, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_04, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_04, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_04, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_04, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_04, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_04, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_05
    ui->dashboard_batt_seg_05 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_05, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_05, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_05, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_05, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_05, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_05, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_05, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_05, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_05, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_05, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_05, 20, 245);
    lv_obj_set_size(ui->dashboard_batt_seg_05, 144, 12);

    //Write style for dashboard_batt_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_05, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_05, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_05, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_05, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_05, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_05, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_05, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_05, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_05, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_05, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_05, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_06
    ui->dashboard_batt_seg_06 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_06, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_06, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_06, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_06, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_06, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_06, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_06, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_06, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_06, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_06, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_06, 20, 260);
    lv_obj_set_size(ui->dashboard_batt_seg_06, 144, 12);

    //Write style for dashboard_batt_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_06, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_06, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_06, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_06, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_06, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_06, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_06, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_06, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_06, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_06, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_06, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_07
    ui->dashboard_batt_seg_07 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_07, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_07, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_07, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_07, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_07, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_07, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_07, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_07, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_07, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_07, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_07, 20, 275);
    lv_obj_set_size(ui->dashboard_batt_seg_07, 144, 12);

    //Write style for dashboard_batt_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_07, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_07, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_07, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_07, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_07, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_07, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_07, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_07, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_07, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_07, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_07, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_08
    ui->dashboard_batt_seg_08 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_08, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_08, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_08, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_08, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_08, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_08, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_08, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_08, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_08, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_08, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_08, 20, 290);
    lv_obj_set_size(ui->dashboard_batt_seg_08, 144, 12);

    //Write style for dashboard_batt_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_08, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_08, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_08, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_08, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_08, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_08, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_08, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_08, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_08, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_08, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_08, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_09
    ui->dashboard_batt_seg_09 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_09, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_09, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_09, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_09, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_09, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_09, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_09, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_09, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_09, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_09, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_09, 20, 305);
    lv_obj_set_size(ui->dashboard_batt_seg_09, 144, 12);

    //Write style for dashboard_batt_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_09, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_09, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_09, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_09, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_09, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_09, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_09, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_09, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_09, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_09, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_09, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_10
    ui->dashboard_batt_seg_10 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_10, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_10, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_10, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_10, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_10, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_10, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_10, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_10, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_10, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_10, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_10, 20, 320);
    lv_obj_set_size(ui->dashboard_batt_seg_10, 144, 12);

    //Write style for dashboard_batt_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_10, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_10, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_10, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_10, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_10, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_10, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_10, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_10, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_10, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_10, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_11
    ui->dashboard_batt_seg_11 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_11, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_11, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_11, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_11, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_11, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_11, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_11, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_11, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_11, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_11, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_11, 20, 335);
    lv_obj_set_size(ui->dashboard_batt_seg_11, 144, 12);

    //Write style for dashboard_batt_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_11, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_11, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_11, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_11, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_11, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_11, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_11, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_11, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_11, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_12
    ui->dashboard_batt_seg_12 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_12, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_12, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_12, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_12, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_12, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_12, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_12, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_12, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_12, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_12, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_12, 20, 350);
    lv_obj_set_size(ui->dashboard_batt_seg_12, 144, 12);

    //Write style for dashboard_batt_seg_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_12, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_12, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_12, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_12, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_12, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_12, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_12, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_12, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_12, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_12, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_12, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_batt_seg_13
    ui->dashboard_batt_seg_13 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_batt_seg_13, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_batt_seg_13, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_batt_seg_13, "");
    lv_textarea_set_placeholder_text(ui->dashboard_batt_seg_13, "");
    lv_textarea_set_password_bullet(ui->dashboard_batt_seg_13, "*");
    lv_textarea_set_password_mode(ui->dashboard_batt_seg_13, false);
    lv_textarea_set_one_line(ui->dashboard_batt_seg_13, true);
    lv_textarea_set_accepted_chars(ui->dashboard_batt_seg_13, "");
    lv_textarea_set_max_length(ui->dashboard_batt_seg_13, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_batt_seg_13, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_batt_seg_13, 20, 365);
    lv_obj_set_size(ui->dashboard_batt_seg_13, 144, 12);

    //Write style for dashboard_batt_seg_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_batt_seg_13, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_batt_seg_13, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_batt_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_batt_seg_13, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_13, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_13, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_batt_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_13, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_batt_seg_13, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_batt_seg_13, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_batt_seg_13, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_batt_seg_13, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_batt_seg_13, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_battery_range_label
    ui->dashboard_battery_range_label = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_battery_range_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_battery_range_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_battery_range_label, "RANGE");
    lv_textarea_set_placeholder_text(ui->dashboard_battery_range_label, "");
    lv_textarea_set_password_bullet(ui->dashboard_battery_range_label, "*");
    lv_textarea_set_password_mode(ui->dashboard_battery_range_label, false);
    lv_textarea_set_one_line(ui->dashboard_battery_range_label, true);
    lv_textarea_set_accepted_chars(ui->dashboard_battery_range_label, "");
    lv_textarea_set_max_length(ui->dashboard_battery_range_label, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_battery_range_label, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_battery_range_label, 20, 380);
    lv_obj_set_size(ui->dashboard_battery_range_label, 60, 30);

    //Write style for dashboard_battery_range_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_battery_range_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_battery_range_label, &lv_font_montserratMedium_14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_battery_range_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_battery_range_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_battery_range_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_battery_range_label, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_battery_range_label, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_battery_range_label, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_battery_range_label, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_battery_range_label, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Range_text
    ui->dashboard_Range_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_Range_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_Range_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_Range_text, "38 KM");
    lv_textarea_set_placeholder_text(ui->dashboard_Range_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Range_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Range_text, false);
    lv_textarea_set_one_line(ui->dashboard_Range_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Range_text, "");
    lv_textarea_set_max_length(ui->dashboard_Range_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Range_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Range_text, 80, 380);
    lv_obj_set_size(ui->dashboard_Range_text, 84, 30);

    //Write style for dashboard_Range_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Range_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Range_text, &lv_font_montserratMedium_14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Range_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Range_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Range_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Range_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Range_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Range_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Range_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Range_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_label
    ui->dashboard_speed_label = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_label, "SPEED · KM/H");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_label, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_label, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_label, false);
    lv_textarea_set_one_line(ui->dashboard_speed_label, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_label, "");
    lv_textarea_set_max_length(ui->dashboard_speed_label, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_label, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_label, 180, 42);
    lv_obj_set_size(ui->dashboard_speed_label, 440, 14);

    //Write style for dashboard_speed_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_label, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_label, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_label, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_label, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_label, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_label, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Speed_text
    ui->dashboard_Speed_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_Speed_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_Speed_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_Speed_text, "32");
    lv_textarea_set_placeholder_text(ui->dashboard_Speed_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Speed_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Speed_text, false);
    lv_textarea_set_one_line(ui->dashboard_Speed_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Speed_text, "");
    lv_textarea_set_max_length(ui->dashboard_Speed_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Speed_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Speed_text, 180, 70);
    lv_obj_set_size(ui->dashboard_Speed_text, 440, 224);

    //Write style for dashboard_Speed_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Speed_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Speed_text, &lv_font_Antonio_Regular_220, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Speed_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Speed_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Speed_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Speed_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Speed_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Speed_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Speed_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Speed_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_00
    ui->dashboard_speed_seg_00 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_00, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_00, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_00, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_00, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_00, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_00, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_00, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_00, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_00, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_00, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_00, 245, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_00, 24, 6);

    //Write style for dashboard_speed_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_00, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_00, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_00, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_00, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_00, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_00, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_00, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_00, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_00, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_00, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_01
    ui->dashboard_speed_seg_01 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_01, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_01, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_01, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_01, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_01, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_01, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_01, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_01, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_01, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_01, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_01, 271, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_01, 24, 6);

    //Write style for dashboard_speed_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_01, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_01, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_01, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_01, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_01, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_01, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_01, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_01, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_01, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_01, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_02
    ui->dashboard_speed_seg_02 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_02, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_02, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_02, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_02, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_02, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_02, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_02, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_02, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_02, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_02, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_02, 297, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_02, 24, 6);

    //Write style for dashboard_speed_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_02, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_02, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_02, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_02, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_02, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_02, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_02, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_02, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_02, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_02, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_03
    ui->dashboard_speed_seg_03 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_03, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_03, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_03, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_03, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_03, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_03, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_03, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_03, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_03, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_03, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_03, 323, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_03, 24, 6);

    //Write style for dashboard_speed_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_03, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_03, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_03, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_03, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_03, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_03, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_03, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_03, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_03, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_03, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_04
    ui->dashboard_speed_seg_04 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_04, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_04, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_04, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_04, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_04, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_04, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_04, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_04, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_04, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_04, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_04, 349, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_04, 24, 6);

    //Write style for dashboard_speed_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_04, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_04, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_04, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_04, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_04, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_04, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_04, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_04, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_04, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_04, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_04, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_05
    ui->dashboard_speed_seg_05 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_05, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_05, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_05, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_05, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_05, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_05, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_05, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_05, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_05, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_05, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_05, 375, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_05, 24, 6);

    //Write style for dashboard_speed_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_05, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_05, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_05, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_05, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_05, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_05, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_05, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_05, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_05, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_05, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_05, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_06
    ui->dashboard_speed_seg_06 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_06, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_06, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_06, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_06, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_06, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_06, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_06, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_06, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_06, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_06, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_06, 401, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_06, 24, 6);

    //Write style for dashboard_speed_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_06, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_06, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_06, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_06, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_06, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_06, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_06, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_06, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_06, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_06, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_06, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_07
    ui->dashboard_speed_seg_07 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_07, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_07, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_07, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_07, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_07, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_07, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_07, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_07, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_07, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_07, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_07, 427, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_07, 24, 6);

    //Write style for dashboard_speed_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_07, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_07, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_07, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_07, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_07, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_07, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_07, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_07, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_07, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_07, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_07, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_08
    ui->dashboard_speed_seg_08 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_08, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_08, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_08, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_08, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_08, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_08, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_08, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_08, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_08, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_08, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_08, 453, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_08, 24, 6);

    //Write style for dashboard_speed_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_08, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_08, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_08, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_08, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_08, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_08, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_08, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_08, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_08, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_08, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_08, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_09
    ui->dashboard_speed_seg_09 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_09, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_09, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_09, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_09, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_09, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_09, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_09, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_09, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_09, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_09, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_09, 479, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_09, 24, 6);

    //Write style for dashboard_speed_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_09, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_09, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_09, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_09, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_09, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_09, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_09, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_09, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_09, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_09, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_09, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_10
    ui->dashboard_speed_seg_10 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_10, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_10, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_10, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_10, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_10, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_10, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_10, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_10, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_10, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_10, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_10, 505, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_10, 24, 6);

    //Write style for dashboard_speed_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_10, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_10, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_10, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_10, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_10, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_10, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_10, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_10, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_10, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_10, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_seg_11
    ui->dashboard_speed_seg_11 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_seg_11, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_seg_11, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_seg_11, "");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_seg_11, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_seg_11, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_seg_11, false);
    lv_textarea_set_one_line(ui->dashboard_speed_seg_11, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_seg_11, "");
    lv_textarea_set_max_length(ui->dashboard_speed_seg_11, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_seg_11, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_seg_11, 531, 320);
    lv_obj_set_size(ui->dashboard_speed_seg_11, 24, 6);

    //Write style for dashboard_speed_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_seg_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_seg_11, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_seg_11, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_11, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_11, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_seg_11, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_seg_11, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_seg_11, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_seg_11, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_seg_11, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_min
    ui->dashboard_speed_min = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_min, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_min, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_min, "0");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_min, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_min, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_min, false);
    lv_textarea_set_one_line(ui->dashboard_speed_min, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_min, "");
    lv_textarea_set_max_length(ui->dashboard_speed_min, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_min, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_min, 240, 332);
    lv_obj_set_size(ui->dashboard_speed_min, 20, 20);

    //Write style for dashboard_speed_min, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_min, lv_color_hex(0x4A5358), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_min, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_min, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_min, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_min, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_min, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_min, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_min, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_min, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_speed_max
    ui->dashboard_speed_max = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_speed_max, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_speed_max, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_speed_max, "MAX 60");
    lv_textarea_set_placeholder_text(ui->dashboard_speed_max, "");
    lv_textarea_set_password_bullet(ui->dashboard_speed_max, "*");
    lv_textarea_set_password_mode(ui->dashboard_speed_max, false);
    lv_textarea_set_one_line(ui->dashboard_speed_max, true);
    lv_textarea_set_accepted_chars(ui->dashboard_speed_max, "");
    lv_textarea_set_max_length(ui->dashboard_speed_max, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_speed_max, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_speed_max, 491, 332);
    lv_obj_set_size(ui->dashboard_speed_max, 80, 20);

    //Write style for dashboard_speed_max, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_speed_max, lv_color_hex(0x4A5358), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_speed_max, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_speed_max, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_speed_max, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_max, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_speed_max, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_speed_max, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_speed_max, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_speed_max, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_speed_max, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_sep
    ui->dashboard_power_sep = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_sep, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_sep, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_sep, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_sep, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_sep, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_sep, false);
    lv_textarea_set_one_line(ui->dashboard_power_sep, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_sep, "");
    lv_textarea_set_max_length(ui->dashboard_power_sep, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_sep, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_sep, 620, 32);
    lv_obj_set_size(ui->dashboard_power_sep, 1, 368);

    //Write style for dashboard_power_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_sep, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_sep, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_sep, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_sep, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_sep, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_sep, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_sep, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_label
    ui->dashboard_power_label = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_label, "POWER");
    lv_textarea_set_placeholder_text(ui->dashboard_power_label, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_label, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_label, false);
    lv_textarea_set_one_line(ui->dashboard_power_label, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_label, "");
    lv_textarea_set_max_length(ui->dashboard_power_label, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_label, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_label, 636, 50);
    lv_obj_set_size(ui->dashboard_power_label, 144, 16);

    //Write style for dashboard_power_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_label, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_label, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_label, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_label, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_label, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_label, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_value
    ui->dashboard_power_value = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_value, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_value, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_value, "1.0");
    lv_textarea_set_placeholder_text(ui->dashboard_power_value, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_value, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_value, false);
    lv_textarea_set_one_line(ui->dashboard_power_value, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_value, "");
    lv_textarea_set_max_length(ui->dashboard_power_value, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_value, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_value, 610, 73);
    lv_obj_set_size(ui->dashboard_power_value, 112, 70);

    //Write style for dashboard_power_value, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_value, lv_color_hex(0xB6FF2E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_value, &lv_font_Antonio_Regular_64, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_value, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_value, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_value, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_value, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_value, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_value, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_unit
    ui->dashboard_power_unit = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_unit, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_unit, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_unit, "kW");
    lv_textarea_set_placeholder_text(ui->dashboard_power_unit, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_unit, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_unit, false);
    lv_textarea_set_one_line(ui->dashboard_power_unit, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_unit, "");
    lv_textarea_set_max_length(ui->dashboard_power_unit, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_unit, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_unit, 724, 96);
    lv_obj_set_size(ui->dashboard_power_unit, 60, 24);

    //Write style for dashboard_power_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_unit, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_unit, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_unit, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_unit, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_unit, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_unit, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Current_text
    ui->dashboard_Current_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_Current_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_Current_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_Current_text, "18.4 A");
    lv_textarea_set_placeholder_text(ui->dashboard_Current_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Current_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Current_text, false);
    lv_textarea_set_one_line(ui->dashboard_Current_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Current_text, "");
    lv_textarea_set_max_length(ui->dashboard_Current_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Current_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Current_text, 636, 140);
    lv_obj_set_size(ui->dashboard_Current_text, 144, 30);

    //Write style for dashboard_Current_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Current_text, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Current_text, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Current_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Current_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Current_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Current_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Current_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Current_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Current_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Current_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_00
    ui->dashboard_power_seg_00 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_00, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_00, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_00, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_00, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_00, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_00, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_00, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_00, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_00, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_00, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_00, 636, 170);
    lv_obj_set_size(ui->dashboard_power_seg_00, 144, 12);

    //Write style for dashboard_power_seg_00, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_00, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_00, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_00, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_00, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_00, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_00, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_00, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_00, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_00, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_00, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_00, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_00, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_00, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_01
    ui->dashboard_power_seg_01 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_01, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_01, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_01, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_01, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_01, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_01, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_01, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_01, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_01, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_01, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_01, 636, 185);
    lv_obj_set_size(ui->dashboard_power_seg_01, 144, 12);

    //Write style for dashboard_power_seg_01, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_01, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_01, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_01, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_01, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_01, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_01, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_01, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_01, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_01, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_01, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_01, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_01, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_01, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_02
    ui->dashboard_power_seg_02 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_02, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_02, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_02, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_02, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_02, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_02, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_02, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_02, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_02, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_02, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_02, 636, 200);
    lv_obj_set_size(ui->dashboard_power_seg_02, 144, 12);

    //Write style for dashboard_power_seg_02, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_02, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_02, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_02, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_02, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_02, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_02, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_02, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_02, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_02, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_02, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_02, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_02, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_02, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_03
    ui->dashboard_power_seg_03 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_03, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_03, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_03, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_03, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_03, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_03, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_03, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_03, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_03, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_03, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_03, 636, 215);
    lv_obj_set_size(ui->dashboard_power_seg_03, 144, 12);

    //Write style for dashboard_power_seg_03, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_03, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_03, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_03, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_03, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_03, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_03, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_03, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_03, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_03, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_03, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_03, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_03, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_03, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_04
    ui->dashboard_power_seg_04 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_04, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_04, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_04, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_04, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_04, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_04, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_04, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_04, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_04, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_04, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_04, 636, 230);
    lv_obj_set_size(ui->dashboard_power_seg_04, 144, 12);

    //Write style for dashboard_power_seg_04, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_04, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_04, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_04, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_04, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_04, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_04, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_04, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_04, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_04, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_04, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_04, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_04, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_04, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_05
    ui->dashboard_power_seg_05 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_05, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_05, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_05, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_05, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_05, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_05, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_05, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_05, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_05, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_05, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_05, 636, 245);
    lv_obj_set_size(ui->dashboard_power_seg_05, 144, 12);

    //Write style for dashboard_power_seg_05, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_05, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_05, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_05, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_05, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_05, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_05, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_05, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_05, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_05, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_05, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_05, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_05, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_05, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_06
    ui->dashboard_power_seg_06 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_06, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_06, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_06, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_06, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_06, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_06, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_06, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_06, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_06, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_06, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_06, 636, 260);
    lv_obj_set_size(ui->dashboard_power_seg_06, 144, 12);

    //Write style for dashboard_power_seg_06, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_06, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_06, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_06, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_06, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_06, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_06, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_06, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_06, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_06, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_06, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_06, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_06, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_06, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_07
    ui->dashboard_power_seg_07 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_07, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_07, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_07, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_07, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_07, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_07, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_07, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_07, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_07, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_07, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_07, 636, 275);
    lv_obj_set_size(ui->dashboard_power_seg_07, 144, 12);

    //Write style for dashboard_power_seg_07, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_07, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_07, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_07, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_07, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_07, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_07, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_07, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_07, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_07, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_07, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_07, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_07, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_07, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_08
    ui->dashboard_power_seg_08 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_08, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_08, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_08, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_08, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_08, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_08, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_08, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_08, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_08, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_08, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_08, 636, 290);
    lv_obj_set_size(ui->dashboard_power_seg_08, 144, 12);

    //Write style for dashboard_power_seg_08, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_08, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_08, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_08, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_08, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_08, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_08, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_08, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_08, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_08, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_08, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_08, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_08, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_08, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_09
    ui->dashboard_power_seg_09 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_09, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_09, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_09, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_09, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_09, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_09, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_09, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_09, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_09, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_09, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_09, 636, 305);
    lv_obj_set_size(ui->dashboard_power_seg_09, 144, 12);

    //Write style for dashboard_power_seg_09, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_09, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_09, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_09, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_09, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_09, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_09, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_09, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_09, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_09, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_09, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_09, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_09, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_09, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_10
    ui->dashboard_power_seg_10 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_10, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_10, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_10, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_10, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_10, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_10, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_10, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_10, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_10, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_10, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_10, 636, 320);
    lv_obj_set_size(ui->dashboard_power_seg_10, 144, 12);

    //Write style for dashboard_power_seg_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_10, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_10, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_10, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_10, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_10, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_10, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_10, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_10, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_10, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_10, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_11
    ui->dashboard_power_seg_11 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_11, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_11, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_11, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_11, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_11, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_11, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_11, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_11, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_11, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_11, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_11, 636, 335);
    lv_obj_set_size(ui->dashboard_power_seg_11, 144, 12);

    //Write style for dashboard_power_seg_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_11, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_11, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_11, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_11, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_11, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_11, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_11, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_11, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_11, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_12
    ui->dashboard_power_seg_12 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_12, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_12, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_12, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_12, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_12, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_12, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_12, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_12, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_12, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_12, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_12, 636, 350);
    lv_obj_set_size(ui->dashboard_power_seg_12, 144, 12);

    //Write style for dashboard_power_seg_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_12, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_12, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_12, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_12, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_12, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_12, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_12, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_12, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_12, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_12, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_12, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_seg_13
    ui->dashboard_power_seg_13 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_seg_13, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_seg_13, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_seg_13, "");
    lv_textarea_set_placeholder_text(ui->dashboard_power_seg_13, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_seg_13, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_seg_13, false);
    lv_textarea_set_one_line(ui->dashboard_power_seg_13, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_seg_13, "");
    lv_textarea_set_max_length(ui->dashboard_power_seg_13, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_seg_13, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_seg_13, 636, 365);
    lv_obj_set_size(ui->dashboard_power_seg_13, 144, 12);

    //Write style for dashboard_power_seg_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_seg_13, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_seg_13, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_seg_13, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_13, lv_color_hex(0x161B1E), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_13, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_seg_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_13, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_seg_13, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_seg_13, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_seg_13, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_seg_13, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_seg_13, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_max_label
    ui->dashboard_power_max_label = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_max_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_max_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_max_label, "MAX");
    lv_textarea_set_placeholder_text(ui->dashboard_power_max_label, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_max_label, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_max_label, false);
    lv_textarea_set_one_line(ui->dashboard_power_max_label, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_max_label, "");
    lv_textarea_set_max_length(ui->dashboard_power_max_label, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_max_label, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_max_label, 636, 380);
    lv_obj_set_size(ui->dashboard_power_max_label, 60, 30);

    //Write style for dashboard_power_max_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_max_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_max_label, &lv_font_montserratMedium_14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_max_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_max_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_max_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_max_label, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_max_label, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_max_label, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_max_label, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_max_label, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_power_max_val
    ui->dashboard_power_max_val = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_power_max_val, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_power_max_val, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_power_max_val, "4.5 KW");
    lv_textarea_set_placeholder_text(ui->dashboard_power_max_val, "");
    lv_textarea_set_password_bullet(ui->dashboard_power_max_val, "*");
    lv_textarea_set_password_mode(ui->dashboard_power_max_val, false);
    lv_textarea_set_one_line(ui->dashboard_power_max_val, true);
    lv_textarea_set_accepted_chars(ui->dashboard_power_max_val, "");
    lv_textarea_set_max_length(ui->dashboard_power_max_val, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_power_max_val, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_power_max_val, 696, 380);
    lv_obj_set_size(ui->dashboard_power_max_val, 84, 30);

    //Write style for dashboard_power_max_val, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_power_max_val, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_power_max_val, &lv_font_montserratMedium_14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_power_max_val, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_power_max_val, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_max_val, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_power_max_val, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_power_max_val, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_power_max_val, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_power_max_val, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_power_max_val, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_bottom_bg
    ui->dashboard_bottom_bg = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_bottom_bg, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_bottom_bg, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_bottom_bg, "");
    lv_textarea_set_placeholder_text(ui->dashboard_bottom_bg, "");
    lv_textarea_set_password_bullet(ui->dashboard_bottom_bg, "*");
    lv_textarea_set_password_mode(ui->dashboard_bottom_bg, false);
    lv_textarea_set_one_line(ui->dashboard_bottom_bg, true);
    lv_textarea_set_accepted_chars(ui->dashboard_bottom_bg, "");
    lv_textarea_set_max_length(ui->dashboard_bottom_bg, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_bottom_bg, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_bottom_bg, 0, 400);
    lv_obj_set_size(ui->dashboard_bottom_bg, 800, 80);

    //Write style for dashboard_bottom_bg, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_bottom_bg, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_bottom_bg, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_bottom_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_bottom_bg, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_bg, lv_color_hex(0x0D1113), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_bg, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_bottom_bg, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_bg, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_bg, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_bg, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_bg, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_bottom_top_sep
    ui->dashboard_bottom_top_sep = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_bottom_top_sep, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_bottom_top_sep, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_bottom_top_sep, "");
    lv_textarea_set_placeholder_text(ui->dashboard_bottom_top_sep, "");
    lv_textarea_set_password_bullet(ui->dashboard_bottom_top_sep, "*");
    lv_textarea_set_password_mode(ui->dashboard_bottom_top_sep, false);
    lv_textarea_set_one_line(ui->dashboard_bottom_top_sep, true);
    lv_textarea_set_accepted_chars(ui->dashboard_bottom_top_sep, "");
    lv_textarea_set_max_length(ui->dashboard_bottom_top_sep, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_bottom_top_sep, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_bottom_top_sep, 0, 400);
    lv_obj_set_size(ui->dashboard_bottom_top_sep, 800, 1);

    //Write style for dashboard_bottom_top_sep, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_bottom_top_sep, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_bottom_top_sep, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_bottom_top_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_bottom_top_sep, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_top_sep, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_top_sep, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_top_sep, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_top_sep, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_bottom_top_sep, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_top_sep, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_top_sep, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_top_sep, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_top_sep, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_bottom_col_sep_0
    ui->dashboard_bottom_col_sep_0 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_bottom_col_sep_0, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_bottom_col_sep_0, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_bottom_col_sep_0, "");
    lv_textarea_set_placeholder_text(ui->dashboard_bottom_col_sep_0, "");
    lv_textarea_set_password_bullet(ui->dashboard_bottom_col_sep_0, "*");
    lv_textarea_set_password_mode(ui->dashboard_bottom_col_sep_0, false);
    lv_textarea_set_one_line(ui->dashboard_bottom_col_sep_0, true);
    lv_textarea_set_accepted_chars(ui->dashboard_bottom_col_sep_0, "");
    lv_textarea_set_max_length(ui->dashboard_bottom_col_sep_0, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_bottom_col_sep_0, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_bottom_col_sep_0, 160, 400);
    lv_obj_set_size(ui->dashboard_bottom_col_sep_0, 1, 80);

    //Write style for dashboard_bottom_col_sep_0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_bottom_col_sep_0, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_bottom_col_sep_0, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_bottom_col_sep_0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_bottom_col_sep_0, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_col_sep_0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_col_sep_0, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_col_sep_0, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_col_sep_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_bottom_col_sep_0, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_col_sep_0, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_col_sep_0, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_col_sep_0, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_col_sep_0, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_bottom_col_sep_1
    ui->dashboard_bottom_col_sep_1 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_bottom_col_sep_1, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_bottom_col_sep_1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_bottom_col_sep_1, "");
    lv_textarea_set_placeholder_text(ui->dashboard_bottom_col_sep_1, "");
    lv_textarea_set_password_bullet(ui->dashboard_bottom_col_sep_1, "*");
    lv_textarea_set_password_mode(ui->dashboard_bottom_col_sep_1, false);
    lv_textarea_set_one_line(ui->dashboard_bottom_col_sep_1, true);
    lv_textarea_set_accepted_chars(ui->dashboard_bottom_col_sep_1, "");
    lv_textarea_set_max_length(ui->dashboard_bottom_col_sep_1, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_bottom_col_sep_1, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_bottom_col_sep_1, 320, 400);
    lv_obj_set_size(ui->dashboard_bottom_col_sep_1, 1, 80);

    //Write style for dashboard_bottom_col_sep_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_bottom_col_sep_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_bottom_col_sep_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_bottom_col_sep_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_bottom_col_sep_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_col_sep_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_col_sep_1, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_col_sep_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_col_sep_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_bottom_col_sep_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_col_sep_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_col_sep_1, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_col_sep_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_col_sep_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_bottom_col_sep_2
    ui->dashboard_bottom_col_sep_2 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_bottom_col_sep_2, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_bottom_col_sep_2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_bottom_col_sep_2, "");
    lv_textarea_set_placeholder_text(ui->dashboard_bottom_col_sep_2, "");
    lv_textarea_set_password_bullet(ui->dashboard_bottom_col_sep_2, "*");
    lv_textarea_set_password_mode(ui->dashboard_bottom_col_sep_2, false);
    lv_textarea_set_one_line(ui->dashboard_bottom_col_sep_2, true);
    lv_textarea_set_accepted_chars(ui->dashboard_bottom_col_sep_2, "");
    lv_textarea_set_max_length(ui->dashboard_bottom_col_sep_2, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_bottom_col_sep_2, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_bottom_col_sep_2, 480, 400);
    lv_obj_set_size(ui->dashboard_bottom_col_sep_2, 1, 80);

    //Write style for dashboard_bottom_col_sep_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_bottom_col_sep_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_bottom_col_sep_2, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_bottom_col_sep_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_bottom_col_sep_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_col_sep_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_col_sep_2, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_col_sep_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_col_sep_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_bottom_col_sep_2, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_col_sep_2, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_col_sep_2, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_col_sep_2, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_col_sep_2, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_bottom_col_sep_3
    ui->dashboard_bottom_col_sep_3 = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_bottom_col_sep_3, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_bottom_col_sep_3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_bottom_col_sep_3, "");
    lv_textarea_set_placeholder_text(ui->dashboard_bottom_col_sep_3, "");
    lv_textarea_set_password_bullet(ui->dashboard_bottom_col_sep_3, "*");
    lv_textarea_set_password_mode(ui->dashboard_bottom_col_sep_3, false);
    lv_textarea_set_one_line(ui->dashboard_bottom_col_sep_3, true);
    lv_textarea_set_accepted_chars(ui->dashboard_bottom_col_sep_3, "");
    lv_textarea_set_max_length(ui->dashboard_bottom_col_sep_3, 1);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_bottom_col_sep_3, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_bottom_col_sep_3, 640, 400);
    lv_obj_set_size(ui->dashboard_bottom_col_sep_3, 1, 80);

    //Write style for dashboard_bottom_col_sep_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_bottom_col_sep_3, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_bottom_col_sep_3, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_bottom_col_sep_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_bottom_col_sep_3, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_col_sep_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_col_sep_3, lv_color_hex(0x1F2629), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_col_sep_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_col_sep_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_bottom_col_sep_3, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_bottom_col_sep_3, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_bottom_col_sep_3, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_bottom_col_sep_3, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_bottom_col_sep_3, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_trip_label
    ui->dashboard_col_trip_label = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_trip_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_trip_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_trip_label, "TRIP");
    lv_textarea_set_placeholder_text(ui->dashboard_col_trip_label, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_trip_label, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_trip_label, false);
    lv_textarea_set_one_line(ui->dashboard_col_trip_label, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_trip_label, "");
    lv_textarea_set_max_length(ui->dashboard_col_trip_label, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_trip_label, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_trip_label, 0, 414);
    lv_obj_set_size(ui->dashboard_col_trip_label, 160, 12);

    //Write style for dashboard_col_trip_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_trip_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_trip_label, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_trip_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_trip_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_trip_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_trip_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_trip_label, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_trip_label, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_trip_label, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_trip_label, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_trip_label, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_TRIP_text
    ui->dashboard_TRIP_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_TRIP_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_TRIP_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_TRIP_text, "14.2");
    lv_textarea_set_placeholder_text(ui->dashboard_TRIP_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_TRIP_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_TRIP_text, false);
    lv_textarea_set_one_line(ui->dashboard_TRIP_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_TRIP_text, "");
    lv_textarea_set_max_length(ui->dashboard_TRIP_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_TRIP_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_TRIP_text, 0, 432);
    lv_obj_set_size(ui->dashboard_TRIP_text, 100, 60);

    //Write style for dashboard_TRIP_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_TRIP_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_TRIP_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_TRIP_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_TRIP_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_TRIP_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_TRIP_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_TRIP_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_TRIP_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_TRIP_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_TRIP_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_trip_unit
    ui->dashboard_col_trip_unit = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_trip_unit, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_trip_unit, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_trip_unit, "KM");
    lv_textarea_set_placeholder_text(ui->dashboard_col_trip_unit, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_trip_unit, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_trip_unit, false);
    lv_textarea_set_one_line(ui->dashboard_col_trip_unit, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_trip_unit, "");
    lv_textarea_set_max_length(ui->dashboard_col_trip_unit, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_trip_unit, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_trip_unit, 104, 442);
    lv_obj_set_size(ui->dashboard_col_trip_unit, 56, 14);

    //Write style for dashboard_col_trip_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_trip_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_trip_unit, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_trip_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_trip_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_trip_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_trip_unit, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_trip_unit, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_trip_unit, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_trip_unit, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_trip_unit, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_odo_label
    ui->dashboard_col_odo_label = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_odo_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_odo_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_odo_label, "ODO");
    lv_textarea_set_placeholder_text(ui->dashboard_col_odo_label, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_odo_label, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_odo_label, false);
    lv_textarea_set_one_line(ui->dashboard_col_odo_label, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_odo_label, "");
    lv_textarea_set_max_length(ui->dashboard_col_odo_label, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_odo_label, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_odo_label, 160, 414);
    lv_obj_set_size(ui->dashboard_col_odo_label, 160, 12);

    //Write style for dashboard_col_odo_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_odo_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_odo_label, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_odo_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_odo_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_odo_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_odo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_odo_label, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_odo_label, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_odo_label, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_odo_label, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_odo_label, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_odo_text
    ui->dashboard_odo_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_odo_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_odo_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_odo_text, "4128");
    lv_textarea_set_placeholder_text(ui->dashboard_odo_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_odo_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_odo_text, false);
    lv_textarea_set_one_line(ui->dashboard_odo_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_odo_text, "");
    lv_textarea_set_max_length(ui->dashboard_odo_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_odo_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_odo_text, 160, 432);
    lv_obj_set_size(ui->dashboard_odo_text, 100, 60);

    //Write style for dashboard_odo_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_odo_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_odo_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_odo_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_odo_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_odo_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_odo_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_odo_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_odo_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_odo_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_odo_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_odo_unit
    ui->dashboard_col_odo_unit = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_odo_unit, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_odo_unit, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_odo_unit, "KM");
    lv_textarea_set_placeholder_text(ui->dashboard_col_odo_unit, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_odo_unit, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_odo_unit, false);
    lv_textarea_set_one_line(ui->dashboard_col_odo_unit, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_odo_unit, "");
    lv_textarea_set_max_length(ui->dashboard_col_odo_unit, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_odo_unit, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_odo_unit, 264, 442);
    lv_obj_set_size(ui->dashboard_col_odo_unit, 56, 14);

    //Write style for dashboard_col_odo_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_odo_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_odo_unit, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_odo_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_odo_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_odo_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_odo_unit, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_odo_unit, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_odo_unit, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_odo_unit, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_odo_unit, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_mtmp_label
    ui->dashboard_col_mtmp_label = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_mtmp_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_mtmp_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_mtmp_label, "M·TEMP");
    lv_textarea_set_placeholder_text(ui->dashboard_col_mtmp_label, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_mtmp_label, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_mtmp_label, false);
    lv_textarea_set_one_line(ui->dashboard_col_mtmp_label, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_mtmp_label, "");
    lv_textarea_set_max_length(ui->dashboard_col_mtmp_label, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_mtmp_label, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_mtmp_label, 320, 414);
    lv_obj_set_size(ui->dashboard_col_mtmp_label, 160, 12);

    //Write style for dashboard_col_mtmp_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_mtmp_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_mtmp_label, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_mtmp_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_mtmp_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_mtmp_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_mtmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_mtmp_label, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_mtmp_label, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_mtmp_label, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_mtmp_label, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_mtmp_label, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_temp_mot_text
    ui->dashboard_temp_mot_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_temp_mot_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_temp_mot_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_temp_mot_text, "62");
    lv_textarea_set_placeholder_text(ui->dashboard_temp_mot_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_temp_mot_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_temp_mot_text, false);
    lv_textarea_set_one_line(ui->dashboard_temp_mot_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_temp_mot_text, "");
    lv_textarea_set_max_length(ui->dashboard_temp_mot_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_temp_mot_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_temp_mot_text, 320, 432);
    lv_obj_set_size(ui->dashboard_temp_mot_text, 100, 60);

    //Write style for dashboard_temp_mot_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_temp_mot_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_temp_mot_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_temp_mot_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_temp_mot_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_temp_mot_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_temp_mot_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_temp_mot_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_temp_mot_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_temp_mot_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_temp_mot_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_mtmp_unit
    ui->dashboard_col_mtmp_unit = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_mtmp_unit, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_mtmp_unit, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_mtmp_unit, "°C");
    lv_textarea_set_placeholder_text(ui->dashboard_col_mtmp_unit, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_mtmp_unit, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_mtmp_unit, false);
    lv_textarea_set_one_line(ui->dashboard_col_mtmp_unit, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_mtmp_unit, "");
    lv_textarea_set_max_length(ui->dashboard_col_mtmp_unit, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_mtmp_unit, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_mtmp_unit, 424, 442);
    lv_obj_set_size(ui->dashboard_col_mtmp_unit, 56, 14);

    //Write style for dashboard_col_mtmp_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_mtmp_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_mtmp_unit, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_mtmp_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_mtmp_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_mtmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_mtmp_unit, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_mtmp_unit, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_mtmp_unit, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_mtmp_unit, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_mtmp_unit, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_ctmp_label
    ui->dashboard_col_ctmp_label = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_ctmp_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_ctmp_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_ctmp_label, "C·TEMP");
    lv_textarea_set_placeholder_text(ui->dashboard_col_ctmp_label, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_ctmp_label, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_ctmp_label, false);
    lv_textarea_set_one_line(ui->dashboard_col_ctmp_label, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_ctmp_label, "");
    lv_textarea_set_max_length(ui->dashboard_col_ctmp_label, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_ctmp_label, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_ctmp_label, 480, 414);
    lv_obj_set_size(ui->dashboard_col_ctmp_label, 160, 12);

    //Write style for dashboard_col_ctmp_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_ctmp_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_ctmp_label, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_ctmp_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_ctmp_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_ctmp_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_ctmp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_ctmp_label, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_ctmp_label, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_ctmp_label, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_ctmp_label, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_ctmp_label, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_temp_esc_text
    ui->dashboard_temp_esc_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_temp_esc_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_temp_esc_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_temp_esc_text, "48");
    lv_textarea_set_placeholder_text(ui->dashboard_temp_esc_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_temp_esc_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_temp_esc_text, false);
    lv_textarea_set_one_line(ui->dashboard_temp_esc_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_temp_esc_text, "");
    lv_textarea_set_max_length(ui->dashboard_temp_esc_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_temp_esc_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_temp_esc_text, 480, 432);
    lv_obj_set_size(ui->dashboard_temp_esc_text, 100, 60);

    //Write style for dashboard_temp_esc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_temp_esc_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_temp_esc_text, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_temp_esc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_temp_esc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_temp_esc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_temp_esc_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_temp_esc_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_temp_esc_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_temp_esc_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_temp_esc_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_ctmp_unit
    ui->dashboard_col_ctmp_unit = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_ctmp_unit, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_ctmp_unit, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_ctmp_unit, "°C");
    lv_textarea_set_placeholder_text(ui->dashboard_col_ctmp_unit, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_ctmp_unit, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_ctmp_unit, false);
    lv_textarea_set_one_line(ui->dashboard_col_ctmp_unit, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_ctmp_unit, "");
    lv_textarea_set_max_length(ui->dashboard_col_ctmp_unit, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_ctmp_unit, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_ctmp_unit, 584, 442);
    lv_obj_set_size(ui->dashboard_col_ctmp_unit, 56, 14);

    //Write style for dashboard_col_ctmp_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_ctmp_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_ctmp_unit, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_ctmp_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_ctmp_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_ctmp_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_ctmp_unit, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_ctmp_unit, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_ctmp_unit, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_ctmp_unit, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_ctmp_unit, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_avg_label
    ui->dashboard_col_avg_label = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_avg_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_avg_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_avg_label, "AVG");
    lv_textarea_set_placeholder_text(ui->dashboard_col_avg_label, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_avg_label, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_avg_label, false);
    lv_textarea_set_one_line(ui->dashboard_col_avg_label, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_avg_label, "");
    lv_textarea_set_max_length(ui->dashboard_col_avg_label, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_avg_label, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_avg_label, 640, 414);
    lv_obj_set_size(ui->dashboard_col_avg_label, 160, 12);

    //Write style for dashboard_col_avg_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_avg_label, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_avg_label, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_avg_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_avg_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_avg_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_avg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_avg_label, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_avg_label, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_avg_label, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_avg_label, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_avg_label, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_avg_value
    ui->dashboard_col_avg_value = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_avg_value, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_avg_value, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_avg_value, "24");
    lv_textarea_set_placeholder_text(ui->dashboard_col_avg_value, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_avg_value, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_avg_value, false);
    lv_textarea_set_one_line(ui->dashboard_col_avg_value, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_avg_value, "");
    lv_textarea_set_max_length(ui->dashboard_col_avg_value, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_avg_value, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_avg_value, 640, 432);
    lv_obj_set_size(ui->dashboard_col_avg_value, 100, 60);

    //Write style for dashboard_col_avg_value, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_avg_value, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_avg_value, &lv_font_Antonio_Regular_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_avg_value, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_avg_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_avg_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_avg_value, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_avg_value, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_avg_value, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_avg_value, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_avg_value, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_col_avg_unit
    ui->dashboard_col_avg_unit = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_col_avg_unit, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_col_avg_unit, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_col_avg_unit, "KM/H");
    lv_textarea_set_placeholder_text(ui->dashboard_col_avg_unit, "");
    lv_textarea_set_password_bullet(ui->dashboard_col_avg_unit, "*");
    lv_textarea_set_password_mode(ui->dashboard_col_avg_unit, false);
    lv_textarea_set_one_line(ui->dashboard_col_avg_unit, true);
    lv_textarea_set_accepted_chars(ui->dashboard_col_avg_unit, "");
    lv_textarea_set_max_length(ui->dashboard_col_avg_unit, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_col_avg_unit, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_col_avg_unit, 744, 442);
    lv_obj_set_size(ui->dashboard_col_avg_unit, 56, 14);

    //Write style for dashboard_col_avg_unit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_col_avg_unit, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_col_avg_unit, &lv_font_montserratMedium_11, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_col_avg_unit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_col_avg_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_avg_unit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_col_avg_unit, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_col_avg_unit, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_col_avg_unit, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_col_avg_unit, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_col_avg_unit, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_cruise_control_img
    ui->dashboard_cruise_control_img = lv_img_create(ui->dashboard);
    lv_obj_add_flag(ui->dashboard_cruise_control_img, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->dashboard_cruise_control_img, &_cruise_control_alpha_38x38);
    lv_img_set_pivot(ui->dashboard_cruise_control_img, 0,0);
    lv_img_set_angle(ui->dashboard_cruise_control_img, 0);
    lv_obj_set_pos(ui->dashboard_cruise_control_img, 565, 45);
    lv_obj_set_size(ui->dashboard_cruise_control_img, 38, 38);

    //Write style for dashboard_cruise_control_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->dashboard_cruise_control_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->dashboard_cruise_control_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_cruise_control_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->dashboard_cruise_control_img, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes dashboard_esc_not_connected_text
    ui->dashboard_esc_not_connected_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_esc_not_connected_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_esc_not_connected_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
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
    lv_obj_set_pos(ui->dashboard_esc_not_connected_text, 250, 5);
    lv_obj_set_size(ui->dashboard_esc_not_connected_text, 240, 32);
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

    //Write codes dashboard_Ah_text
    ui->dashboard_Ah_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_Ah_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_Ah_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_Ah_text, "14.2 Ah");
    lv_textarea_set_placeholder_text(ui->dashboard_Ah_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Ah_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Ah_text, false);
    lv_textarea_set_one_line(ui->dashboard_Ah_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Ah_text, "");
    lv_textarea_set_max_length(ui->dashboard_Ah_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Ah_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Ah_text, 90, 140);
    lv_obj_set_size(ui->dashboard_Ah_text, 130, 30);

    //Write style for dashboard_Ah_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Ah_text, lv_color_hex(0x8A9499), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Ah_text, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Ah_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Ah_text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Ah_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Ah_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Ah_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Ah_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Ah_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Ah_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes dashboard_Speed_cc_text
    ui->dashboard_Speed_cc_text = lv_textarea_create(ui->dashboard);
    /* cockpit-patch: hide cursor + lock input */
    lv_obj_set_style_opa(ui->dashboard_Speed_cc_text, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_clear_flag(ui->dashboard_Speed_cc_text, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_text(ui->dashboard_Speed_cc_text, "32");
    lv_textarea_set_placeholder_text(ui->dashboard_Speed_cc_text, "");
    lv_textarea_set_password_bullet(ui->dashboard_Speed_cc_text, "*");
    lv_textarea_set_password_mode(ui->dashboard_Speed_cc_text, false);
    lv_textarea_set_one_line(ui->dashboard_Speed_cc_text, true);
    lv_textarea_set_accepted_chars(ui->dashboard_Speed_cc_text, "");
    lv_textarea_set_max_length(ui->dashboard_Speed_cc_text, 64);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->dashboard_Speed_cc_text, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->dashboard_Speed_cc_text, 550, 90);
    lv_obj_set_size(ui->dashboard_Speed_cc_text, 60, 70);

    //Write style for dashboard_Speed_cc_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->dashboard_Speed_cc_text, lv_color_hex(0xE8EDEE), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->dashboard_Speed_cc_text, &lv_font_Antonio_Regular_50, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->dashboard_Speed_cc_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->dashboard_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->dashboard_Speed_cc_text, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->dashboard_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->dashboard_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->dashboard_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->dashboard_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->dashboard_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->dashboard_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Speed_cc_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for dashboard_Speed_cc_text, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->dashboard_Speed_cc_text, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->dashboard_Speed_cc_text, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->dashboard_Speed_cc_text, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->dashboard_Speed_cc_text, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //The custom code of dashboard.


    //Update current screen layout.
    lv_obj_update_layout(ui->dashboard);

    //Init events for screen.
    events_init_dashboard(ui);
}
