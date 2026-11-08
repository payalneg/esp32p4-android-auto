#include "ble_bms_client.h"

#include "ble_central_arb.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"

static const char *TAG = "ble_bms";

/* ---- JK-BMS GATT ---- */
static const ble_uuid_t *BMS_SVC_UUID  = BLE_UUID16_DECLARE(0xFFE0);
static const ble_uuid_t *CCCD_UUID     = BLE_UUID16_DECLARE(0x2902);

#define BMS_SVC_UUID16 0xFFE0

/* ---- protocol ---- */
#define FRAME_SIZE          300
#define REQ_SIZE            20
#define BMS_BUF_SIZE        512
#define CMD_CELL_INFO       0x96
#define CMD_DEVICE_INFO     0x97
#define TYPE_SETTINGS       0x01
#define TYPE_CELL_INFO      0x02
#define TYPE_DEVICE_INFO    0x03

#define SCAN_DURATION_MS    6000
#define STATUS_POLL_MS      2000    /* request a status frame this often */
#define DEVICE_POLL_MS      60000   /* model/firmware, rarely changes */
#define FRESH_WINDOW_MS     10000

/* ---- module state ---- */

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static bool     s_inited;
static uint8_t  s_own_addr_type;
static bool     s_synced;

static bool       s_bound;
static ble_addr_t s_bound_addr;

static bool      s_scanning;
static bool      s_connected;          /* link up + subscribed */
static uint16_t  s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static int       s_arb_id = -1;

static uint16_t  s_svc_end;
static uint16_t  s_notify_val;
static uint16_t  s_write_val;
static bool      s_write_no_rsp;
static uint16_t  s_cccd_handle;

static int64_t   s_last_frame_us;      /* last valid cell-info frame */
static int64_t   s_last_dev_us;        /* last valid device-info frame */
static uint32_t  s_frames;
static bool      s_has_data;

/* Decoded snapshot, protected by s_mux. NAN = field not present. */
static float    s_soc;
static float    s_rem_ah;
static float    s_full_ah;
static float    s_voltage;
static float    s_current;
static float    s_temp1, s_temp2, s_temp_mos;
static float    s_cell_min, s_cell_max, s_cell_delta;
static int      s_cell_min_i, s_cell_max_i, s_cell_count;
static float    s_cells[BLE_BMS_MAX_CELLS];
static uint32_t s_cycles;
static float    s_soh;
static bool     s_chg_mos, s_dis_mos, s_balancing, s_heating;
static char     s_model[sizeof(((ble_bms_state_t *)0)->model)];

/* RX reassembly buffer — only touched on the NimBLE host task. */
static uint8_t  s_rx[BMS_BUF_SIZE];
static size_t   s_rx_len;

static ble_bms_scan_cb_t s_scan_cb;

static int  bms_gap_event(struct ble_gap_event *event, void *arg);
static void arm_connect(void);
static void request_frame(uint8_t cmd);

/* ---- little-endian helpers ---- */

static uint16_t rd_u16(const uint8_t *p, int off)
{
    return (uint16_t)(p[off] | ((uint16_t)p[off + 1] << 8));
}

static int16_t rd_i16(const uint8_t *p, int off)
{
    return (int16_t)rd_u16(p, off);
}

static uint32_t rd_u32(const uint8_t *p, int off)
{
    return (uint32_t)p[off] | ((uint32_t)p[off + 1] << 8) |
           ((uint32_t)p[off + 2] << 16) | ((uint32_t)p[off + 3] << 24);
}

static int32_t rd_i32(const uint8_t *p, int off)
{
    return (int32_t)rd_u32(p, off);
}

static float plaus(float v, float lo, float hi)
{
    return (v >= lo && v <= hi) ? v : NAN;
}

/* ---- frame parsing ---- */

