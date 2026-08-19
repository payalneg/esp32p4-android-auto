#include "ble_speed_client.h"

#include "ble_central_arb.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"

static const char *TAG = "ble_spd";

/* ---- stock Bluetooth SIG UUIDs ---- */

/* Cycling Speed and Cadence service / CSC Measurement characteristic */
static const ble_uuid_t *CSC_SVC_UUID  = BLE_UUID16_DECLARE(0x1816);
static const ble_uuid_t *CSC_MEAS_UUID = BLE_UUID16_DECLARE(0x2A5B);
/* Standard: CCCD descriptor, Battery service + Battery Level char */
static const ble_uuid_t *CCCD_UUID     = BLE_UUID16_DECLARE(0x2902);
static const ble_uuid_t *BATT_SVC_UUID = BLE_UUID16_DECLARE(0x180F);
static const ble_uuid_t *BATT_LVL_UUID = BLE_UUID16_DECLARE(0x2A19);

#define CSC_SVC_UUID16 0x1816
/* CSC Measurement flags */
#define CSC_FLAG_WHEEL_DATA 0x01
#define CSC_FLAG_CRANK_DATA 0x02

#define SCAN_DURATION_MS 6000

/* Plausibility gates for one notification's delta. Sensors notify about once
 * a second; 512 revs/notification is far beyond anything a real wheel does.
 * The rev-rate cap of 130 rev/s equals ~147 km/h on the SMALLEST supported
 * wheel (100 mm diameter, 0.314 m circumference); larger wheels get a
 * correspondingly looser effective speed cap — it's a garbage filter, not a
 * speed limit. */
#define CSC_MAX_DELTA_REVS   512u
#define CSC_MAX_REV_PER_SEC  130u

/* ---- module state ---- */

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static bool     s_inited;
static uint8_t  s_own_addr_type;
static bool     s_synced;

static bool       s_bound;
static ble_addr_t s_bound_addr;

static bool      s_scanning;
static bool      s_connected;          /* link up + subscribed to CSC */
static uint16_t  s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static int       s_arb_id = -1;        /* our slot in the central-connect arbiter */

static uint16_t  s_csc_svc_end;
static uint16_t  s_csc_val_handle;
static uint16_t  s_csc_cccd_handle;

/* CSC accumulator — all under s_mux. The raw counters live in the sensor and
 * survive our disconnects but not its battery pulls; we only trust deltas
 * between two notifications of the SAME connection (s_have_baseline). */
static bool      s_have_baseline;
static uint32_t  s_prev_revs;          /* raw sensor counter at last notify */
static uint16_t  s_prev_evt_1024;      /* raw event time at last notify */
static uint64_t  s_total_revs;         /* accumulated, wrap/reset-safe */
static uint16_t  s_last_delta_revs;
static uint16_t  s_last_delta_1024;
static int64_t   s_rx_us;              /* last notification; 0 = never */
static int64_t   s_rev_us;             /* last time revs ADVANCED; 0 = never */
static uint8_t   s_battery = 0xFF;

static ble_speed_scan_cb_t s_scan_cb;

static int speed_gap_event(struct ble_gap_event *event, void *arg);
static void arm_connect(void);

/* ---------- helpers ---------- */

static void reset_link_handles(void)
{
    s_conn_handle     = BLE_HS_CONN_HANDLE_NONE;
    s_csc_svc_end     = 0;
    s_csc_val_handle  = 0;
    s_csc_cccd_handle = 0;
    portENTER_CRITICAL(&s_mux);
    s_connected = false;
    s_have_baseline = false;   /* deltas never span a disconnect */
    portEXIT_CRITICAL(&s_mux);
}

/* ---------- GATT discovery chain (CSC, then best-effort battery) ---------- */

