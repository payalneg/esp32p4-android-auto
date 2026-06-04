/*
 * Trip time-series logging to the raw "triplog" flash partition (no filesystem)
 * — a circular log of fixed 64-byte records appended every ~10 s. Each record
 * carries a trip id that rolls over on "reset trip" / battery swap (hooked via
 * vesc_trip_persist's reset callback); the current trip continues across
 * reboots. At most 50 trips are exposed — the window is computed once on boot.
 *
 * Intended to back a future on-device (or off-device) trip analyzer.
 */
#pragma once

#include <stdint.h>
#include "vesc_can/vesc_datatypes.h"   /* vesc_setup_values_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Scans the log, seeds the running totals, registers the reset hook and starts
 * the background writer task (which also pre-erases the upcoming runway). */
void trip_log_init(void);

/* Call every RT-data tick with the latest VESC snapshot. Internally throttles
 * the time-series sampling to ~10 s and tracks max speed. */
void trip_log_tick(const vesc_setup_values_t *rt);

/* Finalize the current trip and begin a new one. Wired to vesc_trip_persist's
 * reset callback (fires on the dashboard reset button and on battery-swap). */
void trip_log_new_trip(void);

/* Trip-id window exposed to a reader/analyzer. The oldest kept trip is computed
 * once on boot (last MAX_TRIPS trips); the current trip id grows as new trips
 * start. Both are 0 before trip_log_init() runs or if logging is disabled. */
uint32_t trip_log_first_trip_id(void);
uint32_t trip_log_current_trip_id(void);

#ifdef __cplusplus
}
#endif
