/* Navigator frames from the phone — see ble_nav.h for the wire protocol.
 *
 * Shape follows ble_ota.c: the NimBLE host callback only validates and
 * memcpys, everything slow (JPEG decode, PPA scale) and every notification
 * happens on a dedicated worker task. Notifications must never be sent from
 * inside a GATT access callback. */

#include "ble_nav.h"

#include <string.h>

#include "driver/jpeg_decode.h"
#include "driver/ppa.h"
#include "esp_cache.h"
#include "esp_private/esp_cache_private.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_mbuf.h"
#include "os/os_mbuf.h"

#include "ble_link_boost.h"
#include "nav_map.h"
#include "nav_screen.h"
#include "nav_tiles.h"
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

#define NAV_ST_STATE   0x10
#define NAV_ST_ACK     0x11
#define NAV_ST_TILE_ACK 0x12

#define NAV_BEGIN_LEN  (1 + 2 + 2 + 4 + 2)
#define NAV_END_LEN    (1 + 2)
#define NAV_TILE_BEGIN_LEN (1 + 1 + 1 + 4 + 4 + 4 + 2)
#define NAV_VIEW_LEN       (1 + 4 + 4 + 1 + 2)
#define NAV_VIEW_LEN_SPEED (NAV_VIEW_LEN + 2)

/* How often the map is redrawn between the phone's position updates. The
 * phone speaks twice a second, which as a step is plainly visible; the head
 * unit knows the speed and heading, so in between it carries the view
 * forward itself and redraws. 120 ms is about eight frames a second, and a
 * compose costs under 20 ms. */
#define NAV_FRAME_MS 120

/* With no frame for this long the link goes back to its normal duty cycle.
 * Not a fault and not shown on screen: the app skips frames whose pixels did
 * not change, so a parked bike is silent by design. */
#define NAV_IDLE_US (6 * 1000 * 1000)

typedef enum { ST_IDLE, ST_RECEIVING, ST_DECODING } nav_state_t;

/* What the open transfer is carrying. One at a time: the app waits for the
 * acknowledgement before it starts the next. */
typedef enum { RX_FRAME, RX_TILE } rx_kind_t;

typedef enum { EV_FRAME, EV_TILE, EV_VIEW, EV_STATE, EV_STOP, EV_REJECT } ev_kind_t;
typedef struct {
    ev_kind_t kind;
    uint16_t  seq;
    uint8_t   reason;   /* EV_REJECT */
} nav_evt_t;

static QueueHandle_t s_q;
static TaskHandle_t  s_task;

static uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_ctrl_handle;

/* Receive state. Written by the host task, read by the worker only after an
 * EV_FRAME hands it over (the queue carries the barrier), and never written
 * again until the worker has replied — the app sends one frame at a time. */
static volatile nav_state_t s_state = ST_IDLE;
static uint8_t  *s_stage;
static uint32_t  s_expect;
static uint32_t  s_got;
static uint16_t  s_w, s_h;
static uint16_t  s_seq;
static rx_kind_t s_rx_kind;
static uint8_t   s_tile_z, s_tile_fmt;
static uint32_t  s_tile_x, s_tile_y;

/* Decode side, worker task only. */
static jpeg_decoder_handle_t s_jpgd;
static ppa_client_handle_t   s_ppa;
static uint8_t *s_dec;             /* RGB565 at the frame's own size */
static size_t   s_dec_bytes;
static uint16_t s_dec_w, s_dec_h;
static size_t   s_cache_line = 64;
static bool     s_boosted;
static int64_t  s_last_frame_us;
static int64_t  s_last_view_us;   /* last position from the phone */
static int64_t  s_last_step_us;   /* last dead-reckoning step */

static ble_nav_stats_t s_stats;

/* ---- helpers ---- */