static int on_batt_read(uint16_t conn, const struct ble_gatt_error *err,
                        struct ble_gatt_attr *attr, void *arg)
{
    (void)conn; (void)arg;
    if (err->status == 0 && attr && attr->om &&
        OS_MBUF_PKTLEN(attr->om) >= 1) {
        uint8_t pct = 0;
        if (os_mbuf_copydata(attr->om, 0, 1, &pct) == 0) {
            portENTER_CRITICAL(&s_mux);
            s_battery = pct;
            portEXIT_CRITICAL(&s_mux);
            ESP_LOGI(TAG, "battery %u%%", (unsigned)pct);
        }
    }
    return 0;
}

static int on_batt_chr(uint16_t conn, const struct ble_gatt_error *err,
                       const struct ble_gatt_chr *chr, void *arg)
{
    (void)arg;
    if (err->status == 0 && chr) {
        ble_gattc_read(conn, chr->val_handle, on_batt_read, NULL);
    }
    return 0;
}

static int on_batt_svc(uint16_t conn, const struct ble_gatt_error *err,
                       const struct ble_gatt_svc *svc, void *arg)
{
    (void)arg;
    if (err->status == 0 && svc) {
        ble_gattc_disc_chrs_by_uuid(conn, svc->start_handle, svc->end_handle,
                                    BATT_LVL_UUID, on_batt_chr, NULL);
    }
    return 0;
}

static void start_battery_discovery(uint16_t conn)
{
    /* Best-effort one-shot battery read; failures are silent (battery stays
     * unknown). A per-connect read is enough for the settings UI. */
    ble_gattc_disc_svc_by_uuid(conn, BATT_SVC_UUID, on_batt_svc, NULL);
}

static int on_csc_subscribed(uint16_t conn, const struct ble_gatt_error *err,
                             struct ble_gatt_attr *attr, void *arg)
{
    (void)conn; (void)attr; (void)arg;
    if (err->status != 0) {
        ESP_LOGW(TAG, "CCCD write failed status=%d", err->status);
        return 0;
    }
    portENTER_CRITICAL(&s_mux);
    s_connected = true;
    s_have_baseline = false;   /* first notification only sets the baseline */
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "subscribed to CSC notifications (conn=%u)", (unsigned)conn);
    start_battery_discovery(conn);
    return 0;
}

static int on_csc_dsc(uint16_t conn, const struct ble_gatt_error *err,
                      uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc,
                      void *arg)
{
    (void)chr_val_handle; (void)arg;
    if (err->status == 0 && dsc &&
        ble_uuid_cmp(&dsc->uuid.u, CCCD_UUID) == 0 && s_csc_cccd_handle == 0) {
        s_csc_cccd_handle = dsc->handle;
        static const uint8_t en[2] = { 0x01, 0x00 }; /* enable notifications */
        int rc = ble_gattc_write_flat(conn, s_csc_cccd_handle, en, sizeof en,
                                      on_csc_subscribed, NULL);
        if (rc != 0) {
            ESP_LOGW(TAG, "write CCCD rc=%d", rc);
        }
    }
    return 0;
}

static int on_csc_chr(uint16_t conn, const struct ble_gatt_error *err,
                      const struct ble_gatt_chr *chr, void *arg)
{
    (void)arg;
    if (err->status == 0 && chr) {
        s_csc_val_handle = chr->val_handle;
    } else if (err->status == BLE_HS_EDONE) {
        if (s_csc_val_handle != 0) {
            ble_gattc_disc_all_dscs(conn, s_csc_val_handle, s_csc_svc_end,
                                    on_csc_dsc, NULL);
        } else {
            ESP_LOGW(TAG, "CSC Measurement characteristic not found");
        }
    }
    return 0;
}

static int on_csc_svc(uint16_t conn, const struct ble_gatt_error *err,
                      const struct ble_gatt_svc *svc, void *arg)
{
    (void)arg;
    if (err->status == 0 && svc) {
        s_csc_svc_end = svc->end_handle;
        ble_gattc_disc_chrs_by_uuid(conn, svc->start_handle, svc->end_handle,
                                    CSC_MEAS_UUID, on_csc_chr, NULL);
    } else if (err->status == BLE_HS_EDONE && s_csc_svc_end == 0) {
        ESP_LOGW(TAG, "CSC service not found on sensor");
    }
    return 0;
}

