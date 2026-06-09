/* NotifBridge GATT service.
 *
 * Coexists with ble_nus inside ble_host. We share the GAP/connection
 * lifecycle (driven by ble_host.c) but own a separate service + reassembly
 * state. Inbound frames carry an 8-byte chunk header followed by TLV-encoded
 * payload — see flutter-application/lib/ble/protocol.dart for the wire
 * spec. Outbound frames go via a small fixed-size queue and a dedicated
 * task to avoid blocking the LVGL task when it presses play/pause.
 *
 * Memory budget:
 *   - one reassembly buffer (max ICON_MAX_PNG_BYTES, ~40 KB)
 *   - LRU icon cache (NOTIF_ICON_CACHE × ICON_MAX_PNG_BYTES = 10 × 40 KB)
 *   - notification ring (NOTIF_RING_SIZE × sizeof(notif_msg_t) ≈ 16 KB)
 *
 * Total ~440 KB worst case — fits in P4 PSRAM heap, well within the
 * existing budget after wifi/h264. The PNG cache is allocated lazily,
 * not at boot. */

#include "notif_bridge.h"
#include "ble_ota.h"
#include "ble_files.h"
#include "board.h"

#include <string.h>

#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_mbuf.h"
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"

static const char *TAG = "notif_bridge";

/* ---------- UUIDs (mirror Dart `NotifBridgeUuids`) ----------
 * Wire-form: 7b4e4f00-3f8e-4d2a-9d5c-2c9f1a6e000{1,2,3,4}
 * NimBLE stores 128-bit UUIDs little-endian, so the byte arrays below
 * are the wire form reversed. Only the last byte distinguishes service
 * vs. characteristics (the trailing 1/2/3/4). */
#define NB_UUID_TAIL_LE                                              \
    0x01, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,                  \
    0x2A, 0x4D, 0x8E, 0x3F, /* byte 12 */ 0x00, 0x4F, 0x4E, 0x7B

static const ble_uuid128_t NB_SVC_UUID = BLE_UUID128_INIT(
    0x01, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,
    0x2A, 0x4D, 0x8E, 0x3F, 0x00, 0x4F, 0x4E, 0x7B);
static const ble_uuid128_t NB_IN_UUID  = BLE_UUID128_INIT(
    0x02, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,
    0x2A, 0x4D, 0x8E, 0x3F, 0x00, 0x4F, 0x4E, 0x7B);
static const ble_uuid128_t NB_OUT_UUID = BLE_UUID128_INIT(
    0x03, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,
    0x2A, 0x4D, 0x8E, 0x3F, 0x00, 0x4F, 0x4E, 0x7B);
static const ble_uuid128_t NB_ST_UUID  = BLE_UUID128_INIT(
    0x04, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,
    0x2A, 0x4D, 0x8E, 0x3F, 0x00, 0x4F, 0x4E, 0x7B);
/* TIME char — phone pushes wall-clock time so the dashboard can show it
 * without an on-device RTC (see ENABLE_WALL_CLOCK in main/config.h, which
 * we no longer rely on for the coin-cell-free path). The app discovers
 * this characteristic to decide whether the head unit supports the
 * feature; firmware without it simply won't expose ...0005. */
static const ble_uuid128_t NB_TIME_UUID = BLE_UUID128_INIT(
    0x05, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,
    0x2A, 0x4D, 0x8E, 0x3F, 0x00, 0x4F, 0x4E, 0x7B);
/* OTA-INFO char — READ returns the SoftAP credentials + OTA HTTP endpoint
 * + running firmware version so the companion app can auto-join the head
 * unit's AP and POST a bundled firmware image to it. Newline-joined UTF-8:
 *   "<ip>\n<port>\n<ssid>\n<password>\n<version>\n<board-model>"
 * The trailing <board-model> (e.g. "waveshare"/"jc4880") lets the app pick the
 * matching bundled image; it was added later, so older apps that stop at 5
 * fields stay compatible. The app probes for this characteristic; firmware
 * without it doesn't expose ...0006 and the app hides the "update" action. */
static const ble_uuid128_t NB_OTA_UUID = BLE_UUID128_INIT(
    0x06, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,
    0x2A, 0x4D, 0x8E, 0x3F, 0x00, 0x4F, 0x4E, 0x7B);
