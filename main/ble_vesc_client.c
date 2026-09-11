/*
    Copyright 2026 ESP32-P4 Android Auto / VESC Display (GPL-3.0).

    See ble_vesc_client.h. Structure follows ble_speed_client.c — same arbiter
    slot, same discovery cascade, same edge-driven reconnect — with three
    differences that come from this being a data link rather than a sensor:

      * both characteristic handles matter (TX to subscribe to, RX to write to),
        so discovery walks all characteristics of the service instead of asking
        for one by UUID;
      * the connection interval is renegotiated on link-up, because every
        request/reply costs two of them and that sets the dashboard's ceiling;
      * there is a write path, with the same mbuf-exhaustion backoff the
        phone-facing NUS notify path uses.
*/

#include "ble_vesc_client.h"

#include "ble_central_arb.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "host/ble_att.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"

static const char *TAG = "ble_vesc";

/* NimBLE stores UUID-128 little-endian, so 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
 * reverses to the bytes below; only byte 12 differs between service / RX / TX.
 * Same three UUIDs this firmware serves to the phone (see ble_nus.c) — a VESC
 * Express exposes exactly the same service. */
static const ble_uuid128_t NUS_SVC_UUID = BLE_UUID128_INIT(
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E);
static const ble_uuid128_t NUS_RX_UUID = BLE_UUID128_INIT(
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E);
static const ble_uuid128_t NUS_TX_UUID = BLE_UUID128_INIT(
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E);

static const ble_uuid_t *CCCD_UUID = BLE_UUID16_DECLARE(0x2902);

#define SCAN_DURATION_MS 6000

/* A VESC Express advertises the NUS UUID, but so does this very firmware and
 * so do other NUS gadgets, so the list also accepts anything whose name starts
 * with "VESC" — that is what the stock adapter calls itself. The user picks by
 * name either way. */
#define VESC_NAME_HINT "VESC"

/* Connection-interval units are 1.25 ms; supervision-timeout units are 10 ms. */
#define CONN_ITVL_UNITS ((CONFIG_VESC_BLE_CONN_ITVL_MS * 4) / 5)
#define CONN_SUPERVISION_UNITS 400   /* 4 s */

/* Largest write we ever hand to one GATT operation. The adapter's NUS RX
 * attribute is 255 bytes (GATTS_CHAR_VAL_LEN_MAX in vesc_express), and some
 * adapters advertise a big MTU while keeping a small RX attribute — the
 * companion app clamps to the same 244 for the same reason. */
#define WRITE_CHUNK_MAX 244

/* ---- module state ---- */

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static bool     s_inited;
static uint8_t  s_own_addr_type;
static bool     s_synced;

static bool       s_bound;
static ble_addr_t s_bound_addr;
static bool       s_parked;            /* bound, but deliberately not connected */

static bool      s_scanning;
static bool      s_connected;
static bool      s_ready;              /* link up + subscribed to TX */
static uint16_t  s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static int       s_arb_id = -1;
static uint32_t  s_reconnects;

static uint16_t  s_svc_end;
static uint16_t  s_rx_val_handle;      /* write here */
static uint16_t  s_tx_val_handle;      /* notifications arrive from here */
static uint16_t  s_tx_cccd_handle;
static bool      s_rx_no_rsp;          /* RX supports write-without-response */

static ble_vesc_scan_cb_t s_scan_cb;
static ble_vesc_rx_cb_t   s_rx_cb;

static int vesc_gap_event(struct ble_gap_event *event, void *arg);
static void arm_connect(void);

/* ---------- helpers ---------- */

static void reset_link_handles(void)
{
    s_conn_handle    = BLE_HS_CONN_HANDLE_NONE;
    s_svc_end        = 0;
    s_rx_val_handle  = 0;
    s_tx_val_handle  = 0;
    s_tx_cccd_handle = 0;
    s_rx_no_rsp      = false;
    portENTER_CRITICAL(&s_mux);
    s_connected = false;
    s_ready     = false;
    portEXIT_CRITICAL(&s_mux);
}

/* ---------- GATT discovery chain ---------- */

static int on_subscribed(uint16_t conn, const struct ble_gatt_error *err,
                         struct ble_gatt_attr *attr, void *arg)
{
    (void)attr; (void)arg;
    if (err->status != 0) {
        ESP_LOGW(TAG, "CCCD write failed status=%d", err->status);
        return 0;
    }
    portENTER_CRITICAL(&s_mux);
    s_ready = true;
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "adapter ready (conn=%u, mtu=%u, write%s response)",
             (unsigned)conn, (unsigned)ble_att_mtu(conn),
             s_rx_no_rsp ? " without" : " with");
    return 0;
}

