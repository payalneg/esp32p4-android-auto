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
#include "esp_hosted.h"
#include "esp_hosted_api_types.h"
#include "esp_hosted_ota.h"
#include "esp_log.h"

#include "ota_partition.h"

static const char *TAG = "c6_ota_blob";

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 1500
#endif

extern const uint8_t slave_fw_start[] asm("_binary_network_adapter_bin_start");
extern const uint8_t slave_fw_end[]   asm("_binary_network_adapter_bin_end");

static void emit(ota_partition_progress_cb_t cb, void *ctx,
                 ota_partition_stage_t stage, uint32_t done, uint32_t total)
{
    if (cb) {
        cb(stage, done, total, ctx);
    }
}

static esp_err_t parse_embedded_image(size_t blob_size,
                                      size_t *firmware_size,
                                      char *version, size_t version_len)
{
    if (blob_size < sizeof(esp_image_header_t)) {
        return ESP_ERR_INVALID_SIZE;
    }
    const esp_image_header_t *hdr = (const esp_image_header_t *)slave_fw_start;
    if (hdr->magic != ESP_IMAGE_HEADER_MAGIC) {
        ESP_LOGE(TAG, "bad magic 0x%02x in embedded slave fw", hdr->magic);
        return ESP_ERR_INVALID_ARG;
    }

    size_t total = sizeof(esp_image_header_t);
    for (int i = 0; i < hdr->segment_count; i++) {
        if (total + sizeof(esp_image_segment_header_t) > blob_size) {
            return ESP_ERR_INVALID_SIZE;
        }
        const esp_image_segment_header_t *seg =
            (const esp_image_segment_header_t *)(slave_fw_start + total);
        total += sizeof(esp_image_segment_header_t) + seg->data_len;

        if (i == 0 && version) {
            const esp_app_desc_t *desc =
                (const esp_app_desc_t *)(slave_fw_start
                    + sizeof(esp_image_header_t)
                    + sizeof(esp_image_segment_header_t));
            strncpy(version, desc->version, version_len - 1);
            version[version_len - 1] = '\0';
        }
    }

    size_t pad = (16 - (total % 16)) % 16;
    total += pad + 1;
    if (hdr->hash_appended == 1) {
        size_t hash_pad = (16 - (total % 16)) % 16;
        total += hash_pad + 32;
    }

    if (total > blob_size) {
        ESP_LOGE(TAG, "computed image size %u > embedded blob %u",
                 (unsigned)total, (unsigned)blob_size);
        return ESP_ERR_INVALID_SIZE;
    }

    *firmware_size = total;
    return ESP_OK;
}

esp_err_t ota_partition_perform_with_cb(const char *partition_label,
                                        ota_partition_progress_cb_t cb,
                                        void *ctx)
{
    (void)partition_label;

    const size_t blob_size = (size_t)(slave_fw_end - slave_fw_start);
    ESP_LOGI(TAG, "embedded slave fw: %u bytes", (unsigned)blob_size);
    emit(cb, ctx, OTA_PARTITION_STAGE_VALIDATE, 0, 0);

    size_t firmware_size = 0;
    char new_version[32] = { 0 };
    esp_err_t r = parse_embedded_image(blob_size, &firmware_size, new_version, sizeof(new_version));
    if (r != ESP_OK) {
        return ESP_HOSTED_SLAVE_OTA_FAILED;
    }
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