/* OTA-CTRL / OTA-DATA chars — BLE firmware update of the P4 main image,
 * straight over the existing BLE link (no SoftAP / WiFi needed). CTRL carries
 * BEGIN/END/ABORT commands and notifies READY/PROGRESS/DONE/ERROR; DATA
 * carries the raw firmware bytes. See ble_ota.c for the wire protocol. Both
 * are optional/probed by the app, which falls back to the WiFi/HTTP path when
 * the head unit doesn't expose them. */
static const ble_uuid128_t NB_OTA_CTRL_UUID = BLE_UUID128_INIT(
    0x07, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,
    0x2A, 0x4D, 0x8E, 0x3F, 0x00, 0x4F, 0x4E, 0x7B);
static const ble_uuid128_t NB_OTA_DATA_UUID = BLE_UUID128_INIT(
    0x08, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,
    0x2A, 0x4D, 0x8E, 0x3F, 0x00, 0x4F, 0x4E, 0x7B);
/* FILE-CTRL / FILE-DATA chars — browse + transfer the device filesystem from
 * the phone (list/download/upload/delete/rename/mkdir over /vescfs and /sdcard).
 * CTRL carries request opcodes and notifies status + bulk listing/download
 * payload; DATA carries raw upload bytes. See ble_files.c for the protocol.
 * Optional/probed by the app — older firmware just won't expose ...0009/...000A. */
static const ble_uuid128_t NB_FILE_CTRL_UUID = BLE_UUID128_INIT(
    0x09, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,
    0x2A, 0x4D, 0x8E, 0x3F, 0x00, 0x4F, 0x4E, 0x7B);
static const ble_uuid128_t NB_FILE_DATA_UUID = BLE_UUID128_INIT(
    0x0A, 0x00, 0x6E, 0x1A, 0x9F, 0x2C, 0x5C, 0x9D,
    0x2A, 0x4D, 0x8E, 0x3F, 0x00, 0x4F, 0x4E, 0x7B);

/* ---------- PDU layer ---------- */

#define PDU_TYPE_NOTIF     1
#define PDU_TYPE_MEDIA     2
#define PDU_TYPE_ICON      3
#define PDU_TYPE_ART       4
#define PDU_TYPE_CMD       5
#define PDU_TYPE_ACK       6
#define PDU_TYPE_KEEPALIVE 7

#define CHUNK_HDR_LEN      8
#define CHUNK_FLAG_START   0x01
#define CHUNK_FLAG_END     0x02

/* Real-world Yandex Music / Spotify album art PNGs land in the 50-90 KB
 * range; cap at 192 KB so we don't truncate genuine art while still
 * refusing absurd payloads. Buffer is PSRAM-backed and allocated lazily,
 * so the cap costs nothing until the first big PDU lands. */
#define REASM_MAX_BODY     (192 * 1024)
#define NOTIF_RING_SIZE    20
#define NOTIF_ICON_CACHE   10

/* ---------- module state ---------- */

static uint16_t s_in_handle, s_out_handle, s_st_handle, s_time_handle, s_ota_handle;
static uint16_t s_ota_ctrl_handle, s_ota_data_handle;
static uint16_t s_file_ctrl_handle, s_file_data_handle;
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;

/* Phone-pushed wall clock. The app writes [hour, minute] every 15 s; we
 * stamp each write with a monotonic timestamp so the LVGL side can hide
 * the label once updates stop (PHONE_CLOCK_TTL_US). Written from the
 * NimBLE host task, read from the LVGL task — guarded by a tiny critical
 * section since it's just three scalars. */
#define PHONE_CLOCK_TTL_US  (30 * 1000 * 1000)  /* 30 s */
static portMUX_TYPE s_clock_mux = portMUX_INITIALIZER_UNLOCKED;
static int8_t  s_clock_hour = -1;
static int8_t  s_clock_min  = -1;
static int64_t s_clock_seen_us;  /* 0 = never set */

/* Inbound reassembly — only one in-flight message at a time (Dart side
 * sends sequentially within a single direction). */
static struct {
    uint8_t  *buf;
    uint32_t  expected;
    uint32_t  got;
    uint8_t   type;
    uint16_t  seq;
    bool      active;
} s_reasm;

