/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

#include "custom.h"

static void dashboard_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCREEN_LOADED:
    {

        break;
    }
    default:
        break;
    }
}

static void dashboard_Settings_text_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.settings, guider_ui.settings_del, &guider_ui.dashboard_del, setup_scr_settings, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 200, 200, false, false);
        break;
    }
    default:
        break;
    }
}

void events_init_dashboard (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->dashboard, dashboard_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Settings_text, dashboard_Settings_text_event_handler, LV_EVENT_ALL, ui);
}

static void settings_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCREEN_LOADED:
    {
        settings_ui_init(&guider_ui);
        break;
    }
    default:
        break;
    }
}

static void settings_exit_button_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.dashboard, guider_ui.dashboard_del, &guider_ui.settings_del, setup_scr_dashboard, LV_SCR_LOAD_ANIM_MOVE_TOP, 200, 200, false, false);
        break;
    }
    default:
        break;
    }
}

void events_init_settings (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->settings, settings_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->settings_exit_button, settings_exit_button_event_handler, LV_EVENT_ALL, ui);
}


void events_init(lv_ui *ui)
{

}
