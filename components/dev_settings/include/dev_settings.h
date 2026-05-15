#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Numeric values match the kbps payload — settings_wrapper.c maps them
 * to/from a 0..3 dropdown index. */
typedef enum {
    CAN_SPEED_125_KBPS  = 125,
    CAN_SPEED_250_KBPS  = 250,
    CAN_SPEED_500_KBPS  = 500,
    CAN_SPEED_1000_KBPS = 1000,
} can_speed_t;

typedef enum {
    BATTERY_CALC_MODE_DIRECT = 0,
    BATTERY_CALC_MODE_SMART  = 1,
} battery_calc_mode_t;

/* Top-level connection mode chosen by the user in Settings. Decides which
 * stack we bring up at boot (AA stays on the WROOM SPP/Wireless-Helper path,
 * AVRCP switches the WROOM into A2DP-sink+AVRCP-CT and shows Now Playing on
 * LVGL, CarPlay is a placeholder screen for now). The dropdown index in
 * custom.c must match this enum order. */
typedef enum {
    CONN_AVRCP        = 0,
    CONN_ANDROID_AUTO = 1,
    CONN_CARPLAY      = 2,
} connection_mode_t;

/* Loads cache from NVS. Idempotent — settings_ui_init() also calls this
 * via settings_wrapper_init(), so order between main and UI doesn't matter. */
void settings_init(void);

uint8_t              settings_get_target_vesc_id(void);
can_speed_t          settings_get_can_speed(void);
uint8_t              settings_get_screen_brightness(void);
uint8_t              settings_get_controller_id(void);
float                settings_get_battery_capacity(void);
battery_calc_mode_t  settings_get_battery_calc_mode(void);
bool                 settings_get_show_fps(void);
uint16_t             settings_get_wheel_diameter_mm(void);
uint8_t              settings_get_motor_poles(void);
connection_mode_t    settings_get_connection_mode(void);
float                settings_get_power_max_kw(void);
bool                 settings_get_vesc_emulator(void);

void settings_set_target_vesc_id(uint8_t id);
void settings_set_can_speed(can_speed_t speed);
void settings_set_screen_brightness(uint8_t brightness);
void settings_set_controller_id(uint8_t id);
void settings_set_battery_capacity(float capacity);
void settings_set_battery_calc_mode(battery_calc_mode_t mode);
void settings_set_show_fps(bool show);
void settings_set_wheel_diameter_mm(uint16_t diameter_mm);
void settings_set_motor_poles(uint8_t poles);
void settings_set_connection_mode(connection_mode_t mode);
void settings_set_power_max_kw(float power_max_kw);
void settings_set_vesc_emulator(bool on);

/* Debounced setters — update the RAM cache and fire any hot-apply callback
 * immediately, but DO NOT touch NVS. The UI pairs them with the matching
 * settings_persist_* call on a debounce timer, so rapid spinbox/slider
 * activity doesn't issue an nvs_commit on every tick. Without this, the
 * LVGL task spends ~100 ms per value-change in a flash write and starves
 * touch_input of the LVGL lock ("Failed to acquire LVGL lock"). */
void settings_set_target_vesc_id_volatile(uint8_t id);
void settings_set_screen_brightness_volatile(uint8_t brightness);
void settings_set_controller_id_volatile(uint8_t id);
void settings_set_battery_capacity_volatile(float capacity);
void settings_set_power_max_kw_volatile(float power_max_kw);

void settings_persist_target_vesc_id(void);
void settings_persist_screen_brightness(void);
void settings_persist_controller_id(void);
void settings_persist_battery_capacity(void);
void settings_persist_power_max_kw(void);

/* Hot-apply hooks: registered by main once the corresponding subsystem is
 * up. settings_set_* fires the callback synchronously on the caller after
 * persisting (UI thread for set-from-UI). NULL allowed.
 *
 * Wired today: CAN speed, screen brightness, target VESC ID, controller ID.
 * Other setters just persist to NVS and are picked up on next boot. */
typedef void (*settings_can_speed_cb_t)(int new_kbps);
typedef void (*settings_brightness_cb_t)(uint8_t new_pct);
typedef void (*settings_target_id_cb_t)(uint8_t new_id);
typedef void (*settings_controller_id_cb_t)(uint8_t new_id);

void settings_register_can_speed_cb(settings_can_speed_cb_t cb);
void settings_register_brightness_cb(settings_brightness_cb_t cb);
void settings_register_target_id_cb(settings_target_id_cb_t cb);
void settings_register_controller_id_cb(settings_controller_id_cb_t cb);

#ifdef __cplusplus
}
#endif
