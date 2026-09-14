/*
    Copyright 2026 ESP32-P4 Android Auto / VESC Display (GPL-3.0).

    The two OTA slots, from the running firmware's point of view.

    Every update writes the image into whichever slot is not running and points
    otadata at it, so the one you were on a minute ago is still sitting in flash
    — untouched, complete, and bootable. Nothing used to be able to reach it:
    going back meant flashing over USB or pushing an older build from the phone,
    which is a poor answer when the new firmware is the thing that made the
    screen unusable.

    Rollback verification (CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) is off in this
    project, so neither slot is ever marked pending-verify or invalid: if an
    image is there, the bootloader will run it. That makes the switch a single
    otadata write plus a reboot, and it is symmetric — the slot you came from
    becomes the one you can go back to.

    What crosses the switch: NVS settings, the LittleFS backup partition and the
    trip log all live in their own partitions and are shared by both images. An
    older firmware simply ignores NVS keys it does not know.
*/

#include "dev_settings.h"

#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#include <stdio.h>

static const char *TAG = "fw_slots";

/* With exactly two OTA slots and no factory partition, "the next update
 * partition" is the one we are not running. */
static const esp_partition_t *other_slot(void)
{
    return esp_ota_get_next_update_partition(NULL);
}

bool fw_slots_other_label(char *buf, size_t n)
{
    if (!buf || n == 0) return false;
    const esp_partition_t *other = other_slot();
    if (!other) return false;

    esp_app_desc_t d;
    /* Reads and checks the image header, so a blank or half-written slot
     * reports itself as empty instead of as something bootable. */
    if (esp_ota_get_partition_description(other, &d) != ESP_OK) return false;

    snprintf(buf, n, "%s", d.version);
    return true;
}

esp_err_t fw_slots_boot_other(void)
{
    const esp_partition_t *other = other_slot();
    if (!other) return ESP_ERR_NOT_FOUND;

    esp_app_desc_t d;
    esp_err_t err = esp_ota_get_partition_description(other, &d);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "%s holds no valid image: %s", other->label,
                 esp_err_to_name(err));
        return err;
    }

    err = esp_ota_set_boot_partition(other);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_boot_partition(%s): %s", other->label,
                 esp_err_to_name(err));
        return err;
    }
    ESP_LOGW(TAG, "next boot is %s (%s) — reboot to take it",
             other->label, d.version);
    return ESP_OK;
}
