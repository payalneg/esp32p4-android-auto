/*
	Copyright 2025 Super VESC Display
	
	Settings wrapper for LVGL GUI (works in simulator and on device)
*/

#ifndef SETTINGS_WRAPPER_H_
#define SETTINGS_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef LV_REALDEVICE
/* On-device build pulls battery_calc_mode_t from the authoritative store;
 * otherwise the simulator-only enum below stands in. */
#include "dev_settings.h"
#endif

// CAN Speed options
typedef enum {
    CAN_SPEED_125 = 0,
    CAN_SPEED_250 = 1,
    CAN_SPEED_500 = 2,
    CAN_SPEED_1000 = 3
} can_speed_option_t;

#ifndef LV_REALDEVICE
// Simulator-only fallback. Device build uses the dev_settings enum.
typedef enum {
    BATTERY_CALC_MODE_DIRECT = 0,    // Direct from controller
    BATTERY_CALC_MODE_SMART  = 1     // Smart calculation
} battery_calc_mode_option_t;
#endif

// Wrapper functions for simulator compatibility
void settings_wrapper_init(void);
uint8_t settings_wrapper_get_target_vesc_id(void);
uint8_t settings_wrapper_get_can_speed_index(void);
uint8_t settings_wrapper_get_brightness(void);
uint8_t settings_wrapper_get_controller_id(void);
float settings_wrapper_get_battery_capacity(void);
uint8_t settings_wrapper_get_battery_calc_mode(void);
bool settings_wrapper_get_show_fps(void);
uint16_t settings_wrapper_get_wheel_diameter_mm(void);
uint8_t settings_wrapper_get_motor_poles(void);

void settings_wrapper_set_target_vesc_id(uint8_t id);
void settings_wrapper_set_can_speed_index(uint8_t index);
void settings_wrapper_set_brightness(uint8_t brightness);
void settings_wrapper_set_controller_id(uint8_t id);
void settings_wrapper_set_battery_capacity(float capacity);
void settings_wrapper_set_battery_calc_mode(uint8_t mode);
void settings_wrapper_set_show_fps(bool show);
void settings_wrapper_set_wheel_diameter_mm(uint16_t diameter_mm);
void settings_wrapper_set_motor_poles(uint8_t poles);

float settings_wrapper_get_power_max_kw(void);
void  settings_wrapper_set_power_max_kw(float power_max_kw);

bool  settings_wrapper_get_vesc_emulator(void);
void  settings_wrapper_set_vesc_emulator(bool on);

bool  settings_wrapper_get_aa_autoconnect(void);
void  settings_wrapper_set_aa_autoconnect(bool on);

/* Units: false = metric (km, km/h), true = imperial (mi, mph). */
bool  settings_wrapper_get_use_imperial(void);
void  settings_wrapper_set_use_imperial(bool on);

/* Convert canonical km / km/h to the user-selected display unit, and the
 * matching short labels ("KM"/"MI", "KM/H"/"MPH"). */
float        settings_wrapper_dist_to_display(float km);
float        settings_wrapper_speed_to_display(float kmh);
const char  *settings_wrapper_dist_unit(void);
const char  *settings_wrapper_speed_unit(void);

/* Temperature: false = Celsius, true = Fahrenheit. */
bool         settings_wrapper_get_use_fahrenheit(void);
void         settings_wrapper_set_use_fahrenheit(bool on);
float        settings_wrapper_temp_to_display(float celsius);
const char  *settings_wrapper_temp_unit(void);

uint32_t settings_wrapper_get_clock_secs_of_day(void);
void     settings_wrapper_set_clock_secs_of_day(uint32_t secs_of_day);

/* Debounced (volatile + persist) variants: update the cache and hot-apply
 * immediately, defer the NVS commit to a UI-level debounce timer so rapid
 * spinbox/slider clicks don't stall the LVGL task in a flash write. */
void settings_wrapper_set_target_vesc_id_volatile(uint8_t id);
void settings_wrapper_set_brightness_volatile(uint8_t brightness);
void settings_wrapper_set_controller_id_volatile(uint8_t id);
void settings_wrapper_set_battery_capacity_volatile(float capacity);
void settings_wrapper_set_power_max_kw_volatile(float power_max_kw);

void settings_wrapper_persist_target_vesc_id(void);
void settings_wrapper_persist_brightness(void);
void settings_wrapper_persist_controller_id(void);
void settings_wrapper_persist_battery_capacity(void);
void settings_wrapper_persist_power_max_kw(void);

/* Apply pending mode change by rebooting the device. On the device this
 * calls esp_restart() and does not return; in the simulator it just logs
 * a message and returns so the test harness can carry on. */
void settings_wrapper_apply_restart(void);

// Utility function
int settings_wrapper_can_speed_to_kbps(uint8_t index);

// Speed calculation functions
float settings_wrapper_calculate_speed_kmh(float erpm);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_WRAPPER_H_ */

