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
#include "custom.h"
#include "dashboard_theme.h"
#include "custom.h"
#include "custom.h"

static void dashboard_Classic_event_handler (lv_event_t *e)
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

static void dashboard_Classic_status_vesc_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        //for claude
        run_vesc_tool_menu();
        break;
    }
    default:
        break;
    }
}

static void dashboard_Classic_Settings_text_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.settings, guider_ui.settings_del, &guider_ui.dashboard_Classic_del, setup_scr_settings, LV_SCR_LOAD_ANIM_NONE, 200, 200, false, false);
        break;
    }
    default:
        break;
    }
}

static void dashboard_Classic_brightness_slider_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_VALUE_CHANGED:
    {
        lv_obj_t *slider = lv_event_get_target(e);
        dashboard_brightness_slider_changed(lv_slider_get_value(slider));
        break;
    }
    default:
        break;
    }
}

static void dashboard_Classic_statistics_button_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        //for claude code
        show_trips_statistics();
        break;
    }
    default:
        break;
    }
}

void events_init_dashboard_Classic (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->dashboard_Classic, dashboard_Classic_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Classic_status_vesc, dashboard_Classic_status_vesc_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Classic_Settings_text, dashboard_Classic_Settings_text_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Classic_brightness_slider, dashboard_Classic_brightness_slider_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Classic_statistics_button, dashboard_Classic_statistics_button_event_handler, LV_EVENT_ALL, ui);
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
        ui_load_scr_animation(&guider_ui, &guider_ui.dashboard_Classic, guider_ui.dashboard_Classic_del, &guider_ui.settings_del, setup_scr_dashboard_Classic, LV_SCR_LOAD_ANIM_NONE, 200, 200, false, false);
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

static void dashboard_Classic_Max_event_handler (lv_event_t *e)
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

static void dashboard_Classic_Max_status_vesc_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        //for claude
        run_vesc_tool_menu();
        break;
    }
    default:
        break;
    }
}

static void dashboard_Classic_Max_Settings_text_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.settings, guider_ui.settings_del, &guider_ui.dashboard_Classic_Max_del, setup_scr_settings, LV_SCR_LOAD_ANIM_NONE, 200, 200, false, false);
        break;
    }
    default:
        break;
    }
}

static void dashboard_Classic_Max_brightness_slider_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_VALUE_CHANGED:
    {
        lv_obj_t *slider = lv_event_get_target(e);
        dashboard_brightness_slider_changed(lv_slider_get_value(slider));
        break;
    }
    default:
        break;
    }
}

static void dashboard_Classic_Max_statistics_button_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        //for claude code
        show_trips_statistics();
        break;
    }
    default:
        break;
    }
}

static void dashboard_Classic_Max_max_reset_btn_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        dashboard_max_reset();
        break;
    }
    default:
        break;
    }
}

void events_init_dashboard_Classic_Max (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->dashboard_Classic_Max, dashboard_Classic_Max_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Classic_Max_status_vesc, dashboard_Classic_Max_status_vesc_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Classic_Max_Settings_text, dashboard_Classic_Max_Settings_text_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Classic_Max_brightness_slider, dashboard_Classic_Max_brightness_slider_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Classic_Max_statistics_button, dashboard_Classic_Max_statistics_button_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Classic_Max_max_reset_btn, dashboard_Classic_Max_max_reset_btn_event_handler, LV_EVENT_ALL, ui);
}

static void dashboard_Lamborghini_status_vesc_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        run_vesc_tool_menu();
        break;
    }
    default:
        break;
    }
}

static void dashboard_Lamborghini_Settings_text_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.settings, guider_ui.settings_del, &guider_ui.dashboard_Lamborghini_del, setup_scr_settings, LV_SCR_LOAD_ANIM_NONE, 200, 200, false, false);
        break;
    }
    default:
        break;
    }
}

void events_init_dashboard_Lamborghini (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->dashboard_Lamborghini_status_vesc, dashboard_Lamborghini_status_vesc_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Lamborghini_Settings_text, dashboard_Lamborghini_Settings_text_event_handler, LV_EVENT_ALL, ui);
}

static void dashboard_Supermoto_status_vesc_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        run_vesc_tool_menu();
        break;
    }
    default:
        break;
    }
}

static void dashboard_Supermoto_Settings_text_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.settings, guider_ui.settings_del, &guider_ui.dashboard_Supermoto_del, setup_scr_settings, LV_SCR_LOAD_ANIM_NONE, 200, 200, false, false);
        break;
    }
    default:
        break;
    }
}

void events_init_dashboard_Supermoto (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->dashboard_Supermoto_status_vesc, dashboard_Supermoto_status_vesc_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->dashboard_Supermoto_Settings_text, dashboard_Supermoto_Settings_text_event_handler, LV_EVENT_ALL, ui);
}


void events_init(lv_ui *ui)
{

}
