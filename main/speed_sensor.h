#pragma once

/* Wheel-speed sensor feature module — the layer between ble_speed_client
 * (raw CSC revolution counters) and everything that displays or logs speed.
 *
 * Owns:
 *   - the speed-source setting (VESC vs BLE sensor) — an explicit user
 *     choice, no auto-fallback;
 *   - the wheel-revs → km/h conversion (wheel diameter from dev_settings'
 *     "wheel_mm", shared with the dormant VESC erpm→speed helper);
 *   - the LOCAL trip and odometer integrators (Σ revs × circumference).
 *     When the source is the BLE sensor the dashboard's speed/trip/odometer
 *     switch to these; the VESC's own counters keep running untouched;
 *   - their NVS persistence (namespace "spdsns"), written by a dedicated
 *     low-priority task — saves land on ride stops + a 10 min fallback, so
 *     flash writes never stall the display mid-ride (blue-flash rule);
 *   - the bound sensor address blob (same 7-byte format as PAS).
 *
 * Getters are spinlock-backed snapshots, safe from any task (LVGL timer, AA
 * video overlay on the H.264 decoder task, trip_log writer). */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Snapshot for the settings screen. */
typedef struct {
    bool     source_ble;       /* speed-source setting: true = BLE sensor */
    bool     sensor_bound;
    bool     sensor_connected;
    bool     scanning;
    bool     fresh;            /* connected + notifications flowing */
    float    kmh;              /* 0 when stale or wheel stopped */
    float    trip_km;
    float    odo_km;
    uint8_t  battery;          /* 0..100, 0xFF unknown */
} speed_telem_t;

/* Loads settings/sensor/odometer from NVS (rebinds a saved sensor), starts
 * the integrator + NVS writer tasks, and registers itself with trip_log
 * (speed provider) and trip_persist (trip-reset hook). Call from app_main
 * after ble_host_init(), next to pas_init(). */
void speed_sensor_init(void);

/* Speed-source setting (persisted). */
bool speed_source_is_ble(void);
void speed_sensor_set_source(bool ble);

/* Live values from the BLE sensor (valid regardless of the source setting —
 * consumers must gate on speed_source_is_ble() themselves). */
float speed_sensor_get_kmh(void);      /* 0 when stale */
bool  speed_sensor_is_fresh(void);
float speed_sensor_get_trip_km(void);
float speed_sensor_get_odometer_km(void);

/* Zero the BLE trip (odometer is never reset). Registered with
 * trip_persist_add_reset_cb so the Settings "Reset trip" button and the
 * battery-swap rollover cover it too. */
void speed_sensor_trip_reset(void);

/* Pairing — persists the address and (re)binds the client. */
void speed_sensor_select(const uint8_t addr[6], uint8_t addr_type);
void speed_sensor_forget(void);

void speed_sensor_get_telem(speed_telem_t *out);

#ifdef __cplusplus
}
#endif
