/* Navigator frames from the phone over BLE.
 *
 * The companion app renders the map — it has the route, the graph and the
 * tiles — and sends the head unit a small JPEG of what to show. We decode it
 * with the P4's hardware JPEG engine, scale it to the panel with the PPA and
 * hand it to nav_screen.c. The head unit draws no map of its own.
 *
 * Lives on the NotifBridge GATT service (notif_bridge.c owns the chars and
 * routes writes here):
 *   NAV-CTRL CHR 7B4E4F00-...-000B  WRITE | NOTIFY  (phone<->P4 control)
 *   NAV-DATA CHR 7B4E4F00-...-000C  WRITE_NO_RSP    (phone->P4 JPEG bytes)
 *
 * Wire protocol (mirror flutter-application/lib/ble/nav_stream.dart):
 *
 *   CTRL write (phone -> P4):
 *     0x01 FRAME_BEGIN : [op][u16 w][u16 h][u32 len][u16 seq]   (11 bytes)
 *     0x02 FRAME_END   : [op][u16 seq]                          (3 bytes)
 *     0x03 STOP        : [op]        the app stopped streaming
 *     0x04 HELLO       : [op]        please send STATE
 *     0x05 TILE_BEGIN  : [op][u8 fmt][u8 z][u32 x][u32 y][u32 len][u16 seq]
 *     0x06 TILE_END    : [op][u16 seq]
 *     0x07 VIEW        : [op][i32 lat_e7][i32 lon_e7][u8 zoom][u16 heading]
 *                        [u16 speed_cm_s]
 *     0x08 ROUTE_BEGIN : [op][u16 points][u16 seq]   then DATA carries
 *                        points x [i32 lat_e7][i32 lon_e7]. **points = 0
 *                        clears the route** — no DATA follows, and the line
 *                        and the manoeuvre plate come off the screen.
 *     0x09 ROUTE_END   : [op][u16 seq]
 *     0x0A GUIDE       : [op][u8 turn][u16 dist_m][u32 remaining_m]
 *                        [u16 remaining_s][u8 flags]  (bit0 = off route)
 *
 *   CTRL notify (P4 -> phone), 6-byte frame [u8 status][u8 a][u16 b][u16 c] LE:
 *     0x10 STATE      a = ui mode (0 other, 1 navigator screen is live)
 *                     b = 1 while the rider is looking at it
 *                     c = largest DATA write we accept
 *     0x11 FRAME_ACK  a = result (NAV_ACK_*), b = seq, c = decode+scale ms
 *     0x12 TILE_ACK   a = result (NAV_ACK_*), b = seq, c = decode ms
 *     0x13 DEST       [u8 status][i32 lat_e7][i32 lon_e7] — nine bytes, not
 *                     the usual six: the rider picked somewhere to go by
 *                     tapping the head unit's own map. The phone routes to it
 *                     and starts guiding.
 *     0x14 DROPPED    [u8 status][u8 z][i32 x][i32 y] — ten bytes; a tile was
 *                     evicted to make room. The phone forgets it has sent it,
 *                     so the ground can be filled again.
 *     0x15 ZOOM       a = the zoom level the rider chose with the buttons on
 *                     the panel. The phone sends tiles (and VIEW) at that
 *                     level from then on; the head unit has already changed
 *                     what it draws.
 *
 *   DATA write (phone -> P4): raw bytes of whatever BEGIN opened — a picture
 *   for FRAME_BEGIN, a map tile for TILE_BEGIN.
 *
 * Two ways to put a map on the screen, then. The picture path (FRAME_*) has
 * the phone render everything and send it; simple, but every change costs a
 * whole JPEG and it stops the moment the phone's screen goes dark. The tile
 * path (TILE_*, VIEW) sends the tiles the phone already downloaded, once
 * each, and then only says where the rider is — a few tens of bytes a second,
 * with the head unit redrawing from memory as often as it likes. The tile
 * path is the one that keeps working with the phone in a pocket.
 *
 * Pacing is one frame in flight: the app waits for FRAME_ACK before starting
 * the next frame, so a single staging buffer is enough and a slow decode
 * throttles the sender instead of queueing up stale pictures. The app also
 * skips frames whose pixels did not change, so a parked bike costs no air
 * time at all.
 *
 * The JPEG must be baseline **yuv420**: the P4 decoder hardwires a
 * YUV420->RGB565 colour conversion for RGB565 output and turns 4:4:4 input
 * into chroma mush (same constraint as the animated boot splash). Width and
 * height must be multiples of 16 and keep the panel's 5:3 aspect, because the
 * PPA scales the picture to 800x480 without letterboxing. */
#pragma once

#include "sdkconfig.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Largest DATA write the receive path accepts (MTU 512 - 3 ATT header) —
 * notif_bridge's flatten buffer is sized from this, and STATE advertises it. */
#define BLE_NAV_MAX_DATA 509

/* Biggest JPEG we will stage. A 800x480 q80 photo-ish frame is well under
 * 100 KB; anything larger is a bug or a hostile peer. */
#define BLE_NAV_MAX_FRAME (96 * 1024)

/* FRAME_ACK results. */
#define NAV_ACK_OK        0
#define NAV_ACK_BAD_PARAM 1   /* size / aspect / length out of contract */
#define NAV_ACK_TRUNCATED 2   /* fewer (or more) bytes than BEGIN promised */
#define NAV_ACK_DECODE    3   /* the hardware decoder rejected the JPEG */
#define NAV_ACK_BUSY      4   /* previous frame still on its way to the panel */
#define NAV_ACK_HIDDEN    5   /* navigator screen is not the live screen */

void ble_nav_init(void);

/* Tell the phone the current mode / visibility. Safe from any task — the
 * notify itself happens on the worker. */
void ble_nav_notify_state(void);

/* Diagnostics for the `navstat` debug command. */
typedef struct {
    uint32_t frames_ok;
    uint32_t frames_failed;
    uint32_t last_bytes;
    uint16_t last_w, last_h;
    uint32_t last_decode_ms;
    uint8_t  last_ack;
    bool     streaming;
    /* Tile path. */
    uint32_t tiles_ok;
    uint32_t tiles_failed;
    uint32_t tile_last_bytes;
    uint32_t tile_last_ms;
    uint32_t views;
    uint32_t renders;
    uint32_t render_last_ms;
    int      last_have;     /* tiles present for the last composed view */
    int      last_wanted;
    uint32_t stack_free;    /* worker stack high-water mark, bytes */
} ble_nav_stats_t;
void ble_nav_get_stats(ble_nav_stats_t *out);

#if CONFIG_DEBUG_UART_BRIDGE
/* Bench shortcut: decode, scale and show a frame that never came over the
 * air, so the picture path can be exercised without a phone. Returns a
 * NAV_ACK_* code. Console task only, and only while nothing is streaming. */
uint8_t ble_nav_debug_present(const uint8_t *jpeg, uint32_t len,
                              uint16_t w, uint16_t h);
#endif

/* ---- notif_bridge wiring ---- */
void ble_nav_set_link(uint16_t conn_handle, uint16_t ctrl_val_handle);
void ble_nav_on_disconnect(void);

/* ---- routed from notif_bridge access_cb (NimBLE host task) ---- */
void ble_nav_ctrl_write(const uint8_t *data, uint16_t len);
void ble_nav_data_write(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif
