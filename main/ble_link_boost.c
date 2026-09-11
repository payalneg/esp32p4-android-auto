#include "ble_link_boost.h"

#include "esp_log.h"
#include "host/ble_gap.h"
#include "nimble/ble.h"
#include "nimble/nimble_opt.h"
#include "host/ble_hs.h"

#include "ble_central_arb.h"

static const char *TAG = "ble_boost";

static int s_refs;        /* how many callers want the fast interval */
static int s_scan_refs;   /* ...of which want the arbiter parked too */

/* Ask for the fastest radio the pair can manage.
 *
 * Two knobs, both one-shot per link and both worth having before any bulk
 * transfer. The connection interval alone is not the whole story: at the
 * default 27-byte link-layer packet a 509-byte write is fragmented into
 * twenty radio packets, so the interval buys far less than it looks. A long
 * packet cuts that to three, and the 2M PHY halves the air time of each.
 *
 * Best-effort throughout — the central may refuse either, and a controller
 * that does not support them answers with an error we only log.
 *
 * Measured 2026-09-11: both are accepted (rc=0) and neither made a tile
 * transfer faster. 25 KB takes 49 writes and 780 ms — 16 ms each, which is
 * one connection interval. The phone's stack issues one ATT write per
 * interval and waits for its callback, so the round trip is the limit and
 * air time never was. Kept because they cost nothing and the next thing to
 * use this link may well be packet-bound; the lever that would actually
 * help is several writes per interval, which means going around
 * flutter_blue_plus with our own Android code. */
static void ask_for_a_fast_radio(uint16_t conn)
{
    /* 251 bytes is the maximum a 4.2 link layer carries; the time is what the
     * spec allows for it on the 1M PHY. */
    int rc = ble_gap_set_data_len(conn, 251, 2120);
    ESP_LOGD(TAG, "data len 251 rc=%d", rc);
#if MYNEWT_VAL(BLE_PHY_2M) || defined(BLE_HCI_LE_PHY_2M_PREF_MASK)
    rc = ble_gap_set_prefered_le_phy(conn,
                                     BLE_HCI_LE_PHY_2M_PREF_MASK,
                                     BLE_HCI_LE_PHY_2M_PREF_MASK,
                                     BLE_HCI_LE_PHY_CODED_ANY);
    ESP_LOGD(TAG, "2M phy rc=%d", rc);
#endif
}

static void apply(uint16_t conn)
{
    if (conn == BLE_HS_CONN_HANDLE_NONE) return;
    const bool fast = s_refs > 0;
    ESP_LOGI(TAG, "link %s (refs=%d) on conn=%u",
             fast ? "fast" : "normal", s_refs, (unsigned)conn);
    if (fast) ask_for_a_fast_radio(conn);
    struct ble_gap_upd_params p = {
        .itvl_min            = fast ? 9  : BLE_GAP_INITIAL_CONN_ITVL_MIN,  /* 11.25 ms : 30 ms */
        .itvl_max            = fast ? 12 : BLE_GAP_INITIAL_CONN_ITVL_MAX,  /* 15 ms    : 50 ms */
        .latency             = 0,
        .supervision_timeout = 400,    /* 4 s */
        .min_ce_len          = 0,
        .max_ce_len          = 0,
    };
    int rc = ble_gap_update_params(conn, &p);
    if (rc != 0) ESP_LOGW(TAG, "conn param update (%s) rc=%d", fast ? "fast" : "normal", rc);
}

void ble_link_boost_request(uint16_t conn_handle, bool on, bool suspend_scan)
{
    const int was = s_refs;
    if (on) {
        s_refs++;
        if (suspend_scan && s_scan_refs++ == 0) ble_arb_scan_suspend();
    } else {
        if (s_refs > 0) s_refs--;
        if (suspend_scan && s_scan_refs > 0 && --s_scan_refs == 0) ble_arb_scan_resume();
    }
    /* Only the 0↔1 edges change the link. */
    if ((was > 0) != (s_refs > 0)) apply(conn_handle);
}

void ble_link_boost_reset(void)
{
    if (s_scan_refs > 0) ble_arb_scan_resume();
    s_refs = 0;
    s_scan_refs = 0;
}
