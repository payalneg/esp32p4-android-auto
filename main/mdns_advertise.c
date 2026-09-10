#include "mdns_advertise.h"

#include "config.h"
#include "esp_log.h"
#include "mdns.h"
#include "sdkconfig.h"

static const char *TAG = "mdns";

esp_err_t mdns_advertise_start(void)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(AA_MDNS_HOSTNAME));
    ESP_ERROR_CHECK(mdns_instance_name_set(AA_MDNS_INSTANCE_NAME));

#if CONFIG_AA_ENABLE
    /* The AA service record. Wireless Helper browses for _aawireless._tcp,
     * takes only the IP out of it and connects to its own hardcoded port. */
    ESP_ERROR_CHECK(mdns_service_add(NULL, AA_MDNS_SERVICE_TYPE, AA_MDNS_PROTO,
                                     AA_TCP_PORT, NULL, 0));
    ESP_LOGI(TAG, "advertising %s.local %s.%s :%d",
             AA_MDNS_HOSTNAME, AA_MDNS_SERVICE_TYPE, AA_MDNS_PROTO, AA_TCP_PORT);
#else
    /* Dashboard-only board: no AA service, but the hostname still matters —
     * it is how the web UI (/ota, /files, /lisp) and the QR code in Settings
     * are addressed. */
#if CONFIG_OTA_HTTP_ENABLED
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp",
                                     CONFIG_OTA_HTTP_PORT, NULL, 0));
#endif
    ESP_LOGI(TAG, "advertising %s.local", AA_MDNS_HOSTNAME);
#endif
    return ESP_OK;
}