/* Recent notifications — newest at the tail. */
static notif_msg_t      s_ring[NOTIF_RING_SIZE];
static size_t           s_ring_count;
static uint32_t         s_inbox_seq;
static SemaphoreHandle_t s_state_lock;

static media_state_t    s_media;

/* PNG cache — small LRU, lazy-allocated. */
typedef struct {
    uint32_t  hash;       /* 0 = empty slot */
    uint32_t  last_used;  /* monotonic counter */
    size_t    len;
    uint8_t  *data;       /* PSRAM heap */
} icon_slot_t;

static icon_slot_t s_icons[NOTIF_ICON_CACHE];
static uint32_t    s_lru_tick;

/* Outbound CMD queue — fixed-size 8-byte frames. */
typedef struct { uint8_t op; uint32_t arg; } outbound_cmd_t;
static QueueHandle_t s_out_q;
static TaskHandle_t  s_out_task;

/* ---------- helpers ---------- */

static inline uint16_t rd_u16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static inline uint32_t rd_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline uint64_t rd_u64(const uint8_t *p) {
    return (uint64_t)rd_u32(p) | ((uint64_t)rd_u32(p + 4) << 32);
}
static inline void wr_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static void lock(void) { xSemaphoreTake(s_state_lock, portMAX_DELAY); }
static void unlock(void) { xSemaphoreGive(s_state_lock); }

/* ---------- TLV parsing ---------- */

/* Walk TLV (u8 tag, u32 len, bytes value) and invoke cb per field. */
typedef bool (*tlv_visit_fn)(uint8_t tag, const uint8_t *val, uint32_t len, void *ctx);

static void tlv_walk(const uint8_t *body, uint32_t total,
                     tlv_visit_fn cb, void *ctx)
{
    uint32_t off = 0;
    while (off + 5 <= total) {
        uint8_t  tag = body[off];
        uint32_t len = rd_u32(body + off + 1);
        off += 5;
        if (len > total - off) break;
        if (!cb(tag, body + off, len, ctx)) break;
        off += len;
    }
}

static void copy_str(char *dst, size_t dst_sz, const uint8_t *val, uint32_t len)
{
    size_t n = (len < dst_sz - 1) ? len : dst_sz - 1;
    memcpy(dst, val, n);
    dst[n] = '\0';
}

/* ---------- handler: NOTIFICATION ---------- */

static bool notif_visit(uint8_t tag, const uint8_t *val, uint32_t len, void *ctx)
{
    notif_msg_t *n = ctx;
    switch (tag) {
        case 1: if (len >= 4) n->id = rd_u32(val); break;
        case 2: copy_str(n->package, sizeof(n->package), val, len); break;
        case 3: copy_str(n->app_name, sizeof(n->app_name), val, len); break;
        case 4: copy_str(n->title, sizeof(n->title), val, len); break;
        case 5: copy_str(n->text, sizeof(n->text), val, len); break;
        case 6: if (len >= 8) n->posted_at_ms = rd_u64(val); break;
        case 7: if (len >= 4) n->icon_hash = rd_u32(val); break;
        case 8: if (len >= 1) n->removed = val[0] != 0; break;
        case 9: n->is_navigation = (len == 10 && memcmp(val, "navigation", 10) == 0); break;
    }
    return true;
}

static void handle_notification(const uint8_t *body, uint32_t total)
{
    notif_msg_t n = {0};
    tlv_walk(body, total, notif_visit, &n);
    lock();
    if (n.removed) {
        /* Compact removed entry out of the ring. */
        size_t w = 0;
        for (size_t r = 0; r < s_ring_count; r++) {
            if (s_ring[r].id != n.id) {
                if (w != r) s_ring[w] = s_ring[r];
                w++;
            }
        }
        s_ring_count = w;
    } else {
        if (s_ring_count == NOTIF_RING_SIZE) {
            memmove(&s_ring[0], &s_ring[1],
                    sizeof(s_ring[0]) * (NOTIF_RING_SIZE - 1));
            s_ring_count--;
        }
        s_ring[s_ring_count++] = n;
        s_inbox_seq++;
    }
    unlock();
    /* Navigation pushes a fresh notification on every distance tick
     * ("120 м" → "100 м" → …), which floods the console — log those at
     * DEBUG so they're recoverable (raise this tag's level) without
     * spamming INFO. Ordinary notifications stay at INFO. */
    if (n.is_navigation) {
        ESP_LOGD(TAG, "notif %s app=%s title=%s (nav)",
                 n.removed ? "REM" : "NEW", n.app_name, n.title);
    } else {
        ESP_LOGI(TAG, "notif %s app=%s title=%s",
                 n.removed ? "REM" : "NEW", n.app_name, n.title);
    }
}

