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

// CAN Speed options
typedef enum {
    CAN_SPEED_125 = 0,
    CAN_SPEED_250 = 1,
    CAN_SPEED_500 = 2,
    CAN_SPEED_1000 = 3
} can_speed_option_t;

// Battery calculation mode options
typedef enum {
    BATTERY_CALC_MODE_DIRECT = 0,    // Direct from controller
    BATTERY_CALC_MODE_SMART = 1      // Smart calculation
} battery_calc_mode_option_t;

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

// Utility function
int settings_wrapper_can_speed_to_kbps(uint8_t index);

// Speed calculation functions
float settings_wrapper_calculate_speed_kmh(float erpm);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_WRAPPER_H_ */

