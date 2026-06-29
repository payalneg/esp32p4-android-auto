#include "ble_cadence_client.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"

static const char *TAG = "ble_cad";

/* ---- sensor UUIDs (NimBLE native = little-endian byte order, reversed from
 * the human-readable string) ---- */

/* cad00001-eb1c-4f1e-9b2a-6f1c0de0cade — custom RPM service */
static const ble_uuid128_t CAD_SVC_UUID = BLE_UUID128_INIT(
    0xde, 0xca, 0xe0, 0x0d, 0x1c, 0x6f, 0x2a, 0x9b,
    0x1e, 0x4f, 0x1c, 0xeb, 0x01, 0x00, 0xd0, 0xca);
/* cad00002-eb1c-4f1e-9b2a-6f1c0de0cade — Live RPM characteristic (notify) */
static const ble_uuid128_t CAD_RPM_UUID = BLE_UUID128_INIT(
    0xde, 0xca, 0xe0, 0x0d, 0x1c, 0x6f, 0x2a, 0x9b,
    0x1e, 0x4f, 0x1c, 0xeb, 0x02, 0x00, 0xd0, 0xca);
/* Standard: CCCD descriptor, Battery service + Battery Level char */
static const ble_uuid_t *CCCD_UUID  = BLE_UUID16_DECLARE(0x2902);
static const ble_uuid_t *BATT_SVC_UUID = BLE_UUID16_DECLARE(0x180F);
static const ble_uuid_t *BATT_LVL_UUID = BLE_UUID16_DECLARE(0x2A19);
/* CSC service 0x1816 — only used as a scan filter (we read RPM via the custom
 * characteristic, not CSC). */
#define CSC_SVC_UUID16 0x1816

#define SCAN_DURATION_MS 6000
#define SENSOR_NAME_HINT "BK6LS"

/* ---- module state ---- */

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static bool     s_inited;
static uint8_t  s_own_addr_type;
static bool     s_synced;

static bool      s_bound;
static ble_addr_t s_bound_addr;

static bool      s_scanning;
static bool      s_connecting;
static bool      s_connected;          /* link up + subscribed to RPM */
static uint16_t  s_conn_handle = BLE_HS_CONN_HANDLE_NONE;

static uint16_t  s_rpm_svc_end;
static uint16_t  s_rpm_val_handle;
static uint16_t  s_rpm_cccd_handle;

static int16_t   s_centi_rpm;
static int64_t   s_rpm_rx_us;          /* esp_timer_get_time() of last RPM rx; 0 = never */
static uint8_t   s_battery = 0xFF;

static ble_cadence_scan_cb_t s_scan_cb;

static int cadence_gap_event(struct ble_gap_event *event, void *arg);
static void arm_connect(void);

/* ---------- helpers ---------- */

static void reset_link_handles(void)
{
    s_conn_handle    = BLE_HS_CONN_HANDLE_NONE;
    s_rpm_svc_end    = 0;
    s_rpm_val_handle = 0;
    s_rpm_cccd_handle = 0;
    portENTER_CRITICAL(&s_mux);
    s_connected = false;
    portEXIT_CRITICAL(&s_mux);
}

/* ---------- GATT discovery chain (RPM, then best-effort battery) ---------- */

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
     * unknown). Notify-subscription for battery omitted — it changes slowly
     * and a per-connect read is enough for the settings UI. */
    ble_gattc_disc_svc_by_uuid(conn, BATT_SVC_UUID, on_batt_svc, NULL);
}

static int on_rpm_subscribed(uint16_t conn, const struct ble_gatt_error *err,
                             struct ble_gatt_attr *attr, void *arg)
{
    (void)conn; (void)attr; (void)arg;
    if (err->status != 0) {
        ESP_LOGW(TAG, "CCCD write failed status=%d", err->status);
        return 0;
    }
    portENTER_CRITICAL(&s_mux);
    s_connected = true;
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "subscribed to RPM notifications (conn=%u)", (unsigned)conn);
    start_battery_discovery(conn);
    return 0;
}

