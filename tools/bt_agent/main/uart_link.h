#pragma once

#include <stdbool.h>
#include <stddef.h>

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
