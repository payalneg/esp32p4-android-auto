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

/* Wire formats a tile can arrive in. PNG is what the phone already holds, so
 * it costs nothing to forward; JPEG is a third smaller on the wire and
 * decodes in hardware, at the cost of transcoding on the phone and of
 * fringing on coloured labels. The format travels with each tile so the two
 * can be mixed and the choice revisited without a protocol change. */
#define NAV_TILE_FMT_PNG  0
#define NAV_TILE_FMT_JPEG 1

/* How many decoded tiles to keep.
 *
 * 48 x 128 KB = 6 MB. A screenful is twelve, the ring around it another
 * eighteen, and the two blurred fallback layers a dozen more — so this holds
 * everything in view with room to spare while leaving most of the ~25 MB of
 * free PSRAM to Wi-Fi, the video decoder and everything else. It was 96 for a
 * while, which claimed half the PSRAM for tiles nobody was looking at. */
#define NAV_TILE_SLOTS 48

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

/* Forget everything — a new region, or a link that came back after long
 * enough that the phone will resend. */
void nav_tiles_clear(void);

typedef struct {
    uint16_t stored;       /* slots in use */
    uint16_t capacity;
    uint32_t decoded;      /* tiles decoded since boot */
    uint32_t rejected;     /* payloads we could not decode */
    uint32_t evicted;
    uint32_t last_ms;      /* how long the last decode took */
    uint32_t bytes_last;
} nav_tiles_stats_t;
void nav_tiles_get_stats(nav_tiles_stats_t *out);

#ifdef __cplusplus
}
#endif
