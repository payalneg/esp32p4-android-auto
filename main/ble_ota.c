/* BLE OTA for the P4 main firmware — see ble_ota.h for the wire protocol.
 *
 * Design mirrors ota_http.c: the full image is staged in PSRAM during the
 * receive phase (cheap memcpy from the NimBLE host task), then flushed to the
 * spare OTA partition in one concentrated esp_ota_write pass, SHA-256-verified
 * and booted. The flush is the only slow part (flash erase/program), so it
 * runs on a dedicated worker task — never on the NimBLE host task, which must
 * stay responsive and is also the only task allowed to send notifications
 * (matching notif_bridge's out_task; notify must not be called from inside a
 * GATT access callback). */

#include "ble_ota.h"

#include <inttypes.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "mbedtls/sha256.h"

#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_mbuf.h"
#include "os/os_mbuf.h"

#include "ble_central_arb.h"
#include "ota_screen.h"

static const char *TAG = "ble_ota";

/* ---- wire protocol constants (mirror Dart) ---- */
#define OTA_OP_BEGIN     0x01
#define OTA_OP_END       0x02
#define OTA_OP_ABORT     0x03

#define OTA_ST_READY     0x10
#define OTA_ST_PROGRESS  0x11
#define OTA_ST_DONE      0x12
#define OTA_ST_ERROR     0x1F

/* ERROR detail codes */
#define OTA_ERR_NO_PART  1   /* no spare OTA partition */
#define OTA_ERR_SIZE     2   /* image is zero / larger than the slot */
#define OTA_ERR_ALLOC    3   /* PSRAM staging alloc failed */
#define OTA_ERR_SHA      4   /* SHA-256 of received bytes != BEGIN digest */
#define OTA_ERR_BEGIN    5   /* esp_ota_begin (erase) failed */
#define OTA_ERR_WRITE    6   /* esp_ota_write failed */
#define OTA_ERR_END      7   /* esp_ota_end / image verify failed */
#define OTA_ERR_BOOT     8   /* esp_ota_set_boot_partition failed */
#define OTA_ERR_PROTO    9   /* protocol / sequence error (overflow, END w/o data) */

#define OTA_BEGIN_LEN     (1 + 4 + 32)   /* op + u32 total_len + sha256 */
#define OTA_FLASH_CHUNK   4096
#define OTA_PROGRESS_STEP (256 * 1024)   /* UI + liveness cadence */

/* ---- module state ----
 *
 * s_stage / s_total / s_recv / s_sha are written by the host task during
 * receive (ble_ota_data_write) and read by the worker at finalize. The queue
 * orders BEGIN → [PROGRESS] → FINALIZE so the worker never touches them
 * concurrently with the host task (FreeRTOS queue ops carry the barrier). */
typedef enum { ST_IDLE, ST_RECEIVING, ST_FINALIZING, ST_FAILED } ota_state_t;

typedef enum { EV_BEGIN, EV_PROGRESS, EV_FINALIZE, EV_FAIL, EV_ABORT } ev_kind_t;
typedef struct {
    ev_kind_t kind;
    uint32_t  total_len;   /* EV_BEGIN */
    uint32_t  detail;      /* EV_PROGRESS: bytes; EV_FAIL: err code */
    uint8_t   sha[32];     /* EV_BEGIN */
} ota_evt_t;

static QueueHandle_t s_q;
static TaskHandle_t  s_task;

static uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_ctrl_handle;

static volatile ota_state_t s_state = ST_IDLE;
static uint8_t  *s_stage;
static uint32_t  s_total;
static uint32_t  s_recv;
static uint32_t  s_next_progress;
static uint8_t   s_expect_sha[32];
static mbedtls_sha256_context s_sha;
static bool      s_sha_active;

/* ---- helpers ---- */

static void notify_status(uint8_t status, uint32_t detail)
{
    if (s_conn == BLE_HS_CONN_HANDLE_NONE || s_ctrl_handle == 0) return;
    uint8_t f[5] = {
        status,
        (uint8_t)detail, (uint8_t)(detail >> 8),
        (uint8_t)(detail >> 16), (uint8_t)(detail >> 24),
    };
    struct os_mbuf *om = ble_hs_mbuf_from_flat(f, sizeof(f));
    if (!om) return;
    int rc = ble_gatts_notify_custom(s_conn, s_ctrl_handle, om);
    if (rc != 0) ESP_LOGW(TAG, "notify status=0x%02x rc=%d", status, rc);
}

