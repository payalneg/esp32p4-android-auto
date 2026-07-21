#pragma once

/* Pedal-Assist System (PAS).
 *
 * Reads cadence (signed RPM) from the bound BLE sensor (ble_cadence_client),
 * turns it into a motor-current setpoint per the user's settings, and forwards
 * that to the VESC LISP arbiter (vesc_lisp_panel_set_pas). Settings + the bound
 * sensor address persist in NVS. The phone app configures everything and reads
 * live telemetry over the NotifBridge link (see notif_bridge.c PAS PDUs).
 *
 * The control loop runs on its own ~50 ms task. Forwarding to the VESC is
 * fire-and-forget with a firmware-side watchdog (vesc_lisp_panel_pas_loop) plus
 * a LISP-side staleness check, so a sensor dropout coasts the motor safely.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PAS_MODE_SWITCH       = 0,  /* assist = level fraction × max current while pedaling */
    PAS_MODE_PROPORTIONAL = 1,  /* assist scales with cadence (min→full) up to that cap */
} pas_mode_t;

/* User-tunable PAS settings. Mirrored by the phone app; persisted in NVS. */
typedef struct {
    bool     enabled;            /* master throttle/PAS enable */
    bool     reverse;            /* sensor on the other crank → flip RPM sign */
    uint8_t  level;              /* current assist level (0..level_count) */
    uint8_t  level_count;        /* number of assist levels (e.g. 5) */
    float    max_current_a;      /* motor current at level_count / 100 % (A) */
    uint8_t  mode;              /* pas_mode_t */
    uint16_t start_delay_ms;     /* continuous forward pedaling before assist engages */
    uint8_t  start_current_pct;  /* initial kick as % of target on engage (softer start) */
    float    ramp_up_aps;        /* max current slew (A per second) */
    uint16_t stop_delay_ms;      /* keep assisting this long after pedaling stops */
    uint16_t min_cadence_rpm;    /* below this forward cadence → no assist */
    uint16_t full_cadence_rpm;   /* cadence for 100 % assist (PROPORTIONAL mode) */
} pas_settings_t;

/* Live telemetry for the app's PAS settings screen. */
typedef struct {
    int16_t centi_rpm;        /* RAW signed centi-RPM from the sensor (sign = direction) */
    bool    sensor_bound;
    bool    sensor_connected;
    bool    scanning;
    uint8_t battery;          /* 0..100, 0xFF unknown */
    float   assist_a;         /* current PAS output current (A) */
} pas_telem_t;

/* Load settings + bound sensor from NVS and start the control task. Call once
 * during startup AFTER ble_host_init (which sets up the cadence client) and the
 * VESC CAN/panel init. Idempotent. */
void pas_init(void);

/* Settings access. set_settings applies live AND persists to NVS. */
void pas_get_settings(pas_settings_t *out);
void pas_set_settings(const pas_settings_t *s);

/* Quick mutators (also persisted) — used by the app's toggle / level controls. */
void pas_set_enabled(bool enabled);
void pas_set_level(uint8_t level);

/* Sensor binding. select persists the address and (re)connects via the cadence
 * client; forget unbinds + clears NVS. Scanning is started directly through
 * ble_cadence_scan_start() by the protocol handler. */
void pas_sensor_select(const uint8_t addr[6], uint8_t addr_type);
void pas_sensor_forget(void);

/* Live telemetry snapshot (safe from any task). */
void pas_get_telem(pas_telem_t *out);

#ifdef __cplusplus
}
#endif
