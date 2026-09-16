/* Navigator frames from the phone — see ble_nav.h for the wire protocol.
 *
 * Shape follows ble_ota.c: the NimBLE host callback only validates and
 * memcpys, everything slow (JPEG decode, PPA scale) and every notification
 * happens on a dedicated worker task. Notifications must never be sent from
 * inside a GATT access callback. */

#include "ble_nav.h"

#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "esp_cache.h"
#include "esp_private/esp_cache_private.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"

#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_mbuf.h"
#include "os/os_mbuf.h"

#include "ble_link_boost.h"
#include "nav_map.h"
#include "nav_route.h"
#include "nav_screen.h"
#include "nav_tiles.h"
#include "nav_tile_store.h"
#include "ui_mode.h"

static const char *TAG = "ble_nav";

/* ---- wire protocol constants (mirror Dart) ---- */
#define NAV_OP_BEGIN   0x01
#define NAV_OP_END     0x02
#define NAV_OP_STOP    0x03
#define NAV_OP_HELLO   0x04
#define NAV_OP_TILE_BEGIN 0x05
#define NAV_OP_TILE_END   0x06
#define NAV_OP_VIEW       0x07
#define NAV_OP_ROUTE_BEGIN 0x08
#define NAV_OP_ROUTE_END   0x09
#define NAV_OP_GUIDE       0x0A
#define NAV_OP_FOUND_BEGIN 0x0B
#define NAV_OP_FOUND_END   0x0C

#define NAV_ST_STATE   0x10
#define NAV_ST_ACK     0x11
#define NAV_ST_TILE_ACK 0x12
#define NAV_ST_DEST     0x13
#define NAV_ST_DROPPED  0x14
#define NAV_ST_ZOOM     0x15
#define NAV_ST_LOOK     0x18
#define NAV_ST_SEARCH   0x16
#define NAV_ST_EMPTY    0x17

#define NAV_BEGIN_LEN  (1 + 2 + 2 + 4 + 2)
#define NAV_END_LEN    (1 + 2)
#define NAV_TILE_BEGIN_LEN (1 + 1 + 1 + 4 + 4 + 4 + 2)
#define NAV_VIEW_LEN       (1 + 4 + 4 + 1 + 2)
#define NAV_VIEW_LEN_SPEED (NAV_VIEW_LEN + 2)
#define NAV_ROUTE_BEGIN_LEN (1 + 2 + 2)
#define NAV_FOUND_BEGIN_LEN (1 + 1 + 2 + 2)
#define NAV_GUIDE_LEN       (1 + 1 + 2 + 4 + 2 + 1)

/* How often the map is redrawn between the phone's position updates. The
 * phone speaks twice a second, which as a step is plainly visible; the head
 * unit knows the speed and heading, so in between it carries the view
 * forward itself and redraws. 120 ms is about eight frames a second, and a
 * compose costs under 20 ms. */
#define NAV_FRAME_MS 120
/* How often the view is carried forward between the phone's position
 * messages. Those arrive twice a second, which is a visible hop if the map
 * only moves when one lands; in between, the rider's own speed and heading
 * say where they have got to. Ten a second is as fast as the panel swaps
 * buffers, and composing a frame costs about 20 ms of one core. */
#define NAV_STEP_MS 100
/* How soon to come back for a render the buffer was too busy to take. The
 * screen swaps on a 50 ms tick and holds the buffer one tick after that, so
 * anything under that is a wasted wake-up and much over it is a visible
 * stall. */
#define NAV_RENDER_RETRY_MS 25

/* With no frame for this long the link goes back to its normal duty cycle.
 * Not a fault and not shown on screen: the app skips frames whose pixels did
 * not change, so a parked bike is silent by design. */
#define NAV_IDLE_US (6 * 1000 * 1000)

typedef enum { ST_IDLE, ST_RECEIVING, ST_DECODING } nav_state_t;

/* What the open transfer is carrying. One at a time: the app waits for the
 * acknowledgement before it starts the next. */
typedef enum { RX_TILE, RX_ROUTE, RX_FOUND } rx_kind_t;

typedef enum { EV_TILE, EV_VIEW, EV_STATE, EV_STOP, EV_REJECT,
               EV_DEST, EV_ROUTE, EV_ZOOM, EV_LOOK, EV_SEARCH,
               EV_FOUND } ev_kind_t;
typedef struct {
    ev_kind_t kind;
    uint16_t  seq;
    uint8_t   reason;   /* EV_REJECT */
    int32_t   lat_e7;   /* EV_DEST */
    int32_t   lon_e7;
    uint8_t   zoom;     /* EV_ZOOM */
    bool      following; /* EV_LOOK: back on the rider, ignore lat/lon */
    char      query[NAV_SEARCH_QUERY];   /* EV_SEARCH */
} nav_evt_t;

static QueueHandle_t s_q;
static TaskHandle_t  s_task;

static uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_ctrl_handle;

/* Receive state. Written by the host task, read by the worker only after an
 * EV_TILE (or EV_ROUTE, EV_FOUND) hands it over — the queue carries the
 * barrier — and never written again until the worker has replied: the app
 * sends one thing at a time. */
