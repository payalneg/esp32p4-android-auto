#include "ble_host.h"

#include <string.h>

#include "esp_hosted.h"
#include "esp_hosted_misc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "ble_cadence_client.h"
#include "ble_nus.h"
#include "notif_bridge.h"

/* Nordic UART Service UUID, NimBLE LE byte order — same value as the
 * one declared in ble_nus.c (kept in sync manually; only used for adv). */
static const ble_uuid128_t NUS_SVC_UUID_ADV = BLE_UUID128_INIT(
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E);

static const char *TAG = "ble_host";

#define DEVICE_NAME "SuperVESCDisplay"

static uint8_t       s_own_addr_type;
static bool          s_started;
static volatile bool s_connected;

bool ble_host_is_connected(void)
{
    return s_connected;
}

static void start_advertising(void);

/* Fan a single NimBLE register callback out to every co-resident GATT
 * service. NimBLE only takes one `gatts_register_cb`, so we proxy. */
static void gatts_register_dispatcher(struct ble_gatt_register_ctxt *ctxt,
                                      void *arg)
{
    ble_nus_gatts_register_cb(ctxt, arg);
    notif_bridge_gatts_register_cb(ctxt, arg);
}

static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
#if defined(BLE_GAP_EVENT_LINK_ESTAB)
    case BLE_GAP_EVENT_LINK_ESTAB:
#else
    case BLE_GAP_EVENT_CONNECT:
#endif
        if (event->connect.status == 0) {
            ESP_LOGI(TAG, "GAP connect, conn=%u",
                     (unsigned)event->connect.conn_handle);
            s_connected = true;
            ble_nus_on_connect(event->connect.conn_handle);
            notif_bridge_on_connect(event->connect.conn_handle);
        } else {
            ESP_LOGW(TAG, "GAP connect failed, status=%d",
                     event->connect.status);
            start_advertising();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "GAP disconnect, reason=%d", event->disconnect.reason);
        s_connected = false;
        ble_nus_on_disconnect();
        notif_bridge_on_disconnect();
        start_advertising();
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "adv complete, reason=%d", event->adv_complete.reason);
        start_advertising();
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU update, conn=%u mtu=%u",
                 (unsigned)event->mtu.conn_handle,
                 (unsigned)event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "subscribe, conn=%u attr=%u cur_notify=%d",
                 (unsigned)event->subscribe.conn_handle,
                 (unsigned)event->subscribe.attr_handle,
                 event->subscribe.cur_notify);
        /* Gate NUS forwarding on a real subscription so a notifications-only
         * peer doesn't get spammed with VESC RT-poll responses. Ignored for
         * non-NUS handles (NotifBridge). */
        ble_nus_on_subscribe(event->subscribe.attr_handle,
                             event->subscribe.cur_notify);
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        ESP_LOGD(TAG, "conn update, status=%d", event->conn_update.status);
        return 0;

    case BLE_GAP_EVENT_NOTIFY_TX:
        if (event->notify_tx.status != 0) {
            /* On a congested or flaky link EVERY notification completion lands
             * here with status=ENOMEM(6). This callback runs in the NimBLE host
             * task (prio configMAX-4 = 21, far above the prio-6 LVGL worker), so
             * logging each one — synchronous console write + PSRAM ring copy
             * under a spinlock — lets this high-prio task drown the CPU in
             * logging instead of draining the BLE event queue, starving the UI
             * (the dashboard visibly freezes). Aggregate to one line/sec. */
            static int64_t  s_last_us;
            static unsigned s_suppressed;
            s_suppressed++;
            int64_t now = esp_timer_get_time();
            if (now - s_last_us > 1000000) {
                ESP_LOGW(TAG, "notify_tx err: status=%d attr=%u (x%u in last %lld ms)",
                         event->notify_tx.status,
                         (unsigned)event->notify_tx.attr_handle,
                         s_suppressed,
                         s_last_us ? (now - s_last_us) / 1000 : 0);
                s_last_us = now;
                s_suppressed = 0;
            }
        }
        return 0;

    default:
        return 0;
    }
}

