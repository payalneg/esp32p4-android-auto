#include <stdio.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "aa_overclock.h"
#include "bt_link.h"
#include "c6_ota.h"
#include "config.h"
#include "display_video.h"
#include "h264_pipe.h"
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

    /* Bump CPU to 400 MHz before any peripheral / WiFi init so APB ratio
     * stays consistent. No-op unless CONFIG_AA_OVERCLOCK_400 is set. */
    aa_overclock_400mhz_apply();

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

    ota_screen_show_idle("Android Auto", "Initialising Wi-Fi...");

#if CONNECTION_MODE == MODE_WIRELESS_HELPER
    ESP_ERROR_CHECK(wifi_manager_start());
    if (wifi_manager_wait_ready(30000) != ESP_OK) {
        ESP_LOGE(TAG, "wifi setup failed, halting");
        ota_screen_show_idle("Android Auto", "Wi-Fi setup failed");
        return;
    }

    const wifi_ap_info_t *ap = wifi_manager_get_ap_info();
    if (ap) {
        ESP_LOGI(TAG, "AP \"%s\" psk \"%s\" bssid %s ch %u",
                 ap->ssid, ap->password, ap->bssid_str, (unsigned)ap->channel);
    }

    ESP_ERROR_CHECK(mdns_advertise_start());
    ESP_ERROR_CHECK(tcp_server_start(AA_TCP_PORT));

    /* Display sink first — it captures the panel handle from BSP and waits
     * idle until first frame. Then the H.264 pipe; push() is a no-op until
     * the ring buffer is allocated, so it must be ready before the first
     * AVMediaIndication arrives. */
    if (display_video_init() != ESP_OK) {
        ESP_LOGW(TAG, "video sink failed — frames will be decoded but not shown");
    }
    if (h264_pipe_init() != ESP_OK) {
        ESP_LOGW(TAG, "H.264 decoder failed to start — video will be silent");
    }

    /* Hand off the AP credentials + our IP to the external D1 Mini ESP32
     * BT agent over UART1 (P4 GPIO 21/22 ↔ D1 Mini GPIO 16/17). The agent
     * uses these in the AA Wireless setup protocol so the phone joins our
     * SoftAP and connects back to TCP at the IP/port below.
     *
     * AP mode only — STA bench builds skip this since the dev's existing
     * laptop network already has its own credentials/topology. */
    if (ap) {
        /* The D1 Mini ESP32 BT agent's CH2104 USB-UART holds GPIO0 down
         * for ~150-200 ms after we power it from P4's 5V — our UART1 going
         * active during that window leaves the agent stuck in DOWNLOAD_BOOT
         * (`boot:0x3`). 300 ms is enough headroom for CH2104 to release
         * boot-strap lines before we start driving the UART. */
        vTaskDelay(pdMS_TO_TICKS(300));
        bt_link_init();
        esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        esp_netif_ip_info_t ap_ip = {0};
        if (ap_netif) esp_netif_get_ip_info(ap_netif, &ap_ip);
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ap_ip.ip));
        bt_link_publish_wifi(ap->ssid, ap->password, ap->bssid_str,
                             ip_str, AA_TCP_PORT);
    }

    /* Compose a one-line status with our IP for the idle screen.
     * AP mode shows the SSID; STA mode shows the joined network IP. */
    char status_line[80];
    esp_netif_ip_info_t ip_info = {0};
    if (ap) {
        esp_netif_t *n = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (n) esp_netif_get_ip_info(n, &ip_info);
        snprintf(status_line, sizeof(status_line),
                 "AP %s | %d.%d.%d.%d | port %d",
                 ap->ssid, IP2STR(&ip_info.ip), AA_TCP_PORT);
    } else {
        esp_netif_t *n = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (n) esp_netif_get_ip_info(n, &ip_info);
        snprintf(status_line, sizeof(status_line),
                 "%d.%d.%d.%d | port %d",
                 IP2STR(&ip_info.ip), AA_TCP_PORT);
    }
    ota_screen_show_idle("Waiting for phone", status_line);

    ESP_LOGI(TAG, "head unit ready, waiting for Wireless Helper");
#elif CONNECTION_MODE == MODE_BT_CLASSIC
#error "MODE_BT_CLASSIC: not implemented yet (Stage 1 covers Mode B only)"
#else
#error "CONNECTION_MODE not set"
#endif
}
