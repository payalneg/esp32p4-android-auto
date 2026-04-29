#include "mdns_advertise.h"

#include "config.h"
#include "esp_log.h"
#include "mdns.h"

static const char *TAG = "mdns";

esp_err_t mdns_advertise_start(void)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(AA_MDNS_HOSTNAME));
    ESP_ERROR_CHECK(mdns_instance_name_set(AA_MDNS_INSTANCE_NAME));
    ESP_ERROR_CHECK(mdns_service_add(NULL, AA_MDNS_SERVICE_TYPE, AA_MDNS_PROTO,
                                     AA_TCP_PORT, NULL, 0));

    ESP_LOGI(TAG, "advertising %s.local %s.%s :%d",
             AA_MDNS_HOSTNAME, AA_MDNS_SERVICE_TYPE, AA_MDNS_PROTO, AA_TCP_PORT);
    return ESP_OK;
}
