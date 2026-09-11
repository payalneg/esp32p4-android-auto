/* BLE central / GATT-client for a VESC Express adapter.
 *
 * The adapter speaks the Nordic UART Service — service
 * 6E400001-B5A3-F393-E0A9-E50E24DCCA9E, RX (write) …0002, TX (notify) …0003 —
 * carrying framed VESC packets, exactly what VESC Tool talks to it over. This
 * module is the link only: connect, discover, subscribe, write bytes, hand
 * received bytes to a callback. Everything with an opinion about VESC packets
 * (framing, COMM_FORWARD_CAN, request serialisation) lives in vesc_ble_link.c.
 *
 * Connection lifecycle mirrors ble_speed_client / ble_cadence_client: bind an
 * address → request a connection through ble_central_arb (the single NimBLE
 * connect-initiator is shared with the two sensor clients) → on link-up
 * negotiate MTU, ask for a fast connection interval, discover NUS, subscribe;
 * on disconnect re-arm.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool     bound;      /* an adapter address has been bound */
    bool     connected;  /* GATT link up */
    bool     ready;      /* ...and subscribed: packets can flow */
    bool     scanning;   /* a selection scan is currently running */
    uint16_t mtu;        /* negotiated ATT MTU, 0 when not connected */
    uint32_t reconnects; /* link drops since boot — a flapping-link tell */
} ble_vesc_state_t;

/* Scan-result callback: invoked on the NimBLE host task for each candidate
 * adapter found during a selection scan. addr is 6 bytes in NimBLE native
 * (little-endian) order; addr_type is BLE_ADDR_PUBLIC / BLE_ADDR_RANDOM. */
typedef void (*ble_vesc_scan_cb_t)(const uint8_t addr[6], uint8_t addr_type,
                                   const char *name, int8_t rssi);

/* Received notification bytes, on the NimBLE host task. Keep it short: push
 * them somewhere and parse on your own task. */
typedef void (*ble_vesc_rx_cb_t)(const uint8_t *data, uint16_t len);

/* Set up module state. Call once from ble_host_init() before the host task
 * starts (after ble_arb_init). */
void ble_vesc_client_init(void);

/* Called from ble_host's on_sync_cb once the stack is up and the own address
 * type is known. (Re)arms a connect to a previously-bound adapter, if any. */
void ble_vesc_on_ble_sync(uint8_t own_addr_type);

void ble_vesc_set_scan_cb(ble_vesc_scan_cb_t cb);
void ble_vesc_set_rx_cb(ble_vesc_rx_cb_t cb);

/* Start / stop a selection scan. Results stream via the registered scan cb;
 * the scan auto-stops after a few seconds. Suspends the shared connect
 * initiator for the duration and resumes it afterwards. */
void ble_vesc_scan_start(void);
void ble_vesc_scan_stop(void);

/* Bind to a specific adapter and (re)connect to it. Does NOT persist — the
 * caller (vesc_ble_link.c) owns NVS. addr is 6 bytes native order. */
void ble_vesc_bind(const uint8_t addr[6], uint8_t addr_type);

/* Forget the bound adapter: disconnect and stop reconnecting. */
void ble_vesc_forget(void);

/* Drop the link but keep the binding, so a later ble_vesc_reconnect() picks it
 * back up. Used when the user switches the VESC link back to CAN: no reason to
 * hold a radio slot for a transport nobody is talking on. */
void ble_vesc_disconnect(void);
void ble_vesc_reconnect(void);

/* True once the link is up AND subscribed — i.e. a write would actually
 * reach the adapter and its answer would come back. */
bool ble_vesc_is_ready(void);

/* Write one framed VESC packet, chunked to the negotiated MTU. Blocks only on
 * the controller's buffers (never on a reply). Returns 0 on success, or a
 * NimBLE error. MUST NOT be called from the NimBLE host task. */
int ble_vesc_write(const uint8_t *data, uint16_t len);

void ble_vesc_get(ble_vesc_state_t *out);

#ifdef __cplusplus
}
#endif