static int on_rpm_dsc(uint16_t conn, const struct ble_gatt_error *err,
                      uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc,
                      void *arg)
{
    (void)chr_val_handle; (void)arg;
    if (err->status == 0 && dsc &&
        ble_uuid_cmp(&dsc->uuid.u, CCCD_UUID) == 0 && s_rpm_cccd_handle == 0) {
        s_rpm_cccd_handle = dsc->handle;
        static const uint8_t en[2] = { 0x01, 0x00 }; /* enable notifications */
        int rc = ble_gattc_write_flat(conn, s_rpm_cccd_handle, en, sizeof en,
                                      on_rpm_subscribed, NULL);
        if (rc != 0) {
            ESP_LOGW(TAG, "write CCCD rc=%d", rc);
        }
    }
    return 0;
}

static int on_rpm_chr(uint16_t conn, const struct ble_gatt_error *err,
                      const struct ble_gatt_chr *chr, void *arg)
{
    (void)arg;
    if (err->status == 0 && chr) {
        s_rpm_val_handle = chr->val_handle;
    } else if (err->status == BLE_HS_EDONE) {
        if (s_rpm_val_handle != 0) {
            ble_gattc_disc_all_dscs(conn, s_rpm_val_handle, s_rpm_svc_end,
                                    on_rpm_dsc, NULL);
        } else {
            ESP_LOGW(TAG, "RPM characteristic not found");
        }
    }
    return 0;
}

static int on_rpm_svc(uint16_t conn, const struct ble_gatt_error *err,
                      const struct ble_gatt_svc *svc, void *arg)
{
    (void)arg;
    if (err->status == 0 && svc) {
        s_rpm_svc_end = svc->end_handle;
        ble_gattc_disc_chrs_by_uuid(conn, svc->start_handle, svc->end_handle,
                                    &CAD_RPM_UUID.u, on_rpm_chr, NULL);
    } else if (err->status == BLE_HS_EDONE && s_rpm_svc_end == 0) {
        ESP_LOGW(TAG, "RPM service not found on sensor");
    }
    return 0;
}

static void start_discovery(uint16_t conn)
{
    s_rpm_svc_end = 0;
    s_rpm_val_handle = 0;
    s_rpm_cccd_handle = 0;
    ESP_LOGI(TAG, "discovering RPM service on conn=%u", (unsigned)conn);
    ble_gattc_disc_svc_by_uuid(conn, &CAD_SVC_UUID.u, on_rpm_svc, NULL);
}

/* ---------- scan-result filtering ---------- */

static bool adv_is_cadence_sensor(const struct ble_hs_adv_fields *f)
{
    /* Match by complete/short name prefix "BK6LS" OR an advertised 16-bit
     * CSC service UUID (0x1816). */
    if (f->name != NULL && f->name_len >= 5 &&
        memcmp(f->name, SENSOR_NAME_HINT, 5) == 0) {
        return true;
    }
    for (int i = 0; i < f->num_uuids16; i++) {
        if (ble_uuid_u16(&f->uuids16[i].u) == CSC_SVC_UUID16) {
            return true;
        }
    }
    return false;
}

/* ---------- GAP events (scan + our central connection) ---------- */

static int cadence_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
        struct ble_hs_adv_fields fields;
        if (ble_hs_adv_parse_fields(&fields, event->disc.data,
                                    event->disc.length_data) != 0) {
            return 0;
        }
        if (!adv_is_cadence_sensor(&fields)) {
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
        arm_connect();             /* resume connecting to the bound sensor */
        return 0;

#if defined(BLE_GAP_EVENT_LINK_ESTAB)
    case BLE_GAP_EVENT_LINK_ESTAB:
#else
    case BLE_GAP_EVENT_CONNECT:
#endif
        s_connecting = false;
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
        if (event->notify_rx.attr_handle == s_rpm_val_handle &&
            s_rpm_val_handle != 0) {
            uint8_t b[2] = {0, 0};
            if (OS_MBUF_PKTLEN(event->notify_rx.om) >= 2 &&
                os_mbuf_copydata(event->notify_rx.om, 0, 2, b) == 0) {
                int16_t centi = (int16_t)((uint16_t)b[0] |
                                          ((uint16_t)b[1] << 8));
                int64_t now = esp_timer_get_time();
                portENTER_CRITICAL(&s_mux);
                s_centi_rpm = centi;
                s_rpm_rx_us = now;
                portEXIT_CRITICAL(&s_mux);
            }
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
    if (s_scanning || s_connecting) return;
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) return; /* already linked */

    struct ble_gap_conn_params params = { 0 };
    /* Defaults are fine; NimBLE fills sane values when fields are 0. */
    int rc = ble_gap_connect(s_own_addr_type, &s_bound_addr, BLE_HS_FOREVER,
                             NULL, cadence_gap_event, NULL);
    if (rc == 0) {
        s_connecting = true;
        ESP_LOGI(TAG, "connecting to bound sensor "
                 "%02X:%02X:%02X:%02X:%02X:%02X (type %u)",
                 s_bound_addr.val[5], s_bound_addr.val[4], s_bound_addr.val[3],
                 s_bound_addr.val[2], s_bound_addr.val[1], s_bound_addr.val[0],
                 (unsigned)s_bound_addr.type);
    } else if (rc == BLE_HS_EALREADY || rc == BLE_HS_EBUSY) {
        s_connecting = true; /* an attempt is already outstanding */
    } else {
        ESP_LOGW(TAG, "ble_gap_connect rc=%d", rc);
    }
    (void)params;
}

