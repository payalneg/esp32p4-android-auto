#include "tcp_server.h"

#include <errno.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

static const char *TAG = "tcp";

typedef struct {
    uint16_t port;
} server_ctx_t;

static void client_loop(int sock)
{
    char buf[256];
    while (true) {
        int n = recv(sock, buf, sizeof(buf), 0);
        if (n < 0) {
            ESP_LOGW(TAG, "recv errno %d", errno);
            return;
        }
        if (n == 0) {
            ESP_LOGI(TAG, "peer closed");
            return;
        }
        ESP_LOGI(TAG, "rx %d bytes", n);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, n, ESP_LOG_DEBUG);
    }
}

static void accept_task(void *arg)
{
    server_ctx_t *ctx = arg;

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "socket() failed errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int yes = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(ctx->port),
    };

    if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        ESP_LOGE(TAG, "bind() failed errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }
    if (listen(listen_sock, 1) != 0) {
        ESP_LOGE(TAG, "listen() failed errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "listening on :%u", (unsigned)ctx->port);

    while (true) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int sock = accept(listen_sock, (struct sockaddr *)&peer, &peer_len);
        if (sock < 0) {
            ESP_LOGW(TAG, "accept() errno %d", errno);
            continue;
        }
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
        ESP_LOGI(TAG, "client %s:%u connected", ip, (unsigned)ntohs(peer.sin_port));

        client_loop(sock);

        shutdown(sock, SHUT_RDWR);
        close(sock);
        ESP_LOGI(TAG, "client closed");
    }
}

esp_err_t tcp_server_start(uint16_t port)
{
    static server_ctx_t ctx;
    ctx.port = port;

    BaseType_t ok = xTaskCreate(accept_task, "aa_tcp", 4096, &ctx, 5, NULL);
    return (ok == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}