static void start_advertising(void)
{
    /* Adv data carries flags + the NUS service UUID-128 — VESC Tool and
     * similar apps filter scans by this UUID and won't show the device
     * if it's not advertised. UUID-128 (18 B) + flags (3 B) leaves no
     * room for the 16-char device name in the 31-byte adv payload, so
     * the name moves to the scan response. */
    struct ble_hs_adv_fields adv = { 0 };
    adv.flags          = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv.uuids128       = (ble_uuid128_t *)&NUS_SVC_UUID_ADV;
    adv.num_uuids128   = 1;
    adv.uuids128_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&adv);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_set_fields rc=%d", rc);
        return;
    }

    struct ble_hs_adv_fields rsp = { 0 };
    const char *name = ble_svc_gap_device_name();
    rsp.name             = (uint8_t *)name;
    rsp.name_len         = strlen(name);
    rsp.name_is_complete = 1;
    rsp.tx_pwr_lvl_is_present = 1;
    rsp.tx_pwr_lvl            = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    rc = ble_gap_adv_rsp_set_fields(&rsp);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_rsp_set_fields rc=%d", rc);
        return;
    }

    struct ble_gap_adv_params params = { 0 };
    params.conn_mode = BLE_GAP_CONN_MODE_UND;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER,
                           &params, gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_start rc=%d", rc);
        return;
    }
    ESP_LOGI(TAG, "advertising as \"%s\" (NUS in adv, name in scan rsp)",
             name);
}

static void on_reset_cb(int reason)
{
    ESP_LOGW(TAG, "host reset, reason=%d", reason);
}

static void on_sync_cb(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "ensure_addr rc=%d", rc);
        return;
    }
    rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "id_infer_auto rc=%d", rc);
        return;
    }

    uint8_t addr[6] = { 0 };
    ble_hs_id_copy_addr(s_own_addr_type, addr, NULL);
    ESP_LOGI(TAG, "BLE addr %02X:%02X:%02X:%02X:%02X:%02X",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    start_advertising();

    /* Dual-role: now that the stack is synced and the own address type is
     * known, let the cadence (PAS) central client (re)connect to its bound
     * sensor. Scanning/connecting runs concurrently with advertising. */
    ble_cadence_on_ble_sync(s_own_addr_type);
}

static void host_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "NimBLE host task running");
    nimble_port_run();              /* blocks until nimble_port_stop */
    nimble_port_freertos_deinit();
}

esp_err_t ble_host_init(void)
{
    if (s_started) {
        return ESP_OK;
    }

    /* NimBLE logs every GATT notify at INFO ("GATT procedure initiated:
     * notify; att_handle=..."). With CAN data streaming over NUS that's a
     * notify every ~50ms — pure spam. Drop the stack's INFO chatter while
     * keeping its WARN/ERROR. Our own logs use TAG "ble_host", unaffected. */
    esp_log_level_set("NimBLE", ESP_LOG_WARN);

    /* SDIO transport to C6 should already be up via c6_ota's
     * esp_hosted_init/connect_to_slave; calling controller_init/enable
     * here is the BT-specific bring-up step. */
    esp_err_t err = esp_hosted_bt_controller_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_hosted_bt_controller_init: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_hosted_bt_controller_enable();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_hosted_bt_controller_enable: %s", esp_err_to_name(err));
        return err;
    }

    err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init: %s", esp_err_to_name(err));
        return err;
    }

    ble_hs_cfg.reset_cb         = on_reset_cb;
    ble_hs_cfg.sync_cb          = on_sync_cb;
    ble_hs_cfg.gatts_register_cb = gatts_register_dispatcher;

    /* Standard GAP / GATT services + NUS + NotifBridge. Order matters:
     * ble_svc_*_init must run before ble_gatts_count_cfg/add_svcs. */
    ble_svc_gap_init();
    ble_svc_gatt_init();

    notif_bridge_init();
    ble_cadence_client_init();

    const struct ble_gatt_svc_def *nus_svcs   = ble_nus_get_svcs();
    const struct ble_gatt_svc_def *bridge_svcs = notif_bridge_get_svcs();
    int rc = ble_gatts_count_cfg(nus_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg (nus) rc=%d", rc);
        return ESP_FAIL;
    }
    rc = ble_gatts_count_cfg(bridge_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg (bridge) rc=%d", rc);
        return ESP_FAIL;
    }
    rc = ble_gatts_add_svcs(nus_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs (nus) rc=%d", rc);
        return ESP_FAIL;
    }
    rc = ble_gatts_add_svcs(bridge_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs (bridge) rc=%d", rc);
        return ESP_FAIL;
    }

    rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (rc != 0) {
        ESP_LOGW(TAG, "device_name_set rc=%d", rc);
    }

    nimble_port_freertos_init(host_task);
    s_started = true;
    ESP_LOGI(TAG, "NimBLE host started");
    return ESP_OK;
}