static int on_dsc(uint16_t conn, const struct ble_gatt_error *err,
                  uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc,
                  void *arg)
{
    (void)chr_val_handle; (void)arg;
    if (err->status == 0 && dsc &&
        ble_uuid_cmp(&dsc->uuid.u, CCCD_UUID) == 0 && s_tx_cccd_handle == 0) {
        s_tx_cccd_handle = dsc->handle;
        static const uint8_t en[2] = { 0x01, 0x00 };   /* enable notifications */
        int rc = ble_gattc_write_flat(conn, s_tx_cccd_handle, en, sizeof en,
                                      on_subscribed, NULL);
        if (rc != 0) {
            ESP_LOGW(TAG, "write CCCD rc=%d", rc);
        }
    }
    return 0;
}

static int on_chr(uint16_t conn, const struct ble_gatt_error *err,
                  const struct ble_gatt_chr *chr, void *arg)
{
    (void)arg;
    if (err->status == 0 && chr) {
        if (ble_uuid_cmp(&chr->uuid.u, &NUS_RX_UUID.u) == 0) {
            s_rx_val_handle = chr->val_handle;
            s_rx_no_rsp = (chr->properties & BLE_GATT_CHR_PROP_WRITE_NO_RSP) != 0;
        } else if (ble_uuid_cmp(&chr->uuid.u, &NUS_TX_UUID.u) == 0) {
            s_tx_val_handle = chr->val_handle;
        }
    } else if (err->status == BLE_HS_EDONE) {
        if (s_tx_val_handle != 0 && s_rx_val_handle != 0) {
            /* Descriptors of the TX characteristic only — its CCCD is the one
             * that turns notifications on. */
            ble_gattc_disc_all_dscs(conn, s_tx_val_handle, s_svc_end,
                                    on_dsc, NULL);
        } else {
            ESP_LOGW(TAG, "NUS characteristics missing (rx=%u tx=%u) — "
                          "not a VESC adapter?",
                     (unsigned)s_rx_val_handle, (unsigned)s_tx_val_handle);
        }
    }
    return 0;
}

static int on_svc(uint16_t conn, const struct ble_gatt_error *err,
                  const struct ble_gatt_svc *svc, void *arg)
{
    (void)arg;
    if (err->status == 0 && svc) {
        s_svc_end = svc->end_handle;
        /* Both characteristics are needed, and their properties decide the
         * write type — so walk the service instead of asking by UUID twice. */
        ble_gattc_disc_all_chrs(conn, svc->start_handle, svc->end_handle,
                                on_chr, NULL);
    } else if (err->status == BLE_HS_EDONE && s_svc_end == 0) {
        ESP_LOGW(TAG, "no Nordic UART Service on this device");
    }
    return 0;
}

static void start_discovery(uint16_t conn)
{
    s_svc_end = 0;
    s_rx_val_handle = 0;
    s_tx_val_handle = 0;
    s_tx_cccd_handle = 0;
    ESP_LOGI(TAG, "discovering NUS on conn=%u", (unsigned)conn);
    ble_gattc_disc_svc_by_uuid(conn, &NUS_SVC_UUID.u, on_svc, NULL);
}

/* Ask for a short connection interval. Every VESC request costs two of them
 * (one for the write, one for the reply), so this is the single biggest lever
 * on how fast the dashboard can be polled over BLE. Best-effort: the adapter
 * may refuse, in which case we just poll slower. */
static void request_fast_link(uint16_t conn)
{
    struct ble_gap_upd_params p = {
        .itvl_min            = CONN_ITVL_UNITS,
        .itvl_max            = CONN_ITVL_UNITS,
        .latency             = 0,
        .supervision_timeout = CONN_SUPERVISION_UNITS,
    };
    int rc = ble_gap_update_params(conn, &p);
    if (rc != 0) {
        ESP_LOGW(TAG, "conn param update rc=%d (keeping defaults)", rc);
    }
}

/* ---------- scan-result filtering ---------- */