static void parse_cell_info(const uint8_t *d, int len)
{
    if (len != FRAME_SIZE || d[0] != 0x55 || d[1] != 0xAA ||
        d[2] != 0xEB || d[3] != 0x90 || d[4] != TYPE_CELL_INFO) {
        return;
    }

    /* 32S variant: bytes 60..61 hold cell 28's voltage; on 24S that is the
     * (small) delta cell voltage. */
    bool is32 = rd_u16(d, 60) > 1000;
    int off = is32 ? 16 : 0;      /* byte offset from the 24S layout */
    int cells = is32 ? 32 : 24;
    int o = off * 2;

    float cells_v[BLE_BMS_MAX_CELLS];
    float vmin = 100.0f, vmax = -100.0f;
    int imin = 0, imax = 0, n = 0;
    for (int i = 0; i < cells; i++) {
        float v = rd_u16(d, 6 + i * 2) / 1000.0f;
        cells_v[i] = v;
        if (v > 0.05f) {
            n++;
            if (v < vmin) { vmin = v; imin = i + 1; }
            if (v > vmax) { vmax = v; imax = i + 1; }
        }
    }
    if (n == 0) return;

    float voltage = rd_u32(d, 118 + o) / 1000.0f;
    float current = rd_i32(d, 126 + o) / 1000.0f;
    float t1 = rd_i16(d, 130 + o) / 10.0f;
    float t2 = rd_i16(d, 132 + o) / 10.0f;
    float tmos = (is32 ? rd_i16(d, 112 + o) : rd_i16(d, 134 + o)) / 10.0f;
    float soc = d[141 + o];
    float rem = rd_u32(d, 142 + o) / 1000.0f;
    float full = rd_u32(d, 146 + o) / 1000.0f;
    uint32_t cycles = rd_u32(d, 150 + o);
    float soh = d[158 + o];

    if (voltage < 1.0f || voltage > 200.0f) return;
    if (full <= 0.0f || full > 20000.0f) return;

    portENTER_CRITICAL(&s_mux);
    s_soc        = (soc <= 100.0f) ? soc : NAN;
    s_rem_ah     = rem;
    s_full_ah    = full;
    s_voltage    = voltage;
    s_current    = current;
    s_temp1      = plaus(t1, -60.0f, 150.0f);
    s_temp2      = plaus(t2, -60.0f, 150.0f);
    s_temp_mos   = plaus(tmos, -60.0f, 150.0f);
    s_cell_min   = vmin;
    s_cell_max   = vmax;
    s_cell_delta = vmax - vmin;
    s_cell_min_i = imin;
    s_cell_max_i = imax;
    s_cell_count = n;
    memcpy(s_cells, cells_v, sizeof(float) * (size_t)cells);
    s_cycles     = cycles;
    s_soh        = (soh > 0 && soh <= 100) ? soh : NAN;
    s_balancing  = d[140 + o] != 0;
    s_chg_mos    = d[166 + o] != 0;
    s_dis_mos    = d[167 + o] != 0;
    s_heating    = d[183 + o] != 0;
    s_frames++;
    s_has_data   = true;
    s_last_frame_us = esp_timer_get_time();
    portEXIT_CRITICAL(&s_mux);

    ESP_LOGD(TAG, "status: %.0f%% %.2f/%.2f Ah %.2f V %.2f A (%s)",
             (double)soc, (double)rem, (double)full, (double)voltage,
             (double)current, is32 ? "32S" : "24S");
}

static void parse_device_info(const uint8_t *d, int len)
{
    if (len != FRAME_SIZE || d[0] != 0x55 || d[4] != TYPE_DEVICE_INFO) return;

    char model[sizeof(s_model)];
    size_t n = 0;
    while (n < sizeof(model) - 1 && d[6 + n] != 0) {
        char c = (char)d[6 + n];
        model[n] = (c >= 32 && c < 127) ? c : '?';
        n++;
    }
    model[n] = '\0';
    portENTER_CRITICAL(&s_mux);
    memcpy(s_model, model, sizeof(s_model));
    s_last_dev_us = esp_timer_get_time();
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "BMS model: %s", model);
}

static bool frame_checksum_ok(const uint8_t *d)
{
    uint32_t sum = 0;
    for (int i = 0; i < FRAME_SIZE - 1; i++) sum += d[i];
    return (uint8_t)(sum & 0xFF) == d[FRAME_SIZE - 1];
}

