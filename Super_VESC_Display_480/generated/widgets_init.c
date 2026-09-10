/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include "gui_guider.h"
#include "widgets_init.h"
#include <stdlib.h>
#include <string.h>


__attribute__((unused)) void kb_event_cb (lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);
    if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

__attribute__((unused)) void ta_event_cb (lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
#if LV_USE_KEYBOARD || LV_USE_ZH_KEYBOARD
    lv_obj_t *ta = lv_event_get_target(e);
#endif
    lv_obj_t *kb = lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED)
    {
#if LV_USE_ZH_KEYBOARD != 0
        lv_zh_keyboard_set_textarea(kb, ta);
#endif
#if LV_USE_KEYBOARD != 0
        lv_keyboard_set_textarea(kb, ta);
#endif
        lv_obj_move_foreground(kb);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    if (code == LV_EVENT_CANCEL || code == LV_EVENT_DEFOCUSED)
    {

#if LV_USE_ZH_KEYBOARD != 0
        lv_zh_keyboard_set_textarea(kb, ta);
#endif
#if LV_USE_KEYBOARD != 0
        lv_keyboard_set_textarea(kb, ta);
#endif
        lv_obj_move_background(kb);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

#if LV_USE_ANALOGCLOCK != 0
void clock_count(int *hour, int *min, int *sec)
{
    (*sec)++;
    if(*sec == 60)
    {
        *sec = 0;
        (*min)++;
    }
    if(*min == 60)
    {
        *min = 0;
        if(*hour < 12)
        {
            (*hour)++;
        } else {
            (*hour)++;
            *hour = *hour %12;
        }
    }
}
#endif


const lv_img_dsc_t * reference_animimg_1_imgs[1] = { NULL };
const lv_img_dsc_t * reference_image3D_1_imgs[1] = { NULL };
void reference_calendar_1_draw_part_begin_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
    if(dsc->part == LV_PART_ITEMS) {
        if(dsc->id < 7) {
            dsc->label_dsc->color = lv_color_hex(0x0D3055);
            dsc->label_dsc->font = &lv_font_montserratMedium_12;
        } else if (lv_btnmatrix_has_btn_ctrl(obj, dsc->id, LV_BTNMATRIX_CTRL_DISABLED)) {
            dsc->label_dsc->color = lv_color_hex(0xA9A2A2);
            dsc->label_dsc->font = &lv_font_montserratMedium_12;
            dsc->rect_dsc->bg_opa = 0;
        } else if(lv_btnmatrix_has_btn_ctrl(obj, dsc->id, LV_BTNMATRIX_CTRL_CUSTOM_1)) {
            dsc->label_dsc->color = lv_color_hex(0x0D3055);
            dsc->label_dsc->font = &lv_font_montserratMedium_12;
            dsc->rect_dsc->bg_opa = 255;
            dsc->rect_dsc->bg_color = lv_color_hex(0x01a2b1);
            dsc->rect_dsc->border_opa = 255;
            dsc->rect_dsc->border_width = 1;
            dsc->rect_dsc->border_color = lv_color_hex(0xc0c0c0);
        } else if(lv_btnmatrix_has_btn_ctrl(obj, dsc->id, LV_BTNMATRIX_CTRL_CUSTOM_2)) {
            dsc->label_dsc->color = lv_color_hex(0x0D3055);
            dsc->label_dsc->font = &lv_font_montserratMedium_12;
            dsc->rect_dsc->bg_opa = 255;
            dsc->rect_dsc->bg_color = lv_color_hex(0x2195f6);
        } else {
        }
    }
}
void reference_calendar_1_event_handler(lv_event_t * e)
{
    lv_calendar_date_t date;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t * obj = lv_event_get_current_target(e);
        lv_calendar_get_pressed_date(obj,&date);
        lv_calendar_set_highlighted_dates(obj, &date, 1);
    }
}
void lv_reference_spinbox_1_increment_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if(code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_increment(guider_ui.reference_spinbox_1);
    }
}
void lv_reference_spinbox_1_decrement_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if(code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_decrement(guider_ui.reference_spinbox_1);
    }
}

