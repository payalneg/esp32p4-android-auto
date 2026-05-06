#include "tcp_server.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "aa_handshake.h"
#include "aa_service.h"
#include "aa_tls.h"
#include "bsp/esp-bsp.h"
#include "display_video.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "idle_screen.h"
#include "lvgl.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/tcp.h"
#include "ui_mode.h"

static const char *TAG = "tcp";

typedef struct {
    uint16_t port;
} server_ctx_t;

static void client_loop(int sock)
{
    /* aa_tls_t is ~16 KiB — heap, not stack. */
    aa_tls_t *tls = malloc(sizeof(*tls));
    if (!tls) {
        ESP_LOGE(TAG, "malloc tls");
        return;
    }

    if (aa_handshake_run(sock, tls) != ESP_OK) {
        ESP_LOGW(TAG, "handshake failed, dropping client");
        free(tls);
        return;
    }

    aa_service_run(sock, tls);

    aa_tls_deinit(tls);
    free(tls);
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

        /* Disable Nagle. Our ack-on-decode path emits 30-byte AVMediaAck
         * packets that gearhead waits on synchronously; without NODELAY
         * lwIP coalesces them with whatever follows and adds 40-200 ms of
         * latency. That's enough to trip gearhead's WRITER_STALL detector
         * and bounce it into FRAMER_WRITER_SYNCHRONOUS_MODE — a one-way
         * door we then sit behind for ~30-60 s of phone-side keep-alive. */
        int yes = 1;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) != 0) {
            ESP_LOGW(TAG, "TCP_NODELAY: errno %d", errno);
        }

        client_loop(sock);

        shutdown(sock, SHUT_RDWR);
        close(sock);
        ESP_LOGI(TAG, "client closed");

        /* Phone is gone — pry the panel back from the video sink (it had
         * paused LVGL on the first frame) and put up the idle "Waiting
         * for phone" text. Skip the screen flip when the user is looking
         * at the VESC dashboard; the 20 Hz updater is already painting
         * over the stale video frame for them. */
        if (ui_mode_get() == UI_MODE_AA) {
            /* First cycle: apply labels + invalidate while LVGL adapter
             * is still paused — same order as the working VESC mode-
             * switch path (queues dirty, processed on resume). */
            idle_screen_refresh();
        }
        display_video_yield_panel();

        /* The panel runs in TRIPLE_PARTIAL tear-avoid mode — three
         * framebuffers in a ring. A single LVGL render only updates
         * one; the next two scanouts still flash the stale video frame
         * baked into the other two buffers. Walk the chain with two
         * more invalidates (one DPI scanout each) so all three FBs end
         * up holding the idle screen, no flicker. */
        if (ui_mode_get() == UI_MODE_AA) {
            for (int i = 0; i < 2; i++) {
                vTaskDelay(pdMS_TO_TICKS(35));
                if (bsp_display_lock(100) == ESP_OK) {
                    lv_obj_invalidate(lv_scr_act());
                    bsp_display_unlock();
                }
            }
        }
    }
}

esp_err_t tcp_server_start(uint16_t port)
{
    static server_ctx_t ctx;
    ctx.port = port;

    /* mbedTLS handshake puts a few KiB of working state on the stack; 8 KiB is comfortable. */
    BaseType_t ok = xTaskCreate(accept_task, "aa_tcp", 8192, &ctx, 5, NULL);
    return (ok == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}
