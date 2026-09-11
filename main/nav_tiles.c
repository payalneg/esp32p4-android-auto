#include "nav_tiles.h"

#include <string.h>

#include "driver/jpeg_decode.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_private/esp_cache_private.h"
#include "esp_timer.h"
#include "png.h"

static const char *TAG = "nav_tiles";

#define TILE_BYTES ((size_t)NAV_TILE_PX * NAV_TILE_PX * 2)

typedef struct {
    uint8_t   z;
    uint32_t  x, y;
    bool      used;
    uint32_t  age;        /* bumped on every touch; smallest is oldest */
    uint16_t *px;         /* RGB565, TILE_BYTES */
} slot_t;

static slot_t s_slots[NAV_TILE_SLOTS];
static uint32_t s_tick;
static bool s_inited;
static size_t s_cache_line = 64;

static jpeg_decoder_handle_t s_jpgd;
static uint8_t *s_scratch;          /* DMA-aligned copy for the JPEG engine */
static size_t   s_scratch_cap;

static nav_tiles_stats_t s_stats;

static slot_t *find(uint8_t z, uint32_t x, uint32_t y)
{
    for (int i = 0; i < NAV_TILE_SLOTS; i++) {
        slot_t *s = &s_slots[i];
        if (s->used && s->z == z && s->x == x && s->y == y) return s;
    }
    return NULL;
}

/* A free slot, or the least recently touched one. Its pixels are reused, so
 * the caller must fill them before anyone reads. */
static slot_t *claim(void)
{
    slot_t *oldest = NULL;
    for (int i = 0; i < NAV_TILE_SLOTS; i++) {
        slot_t *s = &s_slots[i];
        if (!s->px) {
            s->px = heap_caps_aligned_calloc(s_cache_line, 1, TILE_BYTES,
                                             MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
            if (!s->px) continue;          /* PSRAM is full — reuse instead */
        }
        if (!s->used) return s;
        if (!oldest || s->age < oldest->age) oldest = s;
    }
    if (oldest) s_stats.evicted++;
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

static bool ensure_scratch(size_t need)
{
    if (need <= s_scratch_cap) return true;
    if (s_scratch) heap_caps_free(s_scratch);
    const size_t alloc = (need + 1023) & ~(size_t)1023;
    s_scratch = heap_caps_aligned_calloc(s_cache_line, 1, alloc,
                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    s_scratch_cap = s_scratch ? alloc : 0;
    return s_scratch != NULL;
}

static bool decode_jpeg(const uint8_t *data, size_t len, uint16_t *out)
{
    if (!s_jpgd) {
        jpeg_decode_engine_cfg_t cfg = { .intr_priority = 0, .timeout_ms = 400 };
        if (jpeg_new_decoder_engine(&cfg, &s_jpgd) != ESP_OK) {
            ESP_LOGE(TAG, "jpeg engine unavailable");
            return false;
        }
    }
    if (!ensure_scratch(len)) return false;
    memcpy(s_scratch, data, len);
    jpeg_decode_cfg_t cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
    };
    uint32_t out_size = 0;
    esp_err_t r = jpeg_decoder_process(s_jpgd, &cfg, s_scratch, len,
                                       (uint8_t *)out, TILE_BYTES, &out_size);
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "jpeg decode: %s", esp_err_to_name(r));
        return false;
    }
    return true;
}

/* ---- public ---- */

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

bool nav_tiles_put(uint8_t z, uint32_t x, uint32_t y,
                   uint8_t fmt, const uint8_t *data, size_t len)
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
    bool ok;
    switch (fmt) {
        case NAV_TILE_FMT_PNG:  ok = decode_png(data, len, s->px); break;
        case NAV_TILE_FMT_JPEG: ok = decode_jpeg(data, len, s->px); break;
        default:
            ESP_LOGW(TAG, "unknown tile format %u", fmt);
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
    if (fmt == NAV_TILE_FMT_JPEG) {
        /* The JPEG engine wrote these by DMA, behind the cache: drop what the
         * cache thinks it knows so the render pass reads the real pixels.
         *
         * Only for that path. The PNG decoder writes with the CPU, so the
         * fresh pixels ARE the dirty cache lines — invalidating them threw
         * the tile away line by line and the map came out in black bands. */
        esp_cache_msync(s->px, TILE_BYTES,
                        ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    }
    s->z = z;
    s->x = x;
    s->y = y;
    s->used = true;
    s->age = ++s_tick;
    s_stats.decoded++;
    s_stats.stored = 0;
    for (int i = 0; i < NAV_TILE_SLOTS; i++) if (s_slots[i].used) s_stats.stored++;
    return true;
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
    for (int i = 0; i < NAV_TILE_SLOTS; i++) s_slots[i].used = false;
    s_stats.stored = 0;
}

void nav_tiles_get_stats(nav_tiles_stats_t *out)
{
    if (out) *out = s_stats;
}
