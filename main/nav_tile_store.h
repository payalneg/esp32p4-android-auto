/* The tile cache that survives a slot being reused — a microSD card.
 *
 * nav_tiles.c holds decoded tiles in PSRAM, 96 of them, and has to give a
 * slot up as soon as the ground moves. Today that means telling the phone
 * (it never sends the same tile twice, so it has to hear that one is gone)
 * and getting the tile again over BLE, where a 15 KB PNG costs seconds. A
 * 1.6 km ride evicted 890 tiles — the same ground, over and over.
 *
 * With a card in the slot the compressed original stays on it, so eviction
 * costs a file read instead of a radio round trip, and the phone is never
 * asked twice. Without a card nothing here does anything and the old
 * behaviour is exactly what happens.
 *
 * The layout is the phone's own: {z}/{x}/{y}.png under /sdcard/navtiles. That
 * is deliberate — the app caches raster tiles in the same shape, so filling
 * this cache can be a copy onto the card rather than a week of BLE.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Anything larger is not a map tile; refuse rather than allocate for it. */
#define NAV_TILE_STORE_MAX_BYTES (192 * 1024)

/* Whether a card is mounted and the cache directory is usable.
 *
 * Cheap once it has said yes. While it is saying no it retries the mount now
 * and then, so a card pushed in after boot is picked up without a reboot.
 * Worker task only — it touches the filesystem. */
bool nav_tile_store_ready(void);

/* Keep the bytes a tile arrived as. Returns whether the card now holds it,
 * which is what decides if the phone has to be told about an eviction. */
bool nav_tile_store_save(uint8_t z, uint32_t x, uint32_t y, uint8_t fmt,
                         const uint8_t *data, size_t len);

/* Read a tile back, or NULL when the card does not have it. The buffer is
 * PSRAM the caller frees with heap_caps_free. A tile that was not there is
 * remembered for a while, so a view over ground the card does not cover does
 * not pay for a failed open every pass. */
uint8_t *nav_tile_store_read(uint8_t z, uint32_t x, uint32_t y,
                             uint8_t *fmt, size_t *len);

typedef struct {
    bool     ready;
    uint32_t saved;       /* tiles written to the card since boot */
    uint32_t loaded;      /* tiles read back */
    uint32_t missing;     /* reads the card could not answer */
    uint32_t errors;      /* failed writes and reads */
} nav_tile_store_stats_t;
void nav_tile_store_get_stats(nav_tile_store_stats_t *out);

#ifdef __cplusplus
}
#endif
