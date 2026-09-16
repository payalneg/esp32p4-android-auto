#include "nav_tile_store.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "bsp/esp-bsp.h"          /* BSP_SD_MOUNT_POINT, bsp_sdcard_mount */
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "nav_tiles.h"            /* NAV_TILE_FMT_* */

static const char *TAG = "nav_tile_store";

#define ROOT  BSP_SD_MOUNT_POINT "/navtiles"

/* How often to look for a card while there is none.
 *
 * Mounting a slot with nothing in it is not free — it is a bus timeout on the
 * worker that also composes the map — so the wait grows: a card pushed in is
 * picked up within ten seconds, and a ride that never has one settles down to
 * one pointless attempt a minute. */
#define MOUNT_RETRY_MIN_US (10 * 1000 * 1000)
#define MOUNT_RETRY_MAX_US (60 * 1000 * 1000)

/* Consecutive failures that mean the card has gone (pulled out mid-ride is
 * the usual way). The mount is dropped and retried like a cold start. */
#define FAILS_BEFORE_REMOUNT 5

/* Tiles the card did not have, so a view over uncovered ground does not pay
 * for a failed open on every pass. A ring: the oldest misses fall out, which
 * is also what lets a tile that is later written be found again. */
#define MISS_SLOTS 256

static bool     s_ready;
static int64_t  s_next_try_us;
static int64_t  s_retry_us = MOUNT_RETRY_MIN_US;
static int      s_fails;
/* The card is full. Reading still works and is still worth a great deal —
 * what is already on it is the map — so a full card stops writes only. Without
 * this the ENOSPC run would look like a dying card and take the reads with
 * it. */
static bool     s_full;
static uint64_t s_miss[MISS_SLOTS];
static uint16_t s_miss_head;
static nav_tile_store_stats_t s_stats;

/* z is at most 19 and x, y below 2^19 at that zoom, so this is unique and
 * never zero for a real tile — zero is the empty slot. */
static uint64_t key_of(uint8_t z, uint32_t x, uint32_t y)
{
    return ((uint64_t)z << 48) | ((uint64_t)x << 24) | (uint64_t)y;
}

static bool miss_known(uint64_t k)
{
    for (int i = 0; i < MISS_SLOTS; i++) if (s_miss[i] == k) return true;
    return false;
}

static void miss_add(uint64_t k)
{
    s_miss[s_miss_head] = k;
    s_miss_head = (uint16_t)((s_miss_head + 1) % MISS_SLOTS);
}

static void miss_forget(uint64_t k)
{
    for (int i = 0; i < MISS_SLOTS; i++) if (s_miss[i] == k) s_miss[i] = 0;
}

static void note_failure(void)
{
    s_stats.errors++;
    if (++s_fails < FAILS_BEFORE_REMOUNT) return;
    /* Treat it as no card at all: the next ready() remounts, and until then
     * the map behaves exactly as it does without one. */
    ESP_LOGW(TAG, "%d failures in a row — dropping the card until it mounts again",
             s_fails);
    s_ready = false;
    s_fails = 0;
    s_full = false;
    s_retry_us = MOUNT_RETRY_MIN_US;
    s_stats.ready = false;
    s_next_try_us = esp_timer_get_time() + s_retry_us;
    memset(s_miss, 0, sizeof s_miss);
}

static void note_success(void) { s_fails = 0; }

bool nav_tile_store_ready(void)
{
    if (s_ready) return true;
    const int64_t now = esp_timer_get_time();
    if (now < s_next_try_us) return false;
    s_next_try_us = now + s_retry_us;
    if (s_retry_us < MOUNT_RETRY_MAX_US) s_retry_us *= 2;

    const esp_err_t e = bsp_sdcard_mount();
    /* Already mounted is the normal case: the file browser, the web server
     * and the splash all mount the same card. */
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) return false;
    if (mkdir(ROOT, 0777) != 0 && errno != EEXIST) {
        ESP_LOGW(TAG, "cannot use %s: %s", ROOT, strerror(errno));
        return false;
    }
    s_retry_us = MOUNT_RETRY_MIN_US;
    s_ready = true;
    s_full = false;
    s_stats.ready = true;
    ESP_LOGI(TAG, "tile cache on the card at %s", ROOT);
    return true;
}

/* ROOT/z/x/y.png — the same shape the phone's own tile cache uses, so a
 * region can be put on the card with a file copy.
 *
 * PNG only. The wire carries a format per tile and the decoder handles JPEG
 * too, but the app sends PNG and nothing else (kTileFormatPng in
 * head_unit_feed.dart) — so a second extension here would be a path nothing
 * walks, plus a failed open per tile for the privilege. */
