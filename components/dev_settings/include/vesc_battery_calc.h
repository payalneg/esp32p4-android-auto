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

/* Smart percentage based on net consumed Ah versus battery capacity. The first
 * call after boot pulls remaining_ah from NVS (or seeds it from the current
 * controller reading); subsequent calls subtract the delta in net Ah, where
 * net = rt->amp_hours − rt->amp_hours_charged (so regen credits Ah back).
 * Charge/swap detection no longer lives here — see battery_calc_voltage_boot_check.
 * Falls back to controller_battery_level*100 on bad inputs (capacity <= 0). */
float battery_calc_get_smart_percentage(float controller_battery_level,
                                        float controller_amp_hours,
                                        float controller_amp_hours_charged,
                                        float battery_capacity);

/* Battery percentage as shown on the dashboard / HUD — picks Direct vs Smart
 * per the battery_calc_mode setting and pulls capacity from settings. Use this
 * everywhere a "what the rider sees" battery % is needed (cockpit, AA overlay,
 * trip log) so all readouts stay in agreement. */
float battery_calc_display_percentage(float controller_battery_level,
                                      float controller_amp_hours,
                                      float controller_amp_hours_charged);

bool  battery_calc_is_initialized(void);

/* Hint that capacity has changed — next get_smart_percentage() will reset
 * to full at the new capacity. Settings setter should call this. */
void  battery_calc_capacity_changed(void);

/* Hook for the dashboard reset icon. Today this is a no-op log line — when
 * a trip persistence layer lands it will reset trip / Ah / uptime. Kept as
 * the public API used by Super_VESC_Display/custom/custom.c. */
void  battery_calc_reset_trip_and_ah(void);

/* Once-per-boot charge/swap detector. Pass the live pack voltage (rt->v_in);
 * the first valid call compares it against the voltage saved at the previous
 * power-on and, if it rose by more than VIN_CHARGE_PCT_THRESHOLD percent, rolls
 * the trip over via battery_calc_reset_trip_and_ah(). Records this boot's
 * voltage as the next power-on's baseline. Saves and compares ONLY at startup —
 * later calls in the same session are no-ops. Works in both Direct and Smart. */
void  battery_calc_voltage_boot_check(float v_in);

/* Charge/swap detected by the boot check. With a callback registered the
 * decision is handed over (the UI asks the rider: reset trip or keep); with
 * none the trip is reset automatically as before. Invoked on the thread that
 * runs the boot check — the LVGL updater tick — with no battery_calc lock
 * held, so the callback may build widgets directly. */
typedef void (*battery_calc_charge_cb_t)(float prev_v, float now_v, float change_pct);
void  battery_calc_set_charge_detected_cb(battery_calc_charge_cb_t cb);

/* Remaining capacity in Ah — useful for range estimation. */
float battery_calc_get_remaining_ah(void);

/* Snapshot for the trip log: remaining Ah + the reset epoch, taken atomically.
 * Stamped into every 10 s trip record so the smart-battery state persists via
 * the (pre-erased, GC-free) triplog partition instead of periodic NVS writes. */
void  battery_calc_get_persist(float *remaining_ah, uint32_t *epoch);

/* Boot-time seed from the newest trip-log record. Accepted only when `epoch`
 * matches the NVS reset epoch — a mismatch means a reset/capacity change
 * happened after that record, and the NVS value written by the reset wins. */
void  battery_calc_seed_remaining(float remaining_ah, uint32_t epoch);

/* Boot-time seed from trip_log (boot_scan, before the display is up) for the
 * charge/swap check: `triplog_alive` = the log exists and will carry this
 * boot's voltage forward; `prev_boot_vin` = the pack voltage the previous
 * power-on saw (< 0 = no record carries it yet). With a live log the check
 * never writes NVS any more — the old "last_vin" nvs_commit landed right as
 * the ESC came up, i.e. a blue DSI-underrun flash on every power-on. */
void  battery_calc_seed_boot_vin(bool triplog_alive, float prev_boot_vin);

/* Pack voltage captured at this boot's first valid ESC reading; 0 until then.
 * Stamped into every trip record so the next boot can compare against it. */
float battery_calc_get_boot_vin(void);

#ifdef __cplusplus
}
#endif