static void start_discovery(uint16_t conn)
{
    s_csc_svc_end = 0;
    s_csc_val_handle = 0;
    s_csc_cccd_handle = 0;
    ESP_LOGI(TAG, "discovering CSC service on conn=%u", (unsigned)conn);
    ble_gattc_disc_svc_by_uuid(conn, CSC_SVC_UUID, on_csc_svc, NULL);
}

/* ---------- CSC Measurement parsing ---------- */

/* [flags u8][bit0: cum wheel revs u32 LE, last wheel event time u16 LE
 * (1/1024 s)][bit1: cum crank revs u16 LE, last crank event time u16 LE].
 * Wheel fields always precede crank fields; we never need the crank ones. */
static void csc_notify(const struct os_mbuf *om)
{
    uint8_t buf[7];
    if (OS_MBUF_PKTLEN(om) < 1 ||
        os_mbuf_copydata(om, 0, 1, buf) != 0) {
        return;
    }
    if (!(buf[0] & CSC_FLAG_WHEEL_DATA)) {
        return;   /* cadence-only sensor / mode — nothing for us */
    }
    if (OS_MBUF_PKTLEN(om) < 7 ||
        os_mbuf_copydata(om, 0, 7, buf) != 0) {
        return;
    }
    uint32_t revs = (uint32_t)buf[1] | ((uint32_t)buf[2] << 8) |
                    ((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 24);
    uint16_t evt  = (uint16_t)buf[5] | ((uint16_t)buf[6] << 8);
    int64_t now = esp_timer_get_time();

    portENTER_CRITICAL(&s_mux);
    if (!s_have_baseline) {
        s_have_baseline = true;
        s_prev_revs = revs;
        s_prev_evt_1024 = evt;
        s_rx_us = now;
        portEXIT_CRITICAL(&s_mux);
        return;
    }
    uint32_t drevs  = revs - s_prev_revs;              /* u32 wrap-safe */
    uint16_t dt1024 = (uint16_t)(evt - s_prev_evt_1024); /* u16 wrap-safe */
    s_rx_us = now;
    if (drevs == 0) {
        /* Coasting / standing: counters repeat (or only the flags refresh).
         * rev_age grows → speed_sensor decays the speed to zero. */
        s_prev_evt_1024 = evt;
        portEXIT_CRITICAL(&s_mux);
        return;
    }
    if (drevs > 0x80000000u ||          /* counter went BACKWARDS: sensor
                                         * battery pulled → new epoch */
        drevs > CSC_MAX_DELTA_REVS ||   /* garbage burst */
        dt1024 == 0 ||                  /* rev advanced, time didn't */
        drevs * 1024u > (uint64_t)CSC_MAX_REV_PER_SEC * dt1024) {
        /* Re-baseline without accumulating — one bogus frame must not walk
         * the odometer. */
        s_prev_revs = revs;
        s_prev_evt_1024 = evt;
        portEXIT_CRITICAL(&s_mux);
        return;
    }
    s_total_revs += drevs;
    s_last_delta_revs = (uint16_t)drevs;
    s_last_delta_1024 = dt1024;
    s_prev_revs = revs;
    s_prev_evt_1024 = evt;
    s_rev_us = now;
    portEXIT_CRITICAL(&s_mux);
}

/* ---------- scan-result filtering ---------- */

static bool adv_is_csc_sensor(const struct ble_hs_adv_fields *f)
{
    /* Any advertiser carrying the 16-bit CSC service UUID. No name hint —
     * stock sensor names vary (COOSPO, Magene, XOSS, ...). Note the BK6LS
     * cadence sensor also advertises 0x1816 and thus shows up here too; the
     * user picks their wheel sensor by name, same as the PAS list already
     * shows any Coospo in range. */
    for (int i = 0; i < f->num_uuids16; i++) {
        if (ble_uuid_u16(&f->uuids16[i].u) == CSC_SVC_UUID16) {
            return true;
        }
    }
    return false;
}

/* ---------- GAP events (scan + our central connection) ---------- */

static int speed_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
        struct ble_hs_adv_fields fields;
        if (ble_hs_adv_parse_fields(&fields, event->disc.data,
                                    event->disc.length_data) != 0) {
            return 0;
        }
        if (!adv_is_csc_sensor(&fields)) {
            return 0;
        }
        char name[32] = {0};
        if (fields.name && fields.name_len) {
            size_t n = fields.name_len < sizeof(name) - 1 ? fields.name_len
                                                          : sizeof(name) - 1;
            memcpy(name, fields.name, n);
        }
        ESP_LOGI(TAG, "scan hit %02X:%02X:%02X:%02X:%02X:%02X \"%s\" rssi=%d",
                 event->disc.addr.val[5], event->disc.addr.val[4],
                 event->disc.addr.val[3], event->disc.addr.val[2],
                 event->disc.addr.val[1], event->disc.addr.val[0],
                 name, event->disc.rssi);
        if (s_scan_cb) {
            s_scan_cb(event->disc.addr.val, event->disc.addr.type, name,
                      event->disc.rssi);
        }
        return 0;
    }

    case BLE_GAP_EVENT_DISC_COMPLETE:
        ESP_LOGI(TAG, "scan complete, reason=%d", event->disc_complete.reason);
        portENTER_CRITICAL(&s_mux);
        s_scanning = false;
        portEXIT_CRITICAL(&s_mux);
        arm_connect();             /* record the wish while still suspended */
        ble_arb_scan_resume();     /* ...then let the arbiter reconnect */
        return 0;

