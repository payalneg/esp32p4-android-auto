/* Map tiles the phone has already downloaded, kept on the head unit.
 *
 * The phone caches raster tiles for offline use anyway; rather than render a
 * picture and send it over and over, it forwards the tiles themselves. They
 * are static, so each one crosses the link once — after that the head unit
 * can redraw the map as often as it likes, from memory, and the phone only
 * has to say where the rider is.
 *
 * Tiles are decoded on arrival and held as RGB565 so a redraw is a plain
 * copy. At 256x256 that is 128 KB each, which is why the store is a
 * least-recently-used ring rather than everything the phone ever sent. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Slippy tiles are 256 px square. */
#define NAV_TILE_PX 256

/* Wire formats a tile can arrive in, as the protocol defines them.
 *
 * Only PNG is decoded. That is what the phone holds and what it has always
 * sent; the JPEG value stays here because it is part of the wire format, but
 * the hardware decoder behind it was a P4 part and a path nothing exercised,
 * so a tile that arrives as JPEG is now refused rather than decoded. */
#define NAV_TILE_FMT_PNG  0
#define NAV_TILE_FMT_JPEG 1

/* How many decoded tiles to keep.
 *
 * 96 x 128 KB = 12 MB of the ~25 MB free, which still leaves Wi-Fi, the video
 * decoder and the framebuffers their room.
 *
 * A view wants about forty-five — twelve on screen, eighteen in the ring, the
 * rest across the blurred layers — and a ride in progress wants a few more
 * ahead of it. Forty-eight thrashed within a minute. Sixty-four looked
 * enough and was not: at zoom 18 a screenful plus its ring is four times the
 * ground of zoom 17, and a 1.6 km ride evicted 890 tiles while the phone
 * re-sent the same ground over and over — every eviction is reported, and a
 * tile still wanted comes straight back. Watch `evicted` in `navstat`: it
 * should creep up as ground goes behind, not run away. */
#define NAV_TILE_SLOTS 96

/* How many of those slots the blurred fallback layers may hold. A third: the
 * detail still has room for a panel and its surroundings, and the coarse
 * layers survive long enough to be there when a hole opens. */
#define NAV_TILE_FALLBACK_RESERVE 32

/* PSRAM the cache will not take, however many slots are still empty.
 *
 * Ninety-six slots is 12 MB of tiles, which is fine on the P4 head units and
 * more than the ESP32-S3 board has: it leaves about 5.7 MB after the code it
 * runs from PSRAM and the framebuffers. The cache grew until the allocator
 * started refusing it — and the thing that lost the race was the notification
 * bridge, whose reassembly buffer is a single 192 KB block: three hours on the
 * bench logged 275 "reasm buf alloc fail" in the first four minutes, i.e. every
 * notification the phone sent while the map filled up was dropped.
 *
 * So the cache stops short and leaves this much for everyone else, reusing a
 * slot instead of claiming another. Size it for the largest single block the
 * rest of the firmware asks for, with room to spare. */
#define NAV_TILE_PSRAM_RESERVE (768 * 1024)

/* Told when a tile is dropped to make room. The phone keeps its own list of
 * what it has sent and will never send a tile twice, so it has to hear about
 * this or the hole would never be filled again. Worker task.
 *
 * Not called for a tile the microSD cache holds (nav_tile_store.c): that one
 * is not lost, it is one file read away, and asking the phone for it again
 * would put it back on the radio for no reason. */
typedef void (*nav_tiles_evict_cb_t)(uint8_t z, uint32_t x, uint32_t y);
void nav_tiles_set_evict_cb(nav_tiles_evict_cb_t cb);

void nav_tiles_init(void);

/* Decode and store one tile. Returns false if the format is unsupported, the
 * payload is broken, or there is no memory. Slow (a PNG is tens of
 * milliseconds) — worker task only. */
bool nav_tiles_put(uint8_t z, uint32_t x, uint32_t y,
                   uint8_t fmt, const uint8_t *data, size_t len);

/* Decoded pixels for a tile, or NULL if we do not have it. The pointer stays
 * valid until the tile is evicted, which only happens inside nav_tiles_put —
 * so a render pass that does not overlap a store is safe to hold it. */
const uint16_t *nav_tiles_get(uint8_t z, uint32_t x, uint32_t y);

/* Whether the tile is in the store, without touching its age. */
bool nav_tiles_have(uint8_t z, uint32_t x, uint32_t y);

/* Bring a tile back from the microSD cache into a slot. False when there is
 * no card, the card does not have this tile, or it is already in RAM — the
 * caller then leaves it to the phone, as before.
 *
 * Costs a file read plus a decode (tens of milliseconds for a PNG), which is
 * two orders of magnitude less than the same tile over BLE. Worker task. */
bool nav_tiles_load_cached(uint8_t z, uint32_t x, uint32_t y);

/* Forget everything — a new region, or a link that came back after long
 * enough that the phone will resend. */
void nav_tiles_clear(void);

/* The zoom the map is drawing, so the cache knows which tiles are the blurred
 * fallback layers and keeps a reserve of them against a drag. */
void nav_tiles_set_detail_zoom(uint8_t z);

typedef struct {
    uint16_t stored;       /* slots in use */
    uint16_t capacity;
    uint32_t decoded;      /* tiles decoded since boot */
    uint32_t rejected;     /* payloads we could not decode */
    uint32_t evicted;
    uint32_t evicted_quiet; /* of those, ones the card held — phone not told */
    uint32_t from_sd;      /* tiles brought back from the card */
    uint32_t last_ms;      /* how long the last decode took */
    uint32_t bytes_last;
} nav_tiles_stats_t;
void nav_tiles_get_stats(nav_tiles_stats_t *out);

#ifdef __cplusplus
}
#endif