static volatile nav_state_t s_state = ST_IDLE;
static uint8_t  *s_stage;
static uint32_t  s_expect;
static uint32_t  s_got;
static uint16_t  s_seq;
static rx_kind_t s_rx_kind;
static uint8_t   s_tile_z, s_tile_fmt;
static uint32_t  s_tile_x, s_tile_y;

/* Worker task only. */
static size_t   s_cache_line = 64;
static bool     s_boosted;
static int64_t  s_last_frame_us;
static int64_t  s_last_view_us;   /* last position from the phone */
static int64_t  s_last_step_us;   /* last dead-reckoning step */

static ble_nav_stats_t s_stats;

/* ---- helpers ---- */

static void notify(uint8_t status, uint8_t a, uint16_t b, uint16_t c)
{
    if (s_conn == BLE_HS_CONN_HANDLE_NONE || s_ctrl_handle == 0) {
        /* No link or no CTRL handle means the app hears nothing back and waits
         * on its own timeout — worth a line, because from the phone side it is
         * indistinguishable from a head unit that ignores the protocol. */
        ESP_LOGW(TAG, "notify 0x%02x dropped: conn=%u ctrl_handle=%u",
                 status, (unsigned)s_conn, (unsigned)s_ctrl_handle);
        return;
    }
    uint8_t f[6] = { status, a, (uint8_t)b, (uint8_t)(b >> 8),
                     (uint8_t)c, (uint8_t)(c >> 8) };
    /* Ride out a congested host: an acknowledgement lands right after sixty
     * chunk writes, which is exactly when NimBLE's mbuf pool is emptiest. A
     * dropped one is not cosmetic — the phone waits, times out and sends the
     * whole tile again (seen on the bench as tiles decoded twice). The pool
     * drains in milliseconds, so a bounded retry is enough. */
    for (int attempt = 0; attempt < 200; attempt++) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(f, sizeof(f));
        if (om && ble_gatts_notify_custom(s_conn, s_ctrl_handle, om) == 0) return;
        if (s_conn == BLE_HS_CONN_HANDLE_NONE) return;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGW(TAG, "notify status=0x%02x gave up", status);
}

/* Somewhere to go, picked on the panel. Nine bytes rather than the usual six
 * — the phone tells the two apart by the status byte. */
static void notify_dest(int32_t lat_e7, int32_t lon_e7)
{
    if (s_conn == BLE_HS_CONN_HANDLE_NONE || s_ctrl_handle == 0) return;
    uint8_t f[9] = { NAV_ST_DEST };
    memcpy(&f[1], &lat_e7, 4);
    memcpy(&f[5], &lon_e7, 4);
    for (int attempt = 0; attempt < 200; attempt++) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(f, sizeof(f));
        if (om && ble_gatts_notify_custom(s_conn, s_ctrl_handle, om) == 0) return;
        if (s_conn == BLE_HS_CONN_HANDLE_NONE) return;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGW(TAG, "destination notify gave up");
}

/* Where the head unit is looking, after the rider dragged the map. Ten bytes:
 * the follow flag first, because "back on the rider" carries no position
 * worth reading. The phone fetches tiles around this instead of around the
 * rider — without it a dragged view runs out of ground at the edge of what
 * was already sent. */
static void notify_look(bool following, int32_t lat_e7, int32_t lon_e7)
{
    if (s_conn == BLE_HS_CONN_HANDLE_NONE || s_ctrl_handle == 0) return;
    uint8_t f[10] = { NAV_ST_LOOK, (uint8_t)(following ? 0 : 1) };
    memcpy(&f[2], &lat_e7, 4);
    memcpy(&f[6], &lon_e7, 4);
    for (int attempt = 0; attempt < 200; attempt++) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(f, sizeof(f));
        if (om && ble_gatts_notify_custom(s_conn, s_ctrl_handle, om) == 0) return;
        if (s_conn == BLE_HS_CONN_HANDLE_NONE) return;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGW(TAG, "look notify gave up");
}

/* What the rider typed. Longer than the usual six bytes and the only
 * head-to-phone message that carries text: [status][len][UTF-8]. */
static void notify_search(const char *q)
{
    if (s_conn == BLE_HS_CONN_HANDLE_NONE || s_ctrl_handle == 0 || !q) return;
    size_t n = strlen(q);
    if (n == 0) return;
    if (n > NAV_SEARCH_QUERY - 1) n = NAV_SEARCH_QUERY - 1;
    uint8_t f[2 + NAV_SEARCH_QUERY];
    f[0] = NAV_ST_SEARCH;
    f[1] = (uint8_t)n;
    memcpy(&f[2], q, n);
    for (int attempt = 0; attempt < 100; attempt++) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(f, 2 + n);
        if (om && ble_gatts_notify_custom(s_conn, s_ctrl_handle, om) == 0) return;
        if (s_conn == BLE_HS_CONN_HANDLE_NONE) return;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGW(TAG, "search notify gave up");
}

