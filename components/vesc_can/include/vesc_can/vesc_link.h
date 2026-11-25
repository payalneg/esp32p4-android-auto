/*
    Copyright 2026 ESP32-P4 Android Auto / VESC Display (GPL-3.0).

    Transport selection for the VESC link: CAN bus (TWAI) or BLE (GATT
    central to a VESC Express adapter speaking Nordic UART Service).

    The head unit's protocol boundary was always clean: everything above it
    builds a plain VESC payload (data[0] = COMM_*) and hands it to
    comm_can_send_buffer(); every reassembled reply comes back through one
    handler hook. This module IS that boundary, made switchable — the callers
    above it do not know or care which transport is live.

    What each transport does with the arguments:

      target     CAN: the destination controller id in the frame header.
                 BLE: 0 = the adapter itself, non-zero = a node behind it,
                 which the BLE backend reaches by wrapping the payload in
                 COMM_FORWARD_CAN (exactly what VESC Tool does after its
                 "scan CAN bus").
      send_mode  CAN: the routing byte (0 = reply over CAN, 1 = reply out the
                 node's own UART, 3 = no reply). BLE: there is no such field
                 on a point-to-point link, so only "3 = expect no reply"
                 survives, as a hint not to wait for one.

    Timing differs by an order of magnitude between the two (a CAN round trip
    is ~5-10 ms, a BLE one is two connection intervals plus the adapter's own
    CAN hop), so poll intervals and reply timeouts are asked for here instead
    of being compiled in at the call sites.
*/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VESC_LINK_MODE_CAN = 0,
    VESC_LINK_MODE_BLE = 1,
} vesc_link_mode_t;

/* Reply id embedded in the LISP quick-action panel protocol. On CAN it is our
 * own node id and the script answers with (send-data buf 2 id); on BLE no CAN
 * id exists for us, so we send this sentinel and the script answers on the
 * interface the request arrived on. 255 is the CAN broadcast address, so no
 * real node can ever claim it. */
#define VESC_LINK_REPLY_ID_BLE 255

typedef void (*vesc_link_packet_handler_t)(const uint8_t *data, unsigned int len);

/* Implemented by the BLE backend (main/vesc_ble_link.c) and registered at
 * boot. Lives in main/ because NimBLE does; this component never links
 * against the BLE stack. */
typedef struct {
    void (*send)(uint8_t target, const uint8_t *data, unsigned int len,
                 uint8_t send_mode);
    void (*send_sync)(uint8_t target, const uint8_t *data, unsigned int len,
                      uint8_t send_mode, uint32_t timeout_ms);
    /* Same as send, but the request came from the phone-side NUS bridge: the
     * answer is flagged for forwarding back and nothing else is (see
     * vesc_link_send_from_phone). */
    void (*send_from_phone)(uint8_t target, const uint8_t *data,
                            unsigned int len, uint8_t send_mode);
    bool (*is_up)(void);
    /* One line for the Settings screen, e.g. "BLE: connected, -62 dBm".
     * Returns false when there is nothing to say. */
    bool (*health_text)(char *buf, size_t n);
} vesc_link_ops_t;

void vesc_link_register_ble(const vesc_link_ops_t *ops);

void             vesc_link_set_mode(vesc_link_mode_t mode);
vesc_link_mode_t vesc_link_get_mode(void);

/* True when the live transport can actually carry a request right now: the
 * TWAI driver is installed (CAN) or the adapter link is up and subscribed
 * (BLE). Pollers skip a cycle instead of burning their reply timeout. */
bool vesc_link_is_up(void);

void vesc_link_send(uint8_t target, const uint8_t *data, unsigned int len,
                    uint8_t send_mode);

/* Send a request that arrived from the phone over the NUS bridge (VESC Tool
 * talking through this device). On CAN it also queues behind the dashboard's
 * own polls; on BLE it is just a tagged enqueue, since that transport already
 * carries one request at a time.
 *
 * The tag matters on the way back: only the answer to such a request is
 * forwarded to the phone. Forwarding *every* payload the VESC sends us — the
 * dashboard's 10 Hz COMM_GET_VALUES_SETUP_SELECTIVE reply and, on BLE, the
 * second head's temperatures-only COMM_GET_VALUES_SELECTIVE reply — makes VESC
 * Tool decode a partial dataset: the fields the display did not ask for come
 * out zero, so speed and drawn Ah collapse once a second and jump back. */
