#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "host/ble_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Nordic UART Service UUIDs (VESC Tool / VESC Express convention).
 *   Service: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
 *   RX     : 6E400002-...   (phone → head unit, write)
 *   TX     : 6E400003-...   (head unit → phone, notify) */
#define NUS_UUID_PREFIX_BE_BYTE 0x6E

/* One-shot bring-up: allocates the outbound ring buffer and starts the
 * NUS TX task. Call once before ble_host_init() so forward_response can
 * enqueue from the CAN RX task without losing pending frames during
 * VESC Tool's first read burst. Idempotent. */
void ble_nus_init(void);

/* Returns the GATT service definitions for ble_gatts_count_cfg /
 * ble_gatts_add_svcs. Pointer-to-array of ble_gatt_svc_def with a {0}
 * terminator. Stable for the lifetime of the program. */
const struct ble_gatt_svc_def *ble_nus_get_svcs(void);

/* GATT register callback — wires the TX value handle from the registration
 * event to internal state so we can notify on it later. Pass to
 * ble_hs_cfg.gatts_register_cb in ble_host. */
void ble_nus_gatts_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);

/* Tracking of connected peers — call from ble_host's GAP event hook on
 * connect / disconnect. Up to two peers can be connected at once (phone app
 * + VESC Tool); NUS itself is single-owner and follows whoever last wrote
 * to RX or subscribed to TX. */
void ble_nus_on_connect(uint16_t conn_handle);
void ble_nus_on_disconnect(uint16_t conn_handle);

/* Subscription tracking — call from ble_host's GAP event hook on every
 * BLE_GAP_EVENT_SUBSCRIBE. forward_response stays a no-op until a peer
 * actually enables notifications on the NUS TX characteristic. Without this
 * a notifications-only peer (the phone, which uses NotifBridge and never
 * touches NUS) would still receive a flood of VESC RT-poll responses pushed
 * at it — every one failing with notify_tx ENOMEM and back-pressuring the
 * CAN dispatch task. attr_handle is matched against the NUS TX value handle;
 * other handles (NotifBridge chars) are ignored. A peer enabling TX
 * notifications also takes NUS ownership (that's how VESC Tool announces
 * itself next to an already-connected phone). */
void ble_nus_on_subscribe(uint16_t conn_handle, uint16_t attr_handle,
                          bool cur_notify);

/* Push a reassembled VESC payload back to the phone over the NUS TX
 * characteristic (frames it with packet_build_frame and chunks by MTU-3).
 * No-op if no peer is connected. Safe to call from comm_can's RX task
 * (NimBLE serializes through the host task internally). */
void ble_nus_forward_response(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif
