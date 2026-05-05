/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "custom.h"
#include "settings_wrapper.h"

#ifdef LV_REALDEVICE
#include "vesc_limits.h"
#include "vesc_battery_calc.h"
#endif

int cruise_active = 0;

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

extern lv_ui guider_ui;

// Settings UI objects (dynamically created)
static lv_obj_t *settings_target_id_spinbox = NULL;
static lv_obj_t *settings_target_id_label = NULL;
static lv_obj_t *settings_target_id_plus_btn = NULL;
static lv_obj_t *settings_target_id_minus_btn = NULL;
static lv_obj_t *settings_can_speed_dropdown = NULL;
static lv_obj_t *settings_can_speed_label = NULL;
static lv_obj_t *settings_brightness_slider = NULL;
static lv_obj_t *settings_brightness_label = NULL;
static lv_obj_t *settings_controller_id_slider = NULL;
static lv_obj_t *settings_controller_id_label = NULL;
static lv_obj_t *settings_battery_capacity_spinbox = NULL;
static lv_obj_t *settings_battery_capacity_label = NULL;
static lv_obj_t *settings_battery_capacity_plus_btn = NULL;
static lv_obj_t *settings_battery_capacity_minus_btn = NULL;
static lv_obj_t *settings_battery_calc_mode_dropdown = NULL;
static lv_obj_t *settings_battery_calc_mode_label = NULL;
static lv_obj_t *settings_show_fps_switch = NULL;
static lv_obj_t *settings_show_fps_label = NULL;
static lv_obj_t *settings_wheel_diameter_spinbox = NULL;
static lv_obj_t *settings_wheel_diameter_label = NULL;
static lv_obj_t *settings_wheel_diameter_plus_btn = NULL;
static lv_obj_t *settings_wheel_diameter_minus_btn = NULL;
static lv_obj_t *settings_motor_poles_spinbox = NULL;
static lv_obj_t *settings_motor_poles_label = NULL;
static lv_obj_t *settings_motor_poles_plus_btn = NULL;
static lv_obj_t *settings_motor_poles_minus_btn = NULL;
static lv_obj_t *settings_reset_button = NULL;
static lv_obj_t *settings_info_label = NULL;

// VESC Limits UI objects
static lv_obj_t *settings_limits_title_label = NULL;
static lv_obj_t *settings_read_limits_btn = NULL;
static lv_obj_t *settings_motor_current_spinbox = NULL;
static lv_obj_t *settings_motor_current_label = NULL;
static lv_obj_t *settings_motor_current_plus_btn = NULL;
static lv_obj_t *settings_motor_current_minus_btn = NULL;
static lv_obj_t *settings_battery_current_spinbox = NULL;
static lv_obj_t *settings_battery_current_label = NULL;
static lv_obj_t *settings_battery_current_plus_btn = NULL;
static lv_obj_t *settings_battery_current_minus_btn = NULL;
static lv_obj_t *settings_erpm_max_spinbox = NULL;
static lv_obj_t *settings_erpm_max_label = NULL;
static lv_obj_t *settings_erpm_max_plus_btn = NULL;
static lv_obj_t *settings_erpm_max_minus_btn = NULL;
static lv_obj_t *settings_apply_limits_btn = NULL;
static lv_obj_t *settings_limits_status_label = NULL;
/**
 * Create a demo application
 */

/* set the digital label and steering lamp image style. */
static void set_position_x(void * gui, int32_t temp)
{
    
}

static void set_position_y(void * gui, int32_t temp)
{
  
}