#if defined(BLE_GAP_EVENT_LINK_ESTAB)
    case BLE_GAP_EVENT_LINK_ESTAB:
#else
    case BLE_GAP_EVENT_CONNECT:
#endif
        if (event->connect.status == 0) {
            s_conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG, "sensor connected, conn=%u",
                     (unsigned)s_conn_handle);
            ble_gattc_exchange_mtu(s_conn_handle, NULL, NULL);
            start_discovery(s_conn_handle);
        } else {
            ESP_LOGW(TAG, "sensor connect failed status=%d",
                     event->connect.status);
            reset_link_handles();
            arm_connect();         /* retry */
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "sensor disconnect, reason=%d",
                 event->disconnect.reason);
        reset_link_handles();
        arm_connect();             /* re-arm; the sensor will re-advertise */
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        if (event->notify_rx.attr_handle == s_csc_val_handle &&
            s_csc_val_handle != 0) {
            csc_notify(event->notify_rx.om);
        }
        return 0;

    case BLE_GAP_EVENT_MTU:
        return 0;

    default:
        return 0;
    }
}

/* ---------- connection management ---------- */

static void arm_connect(void)
{
    if (!s_inited || !s_synced || !s_bound) return;
    if (s_scanning) return;
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) return; /* already linked */
    /* The arbiter owns the single NimBLE connect-initiator (shared with the
     * cadence client) and issues the actual ble_gap_connect; GAP events
     * still land in speed_gap_event via its trampoline. */
    ble_arb_want_connect(s_arb_id, &s_bound_addr);
}

static void restart_connection(void)
{
    /* Move toward a connection to the (new) bound address, tearing down any
     * existing link / pending attempt; the resulting GAP event re-arms. */
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    } else {
        ble_arb_stop_connect(s_arb_id); /* cancels a stale pending attempt */
        arm_connect();                  /* re-request with the new address */
    }
}

/* ---------- public API ---------- */