/* ---------- handler: MEDIA ---------- */

static bool media_visit(uint8_t tag, const uint8_t *val, uint32_t len, void *ctx)
{
    media_state_t *m = ctx;
    switch (tag) {
        case 1: copy_str(m->title, sizeof(m->title), val, len); break;
        case 2: copy_str(m->artist, sizeof(m->artist), val, len); break;
        case 3: copy_str(m->album, sizeof(m->album), val, len); break;
        case 4: if (len >= 4) m->duration_ms = rd_u32(val); break;
        case 5: if (len >= 4) m->position_ms = rd_u32(val); break;
        case 6: if (len >= 1) m->is_playing = val[0] != 0; break;
        case 7: if (len >= 4) m->album_art_hash = rd_u32(val); break;
        case 8: copy_str(m->source_app, sizeof(m->source_app), val, len); break;
    }
    return true;
}

static void handle_media(const uint8_t *body, uint32_t total)
{
    media_state_t m = {0};
    tlv_walk(body, total, media_visit, &m);
    lock();
    s_media = m;
    unlock();
    ESP_LOGD(TAG, "media \"%s\" — %s (pos %u/%u)",
             m.title, m.artist, (unsigned)m.position_ms, (unsigned)m.duration_ms);
}

/* ---------- handler: ICON / ART ---------- */

static icon_slot_t *icon_find(uint32_t hash)
{
    for (size_t i = 0; i < NOTIF_ICON_CACHE; i++) {
        if (s_icons[i].hash == hash && s_icons[i].data) return &s_icons[i];
    }
    return NULL;
}

static icon_slot_t *icon_alloc_slot(void)
{
    icon_slot_t *empty = NULL, *oldest = NULL;
    for (size_t i = 0; i < NOTIF_ICON_CACHE; i++) {
        if (!s_icons[i].data) { empty = &s_icons[i]; break; }
        if (!oldest || s_icons[i].last_used < oldest->last_used) {
            oldest = &s_icons[i];
        }
    }
    icon_slot_t *slot = empty ? empty : oldest;
    if (slot && slot->data) {
        heap_caps_free(slot->data);
        slot->data = NULL;
        slot->hash = 0;
        slot->len = 0;
    }
    return slot;
}

static void handle_icon(const uint8_t *body, uint32_t total, bool is_art)
{
    (void)is_art;
    uint32_t hash = 0;
    const uint8_t *png = NULL;
    uint32_t       png_len = 0;
    uint32_t off = 0;
    while (off + 5 <= total) {
        uint8_t tag = body[off];
        uint32_t len = rd_u32(body + off + 1);
        off += 5;
        if (len > total - off) break;
        if (tag == 1 && len >= 4) hash = rd_u32(body + off);
        else if (tag == 2) { png = body + off; png_len = len; }
        off += len;
    }
    if (!png || !png_len || !hash) return;

    lock();
    icon_slot_t *existing = icon_find(hash);
    icon_slot_t *slot = existing ? existing : icon_alloc_slot();
    if (!slot) { unlock(); return; }
    if (!existing) {
        slot->data = heap_caps_malloc(png_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!slot->data) {
            ESP_LOGE(TAG, "icon alloc fail %u B", (unsigned)png_len);
            unlock();
            return;
        }
        memcpy(slot->data, png, png_len);
        slot->len = png_len;
        slot->hash = hash;
    }
    slot->last_used = ++s_lru_tick;
    unlock();
    ESP_LOGI(TAG, "icon hash=0x%08X len=%u %s",
             (unsigned)hash, (unsigned)png_len, existing ? "(refresh)" : "(stored)");
}

/* ---------- inbound chunk reassembly ---------- */

static void reasm_reset(void)
{
    s_reasm.active = false;
    s_reasm.got = 0;
    s_reasm.expected = 0;
}

