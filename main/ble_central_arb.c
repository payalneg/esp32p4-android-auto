#include "ble_central_arb.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "host/ble_hs.h"

static const char *TAG = "ble_arb";

#define ARB_MAX_SLOTS 2
/* Connect window when BOTH sensors are bound and waiting. NimBLE cancels the
 * attempt itself on expiry and reports a failed CONNECT to the owner, which
 * re-requests; the arbiter then hands the initiator to the other slot. */
#define ARB_WINDOW_MS 8000

typedef struct {
    bool              used;
    ble_gap_event_fn *cb;
    bool              want;   /* client wants a connection */
    ble_addr_t        peer;
} arb_slot_t;

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static arb_slot_t s_slots[ARB_MAX_SLOTS];
static int        s_nslots;
static bool       s_synced;
static uint8_t    s_own_addr_type;
static int        s_pending;        /* slot owning the in-flight connect; -1 = none */
static bool       s_pending_forever; /* the in-flight connect was issued with BLE_HS_FOREVER */
static bool       s_scan_suspended;
static int        s_next;           /* round-robin cursor */

static void arb_evaluate(void);

/* Forwards every GAP event to the owning client unchanged; on CONNECT it
 * first releases the initiator so the client's re-arm (or the other slot)
 * can immediately claim it. Runs on the NimBLE host task. */
static int arb_trampoline(struct ble_gap_event *event, void *arg)
{
    int id = (int)(intptr_t)arg;
    bool was_connect = false;

#if defined(BLE_GAP_EVENT_LINK_ESTAB)
    if (event->type == BLE_GAP_EVENT_LINK_ESTAB) was_connect = true;
#else
    if (event->type == BLE_GAP_EVENT_CONNECT) was_connect = true;
#endif

    if (was_connect) {
        portENTER_CRITICAL(&s_mux);
        if (s_pending == id) { s_pending = -1; s_pending_forever = false; }
        if (event->connect.status == 0) s_slots[id].want = false;
        portEXIT_CRITICAL(&s_mux);
    }

    int rc = s_slots[id].cb(event, NULL);

    /* After a connect resolved either way, give the initiator to whoever
     * still wants it (the owner re-requested inside its callback on failure;
     * on success the other slot may be waiting). */
    if (was_connect) arb_evaluate();
    return rc;
}

static void arb_evaluate(void)
{
    portENTER_CRITICAL(&s_mux);
    if (!s_synced || s_scan_suspended || s_pending != -1) {
        portEXIT_CRITICAL(&s_mux);
        return;
    }
    int wanting = 0;
    for (int i = 0; i < s_nslots; i++) {
        if (s_slots[i].used && s_slots[i].want) wanting++;
    }
    if (wanting == 0) {
        portEXIT_CRITICAL(&s_mux);
        return;
    }
    int pick = -1;
    for (int k = 0; k < s_nslots; k++) {
        int i = (s_next + k) % s_nslots;
        if (s_slots[i].used && s_slots[i].want) { pick = i; break; }
    }
    ble_addr_t peer = s_slots[pick].peer;
    uint8_t own = s_own_addr_type;
    int32_t window = (wanting >= 2) ? ARB_WINDOW_MS : BLE_HS_FOREVER;
    /* Mark pending BEFORE the NimBLE call so a racing evaluate from another
     * task backs off; rolled back below if the call is refused. */
    s_pending = pick;
    s_pending_forever = (window == (int32_t)BLE_HS_FOREVER);
    s_next = (pick + 1) % s_nslots;
    portEXIT_CRITICAL(&s_mux);

    int rc = ble_gap_connect(own, &peer, window, NULL,
                             arb_trampoline, (void *)(intptr_t)pick);
    if (rc == 0) {
        ESP_LOGI(TAG, "slot %d connecting to "
                 "%02X:%02X:%02X:%02X:%02X:%02X (type %u, window %s)",
                 pick, peer.val[5], peer.val[4], peer.val[3],
                 peer.val[2], peer.val[1], peer.val[0],
                 (unsigned)peer.type,
                 window == (int32_t)BLE_HS_FOREVER ? "forever" : "bounded");
        return;
    }
    portENTER_CRITICAL(&s_mux);
    if (s_pending == pick) s_pending = -1;
    portEXIT_CRITICAL(&s_mux);
    if (rc == BLE_HS_EALREADY || rc == BLE_HS_EBUSY) {
        /* A scan or a racing attempt owns the initiator — the next GAP event
         * (DISC_COMPLETE / CONNECT) re-evaluates. Same tolerance the sensor
         * clients had before the arbiter. */
        ESP_LOGD(TAG, "initiator busy (rc=%d), will retry on next event", rc);
    } else {
        ESP_LOGW(TAG, "ble_gap_connect rc=%d (slot %d)", rc, pick);
    }
}

/* ---------- public API ---------- */

void ble_arb_init(void)
{
    s_pending = -1;
}

void ble_arb_on_sync(uint8_t own_addr_type)
{
    portENTER_CRITICAL(&s_mux);
    s_own_addr_type = own_addr_type;
    s_synced = true;
    portEXIT_CRITICAL(&s_mux);
    /* Clients arm their connects from their own on_ble_sync hooks, which
     * ble_host calls right after this one. */
}

int ble_arb_register(ble_gap_event_fn *cb)
{
    if (cb == NULL || s_nslots >= ARB_MAX_SLOTS) return -1;
    int id = s_nslots++;
    s_slots[id].used = true;
    s_slots[id].cb = cb;
    s_slots[id].want = false;
    return id;
}

void ble_arb_want_connect(int id, const ble_addr_t *peer)
{
    if (id < 0 || id >= s_nslots || peer == NULL) return;
    portENTER_CRITICAL(&s_mux);
    s_slots[id].want = true;
    s_slots[id].peer = *peer;
    bool already_ours = (s_pending == id);
    /* If the OTHER slot pends with BLE_HS_FOREVER (it was the only wanter
     * when its connect was issued), that attempt would starve us until its
     * sensor wakes — break it; its owner re-requests from the failed-CONNECT
     * callback and both slots then rotate on bounded windows. */
    bool demote = (s_pending != -1 && !already_ours && s_pending_forever);
    portEXIT_CRITICAL(&s_mux);
    if (demote) {
        ble_gap_conn_cancel();
    } else if (!already_ours) {
        arb_evaluate();
    }
}

void ble_arb_stop_connect(int id)
{
    if (id < 0 || id >= s_nslots) return;
    portENTER_CRITICAL(&s_mux);
    s_slots[id].want = false;
    bool cancel = (s_pending == id);
    portEXIT_CRITICAL(&s_mux);
    if (cancel) {
        /* The cancel completion arrives as a failed CONNECT through the
         * trampoline, which clears s_pending and re-evaluates. */
        ble_gap_conn_cancel();
    }
}

void ble_arb_scan_suspend(void)
{
    portENTER_CRITICAL(&s_mux);
    s_scan_suspended = true;
    bool cancel = (s_pending != -1);
    portEXIT_CRITICAL(&s_mux);
    if (cancel) {
        ble_gap_conn_cancel();
    }
}

void ble_arb_scan_resume(void)
{
    portENTER_CRITICAL(&s_mux);
    s_scan_suspended = false;
    portEXIT_CRITICAL(&s_mux);
    arb_evaluate();
}