static void stage_free(void)
{
    if (s_stage) { heap_caps_free(s_stage); s_stage = NULL; }
    if (s_sha_active) { mbedtls_sha256_free(&s_sha); s_sha_active = false; }
    s_total = s_recv = s_next_progress = 0;
}

/* Link tuning for the transfer (see the speed note in ble_ota.h).
 *
 * on:  ask the central for an 11.25–15 ms connection interval — the phone
 *      writes one DATA chunk per ATT round trip, so the interval is the
 *      throughput; Android's default is ~45 ms and it refuses < 11.25 ms.
 *      Also park the sensor-connect arbiter: with a bound sensor asleep it
 *      keeps an initiator scan running (NimBLE's default connect params scan
 *      at 100 % duty) that competes with this link for the radio.
 * off: back to a balanced 30–50 ms interval and let the arbiter resume. The
 *      update request is best-effort — the phone may ignore or override it
 *      (the app asks for high priority on its side too). */
static bool s_link_boosted;

static void link_boost(bool on)
{
    if (on == s_link_boosted) return;
    s_link_boosted = on;
    if (on) ble_arb_scan_suspend();
    else    ble_arb_scan_resume();

    if (s_conn == BLE_HS_CONN_HANDLE_NONE) return;
    struct ble_gap_upd_params p = {
        .itvl_min            = on ? 9  : BLE_GAP_INITIAL_CONN_ITVL_MIN,   /* 11.25 ms : 30 ms */
        .itvl_max            = on ? 12 : BLE_GAP_INITIAL_CONN_ITVL_MAX,   /* 15 ms    : 50 ms */
        .latency             = 0,
        .supervision_timeout = 400,    /* 4 s */
        .min_ce_len          = 0,
        .max_ce_len          = 0,
    };
    int rc = ble_gap_update_params(s_conn, &p);
    if (rc != 0) ESP_LOGW(TAG, "conn param update (%s) rc=%d", on ? "fast" : "normal", rc);
}

/* ---- worker-task handlers (the only place notifies / flash happen) ---- */

static void do_begin(const ota_evt_t *ev)
{
    stage_free();
    s_state = ST_IDLE;

    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    if (!next) {
        ESP_LOGE(TAG, "no next OTA partition");
        notify_status(OTA_ST_ERROR, OTA_ERR_NO_PART);
        return;
    }
    if (ev->total_len == 0 || ev->total_len > next->size) {
        ESP_LOGE(TAG, "bad size %u (slot %u)",
                 (unsigned)ev->total_len, (unsigned)next->size);
        notify_status(OTA_ST_ERROR, OTA_ERR_SIZE);
        return;
    }
    s_stage = heap_caps_malloc(ev->total_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_stage) {
        ESP_LOGE(TAG, "PSRAM staging alloc %u failed", (unsigned)ev->total_len);
        notify_status(OTA_ST_ERROR, OTA_ERR_ALLOC);
        return;
    }

    mbedtls_sha256_init(&s_sha);
    mbedtls_sha256_starts(&s_sha, 0);
    s_sha_active = true;
    memcpy(s_expect_sha, ev->sha, 32);
    s_total = ev->total_len;
    s_recv = 0;
    s_next_progress = OTA_PROGRESS_STEP;

    ota_screen_init();
    ota_screen_set_title("Firmware update");
    ota_screen_show("Receiving over Bluetooth");
    ota_screen_set_status("Don't power off");
    ota_screen_set_progress(0, s_total);

    s_state = ST_RECEIVING;
    ESP_LOGI(TAG, "BLE OTA begin: %u bytes -> %s @ 0x%08" PRIx32,
             (unsigned)s_total, next->label, next->address);
    link_boost(true);
    /* READY carries the largest DATA write we accept; the app sizes its
     * chunks from it (older app versions ignore the detail and send 244). */
    notify_status(OTA_ST_READY, BLE_OTA_MAX_DATA);
}

