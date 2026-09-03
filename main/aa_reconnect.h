#pragma once

#include <stdbool.h>

/* Restarting the wireless Android Auto flow (BT-agent mode).
 *
 * When an AA session dies the phone keeps both its association with our
 * SoftAP and its HFP link to the BT agent, and gearhead then never restarts
 * projection by itself; a plain BT page finds the link already up and does
 * nothing (field observation 2026-09-03: Wi-Fi stayed connected, "Connect"
 * was dead). The dongles restart the whole flow instead: drop the phone off
 * the AP so its Wi-Fi state resets, bounce the BT link so gearhead sees its
 * car kit "come back" and re-runs SPP → WifiStartRequest → Wi-Fi join → TCP. */

/* tcp_server: the AA session with `peer_ip` just ended. Kicks that station
 * off the SoftAP and, when the auto-connect setting is on and the session was
 * lost rather than closed by the phone (peer_closed — the user most likely
 * exited Android Auto, so leave them be), asks the BT agent to bounce HFP and
 * re-page the phone. */
void aa_reconnect_after_drop(const char *peer_ip, bool peer_closed);

/* Idle-screen "Connect": kick the last AA peer (or the only station) off the
 * AP and page the phone over BT regardless of the auto-connect setting. */
void aa_reconnect_manual(void);