/* Reassemble the 300-byte replies from BLE notification chunks. */
static void assemble_and_parse(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;
    if (s_rx_len + len > sizeof(s_rx)) {
        ESP_LOGW(TAG, "RX overflow — dropping buffer");
        s_rx_len = 0;
    }
    memcpy(s_rx + s_rx_len, data, len);
    s_rx_len += len;

    for (;;) {
        size_t start = 0;
        while (start + 4 <= s_rx_len &&
               !(s_rx[start] == 0x55 && s_rx[start + 1] == 0xAA &&
                 s_rx[start + 2] == 0xEB && s_rx[start + 3] == 0x90)) {
            start++;
        }
        if (start > 0) {
            memmove(s_rx, s_rx + start, s_rx_len - start);
            s_rx_len -= start;
        }
        if (s_rx_len < 4) return;
        if (s_rx_len < FRAME_SIZE) return;

        if (!frame_checksum_ok(s_rx)) {
            /* Lost a chunk or caught a partial frame: resync on the next
             * header rather than emitting garbage. */
            size_t next = 1;
            while (next + 4 <= s_rx_len &&
                   !(s_rx[next] == 0x55 && s_rx[next + 1] == 0xAA &&
                     s_rx[next + 2] == 0xEB && s_rx[next + 3] == 0x90)) {
                next++;
            }
            if (next + 4 > s_rx_len) {
                s_rx_len = 0;
                return;
            }
            memmove(s_rx, s_rx + next, s_rx_len - next);
            s_rx_len -= next;
            continue;
        }

        switch (s_rx[4]) {
        case TYPE_CELL_INFO:    parse_cell_info(s_rx, FRAME_SIZE); break;
        case TYPE_DEVICE_INFO:  parse_device_info(s_rx, FRAME_SIZE); break;
        default: break;
        }
        memmove(s_rx, s_rx + FRAME_SIZE, s_rx_len - FRAME_SIZE);
        s_rx_len -= FRAME_SIZE;
    }
}

/* ---- request TX ---- */

static void request_frame(uint8_t cmd)
{
    uint16_t conn = s_conn_handle;
    uint16_t handle = s_write_val;
    if (!s_connected || conn == BLE_HS_CONN_HANDLE_NONE || handle == 0) return;

    uint8_t req[REQ_SIZE] = { 0 };
    req[0] = 0xAA;
    req[1] = 0x55;
    req[2] = 0x90;
    req[3] = 0xEB;
    req[4] = cmd;
    uint32_t sum = 0;
    for (int i = 0; i < REQ_SIZE - 1; i++) sum += req[i];
    req[REQ_SIZE - 1] = (uint8_t)(sum & 0xFF);

    int rc = s_write_no_rsp
                 ? ble_gattc_write_no_rsp_flat(conn, handle, req, sizeof(req))
                 : ble_gattc_write_flat(conn, handle, req, sizeof(req),
                                        NULL, NULL);
    if (rc != 0 && rc != BLE_HS_EBUSY && rc != BLE_HS_ENOMEM) {
        ESP_LOGW(TAG, "request 0x%02X write rc=%d", cmd, rc);
    }
}

/* Periodic request pump. The BMS mostly streams after the first request, but
 * an idle/sleeping unit stops; a 2 s poll makes recovery automatic. */
static void poll_task(void *arg)
{
    (void)arg;
    int64_t last_status = 0;
    int64_t last_device = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
        if (!s_connected) continue;
        int64_t now = esp_timer_get_time();
        if (now - last_status >= (int64_t)STATUS_POLL_MS * 1000) {
            last_status = now;
            request_frame(CMD_CELL_INFO);
        }
        if (now - last_device >= (int64_t)DEVICE_POLL_MS * 1000) {
            last_device = now;
            request_frame(CMD_DEVICE_INFO);
        }
    }
}

/* ---- GATT discovery ---- */

static int on_subscribed(uint16_t conn, const struct ble_gatt_error *err,
                         struct ble_gatt_attr *attr, void *arg)
{
    (void)conn; (void)attr; (void)arg;
    if (err->status != 0) {
        ESP_LOGW(TAG, "CCCD write failed status=%d", err->status);
        return 0;
    }
    portENTER_CRITICAL(&s_mux);
    s_connected = true;
    s_rx_len = 0;
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "subscribed to JK-BMS notifications (conn=%u)",
             (unsigned)conn);
    request_frame(CMD_DEVICE_INFO);
    request_frame(CMD_CELL_INFO);
    return 0;
}

