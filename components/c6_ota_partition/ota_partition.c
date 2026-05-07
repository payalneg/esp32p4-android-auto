/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Reads the slave firmware (network_adapter.bin) embedded into the host
 * binary at link time and pushes it to the C6 over the ESP-Hosted SDIO
 * link in chunks. Progress is reported via a callback.
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_hosted.h"
#include "esp_hosted_api_types.h"
#include "esp_hosted_ota.h"
#include "esp_log.h"
#include "zlib.h"

#include "ota_partition.h"

static const char *TAG = "c6_ota_blob";

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 1500
#endif

/* Embedded as gzipped via tools/pack_fw_blobs.sh — we decompress into PSRAM
 * once at the start of OTA and treat the resulting buffer as the firmware
 * blob the rest of this file iterates over. Saves ~506 KiB of host flash
 * vs embedding the raw .bin. */
extern const uint8_t _slave_fw_gz_start[] asm("_binary_network_adapter_bin_gz_start");
extern const uint8_t _slave_fw_gz_end[]   asm("_binary_network_adapter_bin_gz_end");

static const uint8_t *slave_fw_start;
static size_t         slave_fw_blob_size;
static bool           slave_fw_decompress_failed;

static uint32_t gzip_footer_isize(const uint8_t *gz, size_t gz_size)
{
    if (gz_size < 8) return 0;
    return ((uint32_t)gz[gz_size - 4])       |
           ((uint32_t)gz[gz_size - 3] << 8)  |
           ((uint32_t)gz[gz_size - 2] << 16) |
           ((uint32_t)gz[gz_size - 1] << 24);
}

static esp_err_t slave_fw_decompress_once(void)
{
    if (slave_fw_start) return ESP_OK;
    if (slave_fw_decompress_failed) return ESP_FAIL;

    const size_t gz_size = (size_t)(_slave_fw_gz_end - _slave_fw_gz_start);
    const uint32_t expected = gzip_footer_isize(_slave_fw_gz_start, gz_size);
    if (expected == 0) {
        ESP_LOGE(TAG, "bad gzip blob (size=%u)", (unsigned)gz_size);
        slave_fw_decompress_failed = true;
        return ESP_FAIL;
    }
    uint8_t *buf = heap_caps_malloc(expected, MALLOC_CAP_SPIRAM);
    if (!buf) {
        ESP_LOGE(TAG, "psram alloc %u failed", (unsigned)expected);
        slave_fw_decompress_failed = true;
        return ESP_ERR_NO_MEM;
    }
    z_stream zs = { 0 };
    zs.next_in   = (Bytef *)_slave_fw_gz_start;
    zs.avail_in  = (uInt)gz_size;
    zs.next_out  = (Bytef *)buf;
    zs.avail_out = (uInt)expected;
    /* windowBits 31 = 15 (max) + 16 (gzip wrapper). */
    if (inflateInit2(&zs, 31) != Z_OK) {
        ESP_LOGE(TAG, "inflateInit2 failed");
        free(buf);
        slave_fw_decompress_failed = true;
        return ESP_FAIL;
    }
    int rc = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);
    if (rc != Z_STREAM_END || zs.total_out != expected) {
        ESP_LOGE(TAG, "inflate rc=%d, out=%lu/%u",
                 rc, (unsigned long)zs.total_out, (unsigned)expected);
        free(buf);
        slave_fw_decompress_failed = true;
        return ESP_FAIL;
    }
    slave_fw_start      = buf;
    slave_fw_blob_size  = expected;
    ESP_LOGI(TAG, "decompressed %u → %u bytes (gzip, in PSRAM @%p)",
             (unsigned)gz_size, (unsigned)expected, buf);
    return ESP_OK;
}

static void emit(ota_partition_progress_cb_t cb, void *ctx,
                 ota_partition_stage_t stage, uint32_t done, uint32_t total)
{
    if (cb) {
        cb(stage, done, total, ctx);
    }
}

