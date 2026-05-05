/*
	Copyright 2025 Super VESC Display
	
	Settings wrapper for LVGL GUI (works in simulator and on device)
*/

#include "settings_wrapper.h"
#include "lv_conf.h"

// Determine if we're running in simulator using LVGL's flag
#ifndef LV_REALDEVICE
    #define SIMULATOR_MODE 1
#else
    #define SIMULATOR_MODE 0
    // On device - include real settings
    #include "dev_settings.h"
#endif

// Simulator-mode storage
#if SIMULATOR_MODE
static struct {
    uint8_t target_vesc_id;
    uint8_t can_speed_index;
    uint8_t brightness;
    uint8_t controller_id;
    float battery_capacity;
    uint8_t battery_calc_mode;
    bool show_fps;
    uint16_t wheel_diameter_mm;
    uint8_t motor_poles;
} sim_settings = {
    .target_vesc_id = 10,
    .can_speed_index = 3,  // 1000 kbps
    .brightness = 80,
    .controller_id = 255,
    .battery_capacity = 15.0f,
    .battery_calc_mode = 0,  // Direct
    .show_fps = true,
    .wheel_diameter_mm = 200,  // 200mm default
    .motor_poles = 7  // Standard for VESC motors
};
#endif

void settings_wrapper_init(void) {
#if !SIMULATOR_MODE
    settings_init();
#endif
}

uint8_t settings_wrapper_get_target_vesc_id(void) {
#if SIMULATOR_MODE
    return sim_settings.target_vesc_id;
#else
    return settings_get_target_vesc_id();
#endif
}

uint8_t settings_wrapper_get_can_speed_index(void) {
#if SIMULATOR_MODE
    return sim_settings.can_speed_index;
#else
    // Convert kbps to index
    can_speed_t speed = settings_get_can_speed();
    switch ((int)speed) {
        case 125: return 0;
        case 250: return 1;
        case 500: return 2;
        case 1000: return 3;
        default: return 1; // Default to 250
    }
#endif
}

uint8_t settings_wrapper_get_brightness(void) {
#if SIMULATOR_MODE
    return sim_settings.brightness;
#else
    return settings_get_screen_brightness();
#endif
}

uint8_t settings_wrapper_get_controller_id(void) {
#if SIMULATOR_MODE
    return sim_settings.controller_id;
#else
    return settings_get_controller_id();
#endif
}

float settings_wrapper_get_battery_capacity(void) {
#if SIMULATOR_MODE
    return sim_settings.battery_capacity;
#else
    return settings_get_battery_capacity();
#endif
}

uint8_t settings_wrapper_get_battery_calc_mode(void) {
#if SIMULATOR_MODE
    return sim_settings.battery_calc_mode;
#else
    return (uint8_t)settings_get_battery_calc_mode();
#endif
}

bool settings_wrapper_get_show_fps(void) {
#if SIMULATOR_MODE
    return sim_settings.show_fps;
#else
    return settings_get_show_fps();
#endif
}

uint16_t settings_wrapper_get_wheel_diameter_mm(void) {
#if SIMULATOR_MODE
    return sim_settings.wheel_diameter_mm;
#else
    return settings_get_wheel_diameter_mm();
#endif
}

uint8_t settings_wrapper_get_motor_poles(void) {
#if SIMULATOR_MODE
    return sim_settings.motor_poles;
#else
    return settings_get_motor_poles();
#endif
}

void settings_wrapper_set_target_vesc_id(uint8_t id) {
#if SIMULATOR_MODE
    sim_settings.target_vesc_id = id;
#else
    settings_set_target_vesc_id(id);
#endif
}

void settings_wrapper_set_can_speed_index(uint8_t index) {
#if SIMULATOR_MODE
    sim_settings.can_speed_index = index;
#else
    // Convert index to speed
    can_speed_t speed;
    switch (index) {
        case 0: speed = CAN_SPEED_125_KBPS; break;
        case 1: speed = CAN_SPEED_250_KBPS; break;
        case 2: speed = CAN_SPEED_500_KBPS; break;
        case 3: speed = CAN_SPEED_1000_KBPS; break;
        default: speed = CAN_SPEED_250_KBPS; break;
    }
    settings_set_can_speed(speed);
#endif
}

void settings_wrapper_set_brightness(uint8_t brightness) {
#if SIMULATOR_MODE
    sim_settings.brightness = brightness;
#else
    settings_set_screen_brightness(brightness);
#endif
}

void settings_wrapper_set_controller_id(uint8_t id) {
#if SIMULATOR_MODE
    sim_settings.controller_id = id;
#else
    settings_set_controller_id(id);
#endif
}

void settings_wrapper_set_battery_capacity(float capacity) {
#if SIMULATOR_MODE
    sim_settings.battery_capacity = capacity;
#else
    settings_set_battery_capacity(capacity);
#endif
}

void settings_wrapper_set_battery_calc_mode(uint8_t mode) {
#if SIMULATOR_MODE
    sim_settings.battery_calc_mode = mode;
#else
    settings_set_battery_calc_mode((battery_calc_mode_t)mode);
#endif
}

void settings_wrapper_set_show_fps(bool show) {
#if SIMULATOR_MODE
    sim_settings.show_fps = show;
#else
    settings_set_show_fps(show);
#endif
}

void settings_wrapper_set_wheel_diameter_mm(uint16_t diameter_mm) {
#if SIMULATOR_MODE
    sim_settings.wheel_diameter_mm = diameter_mm;
#else
    settings_set_wheel_diameter_mm(diameter_mm);
#endif
}

void settings_wrapper_set_motor_poles(uint8_t poles) {
#if SIMULATOR_MODE
    sim_settings.motor_poles = poles;
#else
    settings_set_motor_poles(poles);
#endif
}

int settings_wrapper_can_speed_to_kbps(uint8_t index) {
    switch (index) {
        case 0: return 125;
        case 1: return 250;
        case 2: return 500;
        case 3: return 1000;
        default: return 250;
    }
}

// Calculate speed in km/h from ERPM, taking into account wheel diameter and motor poles
float settings_wrapper_calculate_speed_kmh(float erpm) {
    // Get settings
    uint16_t wheel_diameter_mm = settings_wrapper_get_wheel_diameter_mm();
    uint8_t motor_poles = settings_wrapper_get_motor_poles();

    // Avoid division by zero
    if (motor_poles == 0 || wheel_diameter_mm == 0) {
        return 0.0f;
    }

    // Convert ERPM to RPM
    float rpm = erpm / (float)motor_poles;

    // Convert wheel diameter from mm to meters
    float wheel_radius_m = (float)wheel_diameter_mm / 2000.0f; // mm to meters (diameter -> radius)

    // Calculate angular velocity (rad/s)
    float omega_rad_s = rpm * 2.0f * 3.14159f / 60.0f;

    // Calculate linear velocity (m/s)
    float velocity_ms = omega_rad_s * wheel_radius_m;

    // Convert to km/h
    float velocity_kmh = velocity_ms * 3.6f;

    return velocity_kmh;
}