static int on_dsc(uint16_t conn, const struct ble_gatt_error *err,
                  uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc,
                  void *arg)
{
    (void)chr_val_handle; (void)arg;
    if (err->status == 0 && dsc && s_cccd_handle == 0 &&
        ble_uuid_cmp(&dsc->uuid.u, CCCD_UUID) == 0) {
        s_cccd_handle = dsc->handle;
        static const uint8_t en[2] = { 0x01, 0x00 };
        int rc = ble_gattc_write_flat(conn, s_cccd_handle, en, sizeof(en),
                                      on_subscribed, NULL);
        if (rc != 0) ESP_LOGW(TAG, "write CCCD rc=%d", rc);
    }
    return 0;
}

static void start_descriptor_discovery(uint16_t conn)
{
    if (s_notify_val == 0) {
        ESP_LOGW(TAG, "JK-BMS notify characteristic not found");
        return;
    }
    s_cccd_handle = 0;
    ble_gattc_disc_all_dscs(conn, s_notify_val, s_svc_end, on_dsc, NULL);
}

static int on_chr(uint16_t conn, const struct ble_gatt_error *err,
                  const struct ble_gatt_chr *chr, void *arg)
{
    (void)arg;
    if (err->status == 0 && chr) {
        if ((chr->properties & (BLE_GATT_CHR_PROP_NOTIFY |
                                BLE_GATT_CHR_PROP_INDICATE)) && s_notify_val == 0) {
            s_notify_val = chr->val_handle;
            ESP_LOGI(TAG, "notify handle 0x%04X (props 0x%02X)",
                     (unsigned)s_notify_val, (unsigned)chr->properties);
        }
        if ((chr->properties & (BLE_GATT_CHR_PROP_WRITE |
                                BLE_GATT_CHR_PROP_WRITE_NO_RSP)) && s_write_val == 0) {
            s_write_val = chr->val_handle;
            s_write_no_rsp = (chr->properties & BLE_GATT_CHR_PROP_WRITE) == 0;
            ESP_LOGI(TAG, "write handle 0x%04X (%s)",
                     (unsigned)s_write_val,
                     s_write_no_rsp ? "no response" : "with response");
        }
    } else if (err->status == BLE_HS_EDONE) {
        if (s_write_val == 0 && s_notify_val != 0) {
            /* Single bidirectional characteristic. */
            s_write_val = s_notify_val;
            s_write_no_rsp = false;
        }
        start_descriptor_discovery(conn);
    }
    return 0;
}

static int on_svc(uint16_t conn, const struct ble_gatt_error *err,
                  const struct ble_gatt_svc *svc, void *arg)
{
    (void)arg;
    if (err->status == 0 && svc) {
        s_svc_end = svc->end_handle;
        ESP_LOGI(TAG, "JK-BMS service 0xFFE0 found (0x%04X..0x%04X)",
                 (unsigned)svc->start_handle, (unsigned)svc->end_handle);
        ble_gattc_disc_all_chrs(conn, svc->start_handle, svc->end_handle,
                                on_chr, NULL);
    } else if (err->status == BLE_HS_EDONE && s_svc_end == 0) {
        ESP_LOGW(TAG, "JK-BMS service 0xFFE0 not found");
    }
    return 0;
}

static void start_discovery(uint16_t conn)
{
    s_svc_end = 0;
    s_notify_val = 0;
    s_write_val = 0;
    s_write_no_rsp = false;
    s_cccd_handle = 0;
    ESP_LOGI(TAG, "discovering JK-BMS service on conn=%u", (unsigned)conn);
    ble_gattc_disc_svc_by_uuid(conn, BMS_SVC_UUID, on_svc, NULL);
}

/* ---- scan filtering ---- */

