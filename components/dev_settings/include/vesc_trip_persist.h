/*
    Copyright 2025 Super VESC Display
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4

    Trip / Ah / uptime persistence: keeps a running total that survives VESC
    reboots. Mirrors Super_VESC_Display/src/vesc_trip_persist.cpp — every
    update sees the latest raw counters from the VESC; we keep an offset so
    the displayed total stays continuous when the VESC's own counters reset.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void  trip_persist_init(void);

/* Seed the running totals at boot (called by the raw trip log after it reads the
 * last record). Negative values are ignored. */
void  trip_persist_seed_totals(float trip_total_m, float ah_total, uint32_t uptime_total_ms);

/* Feed raw VESC counters in every RT-data tick. Order: trip meters,
 * amp-hours consumed, uptime ms. */
void  trip_persist_update(float vesc_trip_meters,
                          float vesc_amp_hours,
                          uint32_t vesc_uptime_ms);

float    trip_persist_get_trip_km(void);
float    trip_persist_get_amp_hours(void);
uint32_t trip_persist_get_uptime_ms(void);

/* Zero everything and wipe the saved file. Used on battery swap / "reset trip"
 * dashboard button. */
void  trip_persist_reset(void);

/* Hook fired at the end of trip_persist_reset() — both the reset button and
 * battery-swap detection funnel here. The trip logger uses it to roll over to
 * a new trip folder. */
void  trip_persist_set_reset_cb(void (*cb)(void));

bool  trip_persist_is_initialized(void);

#ifdef __cplusplus
}
#endif