static void notify(uint8_t status, uint8_t a, uint16_t b, uint16_t c)
{
    if (s_conn == BLE_HS_CONN_HANDLE_NONE || s_ctrl_handle == 0) return;
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

static void notify_state(void)
{
    const bool live = (ui_mode_get() == UI_MODE_NAV);
    notify(NAV_ST_STATE, live ? 1 : 0, live ? 1 : 0, BLE_NAV_MAX_DATA);
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

static size_t align_up(size_t v, size_t a) { return (v + a - 1) & ~(a - 1); }

static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* RGB565 scratch for a frame that is not already panel-sized. */
static bool ensure_decode_buffer(uint16_t w, uint16_t h)
{
    if (s_dec && s_dec_w == w && s_dec_h == h) return true;
    if (s_dec) { heap_caps_free(s_dec); s_dec = NULL; }
    /* The decoder rounds its output up to a whole 16x16 MCU on both axes, so
     * the buffer must be sized to the aligned frame even when the picture
     * itself is smaller (music_info_view.c learned this the hard way). */
    size_t need = (size_t)align_up(w, 16) * align_up(h, 16) * 2;
    s_dec_bytes = align_up(need, s_cache_line);
    s_dec = heap_caps_aligned_calloc(s_cache_line, 1, s_dec_bytes,
                                     MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (!s_dec) {
        s_dec_bytes = 0;
        s_dec_w = s_dec_h = 0;
        ESP_LOGE(TAG, "no PSRAM for a %ux%u decode buffer", w, h);
        return false;
    }
    s_dec_w = w;
    s_dec_h = h;
    return true;
}

static bool decode_into(uint8_t *dst, size_t dst_bytes,
                        const uint8_t *src, uint32_t len)
{
    jpeg_decode_cfg_t cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        /* BGR is what the ST7701 pipeline expects on this board; RGB order
         * shows up as a red/blue swap (same as the splash and album art). */
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
    };
    uint32_t out_size = 0;
    esp_err_t r = jpeg_decoder_process(s_jpgd, &cfg, (uint8_t *)src, len,
                                       dst, dst_bytes, &out_size);
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "jpeg decode failed: %s", esp_err_to_name(r));
        return false;
    }
    return true;
}

/* PPA: s_dec (w x h RGB565) -> dst (800x480 RGB565), no rotation. The LVGL
 * adapter rotates for the panel, so this is a straight upscale. */
static bool scale_into(uint16_t *dst, size_t dst_bytes, uint16_t w, uint16_t h)
{
    esp_cache_msync(s_dec, s_dec_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    ppa_srm_oper_config_t op = {
        .in = {
            .buffer  = s_dec,
            .pic_w   = align_up(w, 16),
            .pic_h   = align_up(h, 16),
            .block_w = w,
            .block_h = h,
            .srm_cm  = PPA_SRM_COLOR_MODE_RGB565,
        },
        .out = {
            .buffer      = dst,
            .buffer_size = dst_bytes,
            .pic_w       = NAV_SCREEN_W,
            .pic_h       = NAV_SCREEN_H,
            .srm_cm      = PPA_SRM_COLOR_MODE_RGB565,
        },
        .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
        .scale_x = (float)NAV_SCREEN_W / (float)w,
        .scale_y = (float)NAV_SCREEN_H / (float)h,
        .mode    = PPA_TRANS_MODE_BLOCKING,
    };
    esp_err_t e = ppa_do_scale_rotate_mirror(s_ppa, &op);
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "ppa scale %ux%u -> %dx%d: %s", w, h,
                 NAV_SCREEN_W, NAV_SCREEN_H, esp_err_to_name(e));
        return false;
    }
    return true;
}

