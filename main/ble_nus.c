#include "ble_nus.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_mbuf.h"
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"

#include "vesc_can/comm_can.h"
#include "vesc_can/packet_parser.h"
#include "vesc_can/vesc_datatypes.h"
#include "vesc_can/vesc_io_data.h"
#include "vesc_can/vesc_lisp_poll.h"
#include "vesc_can/vesc_rt_data.h"

static const char *TAG = "ble_nus";

/* Outbound queue for VESC responses. CAN's RX task pushes framed payloads
 * here and returns immediately; a dedicated NimBLE-friendly task drains
 * the buffer and notifies in MTU-sized chunks, retrying on transient
 * BLE_HS_ENOMEM / EBUSY without backpressuring CAN.
 *
 * Why this exists: VESC Tool's "read mcconf" / firmware upload triggers
 * a burst of 100+ CAN frames in ~100 ms, each producing one reassembled
 * VESC packet up to ~512 bytes. With the previous synchronous path the
 * CAN RX task called ble_gatts_notify_custom directly — NimBLE's mbuf
 * pool emptied within the first few packets, returned BLE_HS_ENOMEM,
 * and we just bailed and dropped the rest. VESC Tool then timed out
 * and the central tore down the link.
 *
 * 8 KiB holds ~14 max-size framed packets; in practice each batch
 * empties to BLE in 200-400 ms (depends on connection interval + MTU),
 * so the queue is mostly idle. NOSPLIT keeps each framed packet atomic
 * — the TX task pulls one full packet per receive and never interleaves
 * chunks from different replies. */
#define NUS_TX_RB_SIZE  8192
static RingbufHandle_t s_tx_rb;
static TaskHandle_t    s_tx_task;

/* NimBLE stores UUID-128 in little-endian byte order (least-significant
 * byte first). The on-the-wire UUID 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
 * thus reverses to the byte sequence below; only byte[12] changes between
 * the service / RX / TX UUIDs (the trailing "01"/"02"/"03" of the prefix). */
#define NUS_UUID_TAIL_LE                                              \
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,                   \
    0x93, 0xF3, 0xA3, 0xB5, /* byte 12 below */ 0x00, 0x00, 0x40, 0x6E

static const ble_uuid128_t NUS_SVC_UUID = BLE_UUID128_INIT(
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E);

static const ble_uuid128_t NUS_RX_UUID = BLE_UUID128_INIT(
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E);

static const ble_uuid128_t NUS_TX_UUID = BLE_UUID128_INIT(
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E);

static uint16_t        s_tx_val_handle;
static uint16_t        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
/* Whether the peer enabled notifications on the NUS TX characteristic. The
 * phone connects for NotifBridge (time / notification icons) and never
 * subscribes to NUS — so without this gate we'd push the continuous VESC
 * RT-poll stream at it, each notify failing and stalling the CAN dispatch. */
static volatile bool   s_tx_subscribed;
static packet_parser_t s_rx_parser;

/* Two peripheral peers can be connected at once (phone app + VESC Tool),
 * but NUS is single-owner: replies go to whoever last wrote to RX or
 * subscribed to TX. Per-conn CCCD state is kept so ownership can switch
 * back to a peer that subscribed earlier (the app's LISP editor writing
 * while VESC Tool sits idle-connected, or the other way around). */
#define NUS_MAX_PEERS 4
typedef struct { uint16_t conn; bool subscribed; } nus_peer_t;
static nus_peer_t s_peers[NUS_MAX_PEERS] = {
    [0 ... NUS_MAX_PEERS - 1] = { .conn = BLE_HS_CONN_HANDLE_NONE },
};

static nus_peer_t *peer_find(uint16_t conn)
{
    for (int i = 0; i < NUS_MAX_PEERS; i++)
        if (s_peers[i].conn == conn) return &s_peers[i];
    return NULL;
}

static void peer_add(uint16_t conn)
{
    if (peer_find(conn)) return;
    nus_peer_t *slot = peer_find(BLE_HS_CONN_HANDLE_NONE);
    if (!slot) return;                       /* can't happen at 2 links max */
    slot->conn = conn;
    slot->subscribed = false;
}

static void peer_drop(uint16_t conn)
{
    nus_peer_t *p = peer_find(conn);
    if (p) { p->conn = BLE_HS_CONN_HANDLE_NONE; p->subscribed = false; }
}

static void tx_rb_drain(void);

/* Make `conn` the NUS owner. Resets the RX parser and drops frames queued
 * for the previous owner (they'd desync the new session — see tx_rb_drain).
 * Runs on the NimBLE host task, same as every other claimer. */
