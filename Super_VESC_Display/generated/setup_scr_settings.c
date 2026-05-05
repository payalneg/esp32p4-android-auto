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



void setup_scr_settings(lv_ui *ui)
{
    //Write codes settings
    ui->settings = lv_obj_create(NULL);
    lv_obj_set_size(ui->settings, 800, 480);
    lv_obj_set_scrollbar_mode(ui->settings, LV_SCROLLBAR_MODE_OFF);

    //Write style for settings, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->settings, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->settings, lv_color_hex(0x1f1f1f), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->settings, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes settings_exit_button
    ui->settings_exit_button = lv_btn_create(ui->settings);
    ui->settings_exit_button_label = lv_label_create(ui->settings_exit_button);
    lv_label_set_text(ui->settings_exit_button_label, "Back to dashboard");
    lv_label_set_long_mode(ui->settings_exit_button_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->settings_exit_button_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->settings_exit_button, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->settings_exit_button_label, LV_PCT(100));
    lv_obj_set_pos(ui->settings_exit_button, 17, 14);
    lv_obj_set_size(ui->settings_exit_button, 450, 40);

    //Write style for settings_exit_button, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->settings_exit_button, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->settings_exit_button, lv_color_hex(0x2a3440), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->settings_exit_button, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->settings_exit_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->settings_exit_button, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->settings_exit_button, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->settings_exit_button, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->settings_exit_button, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->settings_exit_button, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->settings_exit_button, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of settings.


    //Update current screen layout.
    lv_obj_update_layout(ui->settings);

    //Init events for screen.
    events_init_settings(ui);
}