static void reasm_feed(const uint8_t *chunk, uint16_t chunk_len)
{
    if (chunk_len < CHUNK_HDR_LEN) return;
    uint8_t  type = chunk[0];
    uint8_t  flags = chunk[1];
    uint16_t seq = rd_u16(chunk + 2);
    uint32_t total = rd_u32(chunk + 4);
    const uint8_t *payload = chunk + CHUNK_HDR_LEN;
    uint16_t plen = chunk_len - CHUNK_HDR_LEN;

    if (flags & CHUNK_FLAG_START) {
        if (total > REASM_MAX_BODY) {
            ESP_LOGW(TAG, "reasm: %u > MAX, drop", (unsigned)total);
            reasm_reset();
            return;
        }
        if (!s_reasm.buf) {
            s_reasm.buf = heap_caps_malloc(REASM_MAX_BODY,
                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!s_reasm.buf) {
                ESP_LOGE(TAG, "reasm buf alloc fail");
                return;
            }
        }
        s_reasm.type = type;
        s_reasm.seq = seq;
        s_reasm.expected = total;
        s_reasm.got = 0;
        s_reasm.active = true;
    } else if (!s_reasm.active || s_reasm.type != type || s_reasm.seq != seq) {
        reasm_reset();
        return;
    }

    if (s_reasm.got + plen > REASM_MAX_BODY) { reasm_reset(); return; }
    memcpy(s_reasm.buf + s_reasm.got, payload, plen);
    s_reasm.got += plen;

    if (!(flags & CHUNK_FLAG_END)) return;
    if (s_reasm.got != s_reasm.expected && s_reasm.expected != 0) {
        ESP_LOGW(TAG, "reasm len mismatch got=%u want=%u",
                 (unsigned)s_reasm.got, (unsigned)s_reasm.expected);
        reasm_reset();
        return;
    }
    /* Dispatch. */
    switch (type) {
        case PDU_TYPE_NOTIF: handle_notification(s_reasm.buf, s_reasm.got); break;
        case PDU_TYPE_MEDIA: handle_media(s_reasm.buf, s_reasm.got); break;
        case PDU_TYPE_ICON:  handle_icon(s_reasm.buf, s_reasm.got, false); break;
        case PDU_TYPE_ART:   handle_icon(s_reasm.buf, s_reasm.got, true); break;
        default:
            ESP_LOGW(TAG, "unknown PDU type %u", type);
    }
    reasm_reset();
}

/* ---------- GATT access callback ---------- */