static void nus_claim(uint16_t conn)
{
    if (s_conn_handle == conn) return;
    ESP_LOGI(TAG, "NUS owner -> conn=%u", (unsigned)conn);
    s_conn_handle = conn;
    packet_parser_init(&s_rx_parser);
    tx_rb_drain();
    nus_peer_t *p = peer_find(conn);
    s_tx_subscribed = p && p->subscribed;
}

/* A LISP code transfer from the app (read/write/erase through this bridge)
 * is a long chain of round-trips whose READ replies are multi-frame CAN
 * transfers. The RT/LISP pollers produce their own multi-frame replies from
 * the same VESC id, and the comm_can reassembler holds only one in-flight
 * transfer per sender — the streams collide, frames drop on CRC, the app
 * retries into timeouts (reads took tens of seconds and often failed).
 * Same reasoning as pause_pollers() in vesc_lisp_code.c, but for the app's
 * path: pause on the first code-transfer packet, resume 3 s after the last
 * one (each packet re-arms the timer; erase gaps stay under 3 s because the
 * blocking erase reply itself precedes the next write). */
static esp_timer_handle_t s_poll_resume_timer;
static volatile bool      s_pollers_paused;

static void poll_resume_cb(void *arg)
{
    (void)arg;
    if (!s_pollers_paused) return;
    s_pollers_paused = false;
    vesc_rt_data_start();
    vesc_lisp_poll_start();
    /* io_data is re-enabled by the realtime screen itself; leave off. */
    ESP_LOGI(TAG, "LISP transfer idle — pollers resumed");
}

static void lisp_transfer_touch(void)
{
    if (!s_poll_resume_timer) {
        const esp_timer_create_args_t args = {
            .callback = poll_resume_cb,
            .name     = "nus_poll_resume",
        };
        if (esp_timer_create(&args, &s_poll_resume_timer) != ESP_OK) return;
    }
    if (!s_pollers_paused) {
        s_pollers_paused = true;
        vesc_rt_data_stop();
        vesc_lisp_poll_stop();
        vesc_io_data_set_active(false);
        ESP_LOGI(TAG, "LISP transfer active — pollers paused");
    }
    esp_timer_stop(s_poll_resume_timer);
    esp_timer_start_once(s_poll_resume_timer, 3 * 1000 * 1000);
}

static void rx_packet_complete(const uint8_t *payload, uint16_t len)
{
    if (len == 0) return;
    switch (payload[0]) {
    case COMM_LISP_READ_CODE:
    case COMM_LISP_WRITE_CODE:
    case COMM_LISP_ERASE_CODE:
        lisp_transfer_touch();
        break;
    default:
        break;
    }
    /* Per-command line — useful for first-bringup debugging, but VESC Tool
     * issues bursts of these on every screen open. Demoted to DEBUG so the
     * default INFO log stays readable. */
    ESP_LOGD(TAG, "BLE→CAN cmd 0x%02X len=%u", payload[0], (unsigned)len);
    /* send=0 — VESC controller replies via CAN; comm_can's RX task wraps
     * the response into PROCESS_RX_BUFFER and the handler in main.c
     * fans it back to ble_nus_forward_response. */
    comm_can_send_buffer((uint8_t)CONFIG_VESC_CAN_TARGET_ID,
                         payload, len, 0);
}

static int nus_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
    }

    /* Whoever writes VESC frames owns NUS — the CAN replies must go back to
     * this peer, not to whichever one happened to connect last. */
    nus_claim(conn_handle);

    /* VESC Tool negotiates ATT_MTU up to 512, so a single ATT_WRITE can
     * land here with up to MTU-3 = 509 bytes — and SET_MCCONF often comes
     * as a chain of those. The old 256 B stack buffer silently truncated
     * the tail (ble_hs_mbuf_to_flat does not signal overflow), which is
     * why writes died at the packet_parser CRC check while reads worked.
     * 1024 covers the longest plausible MTU + chain length; the packet
     * parser fans the bytes out one at a time so any chunking is fine. */
    uint8_t  buf[1024];
    uint16_t pkt_len = OS_MBUF_PKTLEN(ctxt->om);
    if (pkt_len > sizeof(buf)) {
        ESP_LOGW(TAG, "write too long for buf: %u > %u — dropping",
                 (unsigned)pkt_len, (unsigned)sizeof(buf));
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    uint16_t out_len = 0;
    int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &out_len);
    if (rc != 0) {
        ESP_LOGW(TAG, "mbuf_to_flat rc=%d", rc);
        return BLE_ATT_ERR_UNLIKELY;
    }

    ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, out_len, ESP_LOG_DEBUG);

    for (uint16_t i = 0; i < out_len; i++) {
        packet_parser_process_byte(&s_rx_parser, buf[i], rx_packet_complete);
    }
    return 0;
}