static void restart_connection(void)
{
    /* Move toward a connection to the (new) bound address, tearing down any
     * existing link / pending attempt; the resulting GAP event re-arms. */
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    } else if (s_connecting) {
        ble_gap_conn_cancel();
        s_connecting = false;
        arm_connect();
    } else {
        arm_connect();
    }
}

/* ---------- public API ---------- */

void ble_cadence_client_init(void)
{
    s_inited = true;
    s_battery = 0xFF;
    s_rpm_rx_us = 0;
    s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
}

void ble_cadence_on_ble_sync(uint8_t own_addr_type)
{
    s_own_addr_type = own_addr_type;
    s_synced = true;
    if (s_bound) arm_connect();
}

void ble_cadence_set_scan_cb(ble_cadence_scan_cb_t cb)
{
    s_scan_cb = cb;
}

void ble_cadence_scan_start(void)
{
    if (!s_synced || s_scanning) return;
    /* A pending connect uses the scanner — cancel it so the selection scan can
     * run; DISC_COMPLETE re-arms the connect afterwards. */
    if (s_connecting) {
        ble_gap_conn_cancel();
        s_connecting = false;
    }
    struct ble_gap_disc_params dp = { 0 };
    dp.passive = 0;          /* active scan to capture the scan-response name */
    dp.filter_duplicates = 1;
    int rc = ble_gap_disc(s_own_addr_type, SCAN_DURATION_MS, &dp,
                          cadence_gap_event, NULL);
    if (rc == 0) {
        portENTER_CRITICAL(&s_mux);
        s_scanning = true;
        portEXIT_CRITICAL(&s_mux);
        ESP_LOGI(TAG, "scanning for cadence sensors (%d ms)", SCAN_DURATION_MS);
    } else {
        ESP_LOGW(TAG, "ble_gap_disc rc=%d", rc);
        arm_connect();
    }
}

void ble_cadence_scan_stop(void)
{
    if (s_scanning) {
        ble_gap_disc_cancel();
        portENTER_CRITICAL(&s_mux);
        s_scanning = false;
        portEXIT_CRITICAL(&s_mux);
        arm_connect();
    }
}

void ble_cadence_bind(const uint8_t addr[6], uint8_t addr_type)
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
    }
    if (s_synced) restart_connection();
}

void ble_cadence_forget(void)
{
    portENTER_CRITICAL(&s_mux);
    s_bound = false;
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "forget sensor");
    if (s_connecting) {
        ble_gap_conn_cancel();
        s_connecting = false;
    }
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
    reset_link_handles();
}

bool ble_cadence_is_bound(void)
{
    return s_bound;
}

void ble_cadence_get(ble_cadence_state_t *out)
{
    if (!out) return;
    portENTER_CRITICAL(&s_mux);
    out->bound     = s_bound;
    out->connected = s_connected;
    out->scanning  = s_scanning;
    out->centi_rpm = s_centi_rpm;
    out->battery   = s_battery;
    int64_t rx = s_rpm_rx_us;
    portEXIT_CRITICAL(&s_mux);
    if (rx == 0) {
        out->age_ms = UINT32_MAX;
    } else {
        int64_t age = (esp_timer_get_time() - rx) / 1000;
        out->age_ms = age < 0 ? 0 : (age > UINT32_MAX ? UINT32_MAX
                                                       : (uint32_t)age);
    }
}