static uint8_t present_frame(const uint8_t *jpeg, uint16_t w, uint16_t h,
                             uint32_t len)
{
    if (ui_mode_get() != UI_MODE_NAV) return NAV_ACK_HIDDEN;

    uint16_t *dst = nav_screen_back_buffer();
    if (!dst) return NAV_ACK_BUSY;
    const size_t dst_bytes = nav_screen_back_buffer_bytes();

    if (w == NAV_SCREEN_W && h == NAV_SCREEN_H) {
        if (!decode_into((uint8_t *)dst, dst_bytes, jpeg, len)) {
            return NAV_ACK_DECODE;
        }
    } else {
        if (!ensure_decode_buffer(w, h)) return NAV_ACK_DECODE;
        if (!decode_into(s_dec, s_dec_bytes, jpeg, len)) return NAV_ACK_DECODE;
        if (!scale_into(dst, dst_bytes, w, h)) return NAV_ACK_DECODE;
    }
    /* DMA wrote the framebuffer; LVGL reads it with the CPU. */
    esp_cache_msync(dst, dst_bytes,
                    ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    nav_screen_commit();
    return NAV_ACK_OK;
}

/* ---- worker ---- */

static void handle_frame(const nav_evt_t *ev)
{
    const int64_t t0 = esp_timer_get_time();
    const uint16_t w = s_w, h = s_h;
    const uint32_t len = s_got;

    uint8_t ack;
    if (s_got != s_expect) {
        ack = NAV_ACK_TRUNCATED;
    } else {
        ack = present_frame(s_stage, w, h, len);
    }

    const uint32_t ms = (uint32_t)((esp_timer_get_time() - t0) / 1000);
    s_state = ST_IDLE;   /* the app may start the next frame */

    s_stats.last_bytes = len;
    s_stats.last_w = w;
    s_stats.last_h = h;
    s_stats.last_decode_ms = ms;
    s_stats.last_ack = ack;
    if (ack == NAV_ACK_OK) {
        s_stats.frames_ok++;
        s_last_frame_us = esp_timer_get_time();
        nav_screen_set_streaming(true);
    } else {
        s_stats.frames_failed++;
        ESP_LOGD(TAG, "frame #%u %ux%u %u B rejected (%u)", ev->seq, w, h,
                 (unsigned)len, ack);
    }
    notify(NAV_ST_ACK, ack, ev->seq, (uint16_t)(ms > 0xFFFF ? 0xFFFF : ms));
}

/* Compose the current view from the tiles we hold and put it on screen.
 * Cheap — a screenful is 768 KB of memcpy — so it runs on every position
 * update and again whenever a tile that might be visible arrives. */
static void render_view(void)
{
    uint16_t *dst = nav_screen_back_buffer();
    if (!dst) return;            /* the last one is still on its way out */
    if (ui_mode_get() != UI_MODE_NAV) return;
    const int64_t t0 = esp_timer_get_time();
    int wanted = 0;
    const int have = nav_map_render(dst, NAV_SCREEN_W, NAV_SCREEN_H, &wanted);
    esp_cache_msync(dst, nav_screen_back_buffer_bytes(),
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    nav_screen_commit();
    nav_screen_set_streaming(true);
    s_stats.renders++;
    s_stats.render_last_ms = (uint32_t)((esp_timer_get_time() - t0) / 1000);
    s_stats.last_have = have;
    s_stats.last_wanted = wanted;
    s_last_frame_us = esp_timer_get_time();
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

static void worker(void *arg)
{
    (void)arg;
    jpeg_decode_engine_cfg_t ecfg = { .intr_priority = 0, .timeout_ms = 400 };
    if (jpeg_new_decoder_engine(&ecfg, &s_jpgd) != ESP_OK) {
        ESP_LOGE(TAG, "jpeg engine init failed — navigator frames disabled");
        s_jpgd = NULL;
    }
    ppa_client_config_t pcfg = { .oper_type = PPA_OPERATION_SRM };
    if (ppa_register_client(&pcfg, &s_ppa) != ESP_OK) {
        ESP_LOGE(TAG, "ppa client init failed — navigator frames disabled");
        s_ppa = NULL;
    }
    if (esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &s_cache_line) != ESP_OK ||
        s_cache_line == 0) {
        s_cache_line = 64;
    }

    for (;;) {
        nav_evt_t ev;
        if (xQueueReceive(s_q, &ev, pdMS_TO_TICKS(NAV_FRAME_MS)) != pdTRUE) {
            const int64_t now = esp_timer_get_time();
            /* Carry the view forward and redraw, so the map glides instead of
             * stepping twice a second. Only while the phone is actually
             * feeding us — otherwise we would drift off on a stale speed. */
            if (s_stats.streaming && s_last_view_us != 0 &&
                now - s_last_view_us < NAV_IDLE_US) {
                const uint32_t dt = (uint32_t)((now - s_last_step_us) / 1000);
                s_last_step_us = now;
                nav_map_dead_reckon(dt);
                render_view();
            }
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
            continue;
        }
        switch (ev.kind) {
            case EV_FRAME:
                if (!s_jpgd || !s_ppa) {
                    s_state = ST_IDLE;
                    notify(NAV_ST_ACK, NAV_ACK_DECODE, ev.seq, 0);
                    break;
                }
                s_stats.streaming = true;
                nav_screen_set_streaming(true);
                set_boost(true);
                handle_frame(&ev);
                break;
            case EV_TILE:
                s_stats.streaming = true;
                set_boost(true);
                handle_tile(&ev);
                break;
            case EV_VIEW:
                s_stats.views++;
                s_stats.streaming = true;
                s_last_view_us = esp_timer_get_time();
                s_last_step_us = s_last_view_us;
                nav_screen_set_streaming(true);
                set_boost(true);
                render_view();
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
                notify_state();
                break;
        }
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
    /* 8 KiB, like the other two workers on this link. Four was enough when
     * this task only memcpy'd, but it now runs libpng — whose simplified read
     * API is generous with the stack — and composes the view on top. A task
     * that overflows here takes the whole device down with it. */
    xTaskCreatePinnedToCore(worker, "ble_nav", 8192, NULL, 5, &s_task, 0);
    ESP_LOGI(TAG, "ble_nav ready");
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

#if CONFIG_DEBUG_UART_BRIDGE
uint8_t ble_nav_debug_present(const uint8_t *jpeg, uint32_t len,
                              uint16_t w, uint16_t h)
{
    if (!s_jpgd || !s_ppa) return NAV_ACK_DECODE;
    /* Runs on the console task, not the worker. Safe only because the phone
     * is not streaming while a human drives the bridge — this is a bench
     * shortcut, not a second producer. */
    nav_screen_set_streaming(true);
    return present_frame(jpeg, w, h, len);
}
#endif

/* ---- routed from notif_bridge access_cb (NimBLE host task) ---- */

static void reject(uint16_t seq, uint8_t reason)
{
    s_state = ST_IDLE;
    nav_evt_t ev = { .kind = EV_REJECT, .seq = seq, .reason = reason };
    if (s_q) xQueueSend(s_q, &ev, 0);
}

void ble_nav_ctrl_write(const uint8_t *data, uint16_t len)
{
    if (len < 1 || !s_q || !s_stage) return;
    switch (data[0]) {
        case NAV_OP_BEGIN: {
            if (len < NAV_BEGIN_LEN) { reject(0, NAV_ACK_BAD_PARAM); return; }
            const uint16_t w   = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
            const uint16_t h   = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
            const uint32_t tot = (uint32_t)data[5] | ((uint32_t)data[6] << 8) |
                                 ((uint32_t)data[7] << 16) | ((uint32_t)data[8] << 24);
            const uint16_t seq = (uint16_t)data[9] | ((uint16_t)data[10] << 8);
            if (s_state == ST_DECODING) { reject(seq, NAV_ACK_BUSY); return; }
            /* The picture is scaled to the whole panel, so it has to arrive in
             * the panel's proportions, and both axes must be whole MCUs. */
            if (tot == 0 || tot > BLE_NAV_MAX_FRAME ||
                w == 0 || h == 0 || w > NAV_SCREEN_W || h > NAV_SCREEN_H ||
                (w % 16) != 0 || (h % 16) != 0 ||
                (uint32_t)w * NAV_SCREEN_H != (uint32_t)h * NAV_SCREEN_W) {
                reject(seq, NAV_ACK_BAD_PARAM);
                return;
            }
            s_w = w;
            s_h = h;
            s_expect = tot;
            s_got = 0;
            s_seq = seq;
            s_rx_kind = RX_FRAME;
            s_state = ST_RECEIVING;
            break;
        }
        case NAV_OP_END:
        case NAV_OP_TILE_END: {
            if (len < NAV_END_LEN) return;
            const uint16_t seq = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
            const rx_kind_t want = (data[0] == NAV_OP_TILE_END) ? RX_TILE : RX_FRAME;
            if (s_state != ST_RECEIVING || seq != s_seq || s_rx_kind != want) {
                reject(seq, NAV_ACK_TRUNCATED);
                return;
            }
            s_state = ST_DECODING;
            nav_evt_t ev = { .kind = (want == RX_TILE) ? EV_TILE : EV_FRAME,
                             .seq = seq };
            xQueueSend(s_q, &ev, 0);
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
            nav_map_set_view(lat_e7 / 1e7, lon_e7 / 1e7, zoom, heading);
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
        /* More than promised: the sender and we disagree about this frame, so
         * drop it rather than decode a mixture of two. */
        s_got = s_expect + 1;   /* handle_frame sees the mismatch */
        s_state = ST_RECEIVING;
        return;
    }
    memcpy(s_stage + s_got, data, len);
    s_got += len;
}