static const struct ble_gatt_svc_def s_nus_svcs[] = {
    {
        .type            = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid            = &NUS_SVC_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid       = &NUS_RX_UUID.u,
                .access_cb  = nus_access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid       = &NUS_TX_UUID.u,
                .access_cb  = nus_access_cb,
                .flags      = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_tx_val_handle,
            },
            { 0 },
        },
    },
    { 0 },
};

const struct ble_gatt_svc_def *ble_nus_get_svcs(void)
{
    return s_nus_svcs;
}

void ble_nus_gatts_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    (void)arg;
    if (ctxt->op == BLE_GATT_REGISTER_OP_CHR) {
        char uuid_buf[BLE_UUID_STR_LEN];
        ble_uuid_to_str(ctxt->chr.chr_def->uuid, uuid_buf);
        ESP_LOGI(TAG, "registered char %s val_handle=%u",
                 uuid_buf, (unsigned)ctxt->chr.val_handle);
    }
}

void ble_nus_on_connect(uint16_t conn_handle)
{
    peer_add(conn_handle);
    ESP_LOGI(TAG, "peer connected, conn=%u", (unsigned)conn_handle);
    /* First peer takes NUS by default; a later peer only takes it over by
     * actually talking to it (RX write or TX subscribe). */
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE) nus_claim(conn_handle);
}

/* Drop everything currently queued for a peer that just left. Otherwise
 * the next connect would receive stale frames meant for the previous
 * session — VESC Tool would mis-parse them as responses to its first
 * handshake and disconnect with "version mismatch". */
static void tx_rb_drain(void)
{
    if (!s_tx_rb) return;
    while (1) {
        size_t sz = 0;
        void *p = xRingbufferReceive(s_tx_rb, &sz, 0);
        if (!p) break;
        vRingbufferReturnItem(s_tx_rb, p);
    }
}

void ble_nus_on_disconnect(uint16_t conn_handle)
{
    peer_drop(conn_handle);
    if (conn_handle != s_conn_handle) {
        ESP_LOGI(TAG, "idle peer disconnected, conn=%u", (unsigned)conn_handle);
        return;
    }
    ESP_LOGI(TAG, "owner disconnected, conn=%u", (unsigned)conn_handle);
    s_conn_handle  = BLE_HS_CONN_HANDLE_NONE;
    s_tx_subscribed = false;
    tx_rb_drain();
    /* Peer vanished mid-transfer — don't leave the dashboard pollers off
     * for the rest of the timer window. */
    if (s_poll_resume_timer) esp_timer_stop(s_poll_resume_timer);
    poll_resume_cb(NULL);
}

void ble_nus_on_subscribe(uint16_t conn_handle, uint16_t attr_handle,
                          bool cur_notify)
{
    /* Only the NUS TX characteristic gates forwarding; ignore CCCD writes on
     * NotifBridge characteristics (the phone subscribes to those, not NUS). */
    if (s_tx_val_handle == 0 || attr_handle != s_tx_val_handle) return;
    peer_add(conn_handle);
    nus_peer_t *p = peer_find(conn_handle);
    if (p) p->subscribed = cur_notify;
    /* Subscribing to TX is how VESC Tool announces itself — take NUS over. */
    if (cur_notify) nus_claim(conn_handle);
    if (conn_handle == s_conn_handle) s_tx_subscribed = cur_notify;
    ESP_LOGI(TAG, "NUS TX notifications %s, conn=%u",
             cur_notify ? "enabled" : "disabled", (unsigned)conn_handle);
}

/* Notify one chunk, retrying on transient NimBLE pool pressure. Returns
 * 0 on success, NimBLE rc on permanent failure (peer gone, etc.). The
 * mbuf is consumed by NimBLE on success AND on error — caller must not
 * touch it after this returns. */
static int notify_chunk_with_retry(const uint8_t *data, uint16_t len)
{
    /* Up to ~250 ms of retries — covers a full burst of MTU-sized
     * packets emptying onto the link. 5 ms is one connection interval
     * at 7.5 ms minimum; longer waits don't buy anything because the
     * controller drains mbufs at the link-layer rate, not faster. */
    const int max_attempts = 50;
    for (int i = 0; i < max_attempts; i++) {
        if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE) return BLE_HS_ENOTCONN;
        struct os_mbuf *txom = ble_hs_mbuf_from_flat(data, len);
        if (!txom) {
            /* mbuf pool empty — wait for controller to drain. */
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        int rc = ble_gatts_notify_custom(s_conn_handle, s_tx_val_handle, txom);
        if (rc == 0) return 0;
        /* BLE_HS_ENOMEM (6) is the typical "queue full" reply from
         * NimBLE; treat ESTALLED / EBUSY / EAGAIN identically — they
         * all clear once the controller transmits the next interval. */
        if (rc == BLE_HS_ENOMEM || rc == BLE_HS_EBUSY) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        /* Anything else (ENOTCONN, EINVAL, …) is permanent for this
         * frame — bail out so the TX task can move on. */
        return rc;
    }
    return BLE_HS_ETIMEOUT;
}