/* Sanity-check the magic byte and pull the app-version string out of the
 * first segment's esp_app_desc_t. The blob is the full image we pass to
 * the slave OTA — its size is exactly blob_size (the gunzipped output),
 * no need to rederive it from segment headers. The previous version had
 * a hand-rolled padding+checksum calc that diverged from the canonical
 * ESP-IDF formula `(len + 1 + 15) & ~15` and rejected legitimate images. */
static esp_err_t parse_embedded_image(size_t blob_size,
                                      char *version, size_t version_len)
{
    if (blob_size < sizeof(esp_image_header_t) +
                    sizeof(esp_image_segment_header_t) +
                    sizeof(esp_app_desc_t)) {
        return ESP_ERR_INVALID_SIZE;
    }
    const esp_image_header_t *hdr = (const esp_image_header_t *)slave_fw_start;
    if (hdr->magic != ESP_IMAGE_HEADER_MAGIC) {
        ESP_LOGE(TAG, "bad magic 0x%02x in embedded slave fw", hdr->magic);
        return ESP_ERR_INVALID_ARG;
    }

    if (version) {
        const esp_app_desc_t *desc =
            (const esp_app_desc_t *)(slave_fw_start
                + sizeof(esp_image_header_t)
                + sizeof(esp_image_segment_header_t));
        strncpy(version, desc->version, version_len - 1);
        version[version_len - 1] = '\0';
    }
    return ESP_OK;
}

esp_err_t ota_partition_perform_with_cb(const char *partition_label,
                                        ota_partition_progress_cb_t cb,
                                        void *ctx)
{
    (void)partition_label;

    if (slave_fw_decompress_once() != ESP_OK) {
        return ESP_HOSTED_SLAVE_OTA_FAILED;
    }
    const size_t blob_size = slave_fw_blob_size;
    ESP_LOGI(TAG, "embedded slave fw: %u bytes (decompressed)",
             (unsigned)blob_size);
    emit(cb, ctx, OTA_PARTITION_STAGE_VALIDATE, 0, 0);

    char new_version[32] = { 0 };
    esp_err_t r = parse_embedded_image(blob_size, new_version, sizeof(new_version));
    if (r != ESP_OK) {
        return ESP_HOSTED_SLAVE_OTA_FAILED;
    }
    /* Send the whole gunzipped blob — slave OTA validates checksums
     * internally and stops at the documented image end. */
    size_t firmware_size = blob_size;
    ESP_LOGI(TAG, "image verified: %u bytes, version '%s'",
             (unsigned)firmware_size, new_version);

    emit(cb, ctx, OTA_PARTITION_STAGE_BEGIN, 0, firmware_size);
    if (esp_hosted_slave_ota_begin() != ESP_OK) {
        ESP_LOGE(TAG, "ota_begin failed");
        return ESP_HOSTED_SLAVE_OTA_FAILED;
    }

    size_t offset = 0;
    while (offset < firmware_size) {
        size_t to_send = (firmware_size - offset > CHUNK_SIZE) ? CHUNK_SIZE
                                                              : (firmware_size - offset);
        if (esp_hosted_slave_ota_write(slave_fw_start + offset, to_send) != ESP_OK) {
            ESP_LOGE(TAG, "ota_write failed @%u", (unsigned)offset);
            esp_hosted_slave_ota_end();
            return ESP_HOSTED_SLAVE_OTA_FAILED;
        }
        offset += to_send;
        emit(cb, ctx, OTA_PARTITION_STAGE_WRITE, offset, firmware_size);
    }

    emit(cb, ctx, OTA_PARTITION_STAGE_END, firmware_size, firmware_size);
    if (esp_hosted_slave_ota_end() != ESP_OK) {
        ESP_LOGE(TAG, "ota_end failed");
        return ESP_HOSTED_SLAVE_OTA_FAILED;
    }

    ESP_LOGI(TAG, "slave OTA complete (%u bytes)", (unsigned)firmware_size);
    emit(cb, ctx, OTA_PARTITION_STAGE_DONE, firmware_size, firmware_size);
    return ESP_HOSTED_SLAVE_OTA_COMPLETED;
}

esp_err_t ota_partition_perform(const char *partition_label)
{
    return ota_partition_perform_with_cb(partition_label, NULL, NULL);
}
