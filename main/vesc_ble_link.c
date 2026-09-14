/*
    Copyright 2026 ESP32-P4 Android Auto / VESC Display (GPL-3.0).

    See vesc_ble_link.h.

    Two tasks, and the reason for both is that a reply arrives on the NimBLE
    host task: anything that blocks waiting for one must not itself run there.

      vesc_ble_tx  drains the request queue, one request at a time: wrap,
                   frame, write, wait for the answer, release the caller.
                   Every producer (the poll task, the LVGL/HTTP config path,
                   the phone bridge on the host task) only enqueues.
      vesc_ble_rx  turns notification bytes into packets and hands them to the
                   shared dispatcher. Parsing off the host task keeps the deep
                   fan-out (config parsing, the phone ring buffer) out of a
                   4 KiB stack that the whole stack shares.

    The third thing here is addressing, and it is the one that decides whether
    the dashboard shows anything at all. Two kinds of device answer to the same
    Nordic UART Service:

      * the controller itself — a BLE module wired to the VESC's UART, or a
        VESC with BLE on board. Its payloads go out exactly as written.
      * an adapter in front of it — a VESC Express on the CAN bus. Reaching the
        controller through one means wrapping every payload in COMM_FORWARD_CAN
        plus the target id, the way VESC Tool does after "scan CAN bus".

    Send a wrapped payload straight to a controller and it forwards it onto CAN
    addressed to itself, which nothing on the bus answers: the link connects,
    every poll times out, the screen stays empty and nothing in the log says
    why. So instead of a setting the user has to get right, the link asks —
    vesc_ble_link_probe_loop() below — and routes per target afterwards.
*/

#include "vesc_ble_link.h"

#include "ble_vesc_client.h"
#include "dev_settings.h"

#include "vesc_can/buffer.h"
#include "vesc_can/packet_parser.h"
#include "vesc_can/vesc_datatypes.h"
#include "vesc_can/vesc_link.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "vesc_ble";

#define NVS_NS       "vescble"
#define NVS_ADAPTER  "adapter"        /* [addr_type][addr 6] */

/* Biggest VESC payload we ever send is a SET_MCCONF (600-800 B on FW 6.x).
 * The frame builder caps at PACKET_PARSER_MAX_PAYLOAD, and the FORWARD_CAN
 * wrapper costs two bytes on top of the caller's payload — so the caller's
 * limit is two below the frame's, or a full-size config write would be
 * accepted here and then silently refused by packet_build_frame. */
#define TX_PAYLOAD_MAX  (PACKET_PARSER_MAX_PAYLOAD - 2)
#define TX_FRAME_MAX    (PACKET_PARSER_MAX_PAYLOAD + 8)

#define TX_RB_SIZE      4096
#define RX_RB_SIZE      4096

/* ---- request queue ---- */

typedef struct {
    uint8_t  target;
    uint8_t  send_mode;
    uint8_t  wait;          /* a caller is blocked in send_sync */
    uint8_t  wrap;          /* send through COMM_FORWARD_CAN */
    uint16_t timeout_ms;
    uint16_t len;
    int64_t  queued_us;
    /* payload follows */
} tx_hdr_t;

/* How stale a fire-and-forget command may get in the queue before sending it
 * does more harm than dropping it.
 *
 * send_mode 3 has exactly one user: the pedal-assist current setpoint, resent
 * at 20 Hz (vesc_lisp_panel.c). It is a motor command with a newer value right
 * behind it, so if the link stalls — a reply timeout ahead of it in the queue
 * costs up to CONFIG_VESC_BLE_SYNC_TIMEOUT_MS — flushing the backlog would
 * feed the VESC a burst of setpoints the rider asked for a second ago. The
 * script's own 0.4 s staleness window then coasts the motor, which is the
 * correct outcome, so dropping is both safer and closer to what CAN did: a
 * frame that could not go out was simply lost. */
#define FIRE_AND_FORGET_STALE_MS 100

