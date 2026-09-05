#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* The one-word answer to "is Android Auto connected?" shown on the AA idle
 * screen, with a line of detail about the step in progress.
 *
 *   Disconnected  nothing in flight; the detail says why / what to do
 *   Connecting    a link is being brought up: paging the phone over BT, the
 *                 phone found, joining Wi-Fi, TCP accepted, handshake
 *   Connected     the AA session is live (the video takes the panel anyway)
 *
 * Fed from wherever the P4 learns something: the Connect tap and the
 * reconnect path (aa_reconnect), the BT agent's BT:* event lines (bt_link),
 * a station joining the SoftAP (WIFI_EVENT), the TCP accept / handshake /
 * session end (tcp_server). "Connecting" that makes no progress for
 * AA_LINK_CONNECTING_TIMEOUT_MS falls back to Disconnected so the screen
 * never claims to be working on something it gave up on. */
typedef enum {
    AA_LINK_DISCONNECTED = 0,
    AA_LINK_CONNECTING,
    AA_LINK_CONNECTED,
} aa_link_state_t;

/* Hook the BT-agent events and the Wi-Fi AP events. Call once wifi_manager
 * and bt_link are up (the default event loop must exist). */
void aa_link_status_init(void);

/* Set the state and its detail line (ASCII only — the idle screen's font
 * has no other glyphs). Safe from any task. */
void aa_link_status_set(aa_link_state_t state, const char *detail);

aa_link_state_t aa_link_status_get(void);

#ifdef __cplusplus
}
#endif
