#include "ble_nus.h"

#include <string.h>

#include "esp_log.h"
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
static packet_parser_t s_rx_parser;

static void rx_packet_complete(const uint8_t *payload, uint16_t len)
{
    if (len == 0) return;
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
    s_conn_handle = conn_handle;
    packet_parser_init(&s_rx_parser);
    ESP_LOGI(TAG, "peer connected, conn=%u", (unsigned)conn_handle);
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

void ble_nus_on_disconnect(void)
{
    ESP_LOGI(TAG, "peer disconnected");
    s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    tx_rb_drain();
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
                ESP_LOGW(TAG, "notify rc=%d off=%u/%u — dropping rest of frame",
                         rc, (unsigned)off, (unsigned)framed_len);
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