void vesc_link_send_from_phone(uint8_t target, const uint8_t *data,
                               unsigned int len, uint8_t send_mode);

/* Blocks until the reply has been delivered to the packet handler, or the
 * timeout expires. The serialisation rule that came with the CAN transport
 * still holds on BLE (one request in flight at a time), so this must only be
 * called from the single poll task — see rt_task() in vesc_rt_data.c. */
void vesc_link_send_sync(uint8_t target, const uint8_t *data, unsigned int len,
                         uint8_t send_mode, uint32_t timeout_ms);

/* Value for the panel protocol's reply_can_id field. */
uint8_t vesc_link_reply_id(void);

/* Reply timeout for vesc_link_send_sync on the live transport. */
uint32_t vesc_link_sync_timeout_ms(void);

/* Scale a CAN-tuned poll interval for the live transport. Identity on CAN. */
uint32_t vesc_link_scale_ms(uint32_t base_ms);

/* Same, for the dashboard's own *polling* cadence — plus one rule that only
 * matters when the NUS bridge has a peer: while the phone (VESC Tool) is
 * talking to the same node, this device's polls are stretched to at most
 * 1 Hz. They share one request/reply rail with the phone's requests, so at
 * 10 Hz the dashboard alone could keep that rail busy enough that VESC Tool's
 * requests queue up behind it and time out (the "connected, one dataset, then
 * nothing" symptom). The dashboard does not need them: every payload the
 * bridge forwards passes through the same fan-out, so VESC Tool's own answers
 * keep the fast fields fresh; the slow ones (odometer, pack level) cost at
 * most a second of staleness. Not for timeouts — use vesc_link_scale_ms(). */
uint32_t vesc_link_poll_ms(uint32_t base_ms);

/* The phone-side bridge sent a request. Called on the bridge task, right
 * before the request is handed to the transport, so it must stay a timestamp
 * write and nothing else. */
void vesc_link_bridge_note_traffic(void);

/* True for a short hold window after the last phone request. Read by the
 * dashboard's poll loops through vesc_link_poll_ms(). */
bool vesc_link_bridge_active(void);

/* Longest prefix vesc_link_reply_prefix() writes (command + 4 mask bytes). */
#define VESC_LINK_REPLY_PREFIX_MAX 5

/* The prefix the answer to `data` must start with, so a transport can tell
 * the phone's own answer from the dashboard's: the echoed command byte, plus
 * the echoed mask for the two SELECTIVE polls (whose replies carry a mask that
 * this device and VESC Tool both use — the command byte alone is not enough).
 * A pre-addressed COMM_FORWARD_CAN payload is unwrapped first, because the
 * answer comes back without the wrapper. Returns the length, 0 for bad input. */
uint8_t vesc_link_reply_prefix(const uint8_t *data, unsigned int len,
                               uint8_t out[VESC_LINK_REPLY_PREFIX_MAX]);

/* Whether `reply` (len bytes) starts with `prefix` (plen bytes). A reply
 * shorter than the prefix — the SET_* family answers with the bare command —
 * is compared as far as it goes. */
bool vesc_link_reply_matches(const uint8_t *reply, unsigned int len,
                             const uint8_t *prefix, uint8_t plen);

/* One hook for reassembled VESC packets, whichever transport produced them.
 * Registering it also wires the CAN backend's own handler. */
void vesc_link_set_packet_handler(vesc_link_packet_handler_t handler);

/* Feed a reassembled payload to the registered handler. Called by the CAN
 * decode task and by the BLE backend's RX task. */
void vesc_link_deliver(const uint8_t *data, unsigned int len);

/* Same, with the bridge tag carried explicitly. The BLE backend uses this
 * (its TX task knows which queued request the answer belongs to); the CAN
 * branch of vesc_link_deliver() gets the tag from comm_can. */
void vesc_link_deliver_tagged(const uint8_t *data, unsigned int len,
                              bool reply_for_phone);

/* True only while the packet handler runs for the answer to a request sent
 * through vesc_link_send_from_phone(). Everything else the VESC answers —
 * the dashboard's own polls, unsolicited output — is false. */
bool vesc_link_reply_for_phone(void);

bool vesc_link_health_text(char *buf, size_t n);

#ifdef __cplusplus
}
#endif
