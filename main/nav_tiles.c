#include "nav_tiles.h"

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_private/esp_cache_private.h"
#include "esp_timer.h"
#include "png.h"

#include "nav_tile_store.h"

static const char *TAG = "nav_tiles";

#define TILE_BYTES ((size_t)NAV_TILE_PX * NAV_TILE_PX * 2)

typedef struct {
    uint8_t   z;
    uint32_t  x, y;
    bool      used;
    uint32_t  age;        /* bumped on every touch; smallest is oldest */
    bool      on_sd;      /* the card holds the compressed original */
    uint16_t *px;         /* RGB565, TILE_BYTES */
} slot_t;

static slot_t s_slots[NAV_TILE_SLOTS];

/* The zoom the map is drawing. Tiles well below it are the blurred layers
 * that fill holes, and they are only ever read while a hole is on screen —
 * which under a plain LRU makes them the oldest thing in the cache and the
 * first to go. They were then missing at the one moment they exist for, and
 * a dragged map showed bare ground instead of a coarse version of itself. */
static uint8_t s_detail_z;

static uint32_t s_tick;
static bool s_inited;
static size_t s_cache_line = 64;

static nav_tiles_stats_t s_stats;
static nav_tiles_evict_cb_t s_evict_cb;

static slot_t *find(uint8_t z, uint32_t x, uint32_t y)
{
    for (int i = 0; i < NAV_TILE_SLOTS; i++) {
        slot_t *s = &s_slots[i];
        if (s->used && s->z == z && s->x == x && s->y == y) return s;
    }
    return NULL;
}

/* A tile from one of the blurred fallback layers rather than the detail the
 * map is drawing. Two levels down is already a quarter of the detail in each
 * direction, which is the point where it is worth keeping for the holes. */
static bool is_fallback(const slot_t *s)
{
    return s_detail_z > 1 && s->z + 2 <= s_detail_z;
}

/* A free slot, or the least recently touched one. Its pixels are reused, so
 * the caller must fill them before anyone reads.
 *
 * Detail tiles are given up first. The fallback layers keep a reserve of the
 * cache: they cover enormous ground for their size — one tile six levels down
 * spans sixty-four by sixty-four of the detail — so holding a few of them is
 * what makes a drag or a fast ride show a blurred map instead of nothing. */
static slot_t *claim(void)
{
    slot_t *oldest = NULL, *oldest_fb = NULL;
    int fallbacks = 0;
    for (int i = 0; i < NAV_TILE_SLOTS; i++) {
        slot_t *s = &s_slots[i];
        if (!s->px) {
            /* Growing into the last of PSRAM is not free: the notification
             * bridge needs one 192 KB block and cannot borrow a tile slot, so
             * a cache that fills the heap silently costs the rider their
             * notifications (see NAV_TILE_PSRAM_RESERVE). Stop short and reuse
             * a slot instead — the cache is a cache, it loses nothing but
             * speed. */
            if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) <
                    TILE_BYTES + NAV_TILE_PSRAM_RESERVE) {
                continue;
            }
            s->px = heap_caps_aligned_calloc(s_cache_line, 1, TILE_BYTES,
                                             MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
            if (!s->px) continue;          /* PSRAM is full — reuse instead */
        }
        if (!s->used) return s;
        if (is_fallback(s)) {
            fallbacks++;
            if (!oldest_fb || s->age < oldest_fb->age) oldest_fb = s;
        } else if (!oldest || s->age < oldest->age) {
            oldest = s;
        }
    }
    /* Past the reserve they are no longer a safety net, just old ground —
     * a ride across a city would otherwise fill the cache with the parts of
     * it already behind us. */
    if (fallbacks > NAV_TILE_FALLBACK_RESERVE && oldest_fb) oldest = oldest_fb;
    if (!oldest) oldest = oldest_fb;
    if (oldest) {
        s_stats.evicted++;
        /* Say so before the pixels are overwritten: the phone's idea of what
         * we hold is the only thing that decides whether this tile is ever
         * sent again.
         *
         * Unless the card has it. Then losing the slot costs a file read, not
         * a radio round trip, and telling the phone would only make it send
         * over BLE what we can already read in a few milliseconds. */
        if (oldest->on_sd) {
            s_stats.evicted_quiet++;
        } else if (s_evict_cb) {
            s_evict_cb(oldest->z, oldest->x, oldest->y);
        }
        oldest->used = false;
        oldest->on_sd = false;
    }
    return oldest;
}

/* ---- decoders ---- */

