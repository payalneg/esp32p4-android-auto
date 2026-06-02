#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "host/ble_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Custom GATT service that lets the Flutter companion app push notifications,
 * media metadata, and app/album icons to the head unit, and lets the head
 * unit send back media-transport commands.
 *
 *   Service: 7B4E4F00-3F8E-4D2A-9D5C-2C9F1A6E0001
 *   IN   CHR: 7B4E4F00-...-0002  WRITE_NO_RSP  (phone → P4)
 *   OUT  CHR: 7B4E4F00-...-0003  NOTIFY        (P4 → phone)
 *   ST   CHR: 7B4E4F00-...-0004  READ          (caps blob; reserved)
 *   TIME CHR: 7B4E4F00-...-0005  WRITE_NO_RSP  (phone → P4, [hour, minute])
 *   OTA  CHR: 7B4E4F00-...-0006  READ          (SoftAP creds + OTA endpoint
 *                                              + fw version for app-side OTA) */

void notif_bridge_init(void);

/* GATT-service wiring — mirrors ble_nus's hooks so ble_host.c can drive
 * both services from the same callbacks. */
const struct ble_gatt_svc_def *notif_bridge_get_svcs(void);
void notif_bridge_gatts_register_cb(struct ble_gatt_register_ctxt *ctxt,
                                    void *arg);
void notif_bridge_on_connect(uint16_t conn_handle);
void notif_bridge_on_disconnect(void);

/* Send a CMD frame (op + optional u32 arg) over OUT char. Used by LVGL
 * screens when the user taps play/pause/next/prev or dismiss. */
typedef enum {
    NOTIF_OP_PLAY            = 0x10,
    NOTIF_OP_PAUSE           = 0x11,
    NOTIF_OP_NEXT            = 0x12,
    NOTIF_OP_PREV            = 0x13,
    NOTIF_OP_DISMISS         = 0x14,
    NOTIF_OP_REQUEST_ICON    = 0x20,
    NOTIF_OP_REQUEST_ART     = 0x21,
} notif_op_t;

void notif_bridge_send_cmd(notif_op_t op, uint32_t arg);

/* ---------- decoded representations ---------- */

#define NOTIF_STR_MAX  128

typedef struct {
    uint32_t id;
    char     package[NOTIF_STR_MAX];
    char     app_name[NOTIF_STR_MAX];
    char     title[NOTIF_STR_MAX];
    char     text[NOTIF_STR_MAX * 2];
    uint64_t posted_at_ms;
    uint32_t icon_hash;
    bool     removed;
    bool     is_navigation;   /* category == "navigation" (turn-by-turn) */
} notif_msg_t;

typedef struct {
    char     title[NOTIF_STR_MAX];
    char     artist[NOTIF_STR_MAX];
    char     album[NOTIF_STR_MAX];
    uint32_t duration_ms;
    uint32_t position_ms;
    bool     is_playing;
    uint32_t album_art_hash;
    char     source_app[NOTIF_STR_MAX];
} media_state_t;

/* Read-only snapshots — UI may poll these from the LVGL task. Each
 * returns a pointer to a static struct guarded by a short critical
 * section; copy out fields you care about and don't hold the pointer. */
const media_state_t *notif_bridge_get_media(void);

/* Iterates the most recent notifications (oldest → newest). Returns
 * count; entries are copied into the caller buffer. */
size_t notif_bridge_recent(notif_msg_t *out, size_t max);

/* Monotonic counter — bumped on every accepted NEW notification. UI
 * code compares against its last-seen value to detect fresh arrivals
 * without scanning the whole ring on every poll tick. */
uint32_t notif_bridge_inbox_seq(void);

/* Phone-pushed wall clock. Returns true and fills hour/minute (local time
 * as sent by the companion app) when a fresh time write arrived within the
 * last 30 s; returns false otherwise so the UI can hide the clock label.
 * Safe to poll from the LVGL task. */
bool notif_bridge_get_phone_time(int *hour, int *minute);

/* PNG cache lookup — used by LVGL image widgets to draw icons/album art.
 * Returns NULL when the hash isn't cached; in that case the UI may call
 * notif_bridge_send_cmd(NOTIF_OP_REQUEST_ICON, hash) to fetch it. */
const uint8_t *notif_bridge_get_icon(uint32_t hash, size_t *out_len);

#ifdef __cplusplus
}
#endif
