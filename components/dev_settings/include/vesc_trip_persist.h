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

/* Feed raw VESC counters in every RT-data tick. Order: trip meters,
 * amp-hours consumed, uptime ms. */
void  trip_persist_update(float vesc_trip_meters,
                          float vesc_amp_hours,
                          uint32_t vesc_uptime_ms);

float    trip_persist_get_trip_km(void);
float    trip_persist_get_amp_hours(void);
uint32_t trip_persist_get_uptime_ms(void);

/* Zero everything and wipe NVS. Used on battery swap / "reset trip"
 * dashboard button. */
void  trip_persist_reset(void);

bool  trip_persist_is_initialized(void);

#ifdef __cplusplus
}
#endif