static bool adv_is_vesc_adapter(const struct ble_hs_adv_fields *f,
                                const char *name)
{
    for (int i = 0; i < f->num_uuids128; i++) {
        if (ble_uuid_cmp(&f->uuids128[i].u, &NUS_SVC_UUID.u) == 0) {
            return true;
        }
    }
    /* Not every adapter puts the service UUID in the advertisement. */
    return strncmp(name, VESC_NAME_HINT, strlen(VESC_NAME_HINT)) == 0;
}

/* ---------- GAP events (scan + our central connection) ---------- */

static int vesc_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
        struct ble_hs_adv_fields fields;
        if (ble_hs_adv_parse_fields(&fields, event->disc.data,
                                    event->disc.length_data) != 0) {
            return 0;
        }
        char name[32] = {0};
        if (fields.name && fields.name_len) {
            size_t n = fields.name_len < sizeof(name) - 1 ? fields.name_len
                                                          : sizeof(name) - 1;
            memcpy(name, fields.name, n);
        }
        if (!adv_is_vesc_adapter(&fields, name)) {
            return 0;
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
            portENTER_CRITICAL(&s_mux);
            s_connected = true;
            portEXIT_CRITICAL(&s_mux);
            ESP_LOGI(TAG, "adapter connected, conn=%u",
                     (unsigned)s_conn_handle);
            ble_gattc_exchange_mtu(s_conn_handle, NULL, NULL);
            request_fast_link(s_conn_handle);
            start_discovery(s_conn_handle);
        } else {
            ESP_LOGW(TAG, "adapter connect failed status=%d",
                     event->connect.status);
            reset_link_handles();
            arm_connect();         /* retry */
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGW(TAG, "adapter disconnect, reason=%d",
                 event->disconnect.reason);
        portENTER_CRITICAL(&s_mux);
        s_reconnects++;
        portEXIT_CRITICAL(&s_mux);
        reset_link_handles();
        arm_connect();             /* re-arm; the adapter re-advertises */
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        if (event->notify_rx.attr_handle == s_tx_val_handle &&
            s_tx_val_handle != 0 && s_rx_cb) {
            /* Copy out and hand over immediately — the consumer pushes the
             * bytes onto its own task; no parsing happens here.
             *
             * Static, not on the stack: one notification can be MTU-3 = 509 B
             * at the 512 B MTU we ask for, which is an eighth of the whole
             * NimBLE host stack. Only this task ever runs this code, and it
             * runs to completion, so one buffer is enough. Truncating instead
             * would desynchronise the packet parser, not just lose a reply. */
            static uint8_t s_notify_buf[BLE_ATT_MTU_MAX];
            uint16_t len = OS_MBUF_PKTLEN(event->notify_rx.om);
            if (len > sizeof(s_notify_buf)) {
                ESP_LOGW(TAG, "notification of %u B exceeds the buffer",
                         (unsigned)len);
                return 0;
            }
            if (os_mbuf_copydata(event->notify_rx.om, 0, len, s_notify_buf) == 0) {
                s_rx_cb(s_notify_buf, len);
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
    if (!s_inited || !s_synced || !s_bound || s_parked) return;
    if (s_scanning) return;
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) return;  /* already linked */
    ble_arb_want_connect(s_arb_id, &s_bound_addr);
}

static void restart_connection(void)
{
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    } else {
        ble_arb_stop_connect(s_arb_id);
        arm_connect();
    }
}

/* ---------- public API ---------- */

void ble_vesc_client_init(void)
{
    s_inited = true;
    s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    s_arb_id = ble_arb_register(vesc_gap_event);
    if (s_arb_id < 0) {
        ESP_LOGE(TAG, "no free central-arbiter slot — BLE link unavailable");
    }
}

void ble_vesc_on_ble_sync(uint8_t own_addr_type)
{
    s_own_addr_type = own_addr_type;
    s_synced = true;
    if (s_bound) arm_connect();
}

void ble_vesc_set_scan_cb(ble_vesc_scan_cb_t cb) { s_scan_cb = cb; }
void ble_vesc_set_rx_cb(ble_vesc_rx_cb_t cb)     { s_rx_cb = cb; }

void ble_vesc_scan_start(void)
{
    if (!s_synced || s_scanning) return;
    ble_arb_scan_suspend();
    struct ble_gap_disc_params dp = { 0 };
    dp.passive = 0;          /* active scan to capture the scan-response name */
    dp.filter_duplicates = 1;
    int rc = ble_gap_disc(s_own_addr_type, SCAN_DURATION_MS, &dp,
                          vesc_gap_event, NULL);
    if (rc == 0) {
        portENTER_CRITICAL(&s_mux);
        s_scanning = true;
        portEXIT_CRITICAL(&s_mux);
        ESP_LOGI(TAG, "scanning for VESC adapters (%d ms)", SCAN_DURATION_MS);
    } else {
        ESP_LOGW(TAG, "ble_gap_disc rc=%d", rc);
        arm_connect();
        ble_arb_scan_resume();
    }
}