static bool adv_is_bms(const struct ble_hs_adv_fields *f)
{
    for (int i = 0; i < f->num_uuids16; i++) {
        if (ble_uuid_u16(&f->uuids16[i].u) == BMS_SVC_UUID16) return true;
    }
    if (f->name && f->name_len) {
        char name[32] = { 0 };
        size_t n = f->name_len < sizeof(name) - 1 ? f->name_len
                                                  : sizeof(name) - 1;
        for (size_t i = 0; i < n; i++) {
            char c = (char)f->name[i];
            name[i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
        }
        if (strstr(name, "JK") || strstr(name, "BMS") ||
            strstr(name, "JIKONG")) {
            return true;
        }
    }
    return false;
}

/* ---- GAP ---- */

static int bms_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
        struct ble_hs_adv_fields fields;
        if (ble_hs_adv_parse_fields(&fields, event->disc.data,
                                    event->disc.length_data) != 0) {
            return 0;
        }
        if (!adv_is_bms(&fields)) return 0;

        char name[32] = { 0 };
        if (fields.name && fields.name_len) {
            size_t n = fields.name_len < sizeof(name) - 1 ? fields.name_len
                                                          : sizeof(name) - 1;
            memcpy(name, fields.name, n);
        }
        if (!name[0]) snprintf(name, sizeof(name), "JK-BMS");
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
        arm_connect();
        ble_arb_scan_resume();
        return 0;

#if defined(BLE_GAP_EVENT_LINK_ESTAB)
    case BLE_GAP_EVENT_LINK_ESTAB:
#else
    case BLE_GAP_EVENT_CONNECT:
#endif
        if (event->connect.status == 0) {
            s_conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG, "JK-BMS connected, conn=%u",
                     (unsigned)s_conn_handle);
            ble_gattc_exchange_mtu(s_conn_handle, NULL, NULL);
            start_discovery(s_conn_handle);
        } else {
            ESP_LOGW(TAG, "JK-BMS connect failed status=%d",
                     event->connect.status);
            s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            portENTER_CRITICAL(&s_mux);
            s_connected = false;
            portEXIT_CRITICAL(&s_mux);
            arm_connect();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "JK-BMS disconnect, reason=%d",
                 event->disconnect.reason);
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        s_svc_end = s_notify_val = s_write_val = s_cccd_handle = 0;
        portENTER_CRITICAL(&s_mux);
        s_connected = false;
        portEXIT_CRITICAL(&s_mux);
        arm_connect();
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        if (s_notify_val != 0 &&
            event->notify_rx.attr_handle == s_notify_val) {
            uint8_t buf[128];
            uint16_t total = OS_MBUF_PKTLEN(event->notify_rx.om);
            uint16_t off = 0;
            while (off < total) {
                uint16_t n = total - off;
                if (n > sizeof(buf)) n = sizeof(buf);
                if (os_mbuf_copydata(event->notify_rx.om, off, n, buf) != 0) {
                    break;
                }
                assemble_and_parse(buf, n);
                off = (uint16_t)(off + n);
            }
        }
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU %u on conn=%u",
                 (unsigned)event->mtu.value, (unsigned)event->mtu.conn_handle);
        return 0;

    default:
        return 0;
    }
}

/* ---- connection management ---- */

static void arm_connect(void)
{
    if (!s_inited || !s_synced || !s_bound) return;
    if (s_scanning) return;
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) return;
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

/* ---- public API ---- */

void ble_bms_client_init(void)
{
    s_inited = true;
    s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    s_temp1 = s_temp2 = s_temp_mos = NAN;
    s_soh = NAN;
    s_cell_min = s_cell_max = s_cell_delta = NAN;
    s_arb_id = ble_arb_register(bms_gap_event);
    if (s_arb_id < 0) {
        ESP_LOGE(TAG, "no free central-arbiter slot — JK-BMS disabled");
        return;
    }
    /* Small, mostly-idle task: 0.5 s tick, writes only twice a second while
     * connected. Priority below the UI so a busy radio never starves it. */
    if (xTaskCreate(poll_task, "bms_poll", 3072, NULL, 3, NULL) != pdPASS) {
        ESP_LOGW(TAG, "poll task create failed — BMS data will be one-shot");
    }
}

void ble_bms_on_ble_sync(uint8_t own_addr_type)
{
    s_own_addr_type = own_addr_type;
    s_synced = true;
    if (s_bound) arm_connect();
}

