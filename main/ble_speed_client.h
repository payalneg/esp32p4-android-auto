#pragma once

/* BLE central / GATT-client for an external wheel-speed sensor.
 *
 * Unlike the cadence client (custom BK6LS firmware), this speaks the STOCK
 * Bluetooth SIG Cycling Speed and Cadence profile, so any off-the-shelf
 * sensor works (Coospo, Magene, Garmin, ...): CSC service 0x1816, CSC
 * Measurement characteristic 0x2A5B (notify) — flags byte, then when bit0 is
 * set a cumulative wheel-revolution counter (u32 LE) and the last wheel
 * event time (u16 LE, 1/1024 s ticks). This module accumulates a wrap- and
 * reset-safe total revolution counter plus the latest revs/time delta; unit
 * conversion (wheel circumference → km/h / meters) lives in speed_sensor.c.
 *
 * Connection lifecycle mirrors ble_cadence_client: bind a specific address →
 * request a connection through ble_central_arb (the initiator is shared with
 * the cadence client) → on link-up discover 0x1816/0x2A5B, subscribe, stream
 * notifications; on disconnect re-arm. The sensor sleeps when the wheel is
 * still and re-advertises on movement.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Live sensor state snapshot (safe from any task; backed by a spinlock). */
typedef struct {
    bool     bound;           /* a sensor address has been bound */
    bool     connected;       /* GATT link up AND subscribed to CSC */
    bool     scanning;        /* a selection scan is currently running */
    uint64_t total_revs;      /* wrap/reset-safe cumulative wheel revolutions
                               * since boot (NOT since sensor power-on) */
    uint16_t last_delta_revs; /* wheel revs in the last rev-advancing notify */
    uint16_t last_delta_1024; /* the matching event-time delta, 1/1024 s */
    uint32_t age_ms;          /* ms since last notification (UINT32_MAX = never) */
    uint32_t rev_age_ms;      /* ms since revs last ADVANCED — the wheel is
                               * standing still when this grows (sensors keep
                               * notifying unchanged counters while stopped) */
    uint8_t  battery;         /* 0..100 percent, 0xFF = unknown */
} ble_speed_state_t;

/* Scan-result callback: invoked on the NimBLE host task for each CSC
 * advertiser found during a selection scan. addr is 6 bytes in NimBLE native
 * (little-endian) order; addr_type is BLE_ADDR_PUBLIC / BLE_ADDR_RANDOM. */
typedef void (*ble_speed_scan_cb_t)(const uint8_t addr[6], uint8_t addr_type,
                                    const char *name, int8_t rssi);

/* Set up module state. Call once from ble_host_init() before the host task
 * starts (after ble_arb_init). */
void ble_speed_client_init(void);

/* Called from ble_host's on_sync_cb once the stack is up and the own address
 * type is known. (Re)arms a connect to a previously-bound sensor, if any. */
void ble_speed_on_ble_sync(uint8_t own_addr_type);

/* Start / stop a selection scan. Results stream via the registered scan cb;
 * the scan auto-stops after a few seconds. Suspends the shared connect
 * initiator for the duration and resumes it afterwards. */
void ble_speed_scan_start(void);
void ble_speed_scan_stop(void);
void ble_speed_set_scan_cb(ble_speed_scan_cb_t cb);

/* Bind to a specific sensor and (re)connect to it. Does NOT persist — the
 * caller (speed_sensor.c) owns NVS. addr is 6 bytes native order. */
void ble_speed_bind(const uint8_t addr[6], uint8_t addr_type);

/* Forget the bound sensor: disconnect and stop reconnecting. */
void ble_speed_forget(void);

/* True if a sensor address is bound. */
bool ble_speed_is_bound(void);

/* Copy the current sensor state. */
void ble_speed_get(ble_speed_state_t *out);

#ifdef __cplusplus
}
#endif