void ble_vesc_scan_stop(void)
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

void ble_vesc_bind(const uint8_t addr[6], uint8_t addr_type)
{
    portENTER_CRITICAL(&s_mux);
    s_bound = true;
    s_parked = false;
    s_bound_addr.type = addr_type;
    memcpy(s_bound_addr.val, addr, 6);
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "bound adapter %02X:%02X:%02X:%02X:%02X:%02X (type %u)",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],
             (unsigned)addr_type);
    if (s_scanning) {
        ble_gap_disc_cancel();
        s_scanning = false;
        ble_arb_scan_resume();  /* scan owned the initiator — release it */
    }
    if (s_synced) restart_connection();
}

void ble_vesc_forget(void)
{
    portENTER_CRITICAL(&s_mux);
    s_bound = false;
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "forget adapter");
    ble_arb_stop_connect(s_arb_id);
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
    reset_link_handles();
}

void ble_vesc_disconnect(void)
{
    portENTER_CRITICAL(&s_mux);
    s_parked = true;
    portEXIT_CRITICAL(&s_mux);
    ble_arb_stop_connect(s_arb_id);
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
    reset_link_handles();
}

void ble_vesc_reconnect(void)
{
    portENTER_CRITICAL(&s_mux);
    s_parked = false;
    portEXIT_CRITICAL(&s_mux);
    arm_connect();
}

bool ble_vesc_is_ready(void)
{
    return s_ready;
}

int ble_vesc_write(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0) return 0;
    if (!s_ready || s_conn_handle == BLE_HS_CONN_HANDLE_NONE ||
        s_rx_val_handle == 0) {
        return BLE_HS_ENOTCONN;
    }

    uint16_t conn = s_conn_handle;
    uint16_t mtu  = ble_att_mtu(conn);
    uint16_t chunk = (mtu > 3) ? (uint16_t)(mtu - 3) : 20;
    if (chunk > WRITE_CHUNK_MAX) chunk = WRITE_CHUNK_MAX;
    if (chunk < 20) chunk = 20;

    /* Same backpressure shape as the phone-facing notify path (ble_nus.c):
     * the only signal a write-without-response gives us is mbuf exhaustion, so
     * wait a tick and try again. pdMS_TO_TICKS(5) is 0 at FREERTOS_HZ=100 —
     * hence one tick, spelled out. */
    const TickType_t drain_wait   = 1;
    const int        max_attempts = 250 / portTICK_PERIOD_MS;

    uint16_t off = 0;
    while (off < len) {
        uint16_t n = (uint16_t)(len - off);
        if (n > chunk) n = chunk;

        int rc = 0;
        for (int attempt = 0; ; attempt++) {
            rc = s_rx_no_rsp
                     ? ble_gattc_write_no_rsp_flat(conn, s_rx_val_handle,
                                                   data + off, n)
                     : ble_gattc_write_flat(conn, s_rx_val_handle,
                                            data + off, n, NULL, NULL);
            if (rc == 0) break;
            if ((rc != BLE_HS_ENOMEM && rc != BLE_HS_EBUSY) ||
                attempt >= max_attempts) {
                ESP_LOGW(TAG, "write rc=%d (offset %u/%u)", rc,
                         (unsigned)off, (unsigned)len);
                return rc;
            }
            vTaskDelay(drain_wait);
            if (!s_ready) return BLE_HS_ENOTCONN;   /* link died mid-packet */
        }
        off = (uint16_t)(off + n);
    }
    return 0;
}

void ble_vesc_get(ble_vesc_state_t *out)
{
    if (!out) return;
    portENTER_CRITICAL(&s_mux);
    out->bound      = s_bound;
    out->connected  = s_connected;
    out->ready      = s_ready;
    out->scanning   = s_scanning;
    out->reconnects = s_reconnects;
    portEXIT_CRITICAL(&s_mux);
    out->mtu = (s_conn_handle != BLE_HS_CONN_HANDLE_NONE)
                   ? ble_att_mtu(s_conn_handle) : 0;
}