void ble_bms_set_scan_cb(ble_bms_scan_cb_t cb)
{
    s_scan_cb = cb;
}

void ble_bms_scan_start(void)
{
    if (!s_synced || s_scanning) return;
    ble_arb_scan_suspend();
    struct ble_gap_disc_params dp = { 0 };
    dp.passive = 0;          /* active scan for the name in scan response */
    dp.filter_duplicates = 1;
    int rc = ble_gap_disc(s_own_addr_type, SCAN_DURATION_MS, &dp,
                          bms_gap_event, NULL);
    if (rc == 0) {
        portENTER_CRITICAL(&s_mux);
        s_scanning = true;
        portEXIT_CRITICAL(&s_mux);
        ESP_LOGI(TAG, "scanning for JK-BMS (%d ms)", SCAN_DURATION_MS);
    } else {
        ESP_LOGW(TAG, "ble_gap_disc rc=%d", rc);
        arm_connect();
        ble_arb_scan_resume();
    }
}

void ble_bms_scan_stop(void)
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

void ble_bms_bind(const uint8_t addr[6], uint8_t addr_type)
{
    portENTER_CRITICAL(&s_mux);
    s_bound = true;
    s_bound_addr.type = addr_type;
    memcpy(s_bound_addr.val, addr, 6);
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "bound BMS %02X:%02X:%02X:%02X:%02X:%02X (type %u)",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],
             (unsigned)addr_type);
    if (s_scanning) {
        ble_gap_disc_cancel();
        portENTER_CRITICAL(&s_mux);
        s_scanning = false;
        portEXIT_CRITICAL(&s_mux);
        ble_arb_scan_resume();
    }
    if (s_synced) restart_connection();
}

void ble_bms_forget(void)
{
    portENTER_CRITICAL(&s_mux);
    s_bound = false;
    portEXIT_CRITICAL(&s_mux);
    ESP_LOGI(TAG, "forget BMS");
    ble_arb_stop_connect(s_arb_id);
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
    s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    s_svc_end = s_notify_val = s_write_val = s_cccd_handle = 0;
    portENTER_CRITICAL(&s_mux);
    s_connected = false;
    portEXIT_CRITICAL(&s_mux);
}

bool ble_bms_is_bound(void)
{
    return s_bound;
}

void ble_bms_get(ble_bms_state_t *out)
{
    if (!out) return;
    portENTER_CRITICAL(&s_mux);
    out->bound         = s_bound;
    out->connected     = s_connected;
    out->scanning      = s_scanning;
    out->has_data      = s_has_data;
    out->soc           = s_soc;
    out->remaining_ah  = s_rem_ah;
    out->full_ah       = s_full_ah;
    out->voltage       = s_voltage;
    out->current       = s_current;
    out->power         = s_voltage * s_current;
    out->temp1         = s_temp1;
    out->temp2         = s_temp2;
    out->temp_mos      = s_temp_mos;
    out->cell_min      = s_cell_min;
    out->cell_max      = s_cell_max;
    out->cell_delta    = s_cell_delta;
    out->cell_min_index = s_cell_min_i;
    out->cell_max_index = s_cell_max_i;
    out->cell_count    = s_cell_count;
    memcpy(out->cells, s_cells, sizeof(out->cells));
    out->cycles        = s_cycles;
    out->soh           = s_soh;
    out->charging_mos  = s_chg_mos;
    out->discharging_mos = s_dis_mos;
    out->balancing     = s_balancing;
    out->heating       = s_heating;
    memcpy(out->model, s_model, sizeof(out->model));
    out->frames        = s_frames;
    int64_t last = s_last_frame_us;
    portEXIT_CRITICAL(&s_mux);

    int64_t now = esp_timer_get_time();
    if (last == 0) {
        out->age_ms = UINT32_MAX;
        out->fresh  = false;
    } else {
        int64_t age = (now - last) / 1000;
        out->age_ms = age < 0 ? 0 : (age > UINT32_MAX ? UINT32_MAX
                                                      : (uint32_t)age);
        out->fresh  = out->age_ms < FRESH_WINDOW_MS;
    }
}
