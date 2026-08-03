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



lv_calendar_date_t reference_calendar_1_today;
lv_calendar_date_t reference_calendar_1_highlihted_days[1];
void setup_scr_reference(lv_ui *ui)
{
    //Write codes reference
    ui->reference = lv_obj_create(NULL);
    lv_obj_set_size(ui->reference, 800, 480);
    lv_obj_set_scrollbar_mode(ui->reference, LV_SCROLLBAR_MODE_OFF);

    //Write style for reference, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes reference_btn_1
    ui->reference_btn_1 = lv_btn_create(ui->reference);
    ui->reference_btn_1_label = lv_label_create(ui->reference_btn_1);
    lv_label_set_text(ui->reference_btn_1_label, "Button");
    lv_label_set_long_mode(ui->reference_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->reference_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->reference_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->reference_btn_1_label, LV_PCT(100));
    lv_obj_set_pos(ui->reference_btn_1, 493, 57);
    lv_obj_set_size(ui->reference_btn_1, 100, 50);

    //Write style for reference_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_btn_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_btn_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_btn_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->reference_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_btn_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->reference_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes reference_imgbtn_1
    ui->reference_imgbtn_1 = lv_imgbtn_create(ui->reference);
    lv_obj_add_flag(ui->reference_imgbtn_1, LV_OBJ_FLAG_CHECKABLE);
    ui->reference_imgbtn_1_label = lv_label_create(ui->reference_imgbtn_1);
    lv_label_set_text(ui->reference_imgbtn_1_label, "");
    lv_label_set_long_mode(ui->reference_imgbtn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->reference_imgbtn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->reference_imgbtn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_pos(ui->reference_imgbtn_1, 609, 53);
    lv_obj_set_size(ui->reference_imgbtn_1, 100, 50);

    //Write style for reference_imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->reference_imgbtn_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_imgbtn_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->reference_imgbtn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->reference_imgbtn_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
    lv_obj_set_style_img_recolor_opa(ui->reference_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_img_opa(ui->reference_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_color(ui->reference_imgbtn_1, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_font(ui->reference_imgbtn_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_opa(ui->reference_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui->reference_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_PRESSED);

    //Write style for reference_imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
    lv_obj_set_style_img_recolor_opa(ui->reference_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_img_opa(ui->reference_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_color(ui->reference_imgbtn_1, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_font(ui->reference_imgbtn_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_opa(ui->reference_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(ui->reference_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_CHECKED);

    //Write style for reference_imgbtn_1, Part: LV_PART_MAIN, State: LV_IMGBTN_STATE_RELEASED.
    lv_obj_set_style_img_recolor_opa(ui->reference_imgbtn_1, 0, LV_PART_MAIN|LV_IMGBTN_STATE_RELEASED);
    lv_obj_set_style_img_opa(ui->reference_imgbtn_1, 255, LV_PART_MAIN|LV_IMGBTN_STATE_RELEASED);

    //Write codes reference_btnm_1
    ui->reference_btnm_1 = lv_btnmatrix_create(ui->reference);
    static const char *reference_btnm_1_text_map[] = {"1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "",};
    lv_btnmatrix_set_map(ui->reference_btnm_1, reference_btnm_1_text_map);
    lv_obj_set_pos(ui->reference_btnm_1, 453, 108);
    lv_obj_set_size(ui->reference_btnm_1, 200, 150);

    //Write style for reference_btnm_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->reference_btnm_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_btnm_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_btnm_1, lv_color_hex(0xc9c9c9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_btnm_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->reference_btnm_1, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->reference_btnm_1, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_btnm_1, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_btnm_1, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(ui->reference_btnm_1, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(ui->reference_btnm_1, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_btnm_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_btnm_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_btnm_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_btnm_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_btnm_1, Part: LV_PART_ITEMS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->reference_btnm_1, 1, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_btnm_1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_btnm_1, lv_color_hex(0xc9c9c9), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_btnm_1, LV_BORDER_SIDE_FULL, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->reference_btnm_1, lv_color_hex(0xffffff), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_btnm_1, &lv_font_montserratMedium_16, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_btnm_1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_btnm_1, 4, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_btnm_1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_btnm_1, lv_color_hex(0x2195f6), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_btnm_1, LV_GRAD_DIR_NONE, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_btnm_1, 0, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write codes reference_sw_1
    ui->reference_sw_1 = lv_switch_create(ui->reference);
    lv_obj_set_pos(ui->reference_sw_1, 414, 39);
    lv_obj_set_size(ui->reference_sw_1, 40, 20);

    //Write style for reference_sw_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_sw_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_sw_1, lv_color_hex(0xe6e2e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_sw_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_sw_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_sw_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_sw_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_sw_1, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
    lv_obj_set_style_bg_opa(ui->reference_sw_1, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(ui->reference_sw_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_bg_grad_dir(ui->reference_sw_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_border_width(ui->reference_sw_1, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

    //Write style for reference_sw_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_sw_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_sw_1, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_sw_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_sw_1, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_sw_1, 10, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes reference_bar_1
    ui->reference_bar_1 = lv_bar_create(ui->reference);
    lv_obj_set_style_anim_time(ui->reference_bar_1, 1000, 0);
    lv_bar_set_mode(ui->reference_bar_1, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->reference_bar_1, 0, 100);
    lv_bar_set_value(ui->reference_bar_1, 50, LV_ANIM_OFF);
    lv_obj_set_pos(ui->reference_bar_1, 413, 99);
    lv_obj_set_size(ui->reference_bar_1, 90, 20);

    //Write style for reference_bar_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_bar_1, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_bar_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_bar_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_bar_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_bar_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_bar_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_bar_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_bar_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_bar_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_bar_1, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes reference_slider_1
    ui->reference_slider_1 = lv_slider_create(ui->reference);
    lv_slider_set_range(ui->reference_slider_1, 0, 100);
    lv_slider_set_mode(ui->reference_slider_1, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->reference_slider_1, 50, LV_ANIM_OFF);
    lv_obj_set_pos(ui->reference_slider_1, 396, 160);
    lv_obj_set_size(ui->reference_slider_1, 160, 8);

    //Write style for reference_slider_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_slider_1, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_slider_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_slider_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_slider_1, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->reference_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_slider_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_slider_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_slider_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_slider_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_slider_1, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for reference_slider_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_slider_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_slider_1, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_slider_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_slider_1, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes reference_img_1
    ui->reference_img_1 = lv_img_create(ui->reference);
    lv_obj_add_flag(ui->reference_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_pivot(ui->reference_img_1, 50,50);
    lv_img_set_angle(ui->reference_img_1, 0);
    lv_obj_set_pos(ui->reference_img_1, 331, 285);
    lv_obj_set_size(ui->reference_img_1, 100, 100);

    //Write style for reference_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->reference_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->reference_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->reference_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes reference_animimg_1
    ui->reference_animimg_1 = lv_animimg_create(ui->reference);
    lv_animimg_set_src(ui->reference_animimg_1, (const void **) reference_animimg_1_imgs, 0, false);
    lv_animimg_set_duration(ui->reference_animimg_1, 30*0);
    lv_animimg_set_repeat_count(ui->reference_animimg_1, LV_ANIM_REPEAT_INFINITE);
    lv_img_set_src(ui->reference_animimg_1, reference_animimg_1_imgs[0]);
    lv_obj_set_pos(ui->reference_animimg_1, 445, 275);
    lv_obj_set_size(ui->reference_animimg_1, 100, 100);

    //Write codes reference_image3D_1
    ui->reference_image3D_1 = lv_animimg_create(ui->reference);
    lv_animimg_set_src(ui->reference_image3D_1, (const void **) reference_image3D_1_imgs, 0, false);
    lv_animimg_set_duration(ui->reference_image3D_1, 200*0);
    lv_animimg_set_repeat_count(ui->reference_image3D_1, LV_ANIM_REPEAT_INFINITE);
    lv_img_set_src(ui->reference_image3D_1, reference_image3D_1_imgs[0]);
    lv_obj_set_pos(ui->reference_image3D_1, 514, 268);
    lv_obj_set_size(ui->reference_image3D_1, 100, 100);

    //Write codes reference_label_1
    ui->reference_label_1 = lv_label_create(ui->reference);
    lv_label_set_text(ui->reference_label_1, "Label");
    lv_label_set_long_mode(ui->reference_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->reference_label_1, 95, 296);
    lv_obj_set_size(ui->reference_label_1, 100, 32);

    //Write style for reference_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->reference_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->reference_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_label_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->reference_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->reference_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->reference_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->reference_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->reference_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes reference_spangroup_1
    ui->reference_spangroup_1 = lv_spangroup_create(ui->reference);
    lv_spangroup_set_align(ui->reference_spangroup_1, LV_TEXT_ALIGN_LEFT);
    lv_spangroup_set_overflow(ui->reference_spangroup_1, LV_SPAN_OVERFLOW_CLIP);
    lv_spangroup_set_mode(ui->reference_spangroup_1, LV_SPAN_MODE_BREAK);
    //create span
    ui->reference_spangroup_1_span = lv_spangroup_new_span(ui->reference_spangroup_1);
    lv_span_set_text(ui->reference_spangroup_1_span, "hello");
    lv_style_set_text_color(&ui->reference_spangroup_1_span->style, lv_color_hex(0x000000));
    lv_style_set_text_decor(&ui->reference_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->reference_spangroup_1_span->style, &lv_font_montserratMedium_12);
    lv_obj_set_pos(ui->reference_spangroup_1, 231, 262);
    lv_obj_set_size(ui->reference_spangroup_1, 200, 100);

    //Write style state: LV_STATE_DEFAULT for &style_reference_spangroup_1_main_main_default
    static lv_style_t style_reference_spangroup_1_main_main_default;
    ui_init_style(&style_reference_spangroup_1_main_main_default);

    lv_style_set_border_width(&style_reference_spangroup_1_main_main_default, 0);
    lv_style_set_radius(&style_reference_spangroup_1_main_main_default, 0);
    lv_style_set_bg_opa(&style_reference_spangroup_1_main_main_default, 0);
    lv_style_set_pad_top(&style_reference_spangroup_1_main_main_default, 0);
    lv_style_set_pad_right(&style_reference_spangroup_1_main_main_default, 0);
    lv_style_set_pad_bottom(&style_reference_spangroup_1_main_main_default, 0);
    lv_style_set_pad_left(&style_reference_spangroup_1_main_main_default, 0);
    lv_style_set_shadow_width(&style_reference_spangroup_1_main_main_default, 0);
    lv_obj_add_style(ui->reference_spangroup_1, &style_reference_spangroup_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_spangroup_refr_mode(ui->reference_spangroup_1);

    //Write codes reference_ddlist_1
    ui->reference_ddlist_1 = lv_dropdown_create(ui->reference);
    lv_dropdown_set_options(ui->reference_ddlist_1, "list1\nlist2\nlist3");
    lv_obj_set_pos(ui->reference_ddlist_1, 401, 365);
    lv_obj_set_size(ui->reference_ddlist_1, 130, 30);

    //Write style for reference_ddlist_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->reference_ddlist_1, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_ddlist_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_ddlist_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_ddlist_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_ddlist_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_ddlist_1, lv_color_hex(0xe1e6ee), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_ddlist_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->reference_ddlist_1, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_ddlist_1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_ddlist_1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_ddlist_1, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_ddlist_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_ddlist_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_ddlist_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_ddlist_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_CHECKED for &style_reference_ddlist_1_extra_list_selected_checked
    static lv_style_t style_reference_ddlist_1_extra_list_selected_checked;
    ui_init_style(&style_reference_ddlist_1_extra_list_selected_checked);

    lv_style_set_border_width(&style_reference_ddlist_1_extra_list_selected_checked, 1);
    lv_style_set_border_opa(&style_reference_ddlist_1_extra_list_selected_checked, 255);
    lv_style_set_border_color(&style_reference_ddlist_1_extra_list_selected_checked, lv_color_hex(0xe1e6ee));
    lv_style_set_border_side(&style_reference_ddlist_1_extra_list_selected_checked, LV_BORDER_SIDE_FULL);
    lv_style_set_radius(&style_reference_ddlist_1_extra_list_selected_checked, 3);
    lv_style_set_bg_opa(&style_reference_ddlist_1_extra_list_selected_checked, 255);
    lv_style_set_bg_color(&style_reference_ddlist_1_extra_list_selected_checked, lv_color_hex(0x00a1b5));
    lv_style_set_bg_grad_dir(&style_reference_ddlist_1_extra_list_selected_checked, LV_GRAD_DIR_NONE);
    lv_obj_add_style(lv_dropdown_get_list(ui->reference_ddlist_1), &style_reference_ddlist_1_extra_list_selected_checked, LV_PART_SELECTED|LV_STATE_CHECKED);

    //Write style state: LV_STATE_DEFAULT for &style_reference_ddlist_1_extra_list_main_default
    static lv_style_t style_reference_ddlist_1_extra_list_main_default;
    ui_init_style(&style_reference_ddlist_1_extra_list_main_default);

    lv_style_set_max_height(&style_reference_ddlist_1_extra_list_main_default, 90);
    lv_style_set_text_color(&style_reference_ddlist_1_extra_list_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_reference_ddlist_1_extra_list_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_ddlist_1_extra_list_main_default, 255);
    lv_style_set_border_width(&style_reference_ddlist_1_extra_list_main_default, 1);
    lv_style_set_border_opa(&style_reference_ddlist_1_extra_list_main_default, 255);
    lv_style_set_border_color(&style_reference_ddlist_1_extra_list_main_default, lv_color_hex(0xe1e6ee));
    lv_style_set_border_side(&style_reference_ddlist_1_extra_list_main_default, LV_BORDER_SIDE_FULL);
    lv_style_set_radius(&style_reference_ddlist_1_extra_list_main_default, 3);
    lv_style_set_bg_opa(&style_reference_ddlist_1_extra_list_main_default, 255);
    lv_style_set_bg_color(&style_reference_ddlist_1_extra_list_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_reference_ddlist_1_extra_list_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(lv_dropdown_get_list(ui->reference_ddlist_1), &style_reference_ddlist_1_extra_list_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_ddlist_1_extra_list_scrollbar_default
    static lv_style_t style_reference_ddlist_1_extra_list_scrollbar_default;
    ui_init_style(&style_reference_ddlist_1_extra_list_scrollbar_default);

    lv_style_set_radius(&style_reference_ddlist_1_extra_list_scrollbar_default, 3);
    lv_style_set_bg_opa(&style_reference_ddlist_1_extra_list_scrollbar_default, 255);
    lv_style_set_bg_color(&style_reference_ddlist_1_extra_list_scrollbar_default, lv_color_hex(0x00ff00));
    lv_style_set_bg_grad_dir(&style_reference_ddlist_1_extra_list_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(lv_dropdown_get_list(ui->reference_ddlist_1), &style_reference_ddlist_1_extra_list_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes reference_ta_1
    ui->reference_ta_1 = lv_textarea_create(ui->reference);
    lv_textarea_set_text(ui->reference_ta_1, "Hello World");
    lv_textarea_set_placeholder_text(ui->reference_ta_1, "");
    lv_textarea_set_password_bullet(ui->reference_ta_1, "*");
    lv_textarea_set_password_mode(ui->reference_ta_1, false);
    lv_textarea_set_one_line(ui->reference_ta_1, false);
    lv_textarea_set_accepted_chars(ui->reference_ta_1, "");
    lv_textarea_set_max_length(ui->reference_ta_1, 32);
#if LV_USE_KEYBOARD != 0 || LV_USE_ZH_KEYBOARD != 0
    lv_obj_add_event_cb(ui->reference_ta_1, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif
    lv_obj_set_pos(ui->reference_ta_1, 475, 225);
    lv_obj_set_size(ui->reference_ta_1, 200, 60);

    //Write style for reference_ta_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->reference_ta_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_ta_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->reference_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->reference_ta_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_ta_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_ta_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_ta_1, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_ta_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_ta_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->reference_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_ta_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_ta_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_ta_1, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_ta_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_ta_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes reference_cb_1
    ui->reference_cb_1 = lv_checkbox_create(ui->reference);
    lv_checkbox_set_text(ui->reference_cb_1, "checkbox");
    lv_obj_set_pos(ui->reference_cb_1, 680, 221);

    //Write style for reference_cb_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_pad_top(ui->reference_cb_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_cb_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_cb_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->reference_cb_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->reference_cb_1, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_cb_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_cb_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->reference_cb_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_cb_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_cb_1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_cb_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_cb_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_cb_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_cb_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_cb_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_pad_all(ui->reference_cb_1, 3, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_cb_1, 2, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_cb_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_cb_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_cb_1, LV_BORDER_SIDE_FULL, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_cb_1, 6, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_cb_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_cb_1, lv_color_hex(0xffffff), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_cb_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes reference_cont_1
    ui->reference_cont_1 = lv_obj_create(ui->reference);
    lv_obj_set_pos(ui->reference_cont_1, 43, 271);
    lv_obj_set_size(ui->reference_cont_1, 300, 200);
    lv_obj_set_scrollbar_mode(ui->reference_cont_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for reference_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->reference_cont_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_cont_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_cont_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_cont_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_cont_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->reference_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->reference_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes reference_tabview_1
    ui->reference_tabview_1 = lv_tabview_create(ui->reference, LV_DIR_TOP, 50);
    lv_obj_set_pos(ui->reference_tabview_1, 315, 307);
    lv_obj_set_size(ui->reference_tabview_1, 280, 160);
    lv_obj_set_scrollbar_mode(ui->reference_tabview_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for reference_tabview_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_tabview_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_tabview_1, lv_color_hex(0xeaeff3), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_tabview_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->reference_tabview_1, lv_color_hex(0x4d4d4d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_tabview_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_tabview_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->reference_tabview_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->reference_tabview_1, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_tabview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_tabview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_tabview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_tabview_1_extra_btnm_main_default
    static lv_style_t style_reference_tabview_1_extra_btnm_main_default;
    ui_init_style(&style_reference_tabview_1_extra_btnm_main_default);

    lv_style_set_bg_opa(&style_reference_tabview_1_extra_btnm_main_default, 255);
    lv_style_set_bg_color(&style_reference_tabview_1_extra_btnm_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_reference_tabview_1_extra_btnm_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_reference_tabview_1_extra_btnm_main_default, 0);
    lv_style_set_radius(&style_reference_tabview_1_extra_btnm_main_default, 0);
    lv_obj_add_style(lv_tabview_get_tab_btns(ui->reference_tabview_1), &style_reference_tabview_1_extra_btnm_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_tabview_1_extra_btnm_items_default
    static lv_style_t style_reference_tabview_1_extra_btnm_items_default;
    ui_init_style(&style_reference_tabview_1_extra_btnm_items_default);

    lv_style_set_text_color(&style_reference_tabview_1_extra_btnm_items_default, lv_color_hex(0x4d4d4d));
    lv_style_set_text_font(&style_reference_tabview_1_extra_btnm_items_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_tabview_1_extra_btnm_items_default, 255);
    lv_obj_add_style(lv_tabview_get_tab_btns(ui->reference_tabview_1), &style_reference_tabview_1_extra_btnm_items_default, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_CHECKED for &style_reference_tabview_1_extra_btnm_items_checked
    static lv_style_t style_reference_tabview_1_extra_btnm_items_checked;
    ui_init_style(&style_reference_tabview_1_extra_btnm_items_checked);

    lv_style_set_text_color(&style_reference_tabview_1_extra_btnm_items_checked, lv_color_hex(0x2195f6));
    lv_style_set_text_font(&style_reference_tabview_1_extra_btnm_items_checked, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_tabview_1_extra_btnm_items_checked, 255);
    lv_style_set_border_width(&style_reference_tabview_1_extra_btnm_items_checked, 4);
    lv_style_set_border_opa(&style_reference_tabview_1_extra_btnm_items_checked, 255);
    lv_style_set_border_color(&style_reference_tabview_1_extra_btnm_items_checked, lv_color_hex(0x2195f6));
    lv_style_set_border_side(&style_reference_tabview_1_extra_btnm_items_checked, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_radius(&style_reference_tabview_1_extra_btnm_items_checked, 0);
    lv_style_set_bg_opa(&style_reference_tabview_1_extra_btnm_items_checked, 60);
    lv_style_set_bg_color(&style_reference_tabview_1_extra_btnm_items_checked, lv_color_hex(0x2195f6));
    lv_style_set_bg_grad_dir(&style_reference_tabview_1_extra_btnm_items_checked, LV_GRAD_DIR_NONE);
    lv_obj_add_style(lv_tabview_get_tab_btns(ui->reference_tabview_1), &style_reference_tabview_1_extra_btnm_items_checked, LV_PART_ITEMS|LV_STATE_CHECKED);

    //Write codes tab
    ui->reference_tabview_1_tab_1 = lv_tabview_add_tab(ui->reference_tabview_1,"tab");
    lv_obj_t * reference_tabview_1_tab_1_label = lv_label_create(ui->reference_tabview_1_tab_1);
    lv_label_set_text(reference_tabview_1_tab_1_label, "con1");

    //Write codes tab
    ui->reference_tabview_1_tab_2 = lv_tabview_add_tab(ui->reference_tabview_1,"tab");
    lv_obj_t * reference_tabview_1_tab_2_label = lv_label_create(ui->reference_tabview_1_tab_2);
    lv_label_set_text(reference_tabview_1_tab_2_label, "con2");

    //Write codes tab
    ui->reference_tabview_1_tab_3 = lv_tabview_add_tab(ui->reference_tabview_1,"tab");
    lv_obj_t * reference_tabview_1_tab_3_label = lv_label_create(ui->reference_tabview_1_tab_3);
    lv_label_set_text(reference_tabview_1_tab_3_label, "con3");

    //Write codes reference_win_1
    ui->reference_win_1 = lv_win_create(ui->reference, 40);
    lv_obj_t * reference_win_1_title = lv_win_add_title(ui->reference_win_1, "title");
    ui->reference_win_1_item0 = lv_win_add_btn(ui->reference_win_1, LV_SYMBOL_CLOSE, 40);
    lv_obj_t *reference_win_1_label = lv_label_create(lv_win_get_content(ui->reference_win_1));
    lv_label_set_text(reference_win_1_label, "this is a \nlong text \nto show \nscrollbar. \nif \nit \nis not \nlong enough, \nadd more content");
    lv_obj_set_scrollbar_mode(lv_win_get_content(ui->reference_win_1), LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_pos(ui->reference_win_1, 572, 165);
    lv_obj_set_size(ui->reference_win_1, 400, 200);
    lv_obj_set_scrollbar_mode(ui->reference_win_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for reference_win_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_win_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_win_1, lv_color_hex(0xeeeef6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_win_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->reference_win_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_win_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_win_1_extra_content_main_default
    static lv_style_t style_reference_win_1_extra_content_main_default;
    ui_init_style(&style_reference_win_1_extra_content_main_default);

    lv_style_set_bg_opa(&style_reference_win_1_extra_content_main_default, 255);
    lv_style_set_bg_color(&style_reference_win_1_extra_content_main_default, lv_color_hex(0xeeeef6));
    lv_style_set_bg_grad_dir(&style_reference_win_1_extra_content_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_text_color(&style_reference_win_1_extra_content_main_default, lv_color_hex(0x393c41));
    lv_style_set_text_font(&style_reference_win_1_extra_content_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_win_1_extra_content_main_default, 255);
    lv_style_set_text_letter_space(&style_reference_win_1_extra_content_main_default, 0);
    lv_style_set_text_line_space(&style_reference_win_1_extra_content_main_default, 2);
    lv_obj_add_style(lv_win_get_content(ui->reference_win_1), &style_reference_win_1_extra_content_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_win_1_extra_header_main_default
    static lv_style_t style_reference_win_1_extra_header_main_default;
    ui_init_style(&style_reference_win_1_extra_header_main_default);

    lv_style_set_bg_opa(&style_reference_win_1_extra_header_main_default, 255);
    lv_style_set_bg_color(&style_reference_win_1_extra_header_main_default, lv_color_hex(0xe6e6e6));
    lv_style_set_bg_grad_dir(&style_reference_win_1_extra_header_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_text_color(&style_reference_win_1_extra_header_main_default, lv_color_hex(0x393c41));
    lv_style_set_text_font(&style_reference_win_1_extra_header_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_win_1_extra_header_main_default, 255);
    lv_style_set_text_letter_space(&style_reference_win_1_extra_header_main_default, 0);
    lv_style_set_text_line_space(&style_reference_win_1_extra_header_main_default, 2);
    lv_style_set_pad_top(&style_reference_win_1_extra_header_main_default, 5);
    lv_style_set_pad_right(&style_reference_win_1_extra_header_main_default, 5);
    lv_style_set_pad_bottom(&style_reference_win_1_extra_header_main_default, 5);
    lv_style_set_pad_left(&style_reference_win_1_extra_header_main_default, 5);
    lv_style_set_pad_column(&style_reference_win_1_extra_header_main_default, 5);
    lv_obj_add_style(lv_win_get_header(ui->reference_win_1), &style_reference_win_1_extra_header_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_win_1_extra_btns_main_default
    static lv_style_t style_reference_win_1_extra_btns_main_default;
    ui_init_style(&style_reference_win_1_extra_btns_main_default);

    lv_style_set_radius(&style_reference_win_1_extra_btns_main_default, 8);
    lv_style_set_border_width(&style_reference_win_1_extra_btns_main_default, 0);
    lv_style_set_bg_opa(&style_reference_win_1_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_reference_win_1_extra_btns_main_default, lv_color_hex(0x2195f6));
    lv_style_set_bg_grad_dir(&style_reference_win_1_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_shadow_width(&style_reference_win_1_extra_btns_main_default, 0);
    lv_obj_add_style(ui->reference_win_1_item0, &style_reference_win_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes reference_tileview_1
    ui->reference_tileview_1 = lv_tileview_create(ui->reference);
    ui->reference_tileview_1_tile = lv_tileview_add_tile(ui->reference_tileview_1, 0, 0, LV_DIR_RIGHT);
    lv_obj_set_pos(ui->reference_tileview_1, 599, 117);
    lv_obj_set_size(ui->reference_tileview_1, 300, 200);
    lv_obj_set_scrollbar_mode(ui->reference_tileview_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for reference_tileview_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_tileview_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_tileview_1, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_tileview_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_tileview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_tileview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_tileview_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_tileview_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_tileview_1, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_tileview_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_tileview_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes reference_menu_1
    ui->reference_menu_1 = lv_menu_create(ui->reference);

    //Create sidebar page for menu reference_menu_1
    ui->reference_menu_1_sidebar_page = lv_menu_page_create(ui->reference_menu_1, "menu");
    lv_menu_set_sidebar_page(ui->reference_menu_1, ui->reference_menu_1_sidebar_page);
    lv_obj_set_scrollbar_mode(ui->reference_menu_1_sidebar_page, LV_SCROLLBAR_MODE_OFF);

    //Create subpage for reference_menu_1
    ui->reference_menu_1_subpage_1 = lv_menu_page_create(ui->reference_menu_1, NULL);
    ui->reference_menu_1_cont_1 = lv_menu_cont_create(ui->reference_menu_1_sidebar_page);
    ui->reference_menu_1_label_1 = lv_label_create(ui->reference_menu_1_cont_1);
    lv_label_set_text(ui->reference_menu_1_label_1, "item");
    lv_obj_set_size(ui->reference_menu_1_label_1, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_scrollbar_mode(ui->reference_menu_1_subpage_1, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->reference_menu_1, ui->reference_menu_1_cont_1, ui->reference_menu_1_subpage_1);

    //Create subpage for reference_menu_1
    ui->reference_menu_1_subpage_2 = lv_menu_page_create(ui->reference_menu_1, NULL);
    ui->reference_menu_1_cont_2 = lv_menu_cont_create(ui->reference_menu_1_sidebar_page);
    ui->reference_menu_1_label_2 = lv_label_create(ui->reference_menu_1_cont_2);
    lv_label_set_text(ui->reference_menu_1_label_2, "item");
    lv_obj_set_size(ui->reference_menu_1_label_2, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_scrollbar_mode(ui->reference_menu_1_subpage_2, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->reference_menu_1, ui->reference_menu_1_cont_2, ui->reference_menu_1_subpage_2);

    //Create subpage for reference_menu_1
    ui->reference_menu_1_subpage_3 = lv_menu_page_create(ui->reference_menu_1, NULL);
    ui->reference_menu_1_cont_3 = lv_menu_cont_create(ui->reference_menu_1_sidebar_page);
    ui->reference_menu_1_label_3 = lv_label_create(ui->reference_menu_1_cont_3);
    lv_label_set_text(ui->reference_menu_1_label_3, "item");
    lv_obj_set_size(ui->reference_menu_1_label_3, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_scrollbar_mode(ui->reference_menu_1_subpage_3, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->reference_menu_1, ui->reference_menu_1_cont_3, ui->reference_menu_1_subpage_3);
    lv_event_send(ui->reference_menu_1_cont_1, LV_EVENT_CLICKED, NULL);
    lv_obj_set_pos(ui->reference_menu_1, 490, 70);
    lv_obj_set_size(ui->reference_menu_1, 280, 210);
    lv_obj_set_scrollbar_mode(ui->reference_menu_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for reference_menu_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_menu_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_menu_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_menu_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_menu_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_menu_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_menu_1_extra_sidebar_page_main_default
    static lv_style_t style_reference_menu_1_extra_sidebar_page_main_default;
    ui_init_style(&style_reference_menu_1_extra_sidebar_page_main_default);

    lv_style_set_bg_opa(&style_reference_menu_1_extra_sidebar_page_main_default, 255);
    lv_style_set_bg_color(&style_reference_menu_1_extra_sidebar_page_main_default, lv_color_hex(0xdaf2f8));
    lv_style_set_bg_grad_dir(&style_reference_menu_1_extra_sidebar_page_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_radius(&style_reference_menu_1_extra_sidebar_page_main_default, 0);
    lv_obj_add_style(ui->reference_menu_1_sidebar_page, &style_reference_menu_1_extra_sidebar_page_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_menu_1_extra_option_btns_main_default
    static lv_style_t style_reference_menu_1_extra_option_btns_main_default;
    ui_init_style(&style_reference_menu_1_extra_option_btns_main_default);

    lv_style_set_text_color(&style_reference_menu_1_extra_option_btns_main_default, lv_color_hex(0x151212));
    lv_style_set_text_font(&style_reference_menu_1_extra_option_btns_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_menu_1_extra_option_btns_main_default, 255);
    lv_style_set_text_align(&style_reference_menu_1_extra_option_btns_main_default, LV_TEXT_ALIGN_CENTER);
    lv_style_set_pad_top(&style_reference_menu_1_extra_option_btns_main_default, 10);
    lv_style_set_pad_bottom(&style_reference_menu_1_extra_option_btns_main_default, 10);
    lv_obj_add_style(ui->reference_menu_1_cont_3, &style_reference_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->reference_menu_1_cont_2, &style_reference_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->reference_menu_1_cont_1, &style_reference_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_CHECKED for &style_reference_menu_1_extra_option_btns_main_checked
    static lv_style_t style_reference_menu_1_extra_option_btns_main_checked;
    ui_init_style(&style_reference_menu_1_extra_option_btns_main_checked);

    lv_style_set_text_color(&style_reference_menu_1_extra_option_btns_main_checked, lv_color_hex(0x9ab700));
    lv_style_set_text_font(&style_reference_menu_1_extra_option_btns_main_checked, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_menu_1_extra_option_btns_main_checked, 255);
    lv_style_set_text_align(&style_reference_menu_1_extra_option_btns_main_checked, LV_TEXT_ALIGN_CENTER);
    lv_style_set_border_width(&style_reference_menu_1_extra_option_btns_main_checked, 0);
    lv_style_set_radius(&style_reference_menu_1_extra_option_btns_main_checked, 0);
    lv_style_set_bg_opa(&style_reference_menu_1_extra_option_btns_main_checked, 60);
    lv_style_set_bg_color(&style_reference_menu_1_extra_option_btns_main_checked, lv_color_hex(0x00e0b8));
    lv_style_set_bg_grad_dir(&style_reference_menu_1_extra_option_btns_main_checked, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->reference_menu_1_cont_3, &style_reference_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->reference_menu_1_cont_2, &style_reference_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->reference_menu_1_cont_1, &style_reference_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);

    //Write style state: LV_STATE_DEFAULT for &style_reference_menu_1_extra_main_title_main_default
    static lv_style_t style_reference_menu_1_extra_main_title_main_default;
    ui_init_style(&style_reference_menu_1_extra_main_title_main_default);

    lv_style_set_text_color(&style_reference_menu_1_extra_main_title_main_default, lv_color_hex(0x41485a));
    lv_style_set_text_font(&style_reference_menu_1_extra_main_title_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_menu_1_extra_main_title_main_default, 255);
    lv_style_set_text_align(&style_reference_menu_1_extra_main_title_main_default, LV_TEXT_ALIGN_CENTER);
    lv_style_set_bg_opa(&style_reference_menu_1_extra_main_title_main_default, 255);
    lv_style_set_bg_color(&style_reference_menu_1_extra_main_title_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_reference_menu_1_extra_main_title_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_pad_top(&style_reference_menu_1_extra_main_title_main_default, 0);
    lv_style_set_pad_bottom(&style_reference_menu_1_extra_main_title_main_default, 0);
    lv_menu_t * reference_menu_1_menu= (lv_menu_t *)ui->reference_menu_1;
    lv_obj_t * reference_menu_1_title = reference_menu_1_menu->sidebar_header_title;
    lv_obj_set_size(reference_menu_1_title, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_add_style(lv_menu_get_sidebar_header(ui->reference_menu_1), &style_reference_menu_1_extra_main_title_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes reference_roller_1
    ui->reference_roller_1 = lv_roller_create(ui->reference_menu_1_subpage_1);
    lv_roller_set_options(ui->reference_roller_1, "1\n2\n3\n4\n5", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_pos(ui->reference_roller_1, -67, 78);
    lv_obj_set_width(ui->reference_roller_1, 100);

    //Write style for reference_roller_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->reference_roller_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_roller_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_roller_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_roller_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->reference_roller_1, lv_color_hex(0x333333), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_roller_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_roller_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->reference_roller_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_roller_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_roller_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_roller_1, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_roller_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_roller_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_roller_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_roller_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_roller_1, Part: LV_PART_SELECTED, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_roller_1, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_roller_1, lv_color_hex(0x2195f6), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_roller_1, LV_GRAD_DIR_NONE, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->reference_roller_1, lv_color_hex(0xFFFFFF), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_roller_1, &lv_font_montserratMedium_12, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_roller_1, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);

    lv_roller_set_visible_row_count(ui->reference_roller_1, 5);
    //Write codes reference_arc_1
    ui->reference_arc_1 = lv_arc_create(ui->reference_menu_1_subpage_1);
    lv_arc_set_mode(ui->reference_arc_1, LV_ARC_MODE_NORMAL);
    lv_arc_set_range(ui->reference_arc_1, 0, 100);
    lv_arc_set_bg_angles(ui->reference_arc_1, 135, 45);
    lv_arc_set_value(ui->reference_arc_1, 70);
    lv_arc_set_rotation(ui->reference_arc_1, 0);
    lv_obj_set_style_arc_rounded(ui->reference_arc_1, 0,  LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(ui->reference_arc_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_pos(ui->reference_arc_1, 16, 157);
    lv_obj_set_size(ui->reference_arc_1, 100, 100);

    //Write style for reference_arc_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_arc_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_arc_1, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_arc_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_arc_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui->reference_arc_1, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->reference_arc_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->reference_arc_1, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_arc_1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->reference_arc_1, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->reference_arc_1, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_arc_1, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_arc_1, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_arc_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_arc_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_arc_width(ui->reference_arc_1, 12, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->reference_arc_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->reference_arc_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for reference_arc_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_arc_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_arc_1, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_arc_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->reference_arc_1, 5, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes reference_line_1
    ui->reference_line_1 = lv_line_create(ui->reference_menu_1_subpage_1);
    static lv_point_t reference_line_1[] = {{0, 0},{0, 60},};
    lv_line_set_points(ui->reference_line_1, reference_line_1, 2);
    lv_obj_set_pos(ui->reference_line_1, 29, 16);
    lv_obj_set_size(ui->reference_line_1, 60, 60);

    //Write style for reference_line_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(ui->reference_line_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->reference_line_1, lv_color_hex(0x757575), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->reference_line_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->reference_line_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);





    //Write codes reference_table_1
    ui->reference_table_1 = lv_table_create(ui->reference);
    lv_table_set_col_cnt(ui->reference_table_1,2);
    lv_table_set_row_cnt(ui->reference_table_1,4);
    lv_table_set_cell_value(ui->reference_table_1,0,0,"Name");
    lv_table_set_cell_value(ui->reference_table_1,1,0,"Apple");
    lv_table_set_cell_value(ui->reference_table_1,2,0,"Banana");
    lv_table_set_cell_value(ui->reference_table_1,3,0,"Citron");
    lv_table_set_cell_value(ui->reference_table_1,0,1,"Price");
    lv_table_set_cell_value(ui->reference_table_1,1,1,"$1");
    lv_table_set_cell_value(ui->reference_table_1,2,1,"$2");
    lv_table_set_cell_value(ui->reference_table_1,3,1,"$3");
    lv_obj_set_pos(ui->reference_table_1, 359, 61);
    lv_obj_set_scrollbar_mode(ui->reference_table_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for reference_table_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_pad_top(ui->reference_table_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->reference_table_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_table_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_table_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_table_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_table_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_table_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_table_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_table_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_table_1, lv_color_hex(0xd5dee6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_table_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_table_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_table_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_table_1, Part: LV_PART_ITEMS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->reference_table_1, lv_color_hex(0x393c41), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_table_1, &lv_font_montserratMedium_12, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_table_1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->reference_table_1, LV_TEXT_ALIGN_CENTER, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_table_1, 0, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_table_1, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_table_1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_table_1, lv_color_hex(0xd5dee6), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_table_1, LV_BORDER_SIDE_FULL, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->reference_table_1, 10, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->reference_table_1, 10, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_table_1, 10, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_table_1, 10, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write codes reference_msgbox_1
    static const char * reference_msgbox_1_btns[] = {"Apply","Close", ""};
    ui->reference_msgbox_1 = lv_msgbox_create(ui->reference, "Title", "content", reference_msgbox_1_btns, true);
    lv_obj_set_size(lv_msgbox_get_btns(ui->reference_msgbox_1), 120, 30);
    lv_obj_set_pos(ui->reference_msgbox_1, 338, 122);
    lv_obj_set_size(ui->reference_msgbox_1, 280, 150);

    //Write style for reference_msgbox_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_msgbox_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_msgbox_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_msgbox_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_msgbox_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_msgbox_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_msgbox_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_msgbox_1_extra_title_main_default
    static lv_style_t style_reference_msgbox_1_extra_title_main_default;
    ui_init_style(&style_reference_msgbox_1_extra_title_main_default);

    lv_style_set_text_color(&style_reference_msgbox_1_extra_title_main_default, lv_color_hex(0x4e4e4e));
    lv_style_set_text_font(&style_reference_msgbox_1_extra_title_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_msgbox_1_extra_title_main_default, 255);
    lv_style_set_text_letter_space(&style_reference_msgbox_1_extra_title_main_default, 0);
    lv_style_set_text_line_space(&style_reference_msgbox_1_extra_title_main_default, 15);
    lv_obj_add_style(lv_msgbox_get_title(ui->reference_msgbox_1), &style_reference_msgbox_1_extra_title_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_msgbox_1_extra_content_main_default
    static lv_style_t style_reference_msgbox_1_extra_content_main_default;
    ui_init_style(&style_reference_msgbox_1_extra_content_main_default);

    lv_style_set_text_color(&style_reference_msgbox_1_extra_content_main_default, lv_color_hex(0x4e4e4e));
    lv_style_set_text_font(&style_reference_msgbox_1_extra_content_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_msgbox_1_extra_content_main_default, 255);
    lv_style_set_text_letter_space(&style_reference_msgbox_1_extra_content_main_default, 0);
    lv_style_set_text_line_space(&style_reference_msgbox_1_extra_content_main_default, 10);
    lv_obj_add_style(lv_msgbox_get_text(ui->reference_msgbox_1), &style_reference_msgbox_1_extra_content_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_msgbox_1_extra_btns_items_default
    static lv_style_t style_reference_msgbox_1_extra_btns_items_default;
    ui_init_style(&style_reference_msgbox_1_extra_btns_items_default);

    lv_style_set_bg_opa(&style_reference_msgbox_1_extra_btns_items_default, 255);
    lv_style_set_bg_color(&style_reference_msgbox_1_extra_btns_items_default, lv_color_hex(0xe6e6e6));
    lv_style_set_bg_grad_dir(&style_reference_msgbox_1_extra_btns_items_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_reference_msgbox_1_extra_btns_items_default, 0);
    lv_style_set_radius(&style_reference_msgbox_1_extra_btns_items_default, 10);
    lv_style_set_text_color(&style_reference_msgbox_1_extra_btns_items_default, lv_color_hex(0x4e4e4e));
    lv_style_set_text_font(&style_reference_msgbox_1_extra_btns_items_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_msgbox_1_extra_btns_items_default, 255);
    lv_obj_add_style(lv_msgbox_get_btns(ui->reference_msgbox_1), &style_reference_msgbox_1_extra_btns_items_default, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write codes reference_calendar_1
    ui->reference_calendar_1 = lv_calendar_create(ui->reference);
    reference_calendar_1_today.year = 2026;
    reference_calendar_1_today.month = 8;
    reference_calendar_1_today.day = 3;
    lv_calendar_set_today_date(ui->reference_calendar_1, reference_calendar_1_today.year, reference_calendar_1_today.month, reference_calendar_1_today.day);
    lv_calendar_set_showed_date(ui->reference_calendar_1, reference_calendar_1_today.year, reference_calendar_1_today.month);
    reference_calendar_1_highlihted_days[0].year = 2026;
    reference_calendar_1_highlihted_days[0].month = 8;
    reference_calendar_1_highlihted_days[0].day = 4;
    lv_calendar_set_highlighted_dates(ui->reference_calendar_1, reference_calendar_1_highlihted_days, 1);
    lv_obj_t *reference_calendar_1_header = lv_calendar_header_arrow_create(ui->reference_calendar_1);
    lv_calendar_t *reference_calendar_1 = (lv_calendar_t *)ui->reference_calendar_1;
    lv_obj_add_event_cb(reference_calendar_1->btnm, reference_calendar_1_draw_part_begin_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_obj_add_event_cb(ui->reference_calendar_1, reference_calendar_1_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_pos(ui->reference_calendar_1, 479, 164);
    lv_obj_set_size(ui->reference_calendar_1, 280, 210);

    //Write style state: LV_STATE_DEFAULT for &style_reference_calendar_1_main_main_default
    static lv_style_t style_reference_calendar_1_main_main_default;
    ui_init_style(&style_reference_calendar_1_main_main_default);

    lv_style_set_border_width(&style_reference_calendar_1_main_main_default, 1);
    lv_style_set_border_opa(&style_reference_calendar_1_main_main_default, 255);
    lv_style_set_border_color(&style_reference_calendar_1_main_main_default, lv_color_hex(0xc0c0c0));
    lv_style_set_border_side(&style_reference_calendar_1_main_main_default, LV_BORDER_SIDE_FULL);
    lv_style_set_bg_opa(&style_reference_calendar_1_main_main_default, 255);
    lv_style_set_bg_color(&style_reference_calendar_1_main_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_reference_calendar_1_main_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_shadow_width(&style_reference_calendar_1_main_main_default, 0);
    lv_style_set_radius(&style_reference_calendar_1_main_main_default, 0);
    lv_obj_add_style(ui->reference_calendar_1, &style_reference_calendar_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_calendar_1_extra_header_main_default
    static lv_style_t style_reference_calendar_1_extra_header_main_default;
    ui_init_style(&style_reference_calendar_1_extra_header_main_default);

    lv_style_set_text_color(&style_reference_calendar_1_extra_header_main_default, lv_color_hex(0xffffff));
    lv_style_set_text_font(&style_reference_calendar_1_extra_header_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_calendar_1_extra_header_main_default, 255);
    lv_style_set_bg_opa(&style_reference_calendar_1_extra_header_main_default, 255);
    lv_style_set_bg_color(&style_reference_calendar_1_extra_header_main_default, lv_color_hex(0x2195f6));
    lv_style_set_bg_grad_dir(&style_reference_calendar_1_extra_header_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(reference_calendar_1_header, &style_reference_calendar_1_extra_header_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_calendar_1_main_items_default
    static lv_style_t style_reference_calendar_1_main_items_default;
    ui_init_style(&style_reference_calendar_1_main_items_default);

    lv_style_set_bg_opa(&style_reference_calendar_1_main_items_default, 0);
    lv_style_set_border_width(&style_reference_calendar_1_main_items_default, 1);
    lv_style_set_border_opa(&style_reference_calendar_1_main_items_default, 255);
    lv_style_set_border_color(&style_reference_calendar_1_main_items_default, lv_color_hex(0xc0c0c0));
    lv_style_set_border_side(&style_reference_calendar_1_main_items_default, LV_BORDER_SIDE_FULL);
    lv_style_set_text_color(&style_reference_calendar_1_main_items_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_reference_calendar_1_main_items_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_calendar_1_main_items_default, 255);
    lv_obj_add_style(lv_calendar_get_btnmatrix(ui->reference_calendar_1), &style_reference_calendar_1_main_items_default, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write codes reference_list_1
    ui->reference_list_1 = lv_list_create(ui->reference);
    ui->reference_list_1_item0 = lv_list_add_btn(ui->reference_list_1, LV_SYMBOL_SAVE, "save");
    lv_obj_set_pos(ui->reference_list_1, 410, 124);
    lv_obj_set_size(ui->reference_list_1, 160, 100);
    lv_obj_set_scrollbar_mode(ui->reference_list_1, LV_SCROLLBAR_MODE_OFF);

    //Write style state: LV_STATE_DEFAULT for &style_reference_list_1_main_main_default
    static lv_style_t style_reference_list_1_main_main_default;
    ui_init_style(&style_reference_list_1_main_main_default);

    lv_style_set_pad_top(&style_reference_list_1_main_main_default, 5);
    lv_style_set_pad_left(&style_reference_list_1_main_main_default, 5);
    lv_style_set_pad_right(&style_reference_list_1_main_main_default, 5);
    lv_style_set_pad_bottom(&style_reference_list_1_main_main_default, 5);
    lv_style_set_bg_opa(&style_reference_list_1_main_main_default, 255);
    lv_style_set_bg_color(&style_reference_list_1_main_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_reference_list_1_main_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_reference_list_1_main_main_default, 1);
    lv_style_set_border_opa(&style_reference_list_1_main_main_default, 255);
    lv_style_set_border_color(&style_reference_list_1_main_main_default, lv_color_hex(0xe1e6ee));
    lv_style_set_border_side(&style_reference_list_1_main_main_default, LV_BORDER_SIDE_FULL);
    lv_style_set_radius(&style_reference_list_1_main_main_default, 3);
    lv_style_set_shadow_width(&style_reference_list_1_main_main_default, 0);
    lv_obj_add_style(ui->reference_list_1, &style_reference_list_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_list_1_main_scrollbar_default
    static lv_style_t style_reference_list_1_main_scrollbar_default;
    ui_init_style(&style_reference_list_1_main_scrollbar_default);

    lv_style_set_radius(&style_reference_list_1_main_scrollbar_default, 3);
    lv_style_set_bg_opa(&style_reference_list_1_main_scrollbar_default, 255);
    lv_style_set_bg_color(&style_reference_list_1_main_scrollbar_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_reference_list_1_main_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->reference_list_1, &style_reference_list_1_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_list_1_extra_btns_main_default
    static lv_style_t style_reference_list_1_extra_btns_main_default;
    ui_init_style(&style_reference_list_1_extra_btns_main_default);

    lv_style_set_pad_top(&style_reference_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_left(&style_reference_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_right(&style_reference_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_bottom(&style_reference_list_1_extra_btns_main_default, 5);
    lv_style_set_border_width(&style_reference_list_1_extra_btns_main_default, 0);
    lv_style_set_text_color(&style_reference_list_1_extra_btns_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_reference_list_1_extra_btns_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_list_1_extra_btns_main_default, 255);
    lv_style_set_radius(&style_reference_list_1_extra_btns_main_default, 3);
    lv_style_set_bg_opa(&style_reference_list_1_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_reference_list_1_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_reference_list_1_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->reference_list_1_item0, &style_reference_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_list_1_extra_texts_main_default
    static lv_style_t style_reference_list_1_extra_texts_main_default;
    ui_init_style(&style_reference_list_1_extra_texts_main_default);

    lv_style_set_pad_top(&style_reference_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_left(&style_reference_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_right(&style_reference_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_bottom(&style_reference_list_1_extra_texts_main_default, 5);
    lv_style_set_border_width(&style_reference_list_1_extra_texts_main_default, 0);
    lv_style_set_text_color(&style_reference_list_1_extra_texts_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_reference_list_1_extra_texts_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_list_1_extra_texts_main_default, 255);
    lv_style_set_radius(&style_reference_list_1_extra_texts_main_default, 3);
    lv_style_set_transform_width(&style_reference_list_1_extra_texts_main_default, 0);
    lv_style_set_bg_opa(&style_reference_list_1_extra_texts_main_default, 255);
    lv_style_set_bg_color(&style_reference_list_1_extra_texts_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_reference_list_1_extra_texts_main_default, LV_GRAD_DIR_NONE);

    //Write codes reference_spinbox_1
    ui->reference_spinbox_1 = lv_spinbox_create(ui->reference);
    lv_obj_set_pos(ui->reference_spinbox_1, 427, 189);
    lv_obj_set_width(ui->reference_spinbox_1, 70);
    lv_obj_set_height(ui->reference_spinbox_1, 40);
    lv_spinbox_set_digit_format(ui->reference_spinbox_1, 5, 3);
    lv_spinbox_set_range(ui->reference_spinbox_1, -99999, 99999);
    lv_coord_t reference_spinbox_1_h = lv_obj_get_height(ui->reference_spinbox_1);
    ui->reference_spinbox_1_btn_plus = lv_btn_create(ui->reference);
    lv_obj_set_size(ui->reference_spinbox_1_btn_plus, reference_spinbox_1_h, reference_spinbox_1_h);
    lv_obj_align_to(ui->reference_spinbox_1_btn_plus, ui->reference_spinbox_1, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_set_style_bg_img_src(ui->reference_spinbox_1_btn_plus, LV_SYMBOL_PLUS, 0);
    lv_obj_add_event_cb(ui->reference_spinbox_1_btn_plus, lv_reference_spinbox_1_increment_event_cb, LV_EVENT_ALL, NULL);
    ui->reference_spinbox_1_btn_minus = lv_btn_create(ui->reference);
    lv_obj_set_size(ui->reference_spinbox_1_btn_minus, reference_spinbox_1_h, reference_spinbox_1_h);
    lv_obj_align_to(ui->reference_spinbox_1_btn_minus, ui->reference_spinbox_1, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_set_style_bg_img_src(ui->reference_spinbox_1_btn_minus, LV_SYMBOL_MINUS, 0);
    lv_obj_add_event_cb(ui->reference_spinbox_1_btn_minus, lv_reference_spinbox_1_decrement_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_pos(ui->reference_spinbox_1, 427, 189);

    //Write style for reference_spinbox_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_spinbox_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_spinbox_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_spinbox_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_spinbox_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_spinbox_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_spinbox_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_spinbox_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->reference_spinbox_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_spinbox_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->reference_spinbox_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_spinbox_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->reference_spinbox_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_spinbox_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_spinbox_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->reference_spinbox_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_spinbox_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_spinbox_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_spinbox_1, Part: LV_PART_CURSOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->reference_spinbox_1, lv_color_hex(0xffffff), LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_spinbox_1, &lv_font_montserratMedium_12, LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_spinbox_1, 255, LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_spinbox_1, 255, LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_spinbox_1, lv_color_hex(0x2195f6), LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_spinbox_1, LV_GRAD_DIR_NONE, LV_PART_CURSOR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_reference_spinbox_1_extra_btns_main_default
    static lv_style_t style_reference_spinbox_1_extra_btns_main_default;
    ui_init_style(&style_reference_spinbox_1_extra_btns_main_default);

    lv_style_set_text_color(&style_reference_spinbox_1_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_text_font(&style_reference_spinbox_1_extra_btns_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_reference_spinbox_1_extra_btns_main_default, 255);
    lv_style_set_bg_opa(&style_reference_spinbox_1_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_reference_spinbox_1_extra_btns_main_default, lv_color_hex(0x2195f6));
    lv_style_set_bg_grad_dir(&style_reference_spinbox_1_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_reference_spinbox_1_extra_btns_main_default, 0);
    lv_style_set_radius(&style_reference_spinbox_1_extra_btns_main_default, 5);
    lv_style_set_shadow_width(&style_reference_spinbox_1_extra_btns_main_default, 0);
    lv_obj_add_style(ui->reference_spinbox_1_btn_plus, &style_reference_spinbox_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->reference_spinbox_1_btn_minus, &style_reference_spinbox_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes reference_meter_1
    ui->reference_meter_1 = lv_meter_create(ui->reference);
    // add scale ui->reference_meter_1_scale_0
    ui->reference_meter_1_scale_0 = lv_meter_add_scale(ui->reference_meter_1);
    lv_meter_set_scale_ticks(ui->reference_meter_1, ui->reference_meter_1_scale_0, 41, 2, 10, lv_color_hex(0xff0000));
    lv_meter_set_scale_major_ticks(ui->reference_meter_1, ui->reference_meter_1_scale_0, 8, 5, 15, lv_color_hex(0xffff00), 10);
    lv_meter_set_scale_range(ui->reference_meter_1, ui->reference_meter_1_scale_0, 0, 100, 300, 90);

    // add needle line for ui->reference_meter_1_scale_0.
    ui->reference_meter_1_scale_0_ndline_0 = lv_meter_add_needle_line(ui->reference_meter_1, ui->reference_meter_1_scale_0, 5, lv_color_hex(0x000000), -10);
    lv_meter_set_indicator_value(ui->reference_meter_1, ui->reference_meter_1_scale_0_ndline_0, 0);
    lv_obj_set_pos(ui->reference_meter_1, 449, 279);
    lv_obj_set_size(ui->reference_meter_1, 200, 200);

    //Write style for reference_meter_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_meter_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_meter_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_meter_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_meter_1, 100, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_meter_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->reference_meter_1, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->reference_meter_1, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_meter_1, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_meter_1, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_meter_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_meter_1, Part: LV_PART_TICKS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->reference_meter_1, lv_color_hex(0xff0000), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_meter_1, &lv_font_montserratMedium_12, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_meter_1, 255, LV_PART_TICKS|LV_STATE_DEFAULT);

    //Write style for reference_meter_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_meter_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_meter_1, lv_color_hex(0x000000), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_meter_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes reference_chart_1
    ui->reference_chart_1 = lv_chart_create(ui->reference);
    lv_chart_set_type(ui->reference_chart_1, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(ui->reference_chart_1, 3, 5);
    lv_chart_set_point_count(ui->reference_chart_1, 5);
    lv_chart_set_range(ui->reference_chart_1, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_range(ui->reference_chart_1, LV_CHART_AXIS_SECONDARY_Y, 0, 100);
    lv_chart_set_zoom_x(ui->reference_chart_1, 256);
    lv_chart_set_zoom_y(ui->reference_chart_1, 256);
    ui->reference_chart_1_0 = lv_chart_add_series(ui->reference_chart_1, lv_color_hex(0x000000), LV_CHART_AXIS_PRIMARY_Y);
#if LV_USE_FREEMASTER == 0
    lv_chart_set_next_value(ui->reference_chart_1, ui->reference_chart_1_0, 1);
    lv_chart_set_next_value(ui->reference_chart_1, ui->reference_chart_1_0, 20);
    lv_chart_set_next_value(ui->reference_chart_1, ui->reference_chart_1_0, 30);
    lv_chart_set_next_value(ui->reference_chart_1, ui->reference_chart_1_0, 40);
    lv_chart_set_next_value(ui->reference_chart_1, ui->reference_chart_1_0, 5);
#endif
    lv_obj_set_pos(ui->reference_chart_1, 663, 341);
    lv_obj_set_size(ui->reference_chart_1, 205, 155);
    lv_obj_set_scrollbar_mode(ui->reference_chart_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for reference_chart_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->reference_chart_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_chart_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_chart_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->reference_chart_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->reference_chart_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->reference_chart_1, lv_color_hex(0xe8e8e8), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->reference_chart_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->reference_chart_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->reference_chart_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->reference_chart_1, lv_color_hex(0xe8e8e8), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->reference_chart_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_chart_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_chart_1, Part: LV_PART_TICKS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->reference_chart_1, lv_color_hex(0x151212), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->reference_chart_1, &lv_font_montserratMedium_12, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->reference_chart_1, 255, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->reference_chart_1, 2, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->reference_chart_1, lv_color_hex(0xe8e8e8), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->reference_chart_1, 255, LV_PART_TICKS|LV_STATE_DEFAULT);

    //Write codes reference_canvas_1
    ui->reference_canvas_1 = lv_canvas_create(ui->reference);
    static lv_color_t buf_reference_canvas_1[LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(300, 200)];
    lv_canvas_set_buffer(ui->reference_canvas_1, buf_reference_canvas_1, 300, 200, LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_canvas_fill_bg(ui->reference_canvas_1, lv_color_hex(0xffffff), 255);
    //Canvas draw rectangle
    lv_draw_rect_dsc_t reference_canvas_1_rect_dsc_0;
    lv_draw_rect_dsc_init(&reference_canvas_1_rect_dsc_0);
    reference_canvas_1_rect_dsc_0.radius = 0;
    reference_canvas_1_rect_dsc_0.bg_opa = 255;
    reference_canvas_1_rect_dsc_0.bg_color = lv_color_hex(0x0775B7);
    reference_canvas_1_rect_dsc_0.bg_grad.dir = LV_GRAD_DIR_NONE;
    reference_canvas_1_rect_dsc_0.border_width = 0;
    reference_canvas_1_rect_dsc_0.border_opa = 255;
    reference_canvas_1_rect_dsc_0.border_color = lv_color_hex(0x000000);
    lv_canvas_draw_rect(ui->reference_canvas_1, 100, 80, 100, 50, &reference_canvas_1_rect_dsc_0);

    lv_obj_set_pos(ui->reference_canvas_1, 343, 40);
    lv_obj_set_size(ui->reference_canvas_1, 300, 200);
    lv_obj_set_scrollbar_mode(ui->reference_canvas_1, LV_SCROLLBAR_MODE_OFF);

    //Write codes reference_led_1
    ui->reference_led_1 = lv_led_create(ui->reference);
    lv_led_set_brightness(ui->reference_led_1, 255);
    lv_led_set_color(ui->reference_led_1, lv_color_hex(0x00a1b5));
    lv_obj_set_pos(ui->reference_led_1, 512, 45);
    lv_obj_set_size(ui->reference_led_1, 40, 40);

    //Write codes reference_cpicker_1
    ui->reference_cpicker_1 = lv_colorwheel_create(ui->reference, true);
    lv_obj_set_pos(ui->reference_cpicker_1, 450, 68);
    lv_obj_set_size(ui->reference_cpicker_1, 100, 100);

    //Write style for reference_cpicker_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_arc_width(ui->reference_cpicker_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes reference_spinner_1
    ui->reference_spinner_1 = lv_spinner_create(ui->reference, 1000, 60);
    lv_obj_set_pos(ui->reference_spinner_1, 376, 129);
    lv_obj_set_size(ui->reference_spinner_1, 100, 100);

    //Write style for reference_spinner_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_pad_top(ui->reference_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->reference_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->reference_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->reference_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->reference_spinner_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->reference_spinner_1, lv_color_hex(0xeeeef6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->reference_spinner_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui->reference_spinner_1, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->reference_spinner_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->reference_spinner_1, lv_color_hex(0xd5d6de), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->reference_spinner_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for reference_spinner_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_arc_width(ui->reference_spinner_1, 12, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->reference_spinner_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->reference_spinner_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //The custom code of reference.


    //Update current screen layout.
    lv_obj_update_layout(ui->reference);

}
