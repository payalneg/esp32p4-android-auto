/*
 * Trip time-series logging to the raw "triplog" flash partition (no filesystem)
 * — a circular log of fixed 64-byte records appended every ~10 s. Each record
 * carries a trip id that rolls over on "reset trip" / battery swap (hooked via
 * vesc_trip_persist's reset callback); the current trip continues across
 * reboots. The reader exposes at most the 50 most recent (non-deleted) trips.
 *
 * Intended to back a future on-device (or off-device) trip analyzer.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "vesc_can/vesc_datatypes.h"   /* vesc_setup_values_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Registers the reset hook and — only if Settings → "Trip statistics" is on —
 * scans the log, seeds the running totals and starts the background writer
 * task (which also pre-erases the upcoming runway). With the switch off the
 * log stays completely idle: no flash I/O, readers report an empty log. */
void trip_log_init(void);

/* Runtime master switch (wired to the Settings toggle; LVGL thread is fine).
 * Off: sampling and runway erases stop immediately. On: the ring is scanned on
 * first use (without re-seeding the running totals) and a new trip begins. */
void trip_log_set_enabled(bool on);
bool trip_log_is_enabled(void);

/* Call every RT-data tick with the latest VESC snapshot. Internally throttles
 * the time-series sampling to ~10 s and tracks max speed. */
void trip_log_tick(const vesc_setup_values_t *rt);

/* ---- Optional external speed source (BLE wheel-speed sensor) ----
 *
 * When the user selects the BLE sensor as the speed source, the recorded
 * speed samples and the idle detector (which gates flash-erase trickling —
 * every erase blue-flashes the display) must follow the wheel, not the VESC.
 * The provider is polled on the writer/tick paths:
 *   INACTIVE — source is VESC: behave exactly as without a provider;
 *   STALE    — source is BLE but the sensor is silent: treat like a silent
 *              ESC (erases unlock only after a long quiet dwell — a sensor
 *              battery dying mid-ride must not unlock them);
 *   FRESH    — *kmh was written with the current wheel speed. */
typedef enum {
    TRIP_SPEED_PROV_INACTIVE = 0,
    TRIP_SPEED_PROV_STALE,
    TRIP_SPEED_PROV_FRESH,
} trip_speed_prov_res_t;

typedef trip_speed_prov_res_t (*trip_speed_provider_t)(float *kmh);

/* Register (or clear with NULL) the provider. Called once at init time by
 * speed_sensor.c; safe to leave unset — default behavior is unchanged. */
void trip_log_set_speed_provider(trip_speed_provider_t fn);

/* Finalize the current trip and begin a new one. Wired to vesc_trip_persist's
 * reset callback (fires on the dashboard reset button and on battery-swap). */
void trip_log_new_trip(void);

/* Soft-delete (hide) a trip from the statistics list. The record data stays on
 * the circular flash log, but the id is kept in an NVS hidden-set: the reader
 * filters it out and it no longer counts toward the MAX_TRIPS history window,
 * so deleting frees a slot for an older trip. Persists across reboots. The
 * current (live) trip cannot be hidden. Safe to call from any task. */
void trip_log_delete_trip(uint32_t trip_id);
bool trip_log_is_trip_deleted(uint32_t trip_id);

/* ---- Reader API (for the trip-statistics UI / analyzer) ----
 *
 * Both functions are synchronous, read-only and safe to call from any task
 * once trip_log_init() has run (they only read state fixed at init; concurrent
 * appends by the writer are tolerated — a torn or being-erased record fails its
 * CRC and is skipped). They scan the flash, so a caller on the LVGL thread
 * should run them on a worker task. Units mirror trip_rec_t (×10 fixed-point).
 */

/* Per-trip aggregate, newest-first. */
typedef struct {
    uint32_t trip_id;
    uint32_t duration_s;       /* max t_s seen for the trip                    */
    uint32_t distance_m;       /* trip distance, metres                        */
    uint32_t sample_count;
    float    ah;               /* Ah consumed this trip                        */
    float    wh;               /* Wh consumed this trip (max−min of cumulative)*/
    uint16_t avg_speed_dkmh;   /* derived: distance / duration, km/h × 10      */
    uint16_t max_speed_dkmh;   /* km/h × 10                                    */
    uint16_t min_voltage_dv;   /* lowest pack voltage seen, V × 10 (sag)       */
    bool     is_current;       /* this is the live (still-logging) trip        */
} trip_summary_t;

/* One downsampled point of a trip's time-series (for charts). */
typedef struct {
    uint32_t t_s;              /* seconds into the trip                        */
    int16_t  speed_dkmh;       /* km/h × 10                                    */
    int16_t  power_w;          /* (V×A), watts                                 */
    uint16_t voltage_dv;       /* V × 10                                       */
    int16_t  temp_motor_dc;    /* °C × 10                                      */
    int16_t  temp_fet_dc;      /* °C × 10                                      */
    int16_t  temp_motor2_dc;   /* 2nd head motor °C × 10 (0 = none, trips <v2) */
    int16_t  temp_fet2_dc;     /* 2nd head FET   °C × 10 (0 = none, trips <v2) */
    uint8_t  batt_pct;         /* remaining capacity, %                        */
} trip_sample_t;

/* Scan the ring once and fill up to `max` per-trip summaries, newest-first.
 * Returns the number written (0 if the log is empty / logging disabled). */
int trip_log_list_trips(trip_summary_t *out, int max);

/* Read one trip's samples in chronological order, bucket-averaged down to at
 * most `max` points. Returns the number of points written (0 if not found). */
int trip_log_read_series(uint32_t trip_id, trip_sample_t *out, int max);

#ifdef __cplusplus
}
#endif
