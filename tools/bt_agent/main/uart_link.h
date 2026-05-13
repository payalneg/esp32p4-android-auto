#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Thin UART link to the ESP32-P4 on the other side. Used to signal events
 * like "phone paired" or "wifi setup done" so P4 can coordinate its TCP
 * listener if it wants to. Plain text, line-delimited.
 *
 * Wiring (D1 Mini side ↔ P4 J3 header):
 *   D1 Mini GPIO 1  (TX0)  →  P4 GPIO 21  (P4 RX)
 *   D1 Mini GPIO 3  (RX0)  ←  P4 GPIO 22  (P4 TX)
 *   GND  ↔  GND
 *   5V (from P4 J3 5V pin) → D1 Mini 5V/VBUS
 *
 * UART0 (the standard USB-Serial pins) is the only path on this D1 Mini
 * clone that reliably drives a 3.3V signal — the supposed UART2 pads
 * (GPIO 16/17) didn't move data in either direction during bench testing.
 * Trade-off: the on-board CH9102 USB-to-UART chip shares these lines.
 *   - Don't have USB-C plugged into the D1 Mini while the wires to P4 are
 *     connected; CH9102 will fight P4 on the bus.
 *   - To re-flash D1 Mini: unplug wires to P4 → plug USB-C → idf.py flash
 *     → unplug USB-C → reconnect P4 wires.
 *
 * Baud rate 115200, 8N1, no flow control. */

void uart_link_init(void);

/* Send a single line (newline added automatically) to P4. Safe to call from
 * any task/callback. */
void uart_link_say(const char *line);

/* WiFi credentials + endpoint received from P4. None of the strings are
 * valid until uart_link_have_wifi() returns true. They live in static
 * storage owned by the UART RX task — copy if you want to outlive the
 * next P4 update. */
typedef struct {
    char ssid[33];
    char password[65];
    char bssid[18];   /* "XX:XX:XX:XX:XX:XX" */
    char ip[16];      /* dotted-quad */
    int  port;
} uart_link_wifi_t;

bool uart_link_have_wifi(void);
const uart_link_wifi_t *uart_link_get_wifi(void);

/* Top-level mode the P4 selected — received as `MODE|AA\n` or
 * `MODE|AVRCP\n`. main.c waits for this on boot before deciding which BT
 * profile set to enable. Values match the P4 connection_mode_t enum
 * (AVRCP=0, ANDROID_AUTO=1; CARPLAY is not sent because P4 never boots the
 * agent in that mode). */
typedef enum {
    UART_LINK_MODE_AVRCP = 0,
    UART_LINK_MODE_AA    = 1,
    UART_LINK_MODE_NONE  = 0xff,   /* "not received yet" sentinel */
} uart_link_mode_t;

/* Block up to `timeout_ms` waiting for a MODE line from P4. On timeout
 * returns UART_LINK_MODE_AA — that's the historical pre-AVRCP behaviour,
 * so existing boards that never get a MODE line keep doing AA. */
uart_link_mode_t uart_link_wait_mode(uint32_t timeout_ms);

/* AVRCP CT helpers — fire-and-forget tagged lines so the P4 can drive its
 * Now Playing widget. Empty fields are allowed (sender pre-replaces any
 * '|' inside a field with a space to keep the format unambiguous). */
void uart_link_send_meta(const char *title, const char *artist, const char *album);
void uart_link_send_state(bool playing);
