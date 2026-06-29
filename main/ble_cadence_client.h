#pragma once

/* BLE central / GATT-client for an external cadence sensor (pedal-assist).
 *
 * The head unit is normally a BLE peripheral (NotifBridge GATT server, see
 * ble_host.c). For PAS it ALSO acts as a central: it scans for, binds to and
 * connects to one specific BLE cadence sensor (e.g. "BK6LS-Cadence", custom
 * service cad00001-… with a notify characteristic cad00002-… streaming a
 * signed int16 centi-RPM every 100 ms). Dual-role is supported by the C6
 * controller; this module owns the central side and runs its own GAP/GATT
 * callbacks so it never tangles with the peripheral path.
 *
 * Connection lifecycle: bind a specific address → keep an outstanding
 * ble_gap_connect to it (the controller connects whenever the sensor wakes
 * and advertises; the sensor sleeps when the bike is parked) → on link-up
 * discover the RPM service/char, subscribe, then stream notifications. On
 * disconnect we re-arm the connect.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Live sensor state snapshot (safe from any task; backed by a spinlock). */
typedef struct {
    bool     bound;      /* a sensor address has been bound (cadence_bind) */
    bool     connected;  /* GATT link up AND subscribed to RPM notifications */
    bool     scanning;   /* a selection scan is currently running */
    int16_t  centi_rpm;  /* signed centi-RPM (RPM*100); sign = direction */
    uint32_t age_ms;     /* ms since the last RPM notification (UINT32_MAX = never) */
    uint8_t  battery;    /* 0..100 percent, 0xFF = unknown */
} ble_cadence_state_t;

/* Scan-result callback: invoked on the NimBLE host task for each distinct
 * cadence-sensor candidate found during a selection scan. addr is 6 bytes in
 * NimBLE native (little-endian) order; addr_type is a NimBLE peer-address type
 * (BLE_ADDR_PUBLIC / BLE_ADDR_RANDOM). The app layer registers this to forward
 * results to the phone. */
typedef void (*ble_cadence_scan_cb_t)(const uint8_t addr[6], uint8_t addr_type,
                                       const char *name, int8_t rssi);

/* Set up module state. Call once from ble_host_init() before the host task
 * starts (mirrors notif_bridge_init). */
void ble_cadence_client_init(void);

/* Called from ble_host's on_sync_cb once the stack is up and the own address
 * type is known. (Re)arms a connect to a previously-bound sensor, if any. */
void ble_cadence_on_ble_sync(uint8_t own_addr_type);

/* Start / stop a selection scan. Results stream via the registered scan cb;
 * the scan auto-stops after a few seconds. Cancels any pending connect for the
 * duration and resumes it afterwards. */
void ble_cadence_scan_start(void);
void ble_cadence_scan_stop(void);
void ble_cadence_set_scan_cb(ble_cadence_scan_cb_t cb);

/* Bind to a specific sensor and (re)connect to it. Does NOT persist — the
 * caller (PAS settings) owns NVS. addr is 6 bytes native order. */
void ble_cadence_bind(const uint8_t addr[6], uint8_t addr_type);

/* Forget the bound sensor: disconnect and stop reconnecting. */
void ble_cadence_forget(void);

/* True if a sensor address is bound. */
bool ble_cadence_is_bound(void);

/* Copy the current sensor state. */
void ble_cadence_get(ble_cadence_state_t *out);

#ifdef __cplusplus
}
#endif