static RingbufHandle_t   s_tx_rb;
static TaskHandle_t      s_tx_task;
static TaskHandle_t      s_rx_task;
static RingbufHandle_t   s_rx_rb;

/* Given by the RX task when a reply lands, taken by the TX task. */
static SemaphoreHandle_t s_reply_sem;
/* Given by the TX task when the request a caller is blocked on is done. */
static SemaphoreHandle_t s_caller_sem;
static volatile bool     s_awaiting;     /* a reply is expected right now */

static packet_parser_t  *s_parser;
static uint8_t          *s_frame;        /* TX scratch, owned by the TX task */

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t     s_timeouts;
static uint32_t     s_stale_drops;
static uint32_t     s_last_rt_ms;
static uint32_t     s_requests;
static uint32_t     s_replies;
static bool         s_paired;
static uint8_t      s_addr[6];
static uint8_t      s_addr_type;

/* ---- who is on the other end ----
 *
 * Set by the probe below, cleared whenever the link drops. AUTO routing reads
 * it on every request; the VESC link screen shows it so a wrong manual setting
 * is visible rather than merely fatal. */
static volatile vesc_ble_peer_t s_peer = VESC_BLE_PEER_UNKNOWN;
static volatile uint8_t         s_peer_id;        /* valid when peer == CONTROLLER */
static volatile uint8_t         s_probe_tries;

/* The reply the probe is waiting for, matched in on_packet(). A late reply to
 * a request that already timed out can carry the same command byte, so the
 * SETUP_SELECTIVE probe also checks that the echoed mask is its own. */
#define PROBE_MASK_VESC_ID  (1U << 17)   /* GET_VALUES_SETUP_SELECTIVE bit 17 */
#define PROBE_TIMEOUT_MS    500
#define PROBE_RETRY_MS      1000
/* Once the link has been declared silent, keep asking — a controller can boot
 * after the display — but stop spending a second of the poll task on it every
 * second to do so. */
#define PROBE_RETRY_SLOW_MS 5000

static volatile bool    s_probe_wait;
static volatile uint8_t s_probe_cmd;
static volatile bool    s_probe_answered;
static volatile bool    s_probe_id_valid;
static volatile uint8_t s_probe_id;
/* Milliseconds, not the raw 64-bit timer: the settings screen clears this
 * from the LVGL thread while the poll task reads it, and a torn 64-bit write
 * would park the next attempt somewhere in the far future. */
static volatile uint32_t s_probe_next_ms;

/* ---- deferred NVS writer (same reason as speed_sensor.c: a commit costs
 * ~100 ms and the picker taps arrive on the LVGL thread) ---- */

typedef struct {
    bool    erase;
    uint8_t blob[7];
} save_msg_t;

static QueueHandle_t s_save_q;

static void save_task(void *arg)
{
    (void)arg;
    save_msg_t m;
    for (;;) {
        if (xQueueReceive(s_save_q, &m, portMAX_DELAY) != pdTRUE) continue;
        nvs_handle_t h;
        if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
            ESP_LOGW(TAG, "NVS open failed — save skipped");
            continue;
        }
        if (m.erase) {
            nvs_erase_key(h, NVS_ADAPTER);
        } else {
            nvs_set_blob(h, NVS_ADAPTER, m.blob, sizeof(m.blob));
        }
        nvs_commit(h);
        nvs_close(h);
    }
}

static void save_enqueue(const save_msg_t *m)
{
    if (!s_save_q || xQueueSend(s_save_q, m, 0) != pdTRUE) {
        ESP_LOGW(TAG, "save queue unavailable — adapter not persisted");
    }
}

static void load_adapter(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return;
    uint8_t blob[7];
    size_t  n = sizeof(blob);
    if (nvs_get_blob(h, NVS_ADAPTER, blob, &n) == ESP_OK && n == sizeof(blob)) {
        s_addr_type = blob[0];
        memcpy(s_addr, &blob[1], 6);
        s_paired = true;
    }
    nvs_close(h);
}

