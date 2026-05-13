/*
    Copyright 2025 Super VESC Display
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4

    Smart battery percentage calculation: tracks remaining Ah in NVS,
    auto-detects charging / battery swap by watching the controller-reported
    percentage, and persists state across reboots. Mirrors the Arduino-based
    original from Super_VESC_Display/src/vesc_battery_calc.cpp.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Loads persisted state from NVS. Idempotent. */
void  battery_calc_init(void);

/* Force a full-reset to `capacity` Ah and remember the current controller %.
 * Used on first run, on capacity change, and after charging is detected. */
void  battery_calc_reset(float current_battery_percent, float battery_capacity);

/* Smart percentage based on consumed Ah versus battery capacity. The first
 * call after boot pulls remaining_ah from NVS (or seeds it from the current
 * controller reading); subsequent calls subtract the delta in rt->amp_hours.
 * If the controller %  jumps up by >5 % vs. the saved value, treats it as a
 * charge/swap and resets. Falls back to controller_battery_level*100 on bad
 * inputs (capacity <= 0). */
float battery_calc_get_smart_percentage(float controller_battery_level,
                                        float controller_amp_hours,
                                        float battery_capacity);

bool  battery_calc_is_initialized(void);

/* Hint that capacity has changed — next get_smart_percentage() will reset
 * to full at the new capacity. Settings setter should call this. */
void  battery_calc_capacity_changed(void);

/* Hook for the dashboard reset icon. Today this is a no-op log line — when
 * a trip persistence layer lands it will reset trip / Ah / uptime. Kept as
 * the public API used by Super_VESC_Display/custom/custom.c. */
void  battery_calc_reset_trip_and_ah(void);

/* Remaining capacity in Ah — useful for range estimation. */
float battery_calc_get_remaining_ah(void);

#ifdef __cplusplus
}
#endif