void custom_init(lv_ui *ui)
{
    /* Add your codes here */
    
    // Initialize BLE icon as hidden (will be shown when BLE connects)
    if (ui->dashboard_ble_connected_img != NULL) {
        lv_obj_add_flag(ui->dashboard_ble_connected_img, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Initialize ESC not connected text as hidden (will be shown and blink if ESC disconnects)
    if (ui->dashboard_esc_not_connected_text != NULL) {
        lv_obj_add_flag(ui->dashboard_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
    }

    // Initialize cruise-active label as hidden (will be shown when cruise-active is true)
    if (ui->dashboard_cruise_control_img != NULL) {
        lv_obj_add_flag(ui->dashboard_cruise_control_img, LV_OBJ_FLAG_HIDDEN);
    }

    // Initialize cruise-rpm label as hidden (will be shown when cruise-rpm is true)
    if (ui->dashboard_Speed_meter_scale_1_ndline_0 != NULL) {
        lv_meter_set_indicator_value(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_1_ndline_0, -1);
        lv_meter_set_indicator_value(ui->dashboard_Speed_meter, ui->dashboard_Speed_meter_scale_2_ndline_0, -1);  
    }
    
}

void speed_meter_timer_cb(lv_timer_t * t)
{
    
}

void home_label_digit_animation(lv_ui *ui)
{

}

void digital_cluster_chart_timer_cb(lv_timer_t * t)
{
    
}

void play_music(lv_ui *ui)
{
   
}

static const void * lv_demo_music_get_list_img(uint32_t track_id)
{
 
}

void music_album_next(bool next)
{
   
}

void reset_icon_pressed(void)
{
    #ifdef LV_REALDEVICE
    battery_calc_reset_trip_and_ah();
    #endif
}

void update_current(float current)
{
    static float old_value = -999.0f;
    if (current == old_value) {
        return;
    }
    old_value = current;
    
    int value = current;
    int abs_value = abs(value);
    
    lv_meter_set_indicator_value(guider_ui.dashboard_Current_meter, guider_ui.dashboard_Current_meter_scale_0_ndline_0, abs_value);
    
    if (value>0)
    {
        lv_meter_set_indicator_end_value(guider_ui.dashboard_Current_meter, guider_ui.dashboard_Current_meter_scale_0_arc_1, abs_value);
        lv_meter_set_indicator_end_value(guider_ui.dashboard_Current_meter, guider_ui.dashboard_Current_meter_scale_0_arc_2, 0);
    }
    else
    {
        lv_meter_set_indicator_end_value(guider_ui.dashboard_Current_meter, guider_ui.dashboard_Current_meter_scale_0_arc_1, 0);
        lv_meter_set_indicator_end_value(guider_ui.dashboard_Current_meter, guider_ui.dashboard_Current_meter_scale_0_arc_2, abs_value);
    }
    char text[10];
    sprintf(text,"%.1f", current);
    lv_textarea_set_text(guider_ui.dashboard_Current_text,text);
}

void update_speed(float speed)
{
    static float old_value = -999.0f;
    if (speed == old_value) {
        return;
    }
    old_value = speed;
    
    int value = speed;
    
    lv_meter_set_indicator_value(guider_ui.dashboard_Speed_meter, guider_ui.dashboard_Speed_meter_scale_0_ndline_0, value);
    lv_meter_set_indicator_end_value(guider_ui.dashboard_Speed_meter, guider_ui.dashboard_Speed_meter_scale_0_arc_0, value);
    
    char text[10];
    sprintf(text,"%d", value);
    lv_textarea_set_text(guider_ui.dashboard_Speed_text,text);
}

void update_cruise_speed(float speed)
{
    if (cruise_active == 0) {
        return;
    }
    static float old_value = -999.0f;
    if (speed == old_value) {
        return;
    }
    if (speed < 0) {
        return;
    }
    old_value = speed;
    
    int value = speed;
    
    lv_meter_set_indicator_value(guider_ui.dashboard_Speed_meter, guider_ui.dashboard_Speed_meter_scale_1_ndline_0, value);
}


void update_battery_proc(float battery_proc)
{
    static float old_value = -999.0f;
    if (battery_proc == old_value) {
        return;
    }
    old_value = battery_proc;
    
    int value = battery_proc;
    
    lv_meter_set_indicator_value(guider_ui.dashboard_Battery_meter, guider_ui.dashboard_Battery_meter_scale_0_ndline_0, 100-value);
    lv_meter_set_indicator_start_value(guider_ui.dashboard_Battery_meter, guider_ui.dashboard_Battery_meter_scale_0_arc_1, 100-value);
    
    char text[10];
    sprintf(text,"%d", value>99?99:value);
    lv_textarea_set_text(guider_ui.dashboard_Battery_proc_text,text);
}

void update_trip(float trip_distance)
{
    static float old_value = -999.0f;
    if (trip_distance == old_value) {
        return;
    }
    old_value = trip_distance;
    
    char text[10];
    sprintf(text,"%0.1f", trip_distance);
    lv_textarea_set_text(guider_ui.dashboard_TRIP_text,text);
}

void update_range(float range_distance)
{
    static float old_value = -999.0f;
    if (range_distance == old_value) {
        return;
    }
    old_value = range_distance;
    
    char text[10];
    sprintf(text,"%.1f", range_distance);
    lv_textarea_set_text(guider_ui.dashboard_Range_text,text);
}

void update_temp_fet(float temp_fet)
{
    static float old_value = -999.0f;
    if (temp_fet == old_value) {
        return;
    }
    old_value = temp_fet;
    
    int value = temp_fet;
    char text[10];
    sprintf(text,"%d", value);
    lv_textarea_set_text(guider_ui.dashboard_temp_esc_text,text);
}

void update_temp_motor(float temp_motor)
{
    static float old_value = -999.0f;
    if (temp_motor == old_value) {
        return;
    }
    old_value = temp_motor;
    
    int value = temp_motor;
    char text[10];
    sprintf(text,"%d", value);
    lv_textarea_set_text(guider_ui.dashboard_temp_mot_text,text);
}

void update_amp_hours(float amp_hours)
{
    static float old_value = -999.0f;
    if (amp_hours == old_value) {
        return;
    }
    old_value = amp_hours;
    
    char text[10];
    sprintf(text,"%.1f", amp_hours);
    lv_textarea_set_text(guider_ui.dashboard_Ah_text,text);
}

void update_battery_temp(float battery_temp)
{
    static float old_value = -999.0f;
    if (battery_temp == old_value) {
        return;
    }
    old_value = battery_temp;
    
    int value = battery_temp;
    char text[10];
    sprintf(text,"%d", value);

    lv_textarea_set_text(guider_ui.dashboard_temp_bat_text,text);
}

void update_battery_voltage(float battery_voltage)
{
    static float old_value = -999.0f;
    if (battery_voltage == old_value) {
        return;
    }
    old_value = battery_voltage;
    
    char text[10];
    sprintf(text,"%.1f", battery_voltage);

    lv_textarea_set_text(guider_ui.dashboard_Voltage_text,text);
}


void update_odometer(float odometer)
{
    static float old_value = -999.0f;
    if (odometer == old_value) {
        return;
    }
    old_value = odometer;
    
    int value = odometer;   
    char text[10];
    sprintf(text,"%05d", value);

    lv_textarea_set_text(guider_ui.dashboard_odo_text,text);
}

void update_fps(int fps)
{
    static int old_value = -999;
    static int min_fps = 400;
    
    // Check if FPS should be shown
    bool show_fps = settings_wrapper_get_show_fps();
    
    // Hide or show FPS text based on setting
    if (guider_ui.dashboard_fps_text) {
        if (show_fps) {
            lv_obj_clear_flag(guider_ui.dashboard_fps_text, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(guider_ui.dashboard_fps_text, LV_OBJ_FLAG_HIDDEN);
            return; // Don't update if hidden
        }
    }
    
    if (fps == old_value) {
        return;
    }
    old_value = fps;
    if (lv_tick_get() > 8000) {
        if (fps < min_fps) {
            min_fps = fps;
        }
    }
    char text[20];
    sprintf(text,"FPS:%d, Min:%d", fps, min_fps);
    lv_textarea_set_text(guider_ui.dashboard_fps_text,text);
}

void update_uptime(uint32_t uptime)
{
    int value = uptime/1000;
    static uint32_t old_value = -999;
    if (value == old_value) {
        return;
    }
    old_value = value;
    
    char text[20];
    sprintf(text,"%02d:%02d:%02d", value/3600, (value%3600)/60, value%60);
    lv_textarea_set_text(guider_ui.dashboard_uptime_text,text);
}

void update_mode_text(uint8_t mode)
{
    static uint8_t old_mode = -1;
    if (mode == old_mode) {
        return;
    }
    old_mode = mode;
    
    char text[20];
    sprintf(text,"MODE %d", mode+1);
    lv_textarea_set_text(guider_ui.dashboard_mode_text,text);
}

void update_ble_status(bool connected)
{
    static bool old_state = false;
    if (connected == old_state) {
        return;
    }
    old_state = connected;
    
    // Show/hide BLE icon based on connection status
    if (connected) {
        lv_obj_clear_flag(guider_ui.dashboard_ble_connected_img, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(guider_ui.dashboard_ble_connected_img, LV_OBJ_FLAG_HIDDEN);
    }
}

void update_cruise_control_status(bool active)
{
    static bool old_state = false;
    if (active == old_state) {
        return;
    }
    old_state = active;
    
    // Show/hide BLE icon based on connection status
    if (active) {
        lv_obj_clear_flag(guider_ui.dashboard_cruise_control_img, LV_OBJ_FLAG_HIDDEN);
        cruise_active = 1;
    } else {
        lv_meter_set_indicator_value(guider_ui.dashboard_Speed_meter, guider_ui.dashboard_Speed_meter_scale_1_ndline_0, -1);       
        lv_obj_add_flag(guider_ui.dashboard_cruise_control_img, LV_OBJ_FLAG_HIDDEN);
        cruise_active = 0;
    }
}

void update_esc_connection_status(bool connected)
{
    static bool old_state = true;
    static uint32_t last_blink_time = 0;
    static bool blink_state = false;
    
    // Handle connection state change
    if (connected != old_state) {
        old_state = connected;
        if (connected) {
            // ESC connected - hide warning text
            lv_obj_add_flag(guider_ui.dashboard_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_clear_flag(guider_ui.dashboard_Ah_const_text, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_clear_flag(guider_ui.dashboard_Ah_text, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(guider_ui.dashboard_mode_text, LV_OBJ_FLAG_HIDDEN);
        } else {
            // ESC disconnected - show warning text (will start blinking)
            lv_obj_clear_flag(guider_ui.dashboard_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_add_flag(guider_ui.dashboard_Ah_text, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_add_flag(guider_ui.dashboard_Ah_const_text, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(guider_ui.dashboard_mode_text, LV_OBJ_FLAG_HIDDEN);
            blink_state = true;
        }
    }
    
    // Blink warning text if ESC is not connected
    if (!connected) {
        uint32_t now = lv_tick_get();
        
        // Toggle visibility every 500ms
        if (now - last_blink_time >= 500) {
            last_blink_time = now;
            blink_state = !blink_state;
            
            if (blink_state) {
                //lv_obj_add_flag(guider_ui.dashboard_Ah_text, LV_OBJ_FLAG_HIDDEN);
                //lv_obj_add_flag(guider_ui.dashboard_Ah_const_text, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(guider_ui.dashboard_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(guider_ui.dashboard_mode_text, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(guider_ui.dashboard_esc_not_connected_text, LV_OBJ_FLAG_HIDDEN);
                //lv_obj_clear_flag(guider_ui.dashboard_Ah_const_text, LV_OBJ_FLAG_HIDDEN);
                //lv_obj_clear_flag(guider_ui.dashboard_Ah_text, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(guider_ui.dashboard_mode_text, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

void update_navigation_icon(const uint8_t *img_data, uint32_t data_size, uint16_t width, uint16_t height, lv_img_cf_t color_format)
{
    static lv_img_dsc_t *navigation_icon_dsc = NULL;
    
    // Free previous image data if exists
    if (navigation_icon_dsc != NULL) {
        if (navigation_icon_dsc->data != NULL) {
            free((void *)navigation_icon_dsc->data);
        }
        free(navigation_icon_dsc);
        navigation_icon_dsc = NULL;
    }
    
    // Check if valid data provided
    if (img_data == NULL || data_size == 0 || width == 0 || height == 0) {
        // Clear icon if no data provided
        if (guider_ui.dashboard_navigation_icon != NULL) {
            lv_img_set_src(guider_ui.dashboard_navigation_icon, NULL);
        }
        return;
    }
    
    // Allocate memory for image descriptor
    navigation_icon_dsc = (lv_img_dsc_t *)malloc(sizeof(lv_img_dsc_t));
    if (navigation_icon_dsc == NULL) {
        return; // Memory allocation failed
    }
    
    // Allocate memory for image data
    uint8_t *data_copy = (uint8_t *)malloc(data_size);
    if (data_copy == NULL) {
        free(navigation_icon_dsc);
        navigation_icon_dsc = NULL;
        return; // Memory allocation failed
    }
    
    // Copy image data
    memcpy(data_copy, img_data, data_size);
    
    // Fill image descriptor
    navigation_icon_dsc->header.cf = color_format;
    navigation_icon_dsc->header.always_zero = 0;
    navigation_icon_dsc->header.reserved = 0;
    navigation_icon_dsc->header.w = width;
    navigation_icon_dsc->header.h = height;
    navigation_icon_dsc->data_size = data_size;
    navigation_icon_dsc->data = data_copy;
    
    // Update icon
    if (guider_ui.dashboard_navigation_icon != NULL) {
        lv_img_set_src(guider_ui.dashboard_navigation_icon, navigation_icon_dsc);
    }
}

void update_navigation_text(const char *text)
{
    static char old_text[65] = {0}; // Max length is 64 according to setup
    
    if (text == NULL) {
        text = "";
    }
    
    // Check if text changed
    if (strcmp(text, old_text) == 0) {
        return;
    }
    
    // Copy new text
    strncpy(old_text, text, sizeof(old_text) - 1);
    old_text[sizeof(old_text) - 1] = '\0';
    
    // Update UI
    if (guider_ui.dashboard_navigation_text != NULL) {
        lv_textarea_set_text(guider_ui.dashboard_navigation_text, text);
    }
}

void update_music_text(const char *text)
{
    static char old_text[65] = {0}; // Max length is 64 according to setup
    
    if (text == NULL) {
        text = "";
    }
    
    // Check if text changed
    if (strcmp(text, old_text) == 0) {
        return;
    }
    
    // Copy new text
    strncpy(old_text, text, sizeof(old_text) - 1);
    old_text[sizeof(old_text) - 1] = '\0';
    
    // Update UI
    if (guider_ui.dashboard_music_text != NULL) {
        lv_textarea_set_text(guider_ui.dashboard_music_text, text);
    }
}

// ============================================================================
// SETTINGS UI IMPLEMENTATION
// ============================================================================

// Event handler for Target VESC ID spinbox
static void target_id_spinbox_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *spinbox = lv_event_get_target(e);
        int32_t value = lv_spinbox_get_value(spinbox);
        
        // Save to settings
        settings_wrapper_set_target_vesc_id((uint8_t)value);
    }
}

// Event handler for Plus button
static void target_id_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_target_id_spinbox);
        if (current_value < 254) {
            lv_spinbox_increment(settings_target_id_spinbox);
        }
    }
}

// Event handler for Minus button
static void target_id_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_target_id_spinbox);
        if (current_value > 1) {
            lv_spinbox_decrement(settings_target_id_spinbox);
        }
    }
}

// Event handler for CAN speed dropdown
static void can_speed_dropdown_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *dropdown = lv_event_get_target(e);
        uint16_t selected = lv_dropdown_get_selected(dropdown);
        
        // Save to settings
        settings_wrapper_set_can_speed_index((uint8_t)selected);
        
        // Update info label
        lv_label_set_text(settings_info_label, "CAN speed requires restart!");
    }
}

// Event handler for brightness slider
static void brightness_slider_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *slider = lv_event_get_target(e);
        int32_t value = lv_slider_get_value(slider);
        
        // Update label
        char buf[32];
        sprintf(buf, "Brightness: %d%%", (int)value);
        lv_label_set_text(settings_brightness_label, buf);
        
        // Save to settings and apply immediately
        settings_wrapper_set_brightness((uint8_t)value);
    }
}

// Event handler for Controller ID slider
static void controller_id_slider_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *slider = lv_event_get_target(e);
        int32_t value = lv_slider_get_value(slider);
        
        // Update label
        char buf[32];
        sprintf(buf, "Controller ID: %d", (int)value);
        lv_label_set_text(settings_controller_id_label, buf);
        
        // Save to settings
        settings_wrapper_set_controller_id((uint8_t)value);
        
        // Update info label
        lv_label_set_text(settings_info_label, "Controller ID requires restart!");
    }
}

// Event handler for Battery Capacity spinbox
static void battery_capacity_spinbox_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *spinbox = lv_event_get_target(e);
        int32_t value = lv_spinbox_get_value(spinbox);
        
        // Value is stored as integer (e.g., 150 for 15.0 Ah)
        float capacity = (float)value / 10.0f;
        
        // Save to settings (this will trigger battery calc reset)
        settings_wrapper_set_battery_capacity(capacity);
        
        // Update info label
        lv_label_set_text(settings_info_label, "Battery capacity changed - will recalibrate!");
    }
}