static void do_finalize(void)
{
    if (s_state != ST_RECEIVING) {
        notify_status(OTA_ST_ERROR, OTA_ERR_PROTO);
        return;
    }
    s_state = ST_FINALIZING;

    /* The bytes are in; the link no longer needs to be fast (and a failed
     * finalize must not leave the phone stuck on the fast interval). */
    link_boost(false);

    if (s_recv != s_total) {
        ESP_LOGE(TAG, "END with recv=%u want=%u", (unsigned)s_recv, (unsigned)s_total);
        ota_screen_set_status_error("Incomplete transfer");
        notify_status(OTA_ST_ERROR, OTA_ERR_PROTO);
        stage_free();
        s_state = ST_IDLE;
        return;
    }

    unsigned char digest[32];
    mbedtls_sha256_finish(&s_sha, digest);
    mbedtls_sha256_free(&s_sha);
    s_sha_active = false;
    if (memcmp(digest, s_expect_sha, 32) != 0) {
        ESP_LOGE(TAG, "sha256 mismatch — corrupted transfer");
        ota_screen_set_status_error("Checksum error");
        notify_status(OTA_ST_ERROR, OTA_ERR_SHA);
        stage_free();
        s_state = ST_IDLE;
        return;
    }

    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    if (!next) {
        notify_status(OTA_ST_ERROR, OTA_ERR_NO_PART);
        stage_free();
        s_state = ST_IDLE;
        return;
    }

    /* Flush staged image to flash in one concentrated pass. The flash
     * erase / program windows briefly stall the MIPI-DSI DMA, so the panel
     * may flicker — warn the user (same as the HTTP path). */
    ota_screen_show("Writing firmware - screen may flicker");
    ota_screen_set_status("Don't power off");
    ota_screen_set_progress(0, s_total);

    esp_ota_handle_t h = 0;
    esp_err_t err = esp_ota_begin(next, s_total, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin: %s", esp_err_to_name(err));
        ota_screen_set_status_error("Erase failed");
        notify_status(OTA_ST_ERROR, OTA_ERR_BEGIN);
        stage_free();
        s_state = ST_IDLE;
        return;
    }

    int64_t t0 = esp_timer_get_time();
    uint32_t next_ui = OTA_PROGRESS_STEP;
    for (uint32_t off = 0; off < s_total; ) {
        uint32_t chunk = (s_total - off) < OTA_FLASH_CHUNK
                         ? (s_total - off) : OTA_FLASH_CHUNK;
        err = esp_ota_write(h, s_stage + off, chunk);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write @%u: %s", (unsigned)off, esp_err_to_name(err));
            ota_screen_set_status_error("Write failed");
            esp_ota_abort(h);
            notify_status(OTA_ST_ERROR, OTA_ERR_WRITE);
            stage_free();
            s_state = ST_IDLE;
            return;
        }
        off += chunk;
        if (off >= next_ui) {
            ota_screen_set_progress(off, s_total);
            notify_status(OTA_ST_PROGRESS, off);
            next_ui += OTA_PROGRESS_STEP;
        }
    }
    ota_screen_set_progress(s_total, s_total);
    ESP_LOGI(TAG, "flush done in %lld ms", (esp_timer_get_time() - t0) / 1000);

    ota_screen_show("Verifying");
    err = esp_ota_end(h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end: %s", esp_err_to_name(err));
        ota_screen_set_status_error("Verify failed");
        notify_status(OTA_ST_ERROR, OTA_ERR_END);
        stage_free();
        s_state = ST_IDLE;
        return;
    }
    err = esp_ota_set_boot_partition(next);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_boot_partition: %s", esp_err_to_name(err));
        ota_screen_set_status_error("Commit failed");
        notify_status(OTA_ST_ERROR, OTA_ERR_BOOT);
        stage_free();
        s_state = ST_IDLE;
        return;
    }

    stage_free();
    s_state = ST_IDLE;
    ota_screen_show("Rebooting...");
    ota_screen_set_status("");
    ESP_LOGW(TAG, "BLE OTA OK -> %s, rebooting in 1500 ms", next->label);
    notify_status(OTA_ST_DONE, 0);
    /* Give the DONE notify time to traverse SDIO → C6 → air before we drop
     * the link with a reboot. */
    vTaskDelay(pdMS_TO_TICKS(1500));
    esp_restart();
}