/* A tile we had to drop. Same worker task as everything else here, so the
 * notify can go out directly. */
static void on_tile_evicted(uint8_t z, uint32_t x, uint32_t y)
{
    if (s_conn == BLE_HS_CONN_HANDLE_NONE || s_ctrl_handle == 0) return;
    uint8_t f[10] = { NAV_ST_DROPPED, z };
    memcpy(&f[2], &x, 4);
    memcpy(&f[6], &y, 4);
    for (int attempt = 0; attempt < 100; attempt++) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(f, sizeof(f));
        if (om && ble_gatts_notify_custom(s_conn, s_ctrl_handle, om) == 0) return;
        if (s_conn == BLE_HS_CONN_HANDLE_NONE) return;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* From the LVGL task, so it only enqueues — the notify happens on the worker
 * like every other one. */
static void on_dest_picked(int32_t lat_e7, int32_t lon_e7)
{
    if (!s_q) return;
    nav_evt_t ev = { .kind = EV_DEST, .lat_e7 = lat_e7, .lon_e7 = lon_e7 };
    xQueueSend(s_q, &ev, 0);
    /* Sign printed on its own: "-0.5" has an integer part of zero, and
     * printing that part as a number loses the minus entirely. */
    ESP_LOGI(TAG, "destination picked on the panel: %s%ld.%07ld, %s%ld.%07ld",
             lat_e7 < 0 ? "-" : "", (long)labs(lat_e7) / 10000000,
             (long)labs(lat_e7) % 10000000,
             lon_e7 < 0 ? "-" : "", (long)labs(lon_e7) / 10000000,
             (long)labs(lon_e7) % 10000000);
}

/* The rider dragged the map, or asked to go back to following themselves.
 * From the LVGL task, so it only queues; the worker does the notify and the
 * redraw. */
static void on_look_changed(bool following, int32_t lat_e7, int32_t lon_e7)
{
    if (!s_q) return;
    nav_evt_t ev = { .kind = EV_LOOK, .following = following,
                     .lat_e7 = lat_e7, .lon_e7 = lon_e7 };
    xQueueSend(s_q, &ev, 0);
}

/* The rider pressed a zoom button. The map changes level at once — it has
 * layers either side to fall back on — and the phone is asked for tiles at
 * the new level, since only it can fetch them. From the LVGL task, so the
 * notify goes through the worker like every other one. */
static void on_zoom_picked(uint8_t zoom)
{
    nav_map_set_zoom(zoom);
    if (!s_q) return;
    nav_evt_t ev = { .kind = EV_ZOOM, .zoom = zoom };
    xQueueSend(s_q, &ev, 0);
    ESP_LOGI(TAG, "zoom %u chosen on the panel", (unsigned)zoom);
}

/* From the LVGL task when the rider has typed enough to look for. */
static void on_search_typed(const char *q)
{
    if (!s_q || !q) return;
    nav_evt_t ev = { .kind = EV_SEARCH };
    strncpy(ev.query, q, sizeof(ev.query) - 1);
    xQueueSend(s_q, &ev, 0);
}

/* Tell every subscribed link, not only the one that owns the bridge.
 *
 * Acknowledgements belong to whoever sent the thing being acknowledged, but
 * whether this screen is live is a fact about the panel and everybody's
 * business. Sending it only to the bridge owner deadlocked the bench: a
 * second phone took the binding while the navigator phone was quiet over a
 * mode switch, so the navigator never heard that the screen was live again —
 * and since it only writes when it believes the screen is live, it never
 * wrote again, and never took the binding back. */
static void notify_all(uint8_t status, uint8_t a, uint16_t b, uint16_t c)
{
    if (s_ctrl_handle == 0) return;
    const uint8_t f[6] = { status, a, (uint8_t)b, (uint8_t)(b >> 8),
                           (uint8_t)c, (uint8_t)(c >> 8) };
    for (uint16_t h = 0; h < 8; h++) {
        struct ble_gap_conn_desc d;
        if (ble_gap_conn_find(h, &d) != 0) continue;
        if (d.role != BLE_GAP_ROLE_SLAVE) continue;
        for (int attempt = 0; attempt < 20; attempt++) {
            struct os_mbuf *om = ble_hs_mbuf_from_flat(f, sizeof(f));
            /* A link that never subscribed simply refuses; that is fine. */
            if (om && ble_gatts_notify_custom(h, s_ctrl_handle, om) == 0) break;
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

static void notify_state(void)
{
    const bool live = (ui_mode_get() == UI_MODE_NAV);
    notify_all(NAV_ST_STATE, live ? 1 : 0, live ? 1 : 0, BLE_NAV_MAX_DATA);

    /* And say so when the store is empty. The phone never sends the same tile
     * twice, so after a reboot it has to be told that what it sent is gone —
     * otherwise the panel sits on a bare map while the position keeps
     * arriving, which is exactly what the bench showed: 248 position updates
     * and `tiles ok=0`. Eviction notices cover losing tiles one at a time;
     * this covers losing all of them at once. */
    nav_tiles_stats_t ts;
    nav_tiles_get_stats(&ts);
    if (ts.stored == 0) notify_all(NAV_ST_EMPTY, 0, 0, 0);
}

static void set_boost(bool on)
{
    if (on == s_boosted) return;
    s_boosted = on;
    /* Unlike the firmware update, do NOT park the sensor arbiter: frames flow
     * for the whole ride, and a cadence or speed sensor waking up mid-ride
     * still has to be able to connect. */
    ble_link_boost_request(s_conn, on, false);
}

static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* Compose the current view from the tiles we hold and put it on screen.
 * Cheap — a screenful is 768 KB of memcpy — so it runs on every position
 * update and again whenever a tile that might be visible arrives. */
/* Set when a render had to be skipped because the buffer was busy, cleared
 * when that render finally happens. Without it a skipped render was simply
 * lost: the tile that prompted it sat in the store, decoded and ready, while
 * its square on the screen stayed blank until something else happened to ask
 * for a redraw. Panning made that constant — tiles arrive faster than the
 * buffer frees, so most of them were composed by nobody. */
static atomic_bool s_render_owed;

static void render_view(void)
{
    uint16_t *dst = nav_screen_back_buffer();
    if (!dst) {                  /* the last one is still on its way out */
        atomic_store(&s_render_owed, true);
        return;
    }
    atomic_store(&s_render_owed, false);
    if (ui_mode_get() != UI_MODE_NAV) return;
    const int64_t t0 = esp_timer_get_time();
    int wanted = 0;
    const int have = nav_map_render(dst, nav_screen_width(),
                                    nav_screen_height(), &wanted);
    const int64_t t1 = esp_timer_get_time();
    /* Push the frame out of the cache before LVGL's DMA reads it. Timed
     * separately because it is 768 KB of write-back and therefore a fixed
     * cost per frame, unlike composing, which depends on what is missing. */
    esp_cache_msync(dst, nav_screen_back_buffer_bytes(),
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    const int64_t t2 = esp_timer_get_time();
    nav_screen_commit();
    nav_screen_set_streaming(true);
    s_stats.renders++;
    s_stats.render_last_ms = (uint32_t)((t2 - t0) / 1000);
    s_stats.compose_us = (uint32_t)(t1 - t0);
    s_stats.msync_us = (uint32_t)(t2 - t1);
    s_stats.last_have = have;
    s_stats.last_wanted = wanted;
    s_last_frame_us = esp_timer_get_time();
}

/* Carry the view forward and redraw, so the map glides instead of stepping
 * twice a second.
 *
 * This used to live in the queue-timeout branch, which meant it only ran when
 * nothing else was happening — and while tiles stream in, something always
 * is. The map then sat still between position messages and jumped when one
 * arrived, which is exactly when a rider is looking at it. Now it runs on
 * every pass and gates on the clock instead.
 *
 * Only while the phone is actually feeding us: otherwise we would drift off
 * on a stale speed. Dragged away from the rider nothing moves anyway. */
static void step_view(void)
{
    const int64_t now = esp_timer_get_time();
    if (!s_stats.streaming || s_last_view_us == 0 ||
        now - s_last_view_us >= NAV_IDLE_US) {
        return;
    }
    if (nav_map_is_panned()) return;
    if (now - s_last_step_us < NAV_STEP_MS * 1000) return;
    const uint32_t dt = (uint32_t)((now - s_last_step_us) / 1000);
    s_last_step_us = now;
    nav_map_dead_reckon(dt);
    render_view();
}

/* How many tiles one idle pass may lift off the microSD card.
 *
 * A read plus a decode is tens of milliseconds and this runs on the worker
 * that also answers the phone, so it is done a couple at a time between
 * events rather than as one long stall with a frozen map. A screenful comes
 * up over about a second — a different world from the same tiles over BLE,
 * where one of them takes that long on its own. */
#define NAV_SD_PREFETCH_PER_PASS 2

/* Fill the holes in the view from the card, when there is one.
 *
 * Only what the view actually covers, and only what is not already in a slot.
 * Tiles the card does not have cost one failed open each and are then
 * remembered as missing (nav_tile_store.c), so riding over ground the card
 * does not cover does not turn into a stream of filesystem calls. */
static void prefetch_pass(void)
{
    if (ui_mode_get() != UI_MODE_NAV) return;
    if (!nav_tile_store_ready()) return;

    uint8_t z;
    int64_t x0, x1, y0, y1;
    if (!nav_map_view_tiles(&z, &x0, &x1, &y0, &y1)) return;

    /* Widest layer first, sharpest last — the opposite of the order the
     * picture is composed in, and on purpose. One tile six levels down covers
     * sixty-four by sixty-four of the detail, so a single read puts a whole
     * blurred map on the screen; the detail then sharpens it over the next
     * few passes. Fetching detail first would leave the rider looking at bare
     * ground with two sharp squares in it. */
    static const int k_dz[] = { NAV_WIDE_DZ, NAV_COARSE_DZ, 0 };
    int budget = NAV_SD_PREFETCH_PER_PASS;
    bool got = false;
    for (unsigned i = 0; i < sizeof k_dz / sizeof k_dz[0] && budget > 0; i++) {
        const int dz = k_dz[i];
        if (dz > z) continue;
        const uint8_t lz = (uint8_t)(z - dz);
        for (int64_t ty = y0 >> dz; ty <= (y1 >> dz) && budget > 0; ty++) {
            for (int64_t tx = x0 >> dz; tx <= (x1 >> dz) && budget > 0; tx++) {
                if (tx < 0 || ty < 0) continue;
                if (nav_tiles_have(lz, (uint32_t)tx, (uint32_t)ty)) continue;
                if (!nav_tiles_load_cached(lz, (uint32_t)tx, (uint32_t)ty)) continue;
                budget--;
                got = true;
            }
        }
    }
    if (got) render_view();
}

static void handle_tile(const nav_evt_t *ev)
{
    const int64_t t0 = esp_timer_get_time();
    uint8_t ack;
    if (s_got != s_expect) {
        ack = NAV_ACK_TRUNCATED;
    } else if (nav_tiles_put(s_tile_z, s_tile_x, s_tile_y, s_tile_fmt,
                             s_stage, s_got)) {
        ack = NAV_ACK_OK;
    } else {
        ack = NAV_ACK_DECODE;
    }
    const uint32_t ms = (uint32_t)((esp_timer_get_time() - t0) / 1000);
    s_state = ST_IDLE;

    s_stats.tile_last_bytes = s_got;
    s_stats.tile_last_ms = ms;
    if (ack == NAV_ACK_OK) {
        s_stats.tiles_ok++;
        /* A tile that just landed may be one the screen is waiting for. */
        render_view();
    } else {
        s_stats.tiles_failed++;
    }
    notify(NAV_ST_TILE_ACK, ack, ev->seq, (uint16_t)(ms > 0xFFFF ? 0xFFFF : ms));
}

static void handle_route(const nav_evt_t *ev)
{
    uint8_t ack;
    if (s_got != s_expect) {
        ack = NAV_ACK_TRUNCATED;
    } else {
        ack = nav_route_set(s_stage, s_got) ? NAV_ACK_OK : NAV_ACK_BAD_PARAM;
    }
    s_state = ST_IDLE;
    if (ack == NAV_ACK_OK) render_view();
    notify(NAV_ST_TILE_ACK, ack, ev->seq, 0);
}

/* Results for a typed query: count x [i32 lat_e7][i32 lon_e7][u8 len][name].
 * Parsed here rather than in nav_screen so the UI never sees a wire format. */
static void handle_found(const nav_evt_t *ev)
{
    uint8_t ack = NAV_ACK_OK;
    nav_search_hit_t hits[NAV_SEARCH_MAX];
    size_t n = 0;
    if (s_got != s_expect) {
        ack = NAV_ACK_TRUNCATED;
    } else {
        size_t off = 0;
        while (off + 9 <= s_got && n < NAV_SEARCH_MAX) {
            int32_t lat_e7, lon_e7;
            memcpy(&lat_e7, s_stage + off, 4);
            memcpy(&lon_e7, s_stage + off + 4, 4);
            const uint8_t nl = s_stage[off + 8];
            off += 9;
            if (off + nl > s_got) { ack = NAV_ACK_BAD_PARAM; break; }
            size_t copy = nl < NAV_SEARCH_NAME - 1 ? nl : NAV_SEARCH_NAME - 1;
            memcpy(hits[n].name, s_stage + off, copy);
            hits[n].name[copy] = '\0';
            hits[n].lat_e7 = lat_e7;
            hits[n].lon_e7 = lon_e7;
            n++;
            off += nl;
        }
    }
    s_state = ST_IDLE;
    if (ack == NAV_ACK_OK) {
        nav_screen_set_results(hits, n);
        ESP_LOGI(TAG, "search: %u result(s)", (unsigned)n);
    }
    notify(NAV_ST_ACK, ack, ev->seq, 0);
}

static void worker(void *arg)
{
    (void)arg;
    if (esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &s_cache_line) != ESP_OK ||
        s_cache_line == 0) {
        s_cache_line = 64;
    }

    for (;;) {
        nav_evt_t ev;
        /* A render we owe is worth waking up for sooner than the idle
         * cadence — the buffer frees within a screen tick or two. */
        const TickType_t wait = atomic_load(&s_render_owed)
                                    ? pdMS_TO_TICKS(NAV_RENDER_RETRY_MS)
                                    : pdMS_TO_TICKS(NAV_FRAME_MS);
        if (xQueueReceive(s_q, &ev, wait) != pdTRUE) {
            const int64_t now = esp_timer_get_time();
            /* An owed render still has to happen: tiles land while the rider
             * is parked too, and nothing else would ever put them on screen. */
            if (atomic_load(&s_render_owed)) render_view();
            /* Idle sweep: the app stopped without saying so (screen off, out
             * of range, killed) — stop claiming we are streaming and hand the
             * radio back to its normal duty cycle. */
            if (s_stats.streaming && s_last_frame_us != 0 &&
                now - s_last_frame_us > NAV_IDLE_US) {
                s_stats.streaming = false;
                /* Only the radio is stood down — the picture on screen is
                 * still current, so the placeholder stays away. */
                set_boost(false);
            }
            step_view();
            /* Nothing from the phone this pass: a good moment to go to the
             * card for whatever the view is still missing. */
            prefetch_pass();
            continue;
        }
        switch (ev.kind) {
            case EV_TILE:
                s_stats.streaming = true;
                set_boost(true);
                handle_tile(&ev);
                break;
            case EV_ROUTE:
                handle_route(&ev);
                break;
            case EV_VIEW:
                s_stats.views++;
                s_stats.streaming = true;
                s_last_view_us = esp_timer_get_time();
                nav_screen_set_streaming(true);
                set_boost(true);
                /* No render here on purpose. Drawing both on arrival and on
                 * the frame timer made the gaps between frames uneven, and
                 * uneven gaps with a steady dead-reckoning step is exactly
                 * what a stuttering map looks like. The timer draws; this
                 * only moves the target it is chasing. */
                break;
            case EV_REJECT:
                s_state = ST_IDLE;
                notify(s_rx_kind == RX_TILE ? NAV_ST_TILE_ACK : NAV_ST_ACK,
                       ev.reason, ev.seq, 0);
                break;
            case EV_STOP:
                s_state = ST_IDLE;
                s_stats.streaming = false;
                nav_screen_set_streaming(false);
                set_boost(false);
                break;
            case EV_STATE:
#if !CONFIG_AA_ENABLE
                /* Dashboard-only board: bring the navigator up on the first
                 * greeting of a link. The phone only streams tiles while it is
                 * told the navigator is the live screen, and on the head units
                 * that state comes from the rider's three-finger gesture
                 * between the dashboard and Android Auto. Here there is no AA
                 * to swap with, so a phone that has the map open would sit
                 * greeting a dashboard for ever — which is exactly how this
                 * looked on the bench. The gesture still toggles back. */
                if (ui_mode_get() != UI_MODE_NAV) ui_mode_set(UI_MODE_NAV);
#endif
                notify_state();
                break;
            case EV_DEST:
                notify_dest(ev.lat_e7, ev.lon_e7);
                break;
            case EV_ZOOM:
                notify(NAV_ST_ZOOM, ev.zoom, 0, 0);
                render_view();
                break;
            case EV_LOOK:
                notify_look(ev.following, ev.lat_e7, ev.lon_e7);
                /* The drag already moved what we draw; this redraws from the
                 * tiles we hold, and the phone fills the rest in. */
                render_view();
                break;
            case EV_SEARCH:
                notify_search(ev.query);
                break;
            case EV_FOUND:
                handle_found(&ev);
                break;
        }
        /* Every pass ends here, so the glide keeps its own time rather
         * than only running when nothing else arrived. */
        step_view();
    }
}

/* ---- public API ---- */

void ble_nav_init(void)
{
    if (s_q) return;
    s_stage = heap_caps_aligned_calloc(64, 1, BLE_NAV_MAX_FRAME,
                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (!s_stage) {
        ESP_LOGE(TAG, "no PSRAM for the %d-byte frame buffer", BLE_NAV_MAX_FRAME);
        return;
    }
    nav_tiles_init();
    s_q = xQueueCreate(8, sizeof(nav_evt_t));
    nav_screen_set_dest_cb(on_dest_picked);
    nav_screen_set_zoom_cb(on_zoom_picked);
    nav_screen_set_look_cb(on_look_changed);
    nav_screen_set_search_cb(on_search_typed);
    nav_tiles_set_evict_cb(on_tile_evicted);
    /* 8 KiB, like the other two workers on this link. Four was enough when
     * this task only memcpy'd, but it now runs libpng — whose simplified read
     * API is generous with the stack — and composes the view on top. A task
     * that overflows here takes the whole device down with it. */
    /* The stack normally comes out of internal RAM. On the ESP32-S3 board that
     * is the scarce resource: by the time the bridge registers its services
     * the largest free block is around 7.5 KB, so this 8 KB task silently
     * failed to start — and nothing checked. The greeting was then written
     * into a queue nobody read, so the phone kept greeting and the panel kept
     * saying it was waiting for the app, with no error anywhere.
     *
     * So: check, and fall back to a stack in PSRAM. That is sound on the board
     * that needs it — it already runs its code from PSRAM
     * (CONFIG_SPIRAM_FETCH_INSTRUCTIONS), which is what keeps the cache
     * enabled across a flash write, and both ALLOW_EXT_MEM options are on. */
    BaseType_t ok = xTaskCreatePinnedToCore(worker, "ble_nav", 8192, NULL,
                                            5, &s_task, 0);
    if (ok != pdPASS) {
        ESP_LOGW(TAG, "no internal RAM for the worker stack "
                      "(free=%u largest=%u) — retrying in PSRAM",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        ok = xTaskCreatePinnedToCoreWithCaps(worker, "ble_nav", 8192, NULL, 5,
                                             &s_task, 0, MALLOC_CAP_SPIRAM);
    }
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "worker task not created — the navigator cannot answer"
                      " the phone; it will greet for ever");
        s_task = NULL;
        return;
    }
    ESP_LOGI(TAG, "ble_nav ready (internal free=%u)",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
}

void ble_nav_notify_state(void)
{
    if (!s_q) return;
    nav_evt_t ev = { .kind = EV_STATE };
    xQueueSend(s_q, &ev, 0);
}

void ble_nav_get_stats(ble_nav_stats_t *out)
{
    if (!out) return;
    *out = s_stats;
    /* Worth watching: this task decodes PNGs through libpng and composes the
     * view, so it is the one that would overflow and take the device with it. */
    out->stack_free = s_task
        ? (uint32_t)uxTaskGetStackHighWaterMark(s_task) : 0;
}

void ble_nav_set_link(uint16_t conn, uint16_t ctrl_val_handle)
{
    s_conn = conn;
    s_ctrl_handle = ctrl_val_handle;
    s_state = ST_IDLE;
    s_boosted = false;
    nav_screen_set_phone(true);
    ble_nav_notify_state();
}

void ble_nav_on_disconnect(void)
{
    s_conn = BLE_HS_CONN_HANDLE_NONE;
    s_state = ST_IDLE;
    s_boosted = false;
    s_stats.streaming = false;
    nav_screen_set_phone(false);
    if (s_q) {
        nav_evt_t ev = { .kind = EV_STOP };
        xQueueSend(s_q, &ev, 0);
    }
}

/* ---- routed from notif_bridge access_cb (NimBLE host task) ---- */

static void reject(uint16_t seq, uint8_t reason)
{
    s_state = ST_IDLE;
    nav_evt_t ev = { .kind = EV_REJECT, .seq = seq, .reason = reason };
    if (s_q) xQueueSend(s_q, &ev, 0);
}

void ble_nav_ctrl_write(const uint8_t *data, uint16_t len)
{
    if (len < 1 || !s_q || !s_stage) {
        /* Silence here was expensive to debug once: with no queue or staging
         * buffer this returns without answering, and the app retries its HELLO
         * forever while the panel sits on "Waiting for the navigator app". */
        ESP_LOGW(TAG, "ctrl write dropped: len=%u q=%d stage=%d",
                 (unsigned)len, s_q ? 1 : 0, s_stage ? 1 : 0);
        return;
    }
    switch (data[0]) {
        /* The picture path is gone: the head unit composes the map from
         * tiles now, and the app stopped offering a rendered frame long before
         * the decoder that took it was removed. Answered rather than ignored,
         * so an old build hears "not supported" instead of waiting for an ack
         * that will never come. */
        case NAV_OP_BEGIN: {
            const uint16_t seq = (len >= NAV_BEGIN_LEN)
                ? (uint16_t)data[9] | ((uint16_t)data[10] << 8) : 0;
            reject(seq, NAV_ACK_DECODE);
            return;
        }
        case NAV_OP_END: {
            const uint16_t seq = (len >= NAV_END_LEN)
                ? (uint16_t)data[1] | ((uint16_t)data[2] << 8) : 0;
            reject(seq, NAV_ACK_DECODE);
            return;
        }
        case NAV_OP_TILE_END:
        case NAV_OP_ROUTE_END:
        case NAV_OP_FOUND_END: {
            if (len < NAV_END_LEN) return;
            const uint16_t seq = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
            const rx_kind_t want = (data[0] == NAV_OP_TILE_END) ? RX_TILE
                                 : (data[0] == NAV_OP_ROUTE_END) ? RX_ROUTE
                                                                 : RX_FOUND;
            if (s_state != ST_RECEIVING || seq != s_seq || s_rx_kind != want) {
                reject(seq, NAV_ACK_TRUNCATED);
                return;
            }
            s_state = ST_DECODING;
            nav_evt_t ev = { .kind = (want == RX_TILE)  ? EV_TILE
                                   : (want == RX_ROUTE) ? EV_ROUTE
                                                        : EV_FOUND,
                             .seq = seq };
            xQueueSend(s_q, &ev, 0);
            break;
        }

        case NAV_OP_ROUTE_BEGIN: {
            if (len < NAV_ROUTE_BEGIN_LEN) { reject(0, NAV_ACK_BAD_PARAM); return; }
            const uint16_t n   = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
            const uint16_t seq = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
            if (s_state == ST_DECODING) { reject(seq, NAV_ACK_BUSY); return; }
            const uint32_t tot = (uint32_t)n * 8u;
            if (n == 0) {
                /* No points means the ride is over — the phone has no route
                 * to draw any more. Without this the head unit kept the last
                 * line and the last manoeuvre plate for the rest of the day:
                 * "0.0 km | 0 min" in arrival red over a map with no line on
                 * it, because the remaining stub had gone off screen. */
                nav_route_clear();
                notify(NAV_ST_ACK, NAV_ACK_OK, seq, 0);
                return;
            }
            if (n > NAV_ROUTE_MAX_POINTS || tot > BLE_NAV_MAX_FRAME) {
                reject(seq, NAV_ACK_BAD_PARAM);
                return;
            }
            s_expect = tot;
            s_got = 0;
            s_seq = seq;
            s_rx_kind = RX_ROUTE;
            s_state = ST_RECEIVING;
            break;
        }

        case NAV_OP_FOUND_BEGIN: {
            /* [op][u8 count][u16 seq], then DATA with the entries. A count of
             * zero is a valid answer — "nothing found" — and carries no
             * body, so it is acknowledged here and now. */
            if (len < NAV_FOUND_BEGIN_LEN) { reject(0, NAV_ACK_BAD_PARAM); return; }
            const uint8_t  cnt = data[1];
            const uint16_t tot = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
            const uint16_t seq = (uint16_t)data[4] | ((uint16_t)data[5] << 8);
            if (s_state == ST_DECODING) { reject(seq, NAV_ACK_BUSY); return; }
            if (cnt == 0) {
                nav_screen_set_results(NULL, 0);
                notify(NAV_ST_ACK, NAV_ACK_OK, seq, 0);
                return;
            }
            if (cnt > NAV_SEARCH_MAX ||
                tot == 0 || tot > cnt * (9 + NAV_SEARCH_NAME)) {
                reject(seq, NAV_ACK_BAD_PARAM);
                return;
            }
            s_expect = tot;
            s_got = 0;
            s_seq = seq;
            s_rx_kind = RX_FOUND;
            s_state = ST_RECEIVING;
            break;
        }

        case NAV_OP_GUIDE: {
            if (len < NAV_GUIDE_LEN) return;
            nav_guide_t g = {
                .turn = (nav_turn_t)(data[1] < NAV_TURN_COUNT ? data[1]
                                                              : NAV_TURN_STRAIGHT),
                .dist_m = (uint16_t)data[2] | ((uint16_t)data[3] << 8),
                .remaining_m = rd_u32(data + 4),
                .remaining_s = (uint16_t)data[8] | ((uint16_t)data[9] << 8),
                .off_route = (data[10] & 1) != 0,
                .valid = true,
            };
            nav_route_set_guide(&g);
            break;
        }

        case NAV_OP_TILE_BEGIN: {
            if (len < NAV_TILE_BEGIN_LEN) { reject(0, NAV_ACK_BAD_PARAM); return; }
            const uint8_t  fmt = data[1];
            const uint8_t  z   = data[2];
            const uint32_t x   = rd_u32(data + 3);
            const uint32_t y   = rd_u32(data + 7);
            const uint32_t tot = rd_u32(data + 11);
            const uint16_t seq = (uint16_t)data[15] | ((uint16_t)data[16] << 8);
            if (s_state == ST_DECODING) { reject(seq, NAV_ACK_BUSY); return; }
            if (tot == 0 || tot > BLE_NAV_MAX_FRAME || z > 22) {
                reject(seq, NAV_ACK_BAD_PARAM);
                return;
            }
            s_tile_fmt = fmt;
            s_tile_z = z;
            s_tile_x = x;
            s_tile_y = y;
            s_expect = tot;
            s_got = 0;
            s_seq = seq;
            s_rx_kind = RX_TILE;
            s_state = ST_RECEIVING;
            break;
        }

        case NAV_OP_VIEW: {
            if (len < NAV_VIEW_LEN) return;
            const int32_t lat_e7 = (int32_t)rd_u32(data + 1);
            const int32_t lon_e7 = (int32_t)rd_u32(data + 5);
            const uint8_t zoom = data[9];
            const uint16_t heading = (uint16_t)data[10] | ((uint16_t)data[11] << 8);
            if (zoom > 22) return;
            /* Speed arrived later than the rest of the message; a phone that
             * does not send it simply gets no dead reckoning. */
            const uint16_t speed_cms = (len >= NAV_VIEW_LEN_SPEED)
                ? (uint16_t)data[12] | ((uint16_t)data[13] << 8) : 0;
            nav_map_set_view(lat_e7, lon_e7, zoom, heading);
            nav_map_set_speed(speed_cms);
            nav_evt_t ev = { .kind = EV_VIEW };
            xQueueSend(s_q, &ev, 0);
            break;
        }
        case NAV_OP_STOP: {
            s_state = ST_IDLE;
            nav_evt_t ev = { .kind = EV_STOP };
            xQueueSend(s_q, &ev, 0);
            break;
        }
        case NAV_OP_HELLO: {
            nav_evt_t ev = { .kind = EV_STATE };
            xQueueSend(s_q, &ev, 0);
            break;
        }
        default:
            break;
    }
}

void ble_nav_data_write(const uint8_t *data, uint16_t len)
{
    if (s_state != ST_RECEIVING || !s_stage || len == 0) return;
    if (s_got + len > s_expect) {
        /* More than promised: the sender and we disagree about this one, so
         * drop it rather than decode a mixture of two. */
        s_got = s_expect + 1;   /* the handler sees the mismatch */
        s_state = ST_RECEIVING;
        return;
    }
    memcpy(s_stage + s_got, data, len);
    s_got += len;
}