static bool decode_png(const uint8_t *data, size_t len, uint16_t *out)
{
    png_image img;
    memset(&img, 0, sizeof(img));
    img.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_memory(&img, data, len)) {
        ESP_LOGW(TAG, "png header: %s", img.message);
        return false;
    }
    if (img.width != NAV_TILE_PX || img.height != NAV_TILE_PX) {
        ESP_LOGW(TAG, "png is %ux%u, expected %dx%d",
                 (unsigned)img.width, (unsigned)img.height,
                 NAV_TILE_PX, NAV_TILE_PX);
        png_image_free(&img);
        return false;
    }
    /* RGB rather than RGBA: a map tile is opaque, and 192 KB of scratch is
     * better than 256 KB. libpng's simplified API writes whole rows, so the
     * conversion to RGB565 is a second pass over PSRAM — still far cheaper
     * than the inflate it just did. */
    img.format = PNG_FORMAT_RGB;
    const size_t rgb_bytes = (size_t)PNG_IMAGE_SIZE(img);
    uint8_t *rgb = heap_caps_malloc(rgb_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!rgb) {
        png_image_free(&img);
        return false;
    }
    const bool ok = png_image_finish_read(&img, NULL, rgb, 0, NULL);
    if (!ok) ESP_LOGW(TAG, "png body: %s", img.message);
    png_image_free(&img);
    if (ok) {
        const uint8_t *p = rgb;
        for (size_t i = 0; i < (size_t)NAV_TILE_PX * NAV_TILE_PX; i++, p += 3) {
            out[i] = (uint16_t)(((p[0] & 0xF8) << 8) |
                                ((p[1] & 0xFC) << 3) |
                                 (p[2] >> 3));
        }
    }
    heap_caps_free(rgb);
    return ok;
}

/* ---- public ---- */

void nav_tiles_set_evict_cb(nav_tiles_evict_cb_t cb) { s_evict_cb = cb; }

void nav_tiles_init(void)
{
    if (s_inited) return;
    if (esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &s_cache_line) != ESP_OK ||
        s_cache_line == 0) {
        s_cache_line = 64;
    }
    s_stats.capacity = NAV_TILE_SLOTS;
    s_inited = true;
    ESP_LOGI(TAG, "tile store ready (up to %d x %u KB)",
             NAV_TILE_SLOTS, (unsigned)(TILE_BYTES / 1024));
}

/* The body of both ways a tile lands in a slot. `from_sd` says the bytes
 * came off the card, so they are not written back to it. */
static bool put_common(uint8_t z, uint32_t x, uint32_t y,
                       uint8_t fmt, const uint8_t *data, size_t len,
                       bool from_sd)
{
    if (!s_inited) nav_tiles_init();
    if (!data || len == 0) return false;

    slot_t *s = find(z, x, y);
    if (!s) s = claim();
    if (!s || !s->px) {
        s_stats.rejected++;
        return false;
    }

    const int64_t t0 = esp_timer_get_time();
    /* PNG and nothing else. The wire still carries a format byte per tile —
     * the app has always set it to PNG (kTileFormatPng in head_unit_feed.dart)
     * — but the hardware JPEG engine that used to sit behind the other value
     * is a P4 part, and a decoder nobody sends work to is a decoder nobody
     * tests. A tile in any other format is refused here. */
    bool ok;
    if (fmt == NAV_TILE_FMT_PNG) {
        ok = decode_png(data, len, s->px);
    } else {
        ESP_LOGW(TAG, "tile format %u is not PNG — refused", fmt);
        ok = false;
    }
    s_stats.last_ms = (uint32_t)((esp_timer_get_time() - t0) / 1000);
    s_stats.bytes_last = (uint32_t)len;

    if (!ok) {
        /* A slot with half a tile in it is worse than no tile. */
        s->used = false;
        s_stats.rejected++;
        return false;
    }
    s->z = z;
    s->x = x;
    s->y = y;
    s->used = true;
    s->age = ++s_tick;
    /* Keep the compressed original on the card, if there is one. This is what
     * lets the eviction above stay quiet later: the tile crosses the link
     * once in its life instead of once per time the ground comes round. */
    s->on_sd = from_sd ||
               nav_tile_store_save(z, x, y, fmt, data, len);
    s_stats.decoded++;
    s_stats.stored = 0;
    for (int i = 0; i < NAV_TILE_SLOTS; i++) if (s_slots[i].used) s_stats.stored++;
    return true;
}

bool nav_tiles_put(uint8_t z, uint32_t x, uint32_t y,
                   uint8_t fmt, const uint8_t *data, size_t len)
{
    return put_common(z, x, y, fmt, data, len, false);
}

bool nav_tiles_load_cached(uint8_t z, uint32_t x, uint32_t y)
{
    if (!s_inited) nav_tiles_init();
    if (find(z, x, y)) return false;       /* already in RAM */

    uint8_t fmt = 0;
    size_t len = 0;
    uint8_t *data = nav_tile_store_read(z, x, y, &fmt, &len);
    if (!data) return false;
    const bool ok = put_common(z, x, y, fmt, data, len, true);
    heap_caps_free(data);
    if (ok) s_stats.from_sd++;
    return ok;
}

const uint16_t *nav_tiles_get(uint8_t z, uint32_t x, uint32_t y)
{
    slot_t *s = find(z, x, y);
    if (!s) return NULL;
    s->age = ++s_tick;
    return s->px;
}

bool nav_tiles_have(uint8_t z, uint32_t x, uint32_t y)
{
    return find(z, x, y) != NULL;
}

void nav_tiles_clear(void)
{
    for (int i = 0; i < NAV_TILE_SLOTS; i++) {
        s_slots[i].used = false;
        s_slots[i].on_sd = false;
    }
    s_stats.stored = 0;
}

void nav_tiles_set_detail_zoom(uint8_t z)
{
    s_detail_z = z;
}

void nav_tiles_get_stats(nav_tiles_stats_t *out)
{
    if (out) *out = s_stats;
}
