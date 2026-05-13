#include "ota_http.h"

#include "sdkconfig.h"

#if CONFIG_OTA_HTTP_ENABLED

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "esp_app_desc.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ota_http";
static httpd_handle_t s_server;

/* 4 KiB chunk — comfortable for esp_ota_write (must be ≥ 16 bytes) and
 * keeps heap pressure low while the AA pipeline still runs alongside. */
#define OTA_RX_BUF_SIZE 4096

static esp_err_t index_get_handler(httpd_req_t *req)
{
    static const char html[] =
        "<!doctype html><meta charset=utf-8><title>P4 OTA</title>"
        "<style>body{font-family:sans-serif;max-width:40em;margin:2em auto}</style>"
        "<h1>ESP32-P4 OTA</h1>"
        "<form method=POST action=/ota enctype=application/octet-stream>"
        "<input type=file name=fw required> "
        "<button>Upload</button></form>"
        "<p>or from a shell:</p>"
        "<pre>curl --data-binary @build/esp32p4_android_auto.bin \\\n"
        "     -H 'Content-Type: application/octet-stream' \\\n"
        "     http://&lt;device&gt;/ota</pre>"
        "<p><a href=/info>device info</a></p>";
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, sizeof(html) - 1);
}

static esp_err_t info_get_handler(httpd_req_t *req)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next    = esp_ota_get_next_update_partition(NULL);
    esp_app_desc_t desc = {0};
    if (running) esp_ota_get_partition_description(running, &desc);

    char body[384];
    int n = snprintf(body, sizeof(body),
        "running: %s @ 0x%08" PRIx32 " (size %" PRIu32 ")\n"
        "version: %s\n"
        "idf:     %s\n"
        "next:    %s @ 0x%08" PRIx32 " (size %" PRIu32 ")\n",
        running ? running->label : "?",
        running ? running->address : 0,
        running ? running->size : 0,
        desc.version, desc.idf_ver,
        next ? next->label : "(none)",
        next ? next->address : 0,
        next ? next->size : 0);
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, body, n);
}

static esp_err_t ota_post_handler(httpd_req_t *req)
{
    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    if (!next) {
        ESP_LOGE(TAG, "no next OTA partition — wrong partition table?");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "no ota partition (rebuild partitions.csv)");
        return ESP_FAIL;
    }

    int remaining = req->content_len;
    if (remaining <= 0) {
        httpd_resp_send_err(req, HTTPD_411_LENGTH_REQUIRED,
                            "Content-Length required");
        return ESP_FAIL;
    }
    if ((uint32_t)remaining > next->size) {
        ESP_LOGE(TAG, "image %d > slot %" PRIu32, remaining, next->size);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "image larger than ota slot");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA begin: %d bytes -> %s @ 0x%08" PRIx32,
             remaining, next->label, next->address);

    esp_ota_handle_t handle = 0;
    esp_err_t err = esp_ota_begin(next, remaining, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "esp_ota_begin failed");
        return ESP_FAIL;
    }

    char *buf = malloc(OTA_RX_BUF_SIZE);
    if (!buf) {
        esp_ota_abort(handle);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
        return ESP_FAIL;
    }

    int written = 0;
    int next_log_at = 0x40000;  /* log every 256 KiB */
    while (remaining > 0) {
        int want = remaining < OTA_RX_BUF_SIZE ? remaining : OTA_RX_BUF_SIZE;
        int n = httpd_req_recv(req, buf, want);
        if (n == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (n <= 0) {
            ESP_LOGE(TAG, "recv error %d after %d/%d bytes",
                     n, written, written + remaining);
            free(buf);
            esp_ota_abort(handle);
            return ESP_FAIL;
        }
        err = esp_ota_write(handle, buf, n);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write @%d: %s",
                     written, esp_err_to_name(err));
            free(buf);
            esp_ota_abort(handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                "flash write failed");
            return ESP_FAIL;
        }
        written   += n;
        remaining -= n;
        if (written >= next_log_at) {
            ESP_LOGI(TAG, "...%d bytes", written);
            next_log_at += 0x40000;
        }
    }
    free(buf);

    err = esp_ota_end(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "image verify failed");
        return ESP_FAIL;
    }
    err = esp_ota_set_boot_partition(next);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_boot_partition: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "set_boot_partition failed");
        return ESP_FAIL;
    }

    ESP_LOGW(TAG, "OTA OK (%d bytes) -> %s, rebooting in 500 ms",
             written, next->label);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "OK rebooting\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;  /* unreachable */
}

esp_err_t ota_http_start(void)
{
    ESP_LOGI(TAG, "ota_http_start() called");
    if (s_server) {
        ESP_LOGI(TAG, "already running, skipping");
        return ESP_OK;
    }

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port       = CONFIG_OTA_HTTP_PORT;
    cfg.recv_wait_timeout = 30;
    cfg.send_wait_timeout = 30;
    cfg.lru_purge_enable  = true;
    cfg.stack_size        = 8192;

    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start: %s", esp_err_to_name(err));
        s_server = NULL;
        return err;
    }

    static const httpd_uri_t ix = {
        .uri = "/",     .method = HTTP_GET,  .handler = index_get_handler,
    };
    static const httpd_uri_t in = {
        .uri = "/info", .method = HTTP_GET,  .handler = info_get_handler,
    };
    static const httpd_uri_t up = {
        .uri = "/ota",  .method = HTTP_POST, .handler = ota_post_handler,
    };
    httpd_register_uri_handler(s_server, &ix);
    httpd_register_uri_handler(s_server, &in);
    httpd_register_uri_handler(s_server, &up);

    ESP_LOGI(TAG, "OTA HTTP server on :%d", cfg.server_port);
    return ESP_OK;
}

#else  /* !CONFIG_OTA_HTTP_ENABLED */

esp_err_t ota_http_start(void) { return ESP_OK; }

#endif