/* ---- RX: notification bytes -> packets ---- */

/* Is this the answer the probe is blocked on? The id probe additionally checks
 * the echoed mask, because a reply to an earlier poll that already timed out
 * carries the same command byte and would otherwise be read as a node id.
 * Returns true when the packet belongs to the probe and nothing else should
 * see it. */
static bool probe_capture(const uint8_t *data, uint16_t len)
{
    if (!s_probe_wait || data[0] != s_probe_cmd) return false;

    if (data[0] == COMM_GET_VALUES_SETUP_SELECTIVE) {
        if (len < 6) return false;
        int32_t  ind  = 1;
        uint32_t mask = buffer_get_uint32(data, &ind);
        if (mask != PROBE_MASK_VESC_ID) return false;   /* someone else's reply */
        s_probe_id       = data[ind];
        s_probe_id_valid = true;
        s_probe_answered = true;
        /* Swallowed: a mask-only frame would reach the RT parser, which would
         * stamp it as fresh data and put an empty dashboard on the screen. */
        return true;
    }

    /* COMM_FW_VERSION — only asked to find out whether anything at all is
     * listening, so let it through to vesc_config as well. */
    s_probe_answered = true;
    return false;
}

static void on_packet(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;

    /* Unsolicited output from the running LISP script arrives whenever the
     * script prints, including between a request and its reply — completing
     * the wait on one of these would let the next request go out while the
     * real answer is still in flight, and every reply after that would be
     * matched to the wrong request. */
    bool is_async = (data[0] == COMM_LISP_PRINT || data[0] == COMM_PRINT);

    bool mine = probe_capture(data, len);
    if (!mine) vesc_link_deliver(data, len);

    if (!is_async && s_awaiting) {
        portENTER_CRITICAL(&s_mux);
        s_replies++;
        portEXIT_CRITICAL(&s_mux);
        xSemaphoreGive(s_reply_sem);
    }
}

/* NimBLE host task — copy out and leave. */
static void on_notify(const uint8_t *data, uint16_t len)
{
    if (!s_rx_rb || len == 0) return;
    if (xRingbufferSend(s_rx_rb, data, len, 0) != pdTRUE) {
        ESP_LOGW(TAG, "RX ring full — %u bytes dropped", (unsigned)len);
    }
}

static void rx_task(void *arg)
{
    (void)arg;
    for (;;) {
        size_t   sz = 0;
        uint8_t *b  = xRingbufferReceive(s_rx_rb, &sz, portMAX_DELAY);
        if (!b) continue;
        for (size_t i = 0; i < sz; i++) {
            packet_parser_process_byte(s_parser, b[i], on_packet);
        }
        vRingbufferReturnItem(s_rx_rb, b);
    }
}

/* ---- TX: one request at a time ---- */

