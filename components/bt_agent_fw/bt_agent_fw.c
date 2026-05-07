#include "bt_agent_fw.h"

#include "sdkconfig.h"

#ifdef CONFIG_BT_AGENT_OTA_ENABLED

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "zlib.h"

static const char *TAG = "bt_agent_fw";

/* EMBED_FILES with name "bt_agent_fw.bin.gz" — dots become underscores in
 * the generated symbol. */
extern const uint8_t _binary_bt_agent_fw_bin_gz_start[] asm("_binary_bt_agent_fw_bin_gz_start");
extern const uint8_t _binary_bt_agent_fw_bin_gz_end[]   asm("_binary_bt_agent_fw_bin_gz_end");

static uint8_t *s_decompressed;
static size_t   s_decompressed_size;
static bool     s_decompress_failed;

/* Gzip footer: ISIZE = original size mod 2^32, last 4 bytes little-endian.
 * Our blob is ~1 MiB so no wrap-around concern. */
static uint32_t gzip_footer_isize(const uint8_t *gz, size_t gz_size)
{
    if (gz_size < 8) return 0;
    return ((uint32_t)gz[gz_size - 4])       |
           ((uint32_t)gz[gz_size - 3] << 8)  |
           ((uint32_t)gz[gz_size - 2] << 16) |
           ((uint32_t)gz[gz_size - 1] << 24);
}

static esp_err_t decompress_once(void)
{
    if (s_decompressed) return ESP_OK;
    if (s_decompress_failed) return ESP_FAIL;

    const size_t gz_size = (size_t)(_binary_bt_agent_fw_bin_gz_end -
                                    _binary_bt_agent_fw_bin_gz_start);
    const uint32_t expected = gzip_footer_isize(_binary_bt_agent_fw_bin_gz_start,
                                                gz_size);
    if (expected == 0) {
        ESP_LOGE(TAG, "bad gzip blob (size=%u)", (unsigned)gz_size);
        s_decompress_failed = true;
        return ESP_FAIL;
    }

    /* PSRAM — agent firmware is ~1 MiB and we have 32 MiB, but internal RAM
     * is too tight to spare. Once the OTA finishes we don't free this; future
     * boots that find the agent already up will skip OTA so this isn't
     * allocated again. */
    uint8_t *buf = heap_caps_malloc(expected, MALLOC_CAP_SPIRAM);
    if (!buf) {
        ESP_LOGE(TAG, "psram alloc %u failed", (unsigned)expected);
        s_decompress_failed = true;
        return ESP_ERR_NO_MEM;
    }

    z_stream zs = { 0 };
    zs.next_in   = (Bytef *)_binary_bt_agent_fw_bin_gz_start;
    zs.avail_in  = (uInt)gz_size;
    zs.next_out  = (Bytef *)buf;
    zs.avail_out = (uInt)expected;
    /* windowBits 31 = 15 (max) + 16 (gzip wrapper). */
    if (inflateInit2(&zs, 31) != Z_OK) {
        ESP_LOGE(TAG, "inflateInit2 failed");
        free(buf);
        s_decompress_failed = true;
        return ESP_FAIL;
    }
    int rc = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);
    if (rc != Z_STREAM_END || zs.total_out != expected) {
        ESP_LOGE(TAG, "inflate rc=%d, out=%lu/%u",
                 rc, (unsigned long)zs.total_out, (unsigned)expected);
        free(buf);
        s_decompress_failed = true;
        return ESP_FAIL;
    }

    s_decompressed      = buf;
    s_decompressed_size = expected;
    ESP_LOGI(TAG, "decompressed %u → %u bytes (gzip, in PSRAM @%p)",
             (unsigned)gz_size, (unsigned)expected, buf);
    return ESP_OK;
}

const uint8_t *bt_agent_fw_data(void)
{
    return decompress_once() == ESP_OK ? s_decompressed : NULL;
}

size_t bt_agent_fw_size(void)
{
    return decompress_once() == ESP_OK ? s_decompressed_size : 0;
}

#else  /* !CONFIG_BT_AGENT_OTA_ENABLED */

const uint8_t *bt_agent_fw_data(void) { return NULL; }
size_t         bt_agent_fw_size(void) { return 0; }

#endif