static int access_cb(uint16_t conn, uint16_t attr,
                     struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn; (void)arg;
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR && attr == s_in_handle) {
        uint8_t  buf[260];
        uint16_t pkt_len = OS_MBUF_PKTLEN(ctxt->om);
        if (pkt_len > sizeof(buf)) {
            ESP_LOGW(TAG, "write %u > buf", (unsigned)pkt_len);
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        uint16_t out_len = 0;
        int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &out_len);
        if (rc != 0) return BLE_ATT_ERR_UNLIKELY;
        reasm_feed(buf, out_len);
        return 0;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR && attr == s_time_handle) {
        /* Tiny fixed payload: [hour, minute]. No chunking/reassembly —
         * a clock tick never exceeds the smallest MTU. */
        uint8_t  buf[2];
        uint16_t out_len = 0;
        if (OS_MBUF_PKTLEN(ctxt->om) < 2) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &out_len);
        if (rc != 0 || out_len < 2) return BLE_ATT_ERR_UNLIKELY;
        if (buf[0] > 23 || buf[1] > 59) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        portENTER_CRITICAL(&s_clock_mux);
        s_clock_hour    = (int8_t)buf[0];
        s_clock_min     = (int8_t)buf[1];
        s_clock_seen_us = esp_timer_get_time();
        portEXIT_CRITICAL(&s_clock_mux);
        return 0;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR &&
        (attr == s_ota_ctrl_handle || attr == s_ota_data_handle)) {
        /* BLE OTA control / firmware-data channel. Flatten the mbuf and hand
         * it to ble_ota; the firmware bytes are staged in PSRAM there. The
         * 260-byte buffer covers the MTU-247 link the app negotiates
         * (payload ≤ 244). */
        uint8_t  buf[260];
        uint16_t pkt_len = OS_MBUF_PKTLEN(ctxt->om);
        if (pkt_len > sizeof(buf)) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        uint16_t out_len = 0;
        int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &out_len);
        if (rc != 0) return BLE_ATT_ERR_UNLIKELY;
        if (attr == s_ota_ctrl_handle) ble_ota_ctrl_write(buf, out_len);
        else                           ble_ota_data_write(buf, out_len);
        return 0;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR &&
        (attr == s_file_ctrl_handle || attr == s_file_data_handle)) {
        /* File manager control / upload-data channel. Same flatten-and-forward
         * shape as OTA; FS work + notifies happen on ble_files' worker task. */
        uint8_t  buf[260];
        uint16_t pkt_len = OS_MBUF_PKTLEN(ctxt->om);
        if (pkt_len > sizeof(buf)) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        uint16_t out_len = 0;
        int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &out_len);
        if (rc != 0) return BLE_ATT_ERR_UNLIKELY;
        if (attr == s_file_ctrl_handle) ble_files_ctrl_write(buf, out_len);
        else                            ble_files_data_write(buf, out_len);
        return 0;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR && attr == s_ota_handle) {
        /* "<ip>\n<port>\n<ssid>\n<password>\n<version>\n<board-model>" — the
         * app joins the SoftAP and POSTs the matching firmware image to
         * http://<ip>:<port>/ota. IP is the SoftAP gateway (fixed 192.168.4.1
         * by esp_netif default). */
#ifdef CONFIG_OTA_HTTP_PORT
        const int ota_port = CONFIG_OTA_HTTP_PORT;
#else
        const int ota_port = 80;
#endif
        const wifi_ap_info_t *ap = wifi_manager_get_ap_info();
        const esp_app_desc_t *desc = esp_app_get_description();
        char info[160];
        int n = snprintf(info, sizeof(info), "192.168.4.1\n%d\n%s\n%s\n%s\n%s",
                         ota_port,
                         ap ? ap->ssid : "",
                         ap ? ap->password : "",
                         desc ? desc->version : "",
                         BOARD_MODEL_ID);
        if (n < 0) return BLE_ATT_ERR_UNLIKELY;
        if (n > (int)sizeof(info)) n = sizeof(info);
        return os_mbuf_append(ctxt->om, info, n) == 0
            ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR && attr == s_st_handle) {
        /* Static caps blob: u16 proto version + u16 reserved. */
        static const uint8_t caps[] = { 0x01, 0x00, 0x00, 0x00 };
        return os_mbuf_append(ctxt->om, caps, sizeof(caps)) == 0
            ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
}

/* ---------- GATT service definition ---------- */

static const struct ble_gatt_svc_def s_svcs[] = {
    {
        .type            = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid            = &NB_SVC_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid       = &NB_IN_UUID.u,
                .access_cb  = access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .val_handle = &s_in_handle,
            },
            {
                .uuid       = &NB_OUT_UUID.u,
                .access_cb  = access_cb,
                .flags      = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_out_handle,
            },
            {
                .uuid       = &NB_ST_UUID.u,
                .access_cb  = access_cb,
                .flags      = BLE_GATT_CHR_F_READ,
                .val_handle = &s_st_handle,
            },
            {
                .uuid       = &NB_TIME_UUID.u,
                .access_cb  = access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .val_handle = &s_time_handle,
            },
            {
                .uuid       = &NB_OTA_UUID.u,
                .access_cb  = access_cb,
                .flags      = BLE_GATT_CHR_F_READ,
                .val_handle = &s_ota_handle,
            },
            {
                .uuid       = &NB_OTA_CTRL_UUID.u,
                .access_cb  = access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_ota_ctrl_handle,
            },
            {
                .uuid       = &NB_OTA_DATA_UUID.u,
                .access_cb  = access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .val_handle = &s_ota_data_handle,
            },
            {
                .uuid       = &NB_FILE_CTRL_UUID.u,
                .access_cb  = access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_file_ctrl_handle,
            },
            {
                .uuid       = &NB_FILE_DATA_UUID.u,
                .access_cb  = access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .val_handle = &s_file_data_handle,
            },
            { 0 },
        },
    },
    { 0 },
};

const struct ble_gatt_svc_def *notif_bridge_get_svcs(void) { return s_svcs; }

void notif_bridge_gatts_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    (void)arg;
    if (ctxt->op == BLE_GATT_REGISTER_OP_CHR) {
        char uuid_buf[BLE_UUID_STR_LEN];
        ble_uuid_to_str(ctxt->chr.chr_def->uuid, uuid_buf);
        ESP_LOGI(TAG, "registered %s val_handle=%u", uuid_buf,
                 (unsigned)ctxt->chr.val_handle);
    }
}

