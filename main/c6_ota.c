#include "c6_ota.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_hosted.h"
#include "esp_hosted_api_types.h"
#include "esp_hosted_ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ota_partition.h"
#include "ota_screen.h"

static const char *TAG = "c6_ota";

static void on_progress(ota_partition_stage_t stage, uint32_t done, uint32_t total, void *ctx)
{
    (void)ctx;
    static int last_logged_pct = -1;

    switch (stage) {
    case OTA_PARTITION_STAGE_VALIDATE:
        ESP_LOGI(TAG, "validating image");
        ota_screen_set_status("Validating image…");
        last_logged_pct = -1;
        break;
    case OTA_PARTITION_STAGE_BEGIN:
        ESP_LOGI(TAG, "ota_begin (%u bytes)", (unsigned)total);
        ota_screen_set_status("Erasing C6 flash…");
        ota_screen_set_progress(0, total ? total : 1);
        break;
    case OTA_PARTITION_STAGE_WRITE: {
        int pct = total ? (int)((uint64_t)done * 100 / total) : 0;
        if (pct >= last_logged_pct + 5 || pct == 100) {
            ESP_LOGI(TAG, "writing… %d%% (%u/%u)", pct, (unsigned)done, (unsigned)total);
            last_logged_pct = pct;
        }
        ota_screen_set_status("Writing firmware…");
        ota_screen_set_progress(done, total);
        break;
    }
    case OTA_PARTITION_STAGE_END:
        ESP_LOGI(TAG, "ota_end — verifying");
        ota_screen_set_status("Verifying…");
        ota_screen_set_progress(total, total);
        break;
    case OTA_PARTITION_STAGE_DONE:
        ESP_LOGI(TAG, "ota done — activating");
        ota_screen_set_status("Activating new firmware…");
        break;
    }
}

static bool slave_supports_activate(const esp_hosted_coprocessor_fwver_t *v)
{
    return (v->major1 > 2) || (v->major1 == 2 && v->minor1 > 5);
}

c6_ota_status_t c6_ota_check_and_update(void)
{
    if (esp_hosted_init() != ESP_OK) {
        ESP_LOGW(TAG, "esp_hosted_init failed (probably already initialized)");
    }
    esp_err_t conn = esp_hosted_connect_to_slave();
    if (conn != ESP_OK) {
        ESP_LOGE(TAG, "esp_hosted_connect_to_slave: %s", esp_err_to_name(conn));
        return C6_OTA_STATUS_FAILED;
    }

    esp_hosted_coprocessor_fwver_t slave_ver = { 0 };
    esp_err_t vret = esp_hosted_get_coprocessor_fwversion(&slave_ver);
    if (vret == ESP_OK) {
        ESP_LOGI(TAG, "Slave version: %" PRIu32 ".%" PRIu32 ".%" PRIu32,
                 slave_ver.major1, slave_ver.minor1, slave_ver.patch1);
    } else {
        ESP_LOGW(TAG, "fwversion not readable: %s — assuming update needed",
                 esp_err_to_name(vret));
    }

    uint32_t host_ver = ESP_HOSTED_VERSION_VAL(ESP_HOSTED_VERSION_MAJOR_1,
                                              ESP_HOSTED_VERSION_MINOR_1,
                                              ESP_HOSTED_VERSION_PATCH_1);
    uint32_t slave = ESP_HOSTED_VERSION_VAL(slave_ver.major1, slave_ver.minor1, slave_ver.patch1);
    if (vret == ESP_OK && (host_ver & 0xFFFFFF00) == (slave & 0xFFFFFF00)) {
        ESP_LOGI(TAG, "Slave already matches host major.minor — skipping OTA");
        return C6_OTA_STATUS_NOT_REQUIRED;
    }

    ESP_LOGW(TAG, "Slave needs update — running OTA");
    ota_screen_show("Don't power off");
    ota_screen_set_status("Preparing…");

    esp_err_t r = ota_partition_perform_with_cb(NULL, on_progress, NULL);
    if (r != ESP_HOSTED_SLAVE_OTA_COMPLETED) {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(r));
        ota_screen_set_status("Update failed");
        return C6_OTA_STATUS_FAILED;
    }

    if (slave_supports_activate(&slave_ver)) {
        if (esp_hosted_slave_ota_activate() != ESP_OK) {
            ESP_LOGW(TAG, "ota_activate failed (continuing anyway)");
        }
    } else {
        ESP_LOGI(TAG, "Slave < 2.6 — activate API not used; new fw boots after restart");
    }

    ota_screen_set_status("Done — restarting…");
    vTaskDelay(pdMS_TO_TICKS(1500));
    return C6_OTA_STATUS_UPDATED;
}
