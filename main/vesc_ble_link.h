/* The VESC protocol over a BLE adapter link.
 *
 * ble_vesc_client.c owns the GATT link; this is everything above it that
 * knows what the bytes mean:
 *
 *   * framing — VESC's START/LEN/PAYLOAD/CRC16/STOP, the same packet_parser
 *     the phone-facing NUS bridge uses, in a second instance;
 *   * addressing — what sits on the other end of the link decides this. A BLE
 *     module on the controller's UART (or a VESC with built-in BLE) IS the
 *     controller, and its payloads go out untouched. A VESC Express sits on
 *     the CAN bus beside the controller, so reaching the controller means
 *     wrapping each payload in COMM_FORWARD_CAN + the target id, exactly as
 *     VESC Tool does after its "scan CAN bus". Getting this wrong is silent —
 *     the request is accepted and never answered — so the link asks the peer
 *     who it is (vesc_ble_link_probe_loop) instead of assuming;
 *   * serialisation — one request in flight, because replies carry no request
 *     id and are matched only by arriving next.
 *
 * Registers itself as the BLE backend of vesc_link, so the pollers above never
 * learn which transport they are on.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* What answered when the link asked. */
typedef enum {
    VESC_BLE_PEER_UNKNOWN = 0,  /* not probed yet (or link down) */
    VESC_BLE_PEER_CONTROLLER,   /* the attached device is a motor controller */
    VESC_BLE_PEER_BRIDGE,       /* speaks VESC but is not a controller */
    VESC_BLE_PEER_SILENT,       /* answers nothing at all */
} vesc_ble_peer_t;

typedef struct {
    bool     bound;         /* an adapter is paired */
    bool     connected;     /* GATT link up */
    bool     ready;         /* ...and subscribed: packets can flow */
    bool     scanning;
    uint16_t mtu;
    uint32_t reconnects;    /* link drops since boot */
    uint32_t timeouts;      /* requests that never got an answer */
    uint32_t stale_drops;   /* motor setpoints dropped for being out of date */
    uint32_t last_rt_ms;    /* last completed round trip, 0 = none yet */
    uint32_t requests;      /* requests written to the adapter */
    uint32_t replies;       /* answers that came back */
    vesc_ble_peer_t peer;   /* what the probe found */
    uint8_t  peer_id;       /* CAN id of the attached controller (peer == CONTROLLER) */
    uint8_t  probe_tries;   /* probe attempts since the link came up */
    bool     wrapping;      /* requests to Target VESC ID go through COMM_FORWARD_CAN */
} vesc_ble_telem_t;

/* Load the paired adapter from NVS, create the TX/RX tasks and register with
 * vesc_link. Call once, after ble_host_init(). */
void vesc_ble_link_init(void);

/* Connect / drop the adapter link. Called when the VESC link setting changes
 * (and once at boot for whichever mode is stored). */
void vesc_ble_link_start(void);
void vesc_ble_link_stop(void);

/* Pair with a scanned adapter (persists) / unpair. */
void vesc_ble_link_select(const uint8_t addr[6], uint8_t addr_type);
void vesc_ble_link_forget(void);

void vesc_ble_link_get_telem(vesc_ble_telem_t *out);

/* Ask the peer who it is, so AUTO routing knows whether to wrap. Runs on the
 * VESC poll task (registered with vesc_rt_data_register_aux_loop) because it
 * blocks on a reply; no-op unless the link is up and the answer isn't known
 * yet. Forgets the answer when the link drops (each connection is asked
 * afresh) and, once it knows, re-issues the config menu's FW_VERSION probe
 * that the boot-time attempt lost to a link that was not up yet. */
void vesc_ble_link_probe_loop(void);

/* Settings → VESC link → Addressing changed: forget what the probe found so
 * AUTO re-runs it. Called by dev_settings (weak symbol there). */
void vesc_ble_link_route_changed(void);

#ifdef __cplusplus
}
#endif