static void tx_task(void *arg)
{
    (void)arg;
    for (;;) {
        size_t    sz = 0;
        tx_hdr_t *it = xRingbufferReceive(s_tx_rb, &sz, portMAX_DELAY);
        if (!it) continue;

        const uint8_t *payload = (const uint8_t *)(it + 1);
        uint16_t       plen    = it->len;
        bool           wait    = it->wait;
        uint8_t        mode    = it->send_mode;
        uint16_t       tmo     = it->timeout_ms;
        uint8_t        target  = it->target;
        bool           wrap    = it->wrap;

        if (mode == 3 &&
            (esp_timer_get_time() - it->queued_us) >
                (int64_t)FIRE_AND_FORGET_STALE_MS * 1000) {
            portENTER_CRITICAL(&s_mux);
            s_stale_drops++;
            portEXIT_CRITICAL(&s_mux);
            vRingbufferReturnItem(s_tx_rb, it);
            continue;
        }

        /* Reach a controller behind the adapter the way VESC Tool does. The
         * reply comes back unwrapped, so nothing downstream has to know.
         * Whether to wrap was decided when the request was queued — see
         * route_wraps(): it depends on which node this is addressed to, and
         * the probe may resolve between queueing and sending. */
        uint8_t *body = s_frame + TX_FRAME_MAX;   /* second half = wrap buffer */
        uint16_t blen;
        if (wrap) {
            body[0] = COMM_FORWARD_CAN;
            body[1] = target;
            memcpy(body + 2, payload, plen);
            blen = (uint16_t)(plen + 2);
        } else {
            memcpy(body, payload, plen);
            blen = plen;
        }

        uint16_t flen = packet_build_frame(body, blen, s_frame, TX_FRAME_MAX);
        vRingbufferReturnItem(s_tx_rb, it);
        if (flen == 0) {
            ESP_LOGW(TAG, "payload of %u B does not fit a frame", (unsigned)blen);
            if (wait) xSemaphoreGive(s_caller_sem);
            continue;
        }

        /* Drop any reply left over from a request that timed out — it belongs
         * to nobody now, and counting it would satisfy the next wait early. */
        xSemaphoreTake(s_reply_sem, 0);

        int64_t t0 = esp_timer_get_time();
        s_awaiting = (mode != 3);
        portENTER_CRITICAL(&s_mux);
        s_requests++;
        portEXIT_CRITICAL(&s_mux);
        int rc = ble_vesc_write(s_frame, flen);

        if (rc == 0 && mode != 3) {
            if (xSemaphoreTake(s_reply_sem, pdMS_TO_TICKS(tmo)) == pdTRUE) {
                uint32_t rt = (uint32_t)((esp_timer_get_time() - t0) / 1000);
                portENTER_CRITICAL(&s_mux);
                s_last_rt_ms = rt;
                portEXIT_CRITICAL(&s_mux);
            } else {
                portENTER_CRITICAL(&s_mux);
                s_timeouts++;
                portEXIT_CRITICAL(&s_mux);
            }
        }
        s_awaiting = false;

        if (wait) xSemaphoreGive(s_caller_sem);
    }
}

static void enqueue(uint8_t target, const uint8_t *data, unsigned int len,
                    uint8_t send_mode, uint32_t timeout_ms, bool wait, bool wrap)
{
    if (!s_tx_rb || len == 0) return;
    if (len > TX_PAYLOAD_MAX) {
        ESP_LOGW(TAG, "payload of %u B is too large to send", len);
        return;
    }

    uint8_t  buf[sizeof(tx_hdr_t) + 256];
    size_t   need = sizeof(tx_hdr_t) + len;
    uint8_t *item = buf;
    uint8_t *heap = NULL;
    if (need > sizeof(buf)) {
        heap = heap_caps_malloc(need, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!heap) return;
        item = heap;
    }

    tx_hdr_t *h = (tx_hdr_t *)item;
    h->target     = target;
    h->send_mode  = send_mode;
    h->wait       = wait ? 1 : 0;
    h->wrap       = wrap ? 1 : 0;
    h->timeout_ms = (uint16_t)(timeout_ms > UINT16_MAX ? UINT16_MAX : timeout_ms);
    h->len        = (uint16_t)len;
    h->queued_us  = esp_timer_get_time();
    memcpy(item + sizeof(tx_hdr_t), data, len);

    /* Never block a producer: the phone bridge calls this from the NimBLE host
     * task, which is also where replies arrive. A full queue means the link is
     * already behind — dropping the request is what a dropped CAN frame would
     * have been. */
    if (xRingbufferSend(s_tx_rb, item, need, 0) != pdTRUE) {
        ESP_LOGW(TAG, "TX ring full — request 0x%02X dropped", data[0]);
        if (wait) xSemaphoreGive(s_caller_sem);
    }
    if (heap) heap_caps_free(heap);
}

/* ---- addressing ----
 *
 * Per target, not per link: even when we are attached straight to a
 * controller, a second head elsewhere on its CAN bus still has to be reached
 * through COMM_FORWARD_CAN. */