static void nus_tx_task(void *arg)
{
    (void)arg;
    while (1) {
        size_t framed_len = 0;
        uint8_t *framed = (uint8_t *)xRingbufferReceive(s_tx_rb, &framed_len,
                                                        portMAX_DELAY);
        if (!framed) continue;

        /* Snapshot the peer state under no lock — s_conn_handle is a
         * plain word write from the GAP task and a 32-bit read here is
         * atomic on Xtensa. If the peer is gone, drop the item. */
        if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE || s_tx_val_handle == 0) {
            vRingbufferReturnItem(s_tx_rb, framed);
            continue;
        }

        /* ble_att_mtu() returns 23 (BLE_ATT_MTU_DFLT) until MTU exchange
         * completes — re-read every batch so once VESC Tool negotiates
         * 247-byte MTU we instantly switch to bigger chunks. */
        uint16_t mtu = ble_att_mtu(s_conn_handle);
        if (mtu < 23) mtu = 23;
        uint16_t chunk = (uint16_t)(mtu - 3);

        size_t off = 0;
        while (off < framed_len) {
            uint16_t this_chunk = (uint16_t)((framed_len - off > chunk)
                                                 ? chunk : (framed_len - off));
            int rc = notify_chunk_with_retry(framed + off, this_chunk);
            if (rc != 0) {
                /* A flaky link makes every frame time out here; CAN keeps
                 * feeding us, so without a gate this floods the console and
                 * the PSRAM log ring (pushing out useful history). Aggregate
                 * to one line/sec with a dropped-frame count. */
                static int64_t  s_last_us;
                static unsigned s_dropped;
                s_dropped++;
                int64_t now = esp_timer_get_time();
                if (now - s_last_us > 1000000) {
                    ESP_LOGW(TAG, "notify rc=%d off=%u/%u — dropped %u frame(s) in last %lld ms",
                             rc, (unsigned)off, (unsigned)framed_len, s_dropped,
                             s_last_us ? (now - s_last_us) / 1000 : 0);
                    s_last_us = now;
                    s_dropped = 0;
                }
                break;
            }
            off += this_chunk;
        }

        vRingbufferReturnItem(s_tx_rb, framed);
    }
}

void ble_nus_init(void)
{
    if (s_tx_rb) return;
    s_tx_rb = xRingbufferCreate(NUS_TX_RB_SIZE, RINGBUF_TYPE_NOSPLIT);
    if (!s_tx_rb) {
        ESP_LOGE(TAG, "tx ringbuf alloc failed (%d B)", NUS_TX_RB_SIZE);
        return;
    }
    /* prio 5 — same band as the CAN RX task; we don't want this to
     * preempt the system but it must drain faster than 10 ms granularity
     * to keep VESC Tool's parser happy. Pinned to core 0 so all BLE work
     * stays on one core (NimBLE host task is also core 0). */
    xTaskCreatePinnedToCore(nus_tx_task, "ble_nus_tx", 4096, NULL, 5,
                            &s_tx_task, 0);
    ESP_LOGI(TAG, "tx task up (rb=%d B)", NUS_TX_RB_SIZE);
}

void ble_nus_forward_response(const uint8_t *data, uint16_t len)
{
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE || s_tx_val_handle == 0) {
        return;
    }
    /* Nobody listening on NUS TX (e.g. a notifications-only phone). Drop
     * silently — otherwise the continuous CAN RT-poll stream gets queued and
     * notified into the void, flooding notify_tx errors and back-pressuring
     * this very call path (the CAN dispatch task). */
    if (!s_tx_subscribed) return;
    if (len == 0) return;
    if (!s_tx_rb) {
        ESP_LOGW(TAG, "forward called before init — dropping %u B", len);
        return;
    }

    /* Frame the payload (start byte + len + crc + end). 1030 covers the
     * 1024-byte parser cap + 6 B framing overhead (long-header START + 2
     * len + 2 crc + END). Must stay in sync with PACKET_PARSER_MAX_PAYLOAD. */
    uint8_t  framed[1030];
    uint16_t framed_len = packet_build_frame(data, len, framed, sizeof(framed));
    if (framed_len == 0) return;

    /* 10 ms timeout — keeps the CAN RX task moving even if BLE is
     * completely stuck; one lost frame is better than missing all the
     * subsequent CAN traffic on the bus. Most calls return instantly
     * because the ringbuf has plenty of slack. */
    BaseType_t ok = xRingbufferSend(s_tx_rb, framed, framed_len,
                                    pdMS_TO_TICKS(10));
    if (ok != pdTRUE) {
        ESP_LOGW(TAG, "tx ringbuf full — dropping %u B reply", framed_len);
    }
}
