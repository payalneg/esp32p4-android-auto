#pragma once

/* P4 ↔ D1 Mini BT-agent UART link.
 *
 * Wiring (J3 header on the Waveshare board):
 *   P4 GPIO 22 (TX)  →  D1 Mini GPIO 16 (RX2)
 *   P4 GPIO 21 (RX)  ←  D1 Mini GPIO 17 (TX2)
 *
 * Right now it's TX-only — P4 publishes its WiFi-AP credentials and IP
 * once the SoftAP is up so the BT agent can hand them to the phone via
 * the AA Wireless setup protocol over RFCOMM. Read direction reserved
 * for future P4 ← BT agent commands. */

void bt_link_init(void);

/* Send a single line to D1 Mini:
 *
 *   WIFI|<ssid>|<password>|<bssid>|<ip>|<port>\n
 *
 * Pipes are the field separator. None of the values currently contain
 * pipes, so no escaping. */
void bt_link_publish_wifi(const char *ssid, const char *password,
                          const char *bssid, const char *ip, int port);