static bool route_wraps(uint8_t target)
{
    if (target == 0) return false;

    switch (settings_get_vesc_ble_route()) {
    case VESC_BLE_ROUTE_DIRECT: return false;
    case VESC_BLE_ROUTE_CAN:    return true;
    default:                    break;          /* AUTO */
    }

    switch (s_peer) {
    case VESC_BLE_PEER_CONTROLLER:
        /* Talking to the node we are physically attached to: no detour. */
        return target != s_peer_id;
    case VESC_BLE_PEER_BRIDGE:
        /* Speaks the protocol but has no motor — everything goes on to CAN. */
        return true;
    default:
        /* Not probed yet, or nothing answers. Unwrapped is what the probe
         * itself uses, and it is the only one of the two that can ever tell
         * us which it should have been. */
        return false;
    }
}

/* ---- vesc_link backend ---- */

/* The one waiter slot is shared by every sync caller, and that is safe only
 * because they all live on the same VESC poll task — the periodic polls and
 * the probe alike. A second concurrent waiter would be served the wrong
 * completion. */
static void send_sync_raw(uint8_t target, const uint8_t *data, unsigned int len,
                          uint8_t send_mode, uint32_t timeout_ms, bool wrap)
{
    if (!s_caller_sem) return;
    xSemaphoreTake(s_caller_sem, 0);            /* drop a stale completion */
    enqueue(target, data, len, send_mode, timeout_ms, true, wrap);
    xSemaphoreTake(s_caller_sem, pdMS_TO_TICKS(timeout_ms + 100));
}

static void ops_send(uint8_t target, const uint8_t *data, unsigned int len,
                     uint8_t send_mode)
{
    enqueue(target, data, len, send_mode, CONFIG_VESC_BLE_SYNC_TIMEOUT_MS, false,
            route_wraps(target));
}

static void ops_send_sync(uint8_t target, const uint8_t *data, unsigned int len,
                          uint8_t send_mode, uint32_t timeout_ms)
{
    send_sync_raw(target, data, len, send_mode, timeout_ms, route_wraps(target));
}

static bool ops_is_up(void)
{
    return ble_vesc_is_ready();
}

static bool ops_health_text(char *buf, size_t n)
{
    vesc_ble_telem_t t;
    vesc_ble_link_get_telem(&t);
    if (!t.bound) {
        snprintf(buf, n, "BLE: no adapter paired");
    } else if (t.ready && t.peer == VESC_BLE_PEER_SILENT) {
        snprintf(buf, n, "BLE: connected, adapter answers nothing");
    } else if (t.ready) {
        snprintf(buf, n, "BLE: connected, %u ms round trip\nDrops: %lu",
                 (unsigned)t.last_rt_ms, (unsigned long)t.reconnects);
    } else if (t.connected) {
        snprintf(buf, n, "BLE: connected, no service yet");
    } else {
        snprintf(buf, n, "BLE: reconnecting...");
    }
    return true;
}

/* ---- who is on the other end ----
 *
 * Runs on the VESC poll task, so it may block on a reply; it is registered as
 * an aux loop next to the second head's poll (main.c). Two questions, asked
 * unwrapped because that is the only form a controller can answer directly:
 *
 *   1. "which CAN id are you?" — GET_VALUES_SETUP_SELECTIVE with nothing but
 *      the node-id bit. Only a motor controller answers, and the answer is
 *      what per-target routing needs.
 *   2. "are you there at all?" — FW_VERSION, which a VESC Express answers for
 *      itself. It separates "an adapter in front of the controller" from "a
 *      link that carries nothing", and those need very different fixes.
 *
 * The result latches until the link drops. Re-asked once a second while
 * unresolved, so a controller that boots after the display still gets found. */
