#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"

/* P4 ↔ D1 Mini BT-agent UART link.
 *
 * Wiring (J3 header on the Waveshare board):
 *   P4 GPIO 22 (TX)   →  D1 Mini GPIO 16 (RX2)
 *   P4 GPIO 21 (RX)   ←  D1 Mini GPIO 17 (TX2)
 *   P4 GPIO 24 (RST)  →  D1 Mini EN/RST  (push-pull, no external pull-up on this board)
 *   P4 GPIO 25 (IO0)  →  D1 Mini GPIO 0  (push-pull, no external pull-up on this board)
 *
 * Bidirectional: P4 publishes WiFi-AP credentials over the BT agent for the
 * AA Wireless handshake; the agent reports state events + ESP_LOG output back. */

/* Strap pin numbers — exposed so bt_agent_ota can drive them via
 * esp_serial_flasher's port (it manages reset/IO0 itself during flash). */
#define BT_AGENT_RST_PIN  GPIO_NUM_24
#define BT_AGENT_IO0_PIN  GPIO_NUM_25
#define BT_AGENT_UART_TX  GPIO_NUM_22
#define BT_AGENT_UART_RX  GPIO_NUM_21
#define BT_AGENT_UART_PORT  1   /* UART_NUM_1 — keep as plain int for esp_serial_flasher cfg */

/* Bring the agent into normal-boot mode: configure RST/IO0 as open-drain,
 * release IO0 (= high via external pull-up = boot from flash), then pulse
 * RST low → high. Blocks for ~250 ms (50 ms reset + 200 ms ROM → app). */
void bt_agent_reset_to_app(void);

/* Force the agent into ROM serial bootloader: drive IO0 low, pulse RST,
 * release IO0 only after the ROM has latched the strap. Caller is responsible
 * for tearing down the UART driver beforehand if it intends to talk to the
 * ROM (esp_serial_flasher reinstalls it). */
void bt_agent_enter_bootloader(void);

/* Last value parsed from a `BT-VER:<version>` line in rx_task, or NULL if
 * none seen yet. Buffer is owned by bt_link — do not free, do not assume
 * it stays stable across reboots of the agent (overwritten on each new line). */
const char *bt_agent_get_version(void);

/* Block until rx_task sees a `BT-VER:` line or timeout elapses. Returns
 * the version string (same buffer as bt_agent_get_version) on success,
 * NULL on timeout. Used by bt_agent_ota to gate the version comparison. */
const char *bt_agent_wait_version(uint32_t timeout_ms);

void bt_link_init(void);

/* Tear down UART driver so esp_serial_flasher can take over the port.
 * Pair with bt_link_resume_after_flash() once flashing is done. */
void bt_link_suspend_for_flash(void);
void bt_link_resume_after_flash(void);

/* Stop forwarding agent UART lines to the host console while a noisy boot
 * loop is in progress (broken/unflashed agent reboots itself ~9× / sec
 * and floods rx_task with [BT] forwards — enough printf load on core 0 to
 * starve the FreeRTOS idle task and trip TWDT). Parsing for BT-VER: still
 * runs so wait_version() works. */
void bt_link_set_quiet(bool quiet);

/* Send a single line to D1 Mini:
 *
 *   WIFI|<ssid>|<password>|<bssid>|<ip>|<port>\n
 *
 * Pipes are the field separator. None of the values currently contain
 * pipes, so no escaping. */
void bt_link_publish_wifi(const char *ssid, const char *password,
                          const char *bssid, const char *ip, int port);
