/*
	Copyright 2025 Super VESC Display
	
	Settings wrapper for LVGL GUI (works in simulator and on device)
*/

#include "settings_wrapper.h"
#include "lv_conf.h"
#include <stdio.h>
#include <time.h>

// Determine if we're running in simulator using LVGL's flag
#ifndef LV_REALDEVICE
    #define SIMULATOR_MODE 1
#else
    #define SIMULATOR_MODE 0
    // On device - include real settings
    #include "dev_settings.h"
    #include "esp_system.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
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
    float power_max_kw;
    uint32_t clock_offset_secs;
    bool aa_autoconnect;
    bool use_imperial;
    bool use_fahrenheit;
} sim_settings = {
    .target_vesc_id = 10,
    .can_speed_index = 3,  // 1000 kbps
    .brightness = 80,
    .controller_id = 255,
    .battery_capacity = 15.0f,
    .battery_calc_mode = 0,  // Direct
    .show_fps = true,
    .wheel_diameter_mm = 200,  // 200mm default
    .motor_poles = 7,  // Standard for VESC motors
    .power_max_kw = 4.5f,
    .clock_offset_secs = 0,
    .aa_autoconnect = true,
    .use_imperial = false,
    .use_fahrenheit = false,
};
#endif

/* km → miles. Same factor converts km/h → mph. */
#define MILES_PER_KM 0.621371f

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

float settings_wrapper_get_power_max_kw(void) {
#if SIMULATOR_MODE
    return sim_settings.power_max_kw;
#else
    return settings_get_power_max_kw();
#endif
}

void settings_wrapper_set_power_max_kw(float power_max_kw) {
#if SIMULATOR_MODE
    sim_settings.power_max_kw = power_max_kw;
#else
    settings_set_power_max_kw(power_max_kw);
#endif
}

bool settings_wrapper_get_vesc_emulator(void) {
#if SIMULATOR_MODE
    return false;
#else
    return settings_get_vesc_emulator();
#endif
}

void settings_wrapper_set_vesc_emulator(bool on) {
#if SIMULATOR_MODE
    (void)on;
#else
    settings_set_vesc_emulator(on);
#endif
}

bool settings_wrapper_get_aa_autoconnect(void) {
#if SIMULATOR_MODE
    return sim_settings.aa_autoconnect;
#else
    return settings_get_aa_autoconnect();
#endif
}

void settings_wrapper_set_aa_autoconnect(bool on) {
#if SIMULATOR_MODE
    sim_settings.aa_autoconnect = on;
#else
    settings_set_aa_autoconnect(on);
#endif
}

bool settings_wrapper_get_use_imperial(void) {
#if SIMULATOR_MODE
    return sim_settings.use_imperial;
#else
    return settings_get_use_imperial();
#endif
}

void settings_wrapper_set_use_imperial(bool on) {
#if SIMULATOR_MODE
    sim_settings.use_imperial = on;
#else
    settings_set_use_imperial(on);
#endif
}

/* Unit conversion helpers — callers always pass canonical km / km/h; these
 * return the value (and the matching label) in the unit the user picked. */
float settings_wrapper_dist_to_display(float km) {
    return settings_wrapper_get_use_imperial() ? km * MILES_PER_KM : km;
}

float settings_wrapper_speed_to_display(float kmh) {
    return settings_wrapper_get_use_imperial() ? kmh * MILES_PER_KM : kmh;
}

const char *settings_wrapper_dist_unit(void) {
    return settings_wrapper_get_use_imperial() ? "MI" : "KM";
}

const char *settings_wrapper_speed_unit(void) {
    return settings_wrapper_get_use_imperial() ? "MPH" : "KM/H";
}

bool settings_wrapper_get_use_fahrenheit(void) {
#if SIMULATOR_MODE
    return sim_settings.use_fahrenheit;
#else
    return settings_get_use_fahrenheit();
#endif
}

void settings_wrapper_set_use_fahrenheit(bool on) {
#if SIMULATOR_MODE
    sim_settings.use_fahrenheit = on;
#else
    settings_set_use_fahrenheit(on);
#endif
}

/* Canonical temperature is always Celsius; convert to the display unit and
 * return the matching short label ("°C"/"°F"). */
float settings_wrapper_temp_to_display(float celsius) {
    return settings_wrapper_get_use_fahrenheit() ? (celsius * 9.0f / 5.0f + 32.0f)
                                                  : celsius;
}

const char *settings_wrapper_temp_unit(void) {
    return settings_wrapper_get_use_fahrenheit() ? "°F" : "°C";
}

void settings_wrapper_set_power_max_kw_volatile(float power_max_kw) {
#if SIMULATOR_MODE
    sim_settings.power_max_kw = power_max_kw;
#else
    settings_set_power_max_kw_volatile(power_max_kw);
#endif
}

void settings_wrapper_persist_power_max_kw(void) {
#if SIMULATOR_MODE
    /* simulator: nothing to persist */
#else
    settings_persist_power_max_kw();
#endif
}

void settings_wrapper_set_target_vesc_id_volatile(uint8_t id) {
#if SIMULATOR_MODE
    sim_settings.target_vesc_id = id;
#else
    settings_set_target_vesc_id_volatile(id);
#endif
}

void settings_wrapper_persist_target_vesc_id(void) {
#if !SIMULATOR_MODE
    settings_persist_target_vesc_id();
#endif
}

void settings_wrapper_set_brightness_volatile(uint8_t brightness) {
#if SIMULATOR_MODE
    sim_settings.brightness = brightness;
#else
    settings_set_screen_brightness_volatile(brightness);
#endif
}

void settings_wrapper_persist_brightness(void) {
#if !SIMULATOR_MODE
    settings_persist_screen_brightness();
#endif
}

void settings_wrapper_set_controller_id_volatile(uint8_t id) {
#if SIMULATOR_MODE
    sim_settings.controller_id = id;
#else
    settings_set_controller_id_volatile(id);
#endif
}

void settings_wrapper_persist_controller_id(void) {
#if !SIMULATOR_MODE
    settings_persist_controller_id();
#endif
}

void settings_wrapper_set_battery_capacity_volatile(float capacity) {
#if SIMULATOR_MODE
    sim_settings.battery_capacity = capacity;
#else
    settings_set_battery_capacity_volatile(capacity);
#endif
}

void settings_wrapper_persist_battery_capacity(void) {
#if !SIMULATOR_MODE
    settings_persist_battery_capacity();
#endif
}

void settings_wrapper_apply_restart(void) {
#if SIMULATOR_MODE
    printf("[settings] simulator: would esp_restart() now\n");
#else
    /* Give NVS a moment to flush its commit before we yank the CPU. The
     * setter already called nvs_commit synchronously, but the flash write
     * is itself a few hundred ms; sleep one tick so the LVGL task that
     * called us has a chance to unlock the display too. */
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
#endif
}

uint32_t settings_wrapper_get_clock_secs_of_day(void) {
#if SIMULATOR_MODE
    uint32_t now = (uint32_t)time(NULL);
    return (now + sim_settings.clock_offset_secs) % 86400u;
#else
    return settings_get_clock_secs_of_day();
#endif
}

void settings_wrapper_set_clock_secs_of_day(uint32_t secs_of_day) {
#if SIMULATOR_MODE
    secs_of_day %= 86400u;
    uint32_t now = (uint32_t)time(NULL);
    sim_settings.clock_offset_secs = (secs_of_day - now) % 86400u;
#else
    settings_set_clock_secs_of_day(secs_of_day);
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