static void probe_run(void)
{
    uint8_t buf[8];
    int32_t ind = 0;
    buf[ind++] = COMM_GET_VALUES_SETUP_SELECTIVE;
    buffer_append_uint32(buf, PROBE_MASK_VESC_ID, &ind);

    s_probe_answered = false;
    s_probe_id_valid = false;
    s_probe_cmd      = COMM_GET_VALUES_SETUP_SELECTIVE;
    s_probe_wait     = true;
    send_sync_raw(settings_get_target_vesc_id(), buf, (unsigned)ind, 0,
                  PROBE_TIMEOUT_MS, false);
    s_probe_wait = false;

    if (s_probe_id_valid) {
        s_peer_id = s_probe_id;
        s_peer    = VESC_BLE_PEER_CONTROLLER;
        ESP_LOGW(TAG, "attached device is VESC node %u — requests to it go out "
                      "unwrapped, anything else through COMM_FORWARD_CAN",
                 (unsigned)s_peer_id);
        return;
    }

    /* One lost reply should not latch the wrong answer for the rest of the
     * session: a controller that misses the first id probe gets a second go
     * before we decide it must be an adapter. */
    if (s_probe_tries < 2) return;

    uint8_t fw = COMM_FW_VERSION;
    s_probe_answered = false;
    s_probe_cmd      = COMM_FW_VERSION;
    s_probe_wait     = true;
    send_sync_raw(0, &fw, 1, 0, PROBE_TIMEOUT_MS, false);
    s_probe_wait = false;

    if (s_probe_answered) {
        s_peer = VESC_BLE_PEER_BRIDGE;
        ESP_LOGW(TAG, "attached device answers but is not a controller — "
                      "wrapping requests in COMM_FORWARD_CAN");
        return;
    }

    /* Nothing at all. Leave it unresolved for the first few tries (a VESC that
     * boots with the display, a module that needs a moment after the CCCD
     * write), then say so — the screen reads it out of the telemetry. */
    if (s_probe_tries >= 5) {
        s_peer = VESC_BLE_PEER_SILENT;
        ESP_LOGE(TAG, "adapter accepted %u requests and answered none — check "
                      "the module's baud rate and the VESC's UART app",
                 (unsigned)s_probe_tries);
    }
}

void vesc_ble_link_probe_loop(void)
{
    if (vesc_link_get_mode() != VESC_LINK_MODE_BLE) return;

    if (!ble_vesc_is_ready()) {
        /* Link down: whatever we learned belonged to that connection. */
        s_peer          = VESC_BLE_PEER_UNKNOWN;
        s_probe_tries   = 0;
        s_probe_next_ms = 0;
        return;
    }
    if (s_peer == VESC_BLE_PEER_CONTROLLER || s_peer == VESC_BLE_PEER_BRIDGE) {
        return;                                  /* settled */
    }

    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    if (s_probe_next_ms && (int32_t)(now - s_probe_next_ms) < 0) return;
    s_probe_next_ms = now + (s_peer == VESC_BLE_PEER_SILENT ? PROBE_RETRY_SLOW_MS
                                                            : PROBE_RETRY_MS);
    if (s_probe_tries < 255) s_probe_tries++;

    probe_run();
}

void vesc_ble_link_route_changed(void)
{
    /* AUTO re-asks; the manual modes do not care, but clearing is still right:
     * the next switch back to AUTO must not reuse a stale answer. */
    s_peer          = VESC_BLE_PEER_UNKNOWN;
    s_probe_tries   = 0;
    s_probe_next_ms = 0;
}

static const vesc_link_ops_t s_ops = {
    .send        = ops_send,
    .send_sync   = ops_send_sync,
    .is_up       = ops_is_up,
    .health_text = ops_health_text,
};

/* ---- public API ---- */

