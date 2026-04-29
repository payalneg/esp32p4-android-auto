#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "c6_ota.h"
#include "config.h"
#include "mdns_advertise.h"
#include "ota_screen.h"
#include "tcp_server.h"
#include "wifi_manager.h"

static const char *TAG = "main";

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-P4 Android Auto boot, mode=%d", CONNECTION_MODE);

    init_nvs();

    if (ota_screen_init() != ESP_OK) {
        ESP_LOGW(TAG, "display unavailable — OTA progress will be log-only");
    }

#if CONFIG_C6_OTA_ENABLED
    c6_ota_status_t ota = c6_ota_check_and_update();
    if (ota == C6_OTA_STATUS_UPDATED) {
        ESP_LOGW(TAG, "C6 updated — restarting host to resync");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    } else if (ota == C6_OTA_STATUS_FAILED) {
        ESP_LOGE(TAG, "C6 OTA failed — proceeding with current slave fw");
    }
#endif

    ota_screen_hide();

#if CONNECTION_MODE == MODE_WIRELESS_HELPER
    ESP_ERROR_CHECK(wifi_manager_start());
    if (wifi_manager_wait_ready(30000) != ESP_OK) {
        ESP_LOGE(TAG, "wifi setup failed, halting");
        return;
    }

#if CONFIG_AA_WIFI_ROLE_AP
    const wifi_ap_info_t *ap = wifi_manager_get_ap_info();
    ESP_LOGI(TAG, "AP \"%s\" psk \"%s\" bssid %s ch %u",
             ap->ssid, ap->password, ap->bssid_str, (unsigned)ap->channel);
#endif

    ESP_ERROR_CHECK(mdns_advertise_start());
    ESP_ERROR_CHECK(tcp_server_start(AA_TCP_PORT));

    ESP_LOGI(TAG, "head unit ready, waiting for Wireless Helper");
#elif CONNECTION_MODE == MODE_BT_CLASSIC
#error "MODE_BT_CLASSIC: not implemented yet (Stage 1 covers Mode B only)"
#else
#error "CONNECTION_MODE not set"
#endif
}