// Event handler for Battery Capacity Plus button
static void battery_capacity_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_battery_capacity_spinbox);
        if (current_value < 2000) { // Max 200.0 Ah
            lv_spinbox_increment(settings_battery_capacity_spinbox);
        }
    }
}

// Event handler for Battery Capacity Minus button
static void battery_capacity_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_battery_capacity_spinbox);
        if (current_value > 10) { // Min 1.0 Ah
            lv_spinbox_decrement(settings_battery_capacity_spinbox);
        }
    }
}

// Event handler for Battery Calculation Mode dropdown
static void battery_calc_mode_dropdown_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *dropdown = lv_event_get_target(e);
        uint16_t selected = lv_dropdown_get_selected(dropdown);
        
        // Save to settings
        settings_wrapper_set_battery_calc_mode((uint8_t)selected);
        
        // Update info label
        if (selected == 1) { // Smart Calculation
            lv_label_set_text(settings_info_label, "Smart calc enabled - will calibrate on next data!");
        } else {
            lv_label_set_text(settings_info_label, "Direct mode - using controller battery level");
        }
    }
}

// Timer callback for checking limits response
static void limits_response_timer_cb(lv_timer_t* timer) {
#ifdef LV_REALDEVICE
    static int timeout_count = 0;
    
    if (vesc_limits_is_valid()) {
        // Data received, update UI
        const vesc_motor_limits_t* limits = vesc_limits_get();
        
        // Update spinboxes with received values
        lv_spinbox_set_value(settings_motor_current_spinbox, (int32_t)(limits->l_current_max * 10));
        lv_spinbox_set_value(settings_battery_current_spinbox, (int32_t)(limits->l_in_current_max * 10));
        lv_spinbox_set_value(settings_erpm_max_spinbox, (int32_t)(limits->l_erpm_max / 1000));
        
        // Update status
        char buf[128];
        sprintf(buf, "✅ Limits loaded: Mot %.1fA, Bat %.1fA, ERPM %dk", 
                limits->l_current_max, limits->l_in_current_max, limits->l_erpm_max / 1000);
        lv_label_set_text(settings_limits_status_label, buf);
        
        // Delete timer
        lv_timer_del(timer);
        timeout_count = 0;
    } else {
        // Check timeout (5 seconds)
        timeout_count++;
        if (timeout_count >= 50) { // 50 * 100ms = 5 seconds
            lv_label_set_text(settings_limits_status_label, "❌ Timeout: No response from VESC");
            lv_timer_del(timer);
            timeout_count = 0;
        }
    }
#else
    // Simulator mode - show placeholder values
    lv_spinbox_set_value(settings_motor_current_spinbox, 600);  // 60.0A
    lv_spinbox_set_value(settings_battery_current_spinbox, 450); // 45.0A
    lv_spinbox_set_value(settings_erpm_max_spinbox, 60);         // 60k ERPM
    lv_label_set_text(settings_limits_status_label, "✅ Simulator: Placeholder values loaded");
    lv_timer_del(timer);
#endif
}