void vesc_ble_link_init(void)
{
    if (s_tx_task) return;

    s_parser = heap_caps_malloc(sizeof(packet_parser_t),
                                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_frame  = heap_caps_malloc(TX_FRAME_MAX * 2,
                                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_parser || !s_frame) {
        ESP_LOGE(TAG, "out of memory — BLE link unavailable");
        return;
    }
    packet_parser_init(s_parser);

    s_reply_sem  = xSemaphoreCreateBinary();
    s_caller_sem = xSemaphoreCreateBinary();
    s_tx_rb      = xRingbufferCreate(TX_RB_SIZE, RINGBUF_TYPE_NOSPLIT);
    s_rx_rb      = xRingbufferCreate(RX_RB_SIZE, RINGBUF_TYPE_NOSPLIT);
    s_save_q     = xQueueCreate(4, sizeof(save_msg_t));
    if (!s_reply_sem || !s_caller_sem || !s_tx_rb || !s_rx_rb || !s_save_q) {
        ESP_LOGE(TAG, "queue allocation failed — BLE link unavailable");
        return;
    }

    load_adapter();

    ble_vesc_set_rx_cb(on_notify);
    /* Prio 8 to match the CAN decode task this replaces; the dispatcher it
     * feeds is the same one. */
    xTaskCreatePinnedToCore(rx_task, "vesc_ble_rx", 4096, NULL, 8, &s_rx_task, 0);
    xTaskCreatePinnedToCore(tx_task, "vesc_ble_tx", 4096, NULL, 5, &s_tx_task, 0);
    xTaskCreatePinnedToCore(save_task, "vesc_ble_sav", 3072, NULL, 2, NULL, 0);

    vesc_link_register_ble(&s_ops);

    if (s_paired) {
        ESP_LOGI(TAG, "adapter %02X:%02X:%02X:%02X:%02X:%02X paired",
                 s_addr[5], s_addr[4], s_addr[3], s_addr[2], s_addr[1], s_addr[0]);
    } else {
        ESP_LOGI(TAG, "no adapter paired yet");
    }
}

void vesc_ble_link_start(void)
{
    if (!s_tx_task) return;
    if (s_paired) {
        ble_vesc_bind(s_addr, s_addr_type);
    } else {
        ESP_LOGW(TAG, "BLE link selected but no adapter paired — "
                      "pick one in Settings");
    }
}

void vesc_ble_link_stop(void)
{
    if (!s_tx_task) return;
    ble_vesc_disconnect();
    packet_parser_reset(s_parser);
    vesc_ble_link_route_changed();   /* forget who the old link was */
}

void vesc_ble_link_select(const uint8_t addr[6], uint8_t addr_type)
{
    portENTER_CRITICAL(&s_mux);
    memcpy(s_addr, addr, 6);
    s_addr_type = addr_type;
    s_paired    = true;
    portEXIT_CRITICAL(&s_mux);

    save_msg_t m = { .erase = false };
    m.blob[0] = addr_type;
    memcpy(&m.blob[1], addr, 6);
    save_enqueue(&m);

    vesc_ble_link_route_changed();   /* a different adapter, a fresh question */
    ble_vesc_bind(addr, addr_type);
}

void vesc_ble_link_forget(void)
{
    portENTER_CRITICAL(&s_mux);
    s_paired = false;
    portEXIT_CRITICAL(&s_mux);

    save_msg_t m = { .erase = true };
    save_enqueue(&m);

    ble_vesc_forget();
}

void vesc_ble_link_get_telem(vesc_ble_telem_t *out)
{
    if (!out) return;
    ble_vesc_state_t st = { 0 };
    ble_vesc_get(&st);
    out->bound      = st.bound || s_paired;
    out->connected  = st.connected;
    out->ready      = st.ready;
    out->scanning   = st.scanning;
    out->mtu        = st.mtu;
    out->reconnects = st.reconnects;
    portENTER_CRITICAL(&s_mux);
    out->timeouts    = s_timeouts;
    out->stale_drops = s_stale_drops;
    out->last_rt_ms  = s_last_rt_ms;
    out->requests    = s_requests;
    out->replies     = s_replies;
    portEXIT_CRITICAL(&s_mux);
    out->peer        = s_peer;
    out->peer_id     = s_peer_id;
    out->probe_tries = s_probe_tries;
    out->wrapping    = route_wraps(settings_get_target_vesc_id());
}
