/*
    Copyright 2022 Benjamin Vedder benjamin@vedder.se
    Adapted from VESC firmware (GPL-3.0).

    Thin VESC-CAN layer on top of ESP-IDF TWAI. Brings up the TWAI
    driver, runs an RX-decode task, exposes packet send / handler
    register API, and stores the periodic STATUS_* frames keyed by
    controller ID.
*/

#pragma once

#include "esp_err.h"
#include "vesc_can/vesc_datatypes.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_STATUS_MSGS_TO_STORE 10

/* Init / teardown */
esp_err_t comm_can_start(int pin_tx, int pin_rx,
                         uint8_t controller_id, int can_speed_kbps);
void      comm_can_stop(void);
esp_err_t comm_can_reinit(uint8_t controller_id, int can_speed_kbps);

/* Virtual bus — the VESC emulator's transport. Same stack as the real one
 * (fragmenting, per-id reassembly, process_task, the sync-poll semaphore and
 * its timeouts), only the TWAI driver is replaced: every frame this node
 * transmits is handed to `tx` (the simulated VESC, see main/vesc_sim.c), and
 * whatever that node sends back arrives through comm_can_virtual_rx() into the
 * same receive ring rx_task feeds. Exactly one producer may call
 * comm_can_virtual_rx (the ring is single-producer). */
typedef void (*comm_can_virtual_tx_t)(uint32_t eid, const uint8_t *data, uint8_t len);
esp_err_t comm_can_start_virtual(uint8_t controller_id, comm_can_virtual_tx_t tx);
void      comm_can_virtual_rx(uint32_t eid, const uint8_t *data, uint8_t len);
bool      comm_can_is_virtual(void);

/* Raw transmit (extended ID). */
void comm_can_transmit_eid(uint32_t id, const uint8_t *data, uint8_t len);

/* This node's own VESC-CAN controller ID (the sender ID embedded in every
 * comm_can_send_buffer frame). Useful when a remote needs to address a reply
 * back to us at the application layer — e.g. a LISP (send-data … 2 id). */
uint8_t comm_can_get_local_id(void);

/* Send a VESC-protocol payload to a node, fragmenting if >6 bytes.
 * `send` selects the response routing: 0 = wait + reply via CAN, 1 =
 * reply over UART, 3 = no reply expected. The display polls with 0. */
void comm_can_send_buffer(uint8_t controller_id, const uint8_t *data,
                          unsigned int len, uint8_t send);

/* Like comm_can_send_buffer, but blocks (up to reply_timeout_ms) until the
 * VESC's reply has been reassembled and delivered. Use for continuous CAN
 * polls so two multi-frame replies from the same node can't interleave in the
 * shared per-id reassembly buffer (which corrupts both → PROCESS_RX_BUFFER CRC
 * mismatch). MUST be called only from the single CAN poll task (vesc_rt_data's
 * rt_task): there is no lock — serialisation comes from every continuous poll
 * (RT data, LISP stats, IO decode, the quick-action panel) living on that one
 * task and waiting here for each reply before sending the next. Use `send`=0. */
void comm_can_send_buffer_sync(uint8_t controller_id, const uint8_t *data,
                               unsigned int len, uint8_t send,
                               uint32_t reply_timeout_ms);

bool comm_can_ping(uint8_t controller_id, HW_TYPE *hw_type);

/* Live TWAI health snapshot for the Settings screen. bus_err is the driver's
 * cumulative bus-error counter since comm_can_start, recoveries the number of
 * completed bus-off recoveries (see can_health_check). Any pointer may be
 * NULL; returns false (and zeroes the outputs) when the driver is not up. */
bool comm_can_get_bus_health(uint32_t *bus_err, uint32_t *recoveries);

/* Convenience setters (translated to fragments+forward-CAN if needed). */
void comm_can_set_duty(uint8_t controller_id, float duty);
void comm_can_set_current(uint8_t controller_id, float current);
void comm_can_set_current_brake(uint8_t controller_id, float current);
void comm_can_set_rpm(uint8_t controller_id, float rpm);

/* Latest periodic STATUS_* frame for a given controller, or NULL. */
can_status_msg   *comm_can_get_status_msg_id(int id);
can_status_msg_2 *comm_can_get_status_msg_2_id(int id);
can_status_msg_3 *comm_can_get_status_msg_3_id(int id);
can_status_msg_4 *comm_can_get_status_msg_4_id(int id);
can_status_msg_5 *comm_can_get_status_msg_5_id(int id);
can_status_msg_6 *comm_can_get_status_msg_6_id(int id);

/* Hook for reassembled VESC packets (PROCESS_RX_BUFFER /
 * PROCESS_SHORT_BUFFER). The handler runs on the CAN process task
 * — keep it short and don't take blocking locks held by the CAN
 * task itself. */
typedef void (*can_packet_handler_t)(const uint8_t *data, unsigned int len);
void comm_can_set_packet_handler(can_packet_handler_t handler);

/* Identity answered to a COMM_FW_VERSION request arriving over CAN.
 *
 * VESC Tool's CAN scan pings the bus, then asks every node that answered for
 * its firmware version; a node that stays silent is listed as "Unknown". We
 * answer that one request (nothing else is served — this is not a VESC) so the
 * head unit shows up by name next to the motor controllers.
 *
 * `hw_name` is copied (truncated to 31 chars). `uuid` may be NULL, in which
 * case the 12 UUID bytes are zeroed; pass something stable and unique per
 * device (the WiFi MAC works) so VESC Tool can tell two units apart. */
void comm_can_set_fw_info(const char *hw_name, uint8_t major, uint8_t minor,
                          const uint8_t *uuid, unsigned int uuid_len);

#ifdef __cplusplus
}
#endif
