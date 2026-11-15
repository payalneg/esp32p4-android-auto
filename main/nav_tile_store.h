/* The tile cache that survives a slot being reused: a microSD card.
 *
 * nav_tiles.c holds decoded tiles in PSRAM, 96 of them, and has to give a
 * slot up as soon as the ground moves. With a card in the slot, the original
 * bytes stay on it: a PNG the phone sent, or an NVT1 vector tile the rider
 * copied there. Eviction then costs a file read instead of a radio round
 * trip, and the phone is never asked twice.
 *
 * The layout is `{z}/{x}/{y}.png` (phone cache) and `{z}/{x}/{y}.vtile`
 * (packer) under /sdcard/navtiles; /sdcard/vtiles is accepted as the older
 * root for vector packages.
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
 * not pay for a failed open every pass.
 *
 * A `.vtile` is preferred over a `.png` for the same z/x/y. `fmt` says which
 * one came back. A vector package may live at either `/sdcard/navtiles` or
 * the older `/sdcard/vtiles`, so a card prepared for the map prototype works
 * unchanged. */
uint8_t *nav_tile_store_read(uint8_t z, uint32_t x, uint32_t y,
                             uint8_t *fmt, size_t *len);

/* Whether the card holds an NVT1 package (`vector.idx` beside the zoom
 * folders). Cheap after the first answer; a missing marker is re-checked
 * when the card is remounted, not on every pass. */
bool nav_tile_store_vector_ready(void);

/* The zooms the package covers. 0/0 when there is no package. The prefetch
 * uses this to stop asking the card for the z-6 fallback when the package
 * starts at z11: each miss would be a failed open on the render path. */
void nav_tile_store_vector_zooms(uint8_t *zmin, uint8_t *zmax);

typedef struct {
    bool     ready;
    bool     vector;      /* an NVT1 package is present */
    uint32_t saved;       /* tiles written to the card since boot */
    uint32_t loaded;      /* tiles read back */
    uint32_t missing;     /* reads the card could not answer */
    uint32_t errors;      /* failed writes and reads */
} nav_tile_store_stats_t;
void nav_tile_store_get_stats(nav_tile_store_stats_t *out);

#ifdef __cplusplus
}
#endif