// Event handlers for VESC Limits buttons
static void read_limits_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
#ifdef LV_REALDEVICE
        // Real device - communicate with VESC
        uint8_t target_vesc_id = settings_wrapper_get_target_vesc_id();
        
        // Update status label
        lv_label_set_text(settings_limits_status_label, "Reading limits from VESC...");
        
        // Request limits from VESC
        if (vesc_limits_request(target_vesc_id)) {
            // Request sent successfully, wait for response
            lv_label_set_text(settings_limits_status_label, "Request sent, waiting for response...");
            
            // Start a timer to check for response
            lv_timer_create(limits_response_timer_cb, 100, NULL);
            
        } else {
            // Request failed
            lv_label_set_text(settings_limits_status_label, "❌ Failed to send request to VESC");
        }
#else
        // Simulator mode - load placeholder values immediately
        lv_label_set_text(settings_limits_status_label, "Simulator: Loading placeholder values...");
        
        // Start a timer to simulate loading delay
        lv_timer_create(limits_response_timer_cb, 100, NULL);
#endif
    }
}

static void apply_limits_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        // Get values from spinboxes
        int32_t motor_current_raw = lv_spinbox_get_value(settings_motor_current_spinbox);
        int32_t battery_current_raw = lv_spinbox_get_value(settings_battery_current_spinbox);
        int32_t erpm_max_raw = lv_spinbox_get_value(settings_erpm_max_spinbox);
        
        // Convert to actual values
        float motor_current = motor_current_raw / 10.0f;  // Convert from 0.1A units
        float battery_current = battery_current_raw / 10.0f;  // Convert from 0.1A units
        float erpm_max = erpm_max_raw * 1000.0f;  // Convert from kERPM to ERPM
        
#ifdef LV_REALDEVICE
        // Real device - apply to VESC
        uint8_t target_vesc_id = settings_wrapper_get_target_vesc_id();
        
        // Update status
        lv_label_set_text(settings_limits_status_label, "Applying limits to VESC...");
        
        // Apply limits using the vesc_limits module
        bool success = true;
        
        // Set current limits
        if (!vesc_limits_set_current_max(target_vesc_id, motor_current, battery_current)) {
            success = false;
        }
        
        // Set speed limit
        if (success && !vesc_limits_set_speed_max(target_vesc_id, erpm_max)) {
            success = false;
        }
        
        // Update status based on result
        if (success) {
            char buf[128];
            sprintf(buf, "✅ Applied: Mot %.1fA, Bat %.1fA, ERPM %dk", 
                    motor_current, battery_current, erpm_max_raw);
            lv_label_set_text(settings_limits_status_label, buf);
        } else {
            lv_label_set_text(settings_limits_status_label, "❌ Failed to apply limits to VESC");
        }
#else
        // Simulator mode - just show confirmation
        char buf[128];
        sprintf(buf, "✅ Simulator: Values set - Mot %.1fA, Bat %.1fA, ERPM %dk", 
                motor_current, battery_current, erpm_max_raw);
        lv_label_set_text(settings_limits_status_label, buf);
#endif
    }
}

static void motor_current_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_motor_current_spinbox);
        if (current_value < 2000) {
            lv_spinbox_increment(settings_motor_current_spinbox);
        }
    }
}

static void motor_current_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_motor_current_spinbox);
        if (current_value > 50) {
            lv_spinbox_decrement(settings_motor_current_spinbox);
        }
    }
}

static void battery_current_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_battery_current_spinbox);
        if (current_value < 2000) {
            lv_spinbox_increment(settings_battery_current_spinbox);
        }
    }
}

static void battery_current_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_battery_current_spinbox);
        if (current_value > 0) {
            lv_spinbox_decrement(settings_battery_current_spinbox);
        }
    }
}

static void erpm_max_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_erpm_max_spinbox);
        if (current_value < 2000) {
            lv_spinbox_increment(settings_erpm_max_spinbox);
        }
    }
}

static void erpm_max_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_erpm_max_spinbox);
        if (current_value > 0) {
            lv_spinbox_decrement(settings_erpm_max_spinbox);
        }
    }
}

// Event handler for Show FPS switch
static void show_fps_switch_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *obj = lv_event_get_target(e);
        bool checked = lv_obj_has_state(obj, LV_STATE_CHECKED);
        settings_wrapper_set_show_fps(checked);
        
        // Apply immediately - show/hide FPS text
        if (guider_ui.dashboard_fps_text) {
            if (checked) {
                lv_obj_clear_flag(guider_ui.dashboard_fps_text, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(guider_ui.dashboard_fps_text, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

// Event handlers for Wheel Diameter spinbox
static void wheel_diameter_spinbox_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        int32_t value = lv_spinbox_get_value(settings_wheel_diameter_spinbox);
        settings_wrapper_set_wheel_diameter_mm((uint16_t)value);
    }
}

static void wheel_diameter_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_wheel_diameter_spinbox);
        if (current_value < 2000) {
            lv_spinbox_increment(settings_wheel_diameter_spinbox);
        }
    }
}

static void wheel_diameter_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_wheel_diameter_spinbox);
        if (current_value > 0) {
            lv_spinbox_decrement(settings_wheel_diameter_spinbox);
        }
    }
}