void ble_speed_client_init(void)
{
    s_inited = true;
    s_battery = 0xFF;
    s_rx_us = 0;
    s_rev_us = 0;
    s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    s_arb_id = ble_arb_register(speed_gap_event);
}

void ble_speed_on_ble_sync(uint8_t own_addr_type)
{
    s_own_addr_type = own_addr_type;
    s_synced = true;
    if (s_bound) arm_connect();
}

void ble_speed_set_scan_cb(ble_speed_scan_cb_t cb)
{
    s_scan_cb = cb;
}

void ble_speed_scan_start(void)
{
    if (!s_synced || s_scanning) return;
    /* A pending connect uses the scanner — park the arbiter (it cancels the
     * in-flight attempt, whichever sensor owns it); DISC_COMPLETE resumes. */
    ble_arb_scan_suspend();
    struct ble_gap_disc_params dp = { 0 };
    dp.passive = 0;          /* active scan to capture the scan-response name */
    dp.filter_duplicates = 1;
    int rc = ble_gap_disc(s_own_addr_type, SCAN_DURATION_MS, &dp,
                          speed_gap_event, NULL);
    if (rc == 0) {
        portENTER_CRITICAL(&s_mux);
        s_scanning = true;
        portEXIT_CRITICAL(&s_mux);
        ESP_LOGI(TAG, "scanning for CSC sensors (%d ms)", SCAN_DURATION_MS);
    } else {
        ESP_LOGW(TAG, "ble_gap_disc rc=%d", rc);
        arm_connect();
        ble_arb_scan_resume();
    }
}

void ble_speed_scan_stop(void)
{
    if (s_scanning) {
        ble_gap_disc_cancel();
        portENTER_CRITICAL(&s_mux);
        s_scanning = false;
        portEXIT_CRITICAL(&s_mux);
        arm_connect();
        ble_arb_scan_resume();
    }
}

void ble_speed_bind(const uint8_t addr[6], uint8_t addr_type)
{
    portENTER_CRITICAL(&s_mux);
    s_bound = true;
    s_bound_addr.type = addr_type;
    memcpy(s_bound_addr.val, addr, 6);
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "bound sensor %02X:%02X:%02X:%02X:%02X:%02X (type %u)",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],
             (unsigned)addr_type);
    if (s_scanning) {
        ble_gap_disc_cancel();
        s_scanning = false;
        ble_arb_scan_resume();  /* scan owned the initiator — release it */
    }
    if (s_synced) restart_connection();
}

void ble_speed_forget(void)
{
    portENTER_CRITICAL(&s_mux);
    s_bound = false;
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "forget sensor");
    ble_arb_stop_connect(s_arb_id);
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
    reset_link_handles();
}

bool ble_speed_is_bound(void)
{
    return s_bound;
}

void ble_speed_get(ble_speed_state_t *out)
{
    if (!out) return;
    portENTER_CRITICAL(&s_mux);
    out->bound           = s_bound;
    out->connected       = s_connected;
    out->scanning        = s_scanning;
    out->total_revs      = s_total_revs;
    out->last_delta_revs = s_last_delta_revs;
    out->last_delta_1024 = s_last_delta_1024;
    out->battery         = s_battery;
    int64_t rx  = s_rx_us;
    int64_t rev = s_rev_us;
    portEXIT_CRITICAL(&s_mux);
    int64_t now = esp_timer_get_time();
    if (rx == 0) {
        out->age_ms = UINT32_MAX;
    } else {
        int64_t age = (now - rx) / 1000;
        out->age_ms = age < 0 ? 0 : (age > UINT32_MAX ? UINT32_MAX
                                                      : (uint32_t)age);
    }
    if (rev == 0) {
        out->rev_age_ms = UINT32_MAX;
    } else {
        int64_t age = (now - rev) / 1000;
        out->rev_age_ms = age < 0 ? 0 : (age > UINT32_MAX ? UINT32_MAX
                                                          : (uint32_t)age);
    }
}