void notif_bridge_on_connect(uint16_t conn) {
    s_conn_handle = conn;
    ble_ota_set_link(conn, s_ota_ctrl_handle);
    ble_files_set_link(conn, s_file_ctrl_handle);
}
void notif_bridge_on_disconnect(void) {
    s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    ble_ota_on_disconnect();
    ble_files_on_disconnect();
    reasm_reset();
}

/* ---------- outbound CMD path ---------- */

void notif_bridge_send_cmd(notif_op_t op, uint32_t arg)
{
    outbound_cmd_t c = { .op = (uint8_t)op, .arg = arg };
    if (s_out_q) xQueueSend(s_out_q, &c, 0);
}

static void send_cmd_now(const outbound_cmd_t *c)
{
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE || s_out_handle == 0) return;

    /* Build a single START|END frame: 8 B header + 5 B body. */
    uint8_t frame[CHUNK_HDR_LEN + 5];
    uint8_t body_len = 5;
    frame[0] = PDU_TYPE_CMD;
    frame[1] = CHUNK_FLAG_START | CHUNK_FLAG_END;
    frame[2] = 0; frame[3] = 0;            /* seq — OUT direction unused */
    wr_u32(&frame[4], body_len);
    frame[CHUNK_HDR_LEN + 0] = c->op;
    wr_u32(&frame[CHUNK_HDR_LEN + 1], c->arg);

    struct os_mbuf *om = ble_hs_mbuf_from_flat(frame, sizeof(frame));
    if (!om) return;
    int rc = ble_gatts_notify_custom(s_conn_handle, s_out_handle, om);
    if (rc != 0) ESP_LOGW(TAG, "notify cmd rc=%d", rc);
}

static void out_task(void *arg)
{
    (void)arg;
    outbound_cmd_t c;
    while (1) {
        if (xQueueReceive(s_out_q, &c, portMAX_DELAY) == pdTRUE) {
            send_cmd_now(&c);
        }
    }
}

/* ---------- public accessors ---------- */

const media_state_t *notif_bridge_get_media(void) { return &s_media; }

size_t notif_bridge_recent(notif_msg_t *out, size_t max)
{
    if (!out || !max) return 0;
    lock();
    /* Caller wants the freshest entries — copy from the tail of the
     * ring, not the head. With s_ring_count entries laid out oldest
     * first, the last `n` slots are the newest. */
    size_t n = s_ring_count < max ? s_ring_count : max;
    memcpy(out, &s_ring[s_ring_count - n], n * sizeof(notif_msg_t));
    unlock();
    return n;
}

uint32_t notif_bridge_inbox_seq(void)
{
    return s_inbox_seq;
}

bool notif_bridge_get_phone_time(int *hour, int *minute)
{
    portENTER_CRITICAL(&s_clock_mux);
    int8_t  h = s_clock_hour, m = s_clock_min;
    int64_t seen = s_clock_seen_us;
    portEXIT_CRITICAL(&s_clock_mux);
    if (seen == 0 || h < 0 || m < 0) return false;
    if (esp_timer_get_time() - seen > PHONE_CLOCK_TTL_US) return false;
    if (hour)   *hour   = h;
    if (minute) *minute = m;
    return true;
}

const uint8_t *notif_bridge_get_icon(uint32_t hash, size_t *out_len)
{
    icon_slot_t *slot = NULL;
    lock();
    slot = icon_find(hash);
    if (slot) slot->last_used = ++s_lru_tick;
    const uint8_t *data = slot ? slot->data : NULL;
    size_t len = slot ? slot->len : 0;
    unlock();
    if (out_len) *out_len = len;
    return data;
}

/* ---------- init ---------- */

void notif_bridge_init(void)
{
    if (s_state_lock) return;
    s_state_lock = xSemaphoreCreateMutex();
    s_out_q = xQueueCreate(8, sizeof(outbound_cmd_t));
    xTaskCreatePinnedToCore(out_task, "notif_out", 4096, NULL, 5, &s_out_task, 0);
    ble_ota_init();
    ble_files_init();
    ESP_LOGI(TAG, "notif_bridge ready");
}
