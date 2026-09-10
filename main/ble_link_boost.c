#include "ble_link_boost.h"

#include "esp_log.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"

#include "ble_central_arb.h"

static const char *TAG = "ble_boost";

static int s_refs;        /* how many callers want the fast interval */
static int s_scan_refs;   /* ...of which want the arbiter parked too */

static void apply(uint16_t conn)
{
    if (conn == BLE_HS_CONN_HANDLE_NONE) return;
    const bool fast = s_refs > 0;
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
