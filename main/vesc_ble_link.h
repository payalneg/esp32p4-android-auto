/* The VESC protocol over a BLE adapter link.
 *
 * ble_vesc_client.c owns the GATT link; this is everything above it that
 * knows what the bytes mean:
 *
 *   * framing — VESC's START/LEN/PAYLOAD/CRC16/STOP, the same packet_parser
 *     the phone-facing NUS bridge uses, in a second instance;
 *   * addressing — a VESC Express sits on the CAN bus beside the controller,
 *     so reaching the controller means wrapping each payload in
 *     COMM_FORWARD_CAN + the target id, exactly as VESC Tool does after its
 *     "scan CAN bus";
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

#ifdef __cplusplus
}
#endif