// Event handlers for Motor Poles spinbox
static void motor_poles_spinbox_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        int32_t value = lv_spinbox_get_value(settings_motor_poles_spinbox);
        settings_wrapper_set_motor_poles((uint8_t)value);
    }
}

static void motor_poles_plus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_motor_poles_spinbox);
        if (current_value < 50) {
            lv_spinbox_increment(settings_motor_poles_spinbox);
        }
    }
}

static void motor_poles_minus_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int32_t current_value = lv_spinbox_get_value(settings_motor_poles_spinbox);
        if (current_value > 0) {
            lv_spinbox_decrement(settings_motor_poles_spinbox);
        }
    }
}

// Event handler for Reset button
static void reset_button_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        // Reset all settings to defaults
        settings_wrapper_set_target_vesc_id(10);
        settings_wrapper_set_can_speed_index(3); // 1000 kbps
        settings_wrapper_set_brightness(80);
        settings_wrapper_set_controller_id(2);
        settings_wrapper_set_battery_capacity(15.0f);
        settings_wrapper_set_battery_calc_mode(0); // Direct
        settings_wrapper_set_show_fps(true);
        settings_wrapper_set_wheel_diameter_mm(200); // 200mm
        settings_wrapper_set_motor_poles(7); // Standard for VESC
        
        // Update UI elements
        if (settings_target_id_spinbox) {
            lv_spinbox_set_value(settings_target_id_spinbox, 10);
        }
        if (settings_can_speed_dropdown) {
            lv_dropdown_set_selected(settings_can_speed_dropdown, 3);
        }
        if (settings_brightness_slider) {
            lv_slider_set_value(settings_brightness_slider, 80, LV_ANIM_ON);
        }
        if (settings_controller_id_slider) {
            lv_slider_set_value(settings_controller_id_slider, 2, LV_ANIM_ON);
        }
        if (settings_battery_capacity_spinbox) {
            lv_spinbox_set_value(settings_battery_capacity_spinbox, 150); // 15.0 Ah
        }
        if (settings_battery_calc_mode_dropdown) {
            lv_dropdown_set_selected(settings_battery_calc_mode_dropdown, 0);
        }
        if (settings_show_fps_switch) {
            lv_obj_add_state(settings_show_fps_switch, LV_STATE_CHECKED);
        }
        if (settings_wheel_diameter_spinbox) {
            lv_spinbox_set_value(settings_wheel_diameter_spinbox, 200); // 200mm
        }
        if (settings_motor_poles_spinbox) {
            lv_spinbox_set_value(settings_motor_poles_spinbox, 7); // Standard for VESC
        }

        // Apply FPS visibility
        if (guider_ui.dashboard_fps_text) {
            lv_obj_clear_flag(guider_ui.dashboard_fps_text, LV_OBJ_FLAG_HIDDEN);
        }

        // Update info
        lv_label_set_text(settings_info_label, "Settings reset to defaults!");
    }
}

