#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "driver/gpio.h"

/* P4 ↔ D1 Mini BT-agent UART link.
 *
 * Wiring differs per board (pins picked from each board's free header):
 *
 *   Waveshare 4.3" (J3 header):          Guition JC4880P443C:
 *     P4 GPIO 22 (TX) → D1 Mini RX2        P4 GPIO 33 (TX) → external ESP32 RX
 *     P4 GPIO 21 (RX) ← D1 Mini TX2        P4 GPIO 31 (RX) ← external ESP32 TX
 *     P4 GPIO 24 (RST)→ D1 Mini EN/RST     P4 GPIO 30 (RST)→ external ESP32 EN/RST
 *     P4 GPIO 25 (IO0)→ D1 Mini GPIO 0     P4 GPIO 29 (IO0)→ external ESP32 GPIO 0
 *
 * RST/IO0 are driven push-pull (no external pull-ups on these boards).
 *
 * Bidirectional: P4 publishes WiFi-AP credentials over the BT agent for the
 * AA Wireless handshake; the agent reports state events + ESP_LOG output back. */

/* Strap pin numbers — exposed so bt_agent_ota can drive them via
 * esp_serial_flasher's port (it manages reset/IO0 itself during flash).
 * Board-conditional via the Kconfig `choice BOARD_MODEL`. */
#if CONFIG_BOARD_JC4880P443C
#define BT_AGENT_RST_PIN  GPIO_NUM_30
#define BT_AGENT_IO0_PIN  GPIO_NUM_29
#define BT_AGENT_UART_TX  GPIO_NUM_33
#define BT_AGENT_UART_RX  GPIO_NUM_31
#else /* CONFIG_BOARD_WAVESHARE_43 */
#define BT_AGENT_RST_PIN  GPIO_NUM_24
#define BT_AGENT_IO0_PIN  GPIO_NUM_25
#define BT_AGENT_UART_TX  GPIO_NUM_22
#define BT_AGENT_UART_RX  GPIO_NUM_21
#endif
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

/* Total bytes rx_task has received from the agent since boot. Used by
 * bt_agent_ota: if wait_version() times out AND this is still 0, the
 * module is silent (likely not connected / dead) and reflashing it would
 * also fail at SYNC — so we skip the OTA entirely instead of burning 30 s
 * on a doomed flash attempt. */
uint32_t bt_agent_rx_byte_count(void);

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

/* Tell the BT agent whether to proactively HFP-page the last paired phone
 * on power-up. When false, the agent stays silent and only reconnects if
 * the phone initiates BT from its side. Persisted on the agent in NVS so
 * the value survives across agent reboots without P4 having to re-send it
 * every boot — we still re-send on each P4 boot for safety. */
void bt_link_set_auto_reconnect(bool on);

/* Ask the BT agent to kick the BT link so gearhead re-initiates AA. Called
 * by tcp_server right after the AA TCP session ends — the agent bounces
 * HFP (and through its auto-reconnect task pages the phone back), which
 * triggers a fresh SPP → WifiStartRequest cycle and a new AA TCP session.
 * Caller is expected to gate on settings_get_aa_autoconnect(); the agent
 * itself also honours the same toggle so a stale call is harmless. */
void bt_link_request_aa_reconnect(void);

/* Tell the BT agent whether an AA session is live. On `live` the agent goes
 * off air for the duration — closes SPP, drops HFP, stops page/inquiry scan —
 * the way the reference dongle powers its adapter off once the phone is on
 * TCP: any BT event on a bonded car kit makes gearhead re-run the wireless
 * setup, and a completed setup always restarts projection. On `!live` it
 * comes back; `peer_closed` (clean FIN, the user most likely exited AA) holds
 * its auto-page until Connect is tapped, a lost session is re-paged. Sent by
 * tcp_server after the handshake and by aa_reconnect after the session
 * ends; re-sent automatically when the agent is seen rebooting. */
void bt_link_set_aa_session(bool live, bool peer_closed);

/* Last state sent with bt_link_set_aa_session(): true while the agent has
 * been told a session is live and not yet told it ended. */
bool bt_link_aa_session_live(void);

/* Manual "Connect" — user tapped the button on the idle screen. The agent
 * unconditionally HFP-pages the last paired phone regardless of the
 * auto-reconnect toggle. No-op on the agent side if no phone is paired
 * yet, so this is safe to wire to a button that's always visible. */
void bt_link_request_connect_now(void);

/* Every `BT:<event>` line the agent prints (READY, PAIRED, CONNECTED,
 * DISCONNECTED, WIFI_CONNECT_STATUS, ON_AIR, OFF_AIR, BOOT, ...) is handed
 * to this callback with the text after "BT:". Runs on bt_link's UART rx
 * task; keep it short. One subscriber (aa_link_status). */
typedef void (*bt_link_event_cb_t)(const char *event);
void bt_link_set_event_cb(bt_link_event_cb_t cb);