static void worker(void *arg)
{
    (void)arg;
    ota_evt_t ev;
    for (;;) {
        if (xQueueReceive(s_q, &ev, portMAX_DELAY) != pdTRUE) continue;
        switch (ev.kind) {
            case EV_BEGIN:
                do_begin(&ev);
                break;
            case EV_PROGRESS:
                ota_screen_set_progress(ev.detail, s_total);
                notify_status(OTA_ST_PROGRESS, ev.detail);
                break;
            case EV_FINALIZE:
                do_finalize();
                break;
            case EV_FAIL:
                ESP_LOGW(TAG, "OTA fail code=%u", (unsigned)ev.detail);
                ota_screen_set_status_error("Transfer error");
                notify_status(OTA_ST_ERROR, ev.detail);
                stage_free();
                s_state = ST_IDLE;
                link_boost(false);
                break;
            case EV_ABORT:
                ESP_LOGW(TAG, "OTA aborted");
                ota_screen_hide();
                stage_free();
                s_state = ST_IDLE;
                link_boost(false);
                break;
        }
    }
}

/* ---- public API ---- */

void ble_ota_init(void)
{
    if (s_q) return;
    s_q = xQueueCreate(8, sizeof(ota_evt_t));
    xTaskCreatePinnedToCore(worker, "ble_ota", 8192, NULL, 5, &s_task, 0);
    ESP_LOGI(TAG, "ble_ota ready");
}

void ble_ota_set_link(uint16_t conn, uint16_t ctrl_val_handle)
{
    s_conn = conn;
    s_ctrl_handle = ctrl_val_handle;
}

void ble_ota_on_disconnect(void)
{
    s_conn = BLE_HS_CONN_HANDLE_NONE;
    /* Abandon any in-flight transfer. We can't free the staging buffer from
     * here if the worker is mid-finalize, so hand it an ABORT to clean up on
     * its own task. */
    if (s_state == ST_RECEIVING) {
        ota_evt_t ev = { .kind = EV_ABORT };
        if (s_q) xQueueSend(s_q, &ev, 0);
    }
}

/* ---- routed from notif_bridge access_cb (NimBLE host task) ---- */

void ble_ota_ctrl_write(const uint8_t *data, uint16_t len)
{
    if (len < 1 || !s_q) return;
    switch (data[0]) {
        case OTA_OP_BEGIN: {
            ota_evt_t ev = { .kind = EV_BEGIN };
            if (len < OTA_BEGIN_LEN) {
                ev.kind = EV_FAIL;
                ev.detail = OTA_ERR_PROTO;
                xQueueSend(s_q, &ev, 0);
                return;
            }
            ev.total_len = (uint32_t)data[1] | ((uint32_t)data[2] << 8) |
                           ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 24);
            memcpy(ev.sha, data + 5, 32);
            /* Gate data writes until the worker has allocated and switched to
             * RECEIVING (it then sends READY; the app waits for it). */
            s_state = ST_IDLE;
            xQueueSend(s_q, &ev, portMAX_DELAY);
            break;
        }
        case OTA_OP_END: {
            ota_evt_t ev = { .kind = EV_FINALIZE };
            xQueueSend(s_q, &ev, portMAX_DELAY);
            break;
        }
        case OTA_OP_ABORT: {
            ota_evt_t ev = { .kind = EV_ABORT };
            xQueueSend(s_q, &ev, 0);
            break;
        }
        default:
            break;
    }
}

void ble_ota_data_write(const uint8_t *data, uint16_t len)
{
    if (s_state != ST_RECEIVING || !len) return;
    if (s_recv + len > s_total) {
        ESP_LOGE(TAG, "data overflow recv=%u len=%u total=%u",
                 (unsigned)s_recv, (unsigned)len, (unsigned)s_total);
        s_state = ST_FAILED;
        ota_evt_t ev = { .kind = EV_FAIL, .detail = OTA_ERR_PROTO };
        if (s_q) xQueueSend(s_q, &ev, 0);
        return;
    }
    memcpy(s_stage + s_recv, data, len);
    mbedtls_sha256_update(&s_sha, data, len);
    s_recv += len;
    if (s_recv >= s_next_progress) {
        ota_evt_t ev = { .kind = EV_PROGRESS, .detail = s_recv };
        if (s_q) xQueueSend(s_q, &ev, 0);   /* best-effort; drop if full */
        s_next_progress += OTA_PROGRESS_STEP;
    }
}