// Initialize settings UI - called from custom_init or when settings screen loads
void settings_ui_init(lv_ui *ui) {
    if (!ui || !ui->settings) {
        return;
    }
    
    // Check if already initialized (prevent double initialization)
    if (settings_target_id_spinbox != NULL) {
        return; // Already initialized
    }
    
    // Initialize modules
    settings_wrapper_init();
    
#ifdef LV_REALDEVICE
    // Initialize vesc_limits only on real device
    vesc_limits_init();
#endif
    
    // Get current settings
    uint8_t target_id = settings_wrapper_get_target_vesc_id();
    uint8_t can_speed_idx = settings_wrapper_get_can_speed_index();
    uint8_t brightness = settings_wrapper_get_brightness();
    uint8_t controller_id = settings_wrapper_get_controller_id();
    float battery_capacity = settings_wrapper_get_battery_capacity();
    uint8_t battery_calc_mode = settings_wrapper_get_battery_calc_mode();
    
    int y_pos = 70; // Start below "Back to dashboard" button
    int spacing = 90;
    
    // ========== Target VESC ID Spinbox ==========
    settings_target_id_label = lv_label_create(ui->settings);
    char buf[32];
    sprintf(buf, "Target VESC ID:");
    lv_label_set_text(settings_target_id_label, buf);
    lv_obj_set_pos(settings_target_id_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_target_id_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_target_id_label, &lv_font_montserrat_24, 0);
    
    // Create Minus button (left side)
    settings_target_id_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *minus_label = lv_label_create(settings_target_id_minus_btn);
    lv_label_set_text(minus_label, "-");
    lv_obj_align(minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_target_id_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_target_id_minus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_target_id_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_target_id_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_target_id_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_target_id_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_target_id_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_target_id_minus_btn, target_id_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create spinbox for VESC ID selection (center) - positioned relative to minus button
    settings_target_id_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_target_id_spinbox, 1, 254);
    lv_spinbox_set_digit_format(settings_target_id_spinbox, 3, 0); // 3 digits, 0 decimal places
    lv_spinbox_set_value(settings_target_id_spinbox, target_id);
    lv_spinbox_set_step(settings_target_id_spinbox, 1);
    lv_obj_set_pos(settings_target_id_spinbox, 190, y_pos + 30); // 20 + 60 + 10 margin
    lv_obj_set_size(settings_target_id_spinbox, 100, 50);
    
    // Style the spinbox
    lv_obj_set_style_bg_color(settings_target_id_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_target_id_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_target_id_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_target_id_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_target_id_spinbox, lv_color_hex(0x00a9ff), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_target_id_spinbox, 8, LV_PART_MAIN);
    
    lv_obj_add_event_cb(settings_target_id_spinbox, target_id_spinbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Create Plus button (right side) - positioned relative to spinbox
    settings_target_id_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *plus_label = lv_label_create(settings_target_id_plus_btn);
    lv_label_set_text(plus_label, "+");
    lv_obj_align(plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_target_id_plus_btn, 360, y_pos + 30); // 90 + 300 + 10 margin
    lv_obj_set_size(settings_target_id_plus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_target_id_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_target_id_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_target_id_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_target_id_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_target_id_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_target_id_plus_btn, target_id_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += spacing;
    
    // ========== CAN Speed Dropdown ==========
    settings_can_speed_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_can_speed_label, "CAN Speed (kbps)");
    lv_obj_set_pos(settings_can_speed_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_can_speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_can_speed_label, &lv_font_montserrat_24, 0);
    
    settings_can_speed_dropdown = lv_dropdown_create(ui->settings);
    lv_dropdown_set_options(settings_can_speed_dropdown, "125 kbps\n250 kbps\n500 kbps\n1000 kbps");
    lv_dropdown_set_selected(settings_can_speed_dropdown, can_speed_idx);
    lv_obj_set_pos(settings_can_speed_dropdown, 20, y_pos + 30);
    lv_obj_set_size(settings_can_speed_dropdown, 440, 50); // Match height with spinbox group
    lv_obj_set_style_bg_color(settings_can_speed_dropdown, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_text_color(settings_can_speed_dropdown, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_can_speed_dropdown, &lv_font_montserrat_24, 0);
    lv_obj_set_style_border_width(settings_can_speed_dropdown, 0, 0);
    lv_obj_set_style_radius(settings_can_speed_dropdown, 8, 0); // Match radius with other elements
    lv_obj_add_event_cb(settings_can_speed_dropdown, can_speed_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    y_pos += spacing + 10;
    
    // ========== Battery Capacity Spinbox ==========
    settings_battery_capacity_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_battery_capacity_label, "Battery Capacity (Ah):");
    lv_obj_set_pos(settings_battery_capacity_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_battery_capacity_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_capacity_label, &lv_font_montserrat_24, 0);
    
    // Create Minus button (left side)
    settings_battery_capacity_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *bat_minus_label = lv_label_create(settings_battery_capacity_minus_btn);
    lv_label_set_text(bat_minus_label, "-");
    lv_obj_align(bat_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_battery_capacity_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_battery_capacity_minus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_battery_capacity_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_battery_capacity_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_capacity_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_battery_capacity_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_battery_capacity_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_battery_capacity_minus_btn, battery_capacity_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create spinbox for Battery Capacity (center)
    settings_battery_capacity_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_battery_capacity_spinbox, 10, 2000); // 1.0 to 200.0 Ah
    lv_spinbox_set_digit_format(settings_battery_capacity_spinbox, 4, 3); // 4 digits, 1 decimal place (e.g., "15.0")
    lv_spinbox_set_value(settings_battery_capacity_spinbox, (int32_t)(battery_capacity * 10.0f));
    lv_spinbox_set_step(settings_battery_capacity_spinbox, 1); // 0.1 Ah steps
    lv_obj_set_pos(settings_battery_capacity_spinbox, 190, y_pos + 30);
    lv_obj_set_size(settings_battery_capacity_spinbox, 100, 50);
    
    // Style the spinbox
    lv_obj_set_style_bg_color(settings_battery_capacity_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_battery_capacity_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_battery_capacity_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_battery_capacity_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_battery_capacity_spinbox, lv_color_hex(0xffa500), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_battery_capacity_spinbox, 8, LV_PART_MAIN);
    
    lv_obj_add_event_cb(settings_battery_capacity_spinbox, battery_capacity_spinbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Create Plus button (right side)
    settings_battery_capacity_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *bat_plus_label = lv_label_create(settings_battery_capacity_plus_btn);
    lv_label_set_text(bat_plus_label, "+");
    lv_obj_align(bat_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_battery_capacity_plus_btn, 360, y_pos + 30);
    lv_obj_set_size(settings_battery_capacity_plus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_battery_capacity_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_battery_capacity_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_capacity_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_battery_capacity_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_battery_capacity_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_battery_capacity_plus_btn, battery_capacity_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += spacing;
    
    // ========== Battery Calculation Mode Dropdown ==========
    settings_battery_calc_mode_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_battery_calc_mode_label, "Battery Calculation:");
    lv_obj_set_pos(settings_battery_calc_mode_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_battery_calc_mode_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_calc_mode_label, &lv_font_montserrat_24, 0);
    
    settings_battery_calc_mode_dropdown = lv_dropdown_create(ui->settings);
    lv_dropdown_set_options(settings_battery_calc_mode_dropdown, "Direct from Controller\nSmart Calculation");
    lv_dropdown_set_selected(settings_battery_calc_mode_dropdown, battery_calc_mode);
    lv_obj_set_pos(settings_battery_calc_mode_dropdown, 20, y_pos + 30);
    lv_obj_set_size(settings_battery_calc_mode_dropdown, 440, 50);
    lv_obj_set_style_bg_color(settings_battery_calc_mode_dropdown, lv_color_hex(0x2a3440), 0);
    lv_obj_set_style_text_color(settings_battery_calc_mode_dropdown, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_calc_mode_dropdown, &lv_font_montserrat_24, 0);
    lv_obj_set_style_border_width(settings_battery_calc_mode_dropdown, 0, 0);
    lv_obj_set_style_radius(settings_battery_calc_mode_dropdown, 8, 0);
    lv_obj_add_event_cb(settings_battery_calc_mode_dropdown, battery_calc_mode_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    y_pos += spacing + 10;
    /*
    // ========== VESC LIMITS SECTION ==========
    // Title
    settings_limits_title_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_limits_title_label, "=== VESC Motor Limits ===");
    lv_obj_set_pos(settings_limits_title_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_limits_title_label, lv_color_hex(0xffa500), 0);
    lv_obj_set_style_text_font(settings_limits_title_label, &lv_font_montserrat_24, 0);
    
    y_pos += 35;
    
    // Read Limits Button
    settings_read_limits_btn = lv_btn_create(ui->settings);
    lv_obj_t *read_limits_label = lv_label_create(settings_read_limits_btn);
    lv_label_set_text(read_limits_label, "Read Limits from VESC");
    lv_obj_align(read_limits_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_read_limits_btn, 20, y_pos);
    lv_obj_set_size(settings_read_limits_btn, 440, 50);
    lv_obj_set_style_bg_color(settings_read_limits_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_read_limits_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_read_limits_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_read_limits_btn, 8, 0);
    lv_obj_set_style_border_width(settings_read_limits_btn, 0, 0);
    lv_obj_add_event_cb(settings_read_limits_btn, read_limits_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += 70;
    
    // Motor Current Max Spinbox
    settings_motor_current_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_motor_current_label, "Motor Current Max (A):");
    lv_obj_set_pos(settings_motor_current_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_motor_current_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_current_label, &lv_font_montserrat_24, 0);
    
    // Minus button
    settings_motor_current_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *motor_curr_minus_label = lv_label_create(settings_motor_current_minus_btn);
    lv_label_set_text(motor_curr_minus_label, "-");
    lv_obj_align(motor_curr_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_motor_current_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_motor_current_minus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_motor_current_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_motor_current_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_current_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_motor_current_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_motor_current_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_motor_current_minus_btn, motor_current_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Spinbox
    settings_motor_current_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_motor_current_spinbox, 50, 2000); // 5.0 to 200.0 A
    lv_spinbox_set_digit_format(settings_motor_current_spinbox, 4, 3); // Format: "60.0"
    lv_spinbox_set_value(settings_motor_current_spinbox, 600); // Default 60.0A
    lv_spinbox_set_step(settings_motor_current_spinbox, 10); // 1.0 A steps
    lv_obj_set_pos(settings_motor_current_spinbox, 190, y_pos + 30);
    lv_obj_set_size(settings_motor_current_spinbox, 100, 50);
    lv_obj_set_style_bg_color(settings_motor_current_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_motor_current_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_motor_current_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_motor_current_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_motor_current_spinbox, lv_color_hex(0xff6600), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_motor_current_spinbox, 8, LV_PART_MAIN);
    
    // Plus button
    settings_motor_current_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *motor_curr_plus_label = lv_label_create(settings_motor_current_plus_btn);
    lv_label_set_text(motor_curr_plus_label, "+");
    lv_obj_align(motor_curr_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_motor_current_plus_btn, 360, y_pos + 30);
    lv_obj_set_size(settings_motor_current_plus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_motor_current_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_motor_current_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_current_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_motor_current_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_motor_current_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_motor_current_plus_btn, motor_current_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += spacing;
    
    // Battery Current Max Spinbox
    settings_battery_current_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_battery_current_label, "Battery Current Max (A):");
    lv_obj_set_pos(settings_battery_current_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_battery_current_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_current_label, &lv_font_montserrat_24, 0);
    
    // Minus button
    settings_battery_current_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *batt_curr_minus_label = lv_label_create(settings_battery_current_minus_btn);
    lv_label_set_text(batt_curr_minus_label, "-");
    lv_obj_align(batt_curr_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_battery_current_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_battery_current_minus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_battery_current_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_battery_current_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_current_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_battery_current_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_battery_current_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_battery_current_minus_btn, battery_current_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Spinbox
    settings_battery_current_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_battery_current_spinbox, 50, 2000); // 5.0 to 200.0 A
    lv_spinbox_set_digit_format(settings_battery_current_spinbox, 4, 3); // Format: "45.0"
    lv_spinbox_set_value(settings_battery_current_spinbox, 450); // Default 45.0A
    lv_spinbox_set_step(settings_battery_current_spinbox, 10); // 1.0 A steps
    lv_obj_set_pos(settings_battery_current_spinbox, 190, y_pos + 30);
    lv_obj_set_size(settings_battery_current_spinbox, 100, 50);
    lv_obj_set_style_bg_color(settings_battery_current_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_battery_current_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_battery_current_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_battery_current_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_battery_current_spinbox, lv_color_hex(0xffaa00), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_battery_current_spinbox, 8, LV_PART_MAIN);
    
    // Plus button
    settings_battery_current_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *batt_curr_plus_label = lv_label_create(settings_battery_current_plus_btn);
    lv_label_set_text(batt_curr_plus_label, "+");
    lv_obj_align(batt_curr_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_battery_current_plus_btn, 360, y_pos + 30);
    lv_obj_set_size(settings_battery_current_plus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_battery_current_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_battery_current_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_battery_current_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_battery_current_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_battery_current_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_battery_current_plus_btn, battery_current_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += spacing;
    
    // ERPM Max Spinbox
    settings_erpm_max_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_erpm_max_label, "ERPM Max (x1000):");
    lv_obj_set_pos(settings_erpm_max_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_erpm_max_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_erpm_max_label, &lv_font_montserrat_24, 0);
    
    // Minus button
    settings_erpm_max_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *erpm_minus_label = lv_label_create(settings_erpm_max_minus_btn);
    lv_label_set_text(erpm_minus_label, "-");
    lv_obj_align(erpm_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_erpm_max_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_erpm_max_minus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_erpm_max_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_erpm_max_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_erpm_max_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_erpm_max_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_erpm_max_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_erpm_max_minus_btn, erpm_max_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Spinbox  
    settings_erpm_max_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_erpm_max_spinbox, 10, 200); // 10k to 200k ERPM
    lv_spinbox_set_digit_format(settings_erpm_max_spinbox, 3, 0); // Format: "60" (means 60k)
    lv_spinbox_set_value(settings_erpm_max_spinbox, 60); // Default 60k ERPM
    lv_spinbox_set_step(settings_erpm_max_spinbox, 5); // 5k ERPM steps
    lv_obj_set_pos(settings_erpm_max_spinbox, 190, y_pos + 30);
    lv_obj_set_size(settings_erpm_max_spinbox, 100, 50);
    lv_obj_set_style_bg_color(settings_erpm_max_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_erpm_max_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_erpm_max_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_erpm_max_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_erpm_max_spinbox, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_erpm_max_spinbox, 8, LV_PART_MAIN);
    
    // Plus button
    settings_erpm_max_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *erpm_plus_label = lv_label_create(settings_erpm_max_plus_btn);
    lv_label_set_text(erpm_plus_label, "+");
    lv_obj_align(erpm_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_erpm_max_plus_btn, 360, y_pos + 30);
    lv_obj_set_size(settings_erpm_max_plus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_erpm_max_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_erpm_max_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_erpm_max_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_erpm_max_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_erpm_max_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_erpm_max_plus_btn, erpm_max_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += spacing;
    
    // Apply Limits Button
    settings_apply_limits_btn = lv_btn_create(ui->settings);
    lv_obj_t *apply_limits_label = lv_label_create(settings_apply_limits_btn);
    lv_label_set_text(apply_limits_label, "Apply Limits to VESC");
    lv_obj_align(apply_limits_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_apply_limits_btn, 20, y_pos);
    lv_obj_set_size(settings_apply_limits_btn, 440, 50);
    lv_obj_set_style_bg_color(settings_apply_limits_btn, lv_color_hex(0xff6600), 0);
    lv_obj_set_style_text_color(settings_apply_limits_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_apply_limits_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_apply_limits_btn, 8, 0);
    lv_obj_set_style_border_width(settings_apply_limits_btn, 0, 0);
    lv_obj_add_event_cb(settings_apply_limits_btn, apply_limits_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += 70;
    
    // Limits Status Label
    settings_limits_status_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_limits_status_label, "Press 'Read Limits' to load VESC values");
    lv_obj_set_pos(settings_limits_status_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_limits_status_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(settings_limits_status_label, &lv_font_montserrat_24, 0);
    
    y_pos += spacing;
    */
    /*
    // ========== Brightness Slider ==========
    settings_brightness_label = lv_label_create(ui->settings);
    sprintf(buf, "Brightness: %d%%", brightness);
    lv_label_set_text(settings_brightness_label, buf);
    lv_obj_set_pos(settings_brightness_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_brightness_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_brightness_label, &lv_font_montserrat_24, 0);
    
    settings_brightness_slider = lv_slider_create(ui->settings);
    lv_slider_set_range(settings_brightness_slider, 10, 100);
    lv_slider_set_value(settings_brightness_slider, brightness, LV_ANIM_OFF);
    lv_obj_set_pos(settings_brightness_slider, 20, y_pos + 35);
    lv_obj_set_size(settings_brightness_slider, 440, 20); // Slightly thicker for better touch
    lv_obj_set_style_bg_color(settings_brightness_slider, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_brightness_slider, lv_color_hex(0xffa500), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(settings_brightness_slider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_set_style_radius(settings_brightness_slider, 10, LV_PART_MAIN); // Rounded slider
    lv_obj_set_style_radius(settings_brightness_slider, 10, LV_PART_INDICATOR);
    lv_obj_add_event_cb(settings_brightness_slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    y_pos += spacing;
    */
    /*
    // ========== Controller ID Slider ==========
    settings_controller_id_label = lv_label_create(ui->settings);
    sprintf(buf, "Controller ID: %d", controller_id);
    lv_label_set_text(settings_controller_id_label, buf);
    lv_obj_set_pos(settings_controller_id_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_controller_id_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_controller_id_label, &lv_font_montserratMedium_16, 0);
    
    settings_controller_id_slider = lv_slider_create(ui->settings);
    lv_slider_set_range(settings_controller_id_slider, 1, 254);
    lv_slider_set_value(settings_controller_id_slider, controller_id, LV_ANIM_OFF);
    lv_obj_set_pos(settings_controller_id_slider, 20, y_pos + 30);
    lv_obj_set_size(settings_controller_id_slider, 440, 15);
    lv_obj_set_style_bg_color(settings_controller_id_slider, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_controller_id_slider, lv_color_hex(0x00ff00), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(settings_controller_id_slider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_add_event_cb(settings_controller_id_slider, controller_id_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    y_pos += spacing;
    */
    
    // ========== Show FPS Switch ==========
    bool show_fps = settings_wrapper_get_show_fps();
    
    settings_show_fps_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_show_fps_label, "Show FPS Counter");
    lv_obj_set_pos(settings_show_fps_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_show_fps_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_show_fps_label, &lv_font_montserrat_24, 0);
    
    settings_show_fps_switch = lv_switch_create(ui->settings);
    lv_obj_set_pos(settings_show_fps_switch, 380, y_pos - 5);
    lv_obj_set_size(settings_show_fps_switch, 60, 30);
    
    // Set switch state based on current setting
    if (show_fps) {
        lv_obj_add_state(settings_show_fps_switch, LV_STATE_CHECKED);
    }
    
    // Style the switch
    lv_obj_set_style_bg_color(settings_show_fps_switch, lv_color_hex(0x2a3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_show_fps_switch, lv_color_hex(0x00a9ff), LV_PART_INDICATOR | LV_STATE_CHECKED);
    
    lv_obj_add_event_cb(settings_show_fps_switch, show_fps_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    y_pos += spacing;
    /*
    // ========== Wheel Diameter Spinbox ==========
    uint16_t wheel_diameter = settings_wrapper_get_wheel_diameter_mm();
    
    settings_wheel_diameter_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_wheel_diameter_label, "Wheel Diameter (mm):");
    lv_obj_set_pos(settings_wheel_diameter_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_wheel_diameter_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_wheel_diameter_label, &lv_font_montserrat_24, 0);
    
    // Minus button
    settings_wheel_diameter_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *wheel_diam_minus_label = lv_label_create(settings_wheel_diameter_minus_btn);
    lv_label_set_text(wheel_diam_minus_label, "-");
    lv_obj_align(wheel_diam_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_wheel_diameter_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_wheel_diameter_minus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_wheel_diameter_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_wheel_diameter_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_wheel_diameter_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_wheel_diameter_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_wheel_diameter_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_wheel_diameter_minus_btn, wheel_diameter_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Spinbox
    settings_wheel_diameter_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_wheel_diameter_spinbox, 0, 2000);
    lv_spinbox_set_digit_format(settings_wheel_diameter_spinbox, 3, 0); // 3 digits, 0 decimal places
    lv_spinbox_set_value(settings_wheel_diameter_spinbox, wheel_diameter);
    lv_spinbox_set_step(settings_wheel_diameter_spinbox, 5); // 5mm increments
    lv_obj_set_pos(settings_wheel_diameter_spinbox, 190, y_pos + 30);
    lv_obj_set_size(settings_wheel_diameter_spinbox, 100, 50);
    
    // Style the spinbox
    lv_obj_set_style_bg_color(settings_wheel_diameter_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_wheel_diameter_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_wheel_diameter_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_wheel_diameter_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_wheel_diameter_spinbox, lv_color_hex(0x00a9ff), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_wheel_diameter_spinbox, 8, LV_PART_MAIN);
    
    lv_obj_add_event_cb(settings_wheel_diameter_spinbox, wheel_diameter_spinbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Plus button
    settings_wheel_diameter_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *wheel_diam_plus_label = lv_label_create(settings_wheel_diameter_plus_btn);
    lv_label_set_text(wheel_diam_plus_label, "+");
    lv_obj_align(wheel_diam_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_wheel_diameter_plus_btn, 360, y_pos + 30);
    lv_obj_set_size(settings_wheel_diameter_plus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_wheel_diameter_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_wheel_diameter_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_wheel_diameter_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_wheel_diameter_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_wheel_diameter_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_wheel_diameter_plus_btn, wheel_diameter_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);

    y_pos += spacing;

    // ========== Motor Poles Spinbox ==========
    uint8_t motor_poles = settings_wrapper_get_motor_poles();

    settings_motor_poles_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_motor_poles_label, "Motor Poles:");
    lv_obj_set_pos(settings_motor_poles_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_motor_poles_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_poles_label, &lv_font_montserrat_24, 0);

    // Minus button
    settings_motor_poles_minus_btn = lv_btn_create(ui->settings);
    lv_obj_t *motor_poles_minus_label = lv_label_create(settings_motor_poles_minus_btn);
    lv_label_set_text(motor_poles_minus_label, "-");
    lv_obj_align(motor_poles_minus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_motor_poles_minus_btn, 20, y_pos + 30);
    lv_obj_set_size(settings_motor_poles_minus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_motor_poles_minus_btn, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_motor_poles_minus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_poles_minus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_motor_poles_minus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_motor_poles_minus_btn, 0, 0);
    lv_obj_add_event_cb(settings_motor_poles_minus_btn, motor_poles_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Spinbox
    settings_motor_poles_spinbox = lv_spinbox_create(ui->settings);
    lv_spinbox_set_range(settings_motor_poles_spinbox, 0, 50);
    lv_spinbox_set_digit_format(settings_motor_poles_spinbox, 2, 0); // 2 digits, 0 decimal places
    lv_spinbox_set_value(settings_motor_poles_spinbox, motor_poles);
    lv_spinbox_set_step(settings_motor_poles_spinbox, 1);
    lv_obj_set_pos(settings_motor_poles_spinbox, 190, y_pos + 30);
    lv_obj_set_size(settings_motor_poles_spinbox, 100, 50);

    // Style the spinbox
    lv_obj_set_style_bg_color(settings_motor_poles_spinbox, lv_color_hex(0x1f1f1f), LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_motor_poles_spinbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(settings_motor_poles_spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_motor_poles_spinbox, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_motor_poles_spinbox, lv_color_hex(0x00a9ff), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_motor_poles_spinbox, 8, LV_PART_MAIN);

    lv_obj_add_event_cb(settings_motor_poles_spinbox, motor_poles_spinbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Plus button
    settings_motor_poles_plus_btn = lv_btn_create(ui->settings);
    lv_obj_t *motor_poles_plus_label = lv_label_create(settings_motor_poles_plus_btn);
    lv_label_set_text(motor_poles_plus_label, "+");
    lv_obj_align(motor_poles_plus_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_motor_poles_plus_btn, 360, y_pos + 30);
    lv_obj_set_size(settings_motor_poles_plus_btn, 100, 50);
    lv_obj_set_style_bg_color(settings_motor_poles_plus_btn, lv_color_hex(0x00a9ff), 0);
    lv_obj_set_style_text_color(settings_motor_poles_plus_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_motor_poles_plus_btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_motor_poles_plus_btn, 8, 0);
    lv_obj_set_style_border_width(settings_motor_poles_plus_btn, 0, 0);
    lv_obj_add_event_cb(settings_motor_poles_plus_btn, motor_poles_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);

    y_pos += spacing;
*/
    // ========== Reset Button ==========
    settings_reset_button = lv_btn_create(ui->settings);
    lv_obj_t *reset_label = lv_label_create(settings_reset_button);
    lv_label_set_text(reset_label, "Reset to Defaults");
    lv_obj_align(reset_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(settings_reset_button, 20, y_pos);
    lv_obj_set_size(settings_reset_button, 440, 50); // Match height with other elements
    lv_obj_set_style_bg_color(settings_reset_button, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_color(settings_reset_button, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(settings_reset_button, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(settings_reset_button, 8, 0); // Match radius with other elements
    lv_obj_set_style_border_width(settings_reset_button, 0, 0);
    lv_obj_add_event_cb(settings_reset_button, reset_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    y_pos += spacing;
    
    // ========== Info Label ==========
    settings_info_label = lv_label_create(ui->settings);
    lv_label_set_text(settings_info_label, "Settings saved automatically");
    lv_obj_set_pos(settings_info_label, 20, y_pos);
    lv_obj_set_style_text_color(settings_info_label, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_font(settings_info_label, &lv_font_montserrat_24, 0);
}

