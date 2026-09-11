#include "aa_reconnect.h"

#include <string.h>

#include "aa_link_status.h"
#include "bt_link.h"
#include "dev_settings.h"
#include "esp_log.h"
#include "wifi_manager.h"

static const char *TAG = "aa_reconnect";

/* Peer of the last AA session — the manual path has no socket to read it
 * from, and with a laptop on the bench AP "kick everyone" is not an option. */
static char s_last_peer_ip[16];

void aa_reconnect_after_drop(const char *peer_ip, bool peer_closed)
{
    if (peer_ip) {
        strlcpy(s_last_peer_ip, peer_ip, sizeof(s_last_peer_ip));
    }
    esp_err_t kicked = wifi_manager_kick_sta(peer_ip);
    bool restart = !peer_closed && settings_get_aa_autoconnect();
    ESP_LOGI(TAG, "AA session %s: phone %s the AP; %s",
             peer_closed ? "closed by phone" : "lost",
             kicked == ESP_OK ? "kicked off" : "not found on",
             restart ? "asking BT agent to restart AA"
                     : peer_closed ? "not paging back (phone's choice)"
                                   : "auto-connect off, waiting for Connect");
    /* Agent back on air. Station is already off the AP, so the phone's next
     * setup run starts from a clean Wi-Fi state. For a lost session the
     * agent's own auto-reconnect loop pages from cold (its HFP was dropped
     * for the session); AA_RECONNECT below covers the case where HFP is
     * somehow still up (agent rebooted mid-session and missed the gate). */
    bt_link_set_aa_session(false, peer_closed);
    if (restart) {
        aa_link_status_set(AA_LINK_CONNECTING, "Link lost, paging the phone...");
        bt_link_request_aa_reconnect();
    } else if (peer_closed) {
        aa_link_status_set(AA_LINK_DISCONNECTED,
                           "Phone ended the session. Tap Connect to start again");
    } else {
        aa_link_status_set(AA_LINK_DISCONNECTED, "Link lost. Tap Connect to reconnect");
    }
}

void aa_reconnect_manual(void)
{
    esp_err_t kicked = wifi_manager_kick_sta(s_last_peer_ip[0] ? s_last_peer_ip : NULL);
    ESP_LOGI(TAG, "Connect tapped: %s; paging phone",
             kicked == ESP_OK ? "phone kicked off the AP" : "no station to kick");
    /* The idle screen is up, so there is no session: make sure the agent is
     * back on air (it ignores BT_CONNECT while it believes one is live). */
    bt_link_set_aa_session(false, false);
    aa_link_status_set(AA_LINK_CONNECTING, "Paging the phone over Bluetooth...");
    bt_link_request_connect_now();
}