static void tile_path(char *buf, size_t n, uint8_t z, uint32_t x, uint32_t y)
{
    snprintf(buf, n, ROOT "/%u/%lu/%lu.png", (unsigned)z, (unsigned long)x,
             (unsigned long)y);
}

/* Make ROOT/z and ROOT/z/x. Only called after a write failed for want of
 * them, so the common case is two syscalls lighter. */
static bool make_dirs(uint8_t z, uint32_t x)
{
    char dir[64];
    snprintf(dir, sizeof dir, ROOT "/%u", (unsigned)z);
    if (mkdir(dir, 0777) != 0 && errno != EEXIST) return false;
    snprintf(dir, sizeof dir, ROOT "/%u/%lu", (unsigned)z, (unsigned long)x);
    if (mkdir(dir, 0777) != 0 && errno != EEXIST) return false;
    return true;
}

static bool write_file(const char *path, const uint8_t *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    const bool ok = fwrite(data, 1, len, f) == len;
    /* fclose can be where a full card finally says so. */
    return (fclose(f) == 0) && ok;
}

bool nav_tile_store_save(uint8_t z, uint32_t x, uint32_t y, uint8_t fmt,
                         const uint8_t *data, size_t len)
{
    if (!data || len == 0 || len > NAV_TILE_STORE_MAX_BYTES) return false;
    /* Anything but PNG is not kept, and says so: the slot it is in must then
     * tell the phone when it goes, because nothing here can give it back. */
    if (fmt != NAV_TILE_FMT_PNG) return false;
    if (s_full) return false;
    if (!nav_tile_store_ready()) return false;

    char path[96], tmp[96];
    tile_path(path, sizeof path, z, x, y);
    /* Write beside the real name and rename: a card pulled out mid-write
     * then leaves no half tile for the next ride to decode. */
    snprintf(tmp, sizeof tmp, ROOT "/%u/%lu/%lu.tmp", (unsigned)z,
             (unsigned long)x, (unsigned long)y);

    bool ok = write_file(tmp, data, len);
    if (!ok && errno != ENOSPC && make_dirs(z, x)) ok = write_file(tmp, data, len);
    if (!ok) {
        remove(tmp);
        if (errno == ENOSPC) {
            ESP_LOGW(TAG, "card full — keeping what is on it, storing no more");
            s_full = true;
            return false;
        }
        ESP_LOGD(TAG, "save %s: %s", path, strerror(errno));
        note_failure();
        return false;
    }
    /* rename over an existing file fails on FATFS, so clear the way first.
     * The phone does not send a tile twice, but a card carried over from an
     * earlier ride has plenty of them. */
    remove(path);
    if (rename(tmp, path) != 0) {
        remove(tmp);
        note_failure();
        return false;
    }
    note_success();
    s_stats.saved++;
    /* It exists now, so forget any earlier answer that it did not. */
    miss_forget(key_of(z, x, y));
    return true;
}

static uint8_t *read_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    uint8_t *buf = NULL;
    long len = 0;
    if (fseek(f, 0, SEEK_END) == 0 && (len = ftell(f)) > 0 &&
        len <= NAV_TILE_STORE_MAX_BYTES && fseek(f, 0, SEEK_SET) == 0) {
        buf = heap_caps_malloc((size_t)len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (buf && fread(buf, 1, (size_t)len, f) != (size_t)len) {
            heap_caps_free(buf);
            buf = NULL;
        }
    }
    fclose(f);
    if (buf) *out_len = (size_t)len;
    return buf;
}

uint8_t *nav_tile_store_read(uint8_t z, uint32_t x, uint32_t y,
                             uint8_t *fmt, size_t *len)
{
    if (!fmt || !len) return NULL;
    if (!nav_tile_store_ready()) return NULL;

    const uint64_t key = key_of(z, x, y);
    if (miss_known(key)) return NULL;

    char path[96];
    tile_path(path, sizeof path, z, x, y);
    size_t n = 0;
    uint8_t *buf = read_file(path, &n);
    if (buf) {
        note_success();
        s_stats.loaded++;
        *fmt = NAV_TILE_FMT_PNG;
        *len = n;
        return buf;
    }
    /* Not there — remember, so the next pass over the same view is free. A
     * plain absence is not a card failure, so the remount counter is left
     * alone. */
    s_stats.missing++;
    miss_add(key);
    return NULL;
}

void nav_tile_store_get_stats(nav_tile_store_stats_t *out)
{
    if (out) *out = s_stats;
}
