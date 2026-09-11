#include "tcp_server.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

#include "aa_handshake.h"
#include "aa_link_status.h"
#include "aa_reconnect.h"
#include "aa_service.h"
#include "aa_tls.h"
#include "bsp/esp-bsp.h"
#include "bt_link.h"
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

typedef enum {
    SESSION_NONE,          /* client never got past the handshake */
    SESSION_LOST,          /* ran, then died: socket error, our failure */
    SESSION_PEER_CLOSED,   /* ran, then the phone closed the socket cleanly */
} session_end_t;

/* How the client went away decides what the reconnect logic does next: a
 * phone that said goodbye (ShutdownRequest, the user exited Android Auto or
 * switched cars) is left alone — paging it back would undo that; anything
 * else is a LOST session and gets restarted. A bare FIN is NOT "the user
 * exited": gearhead closes the socket cleanly on its own read timeout, when
 * the OS kills it, and when it restarts projection after a wireless-setup
 * re-run — none of those are the user's choice. aa_frame_recv maps
 * recv()==0 (FIN) to ESP_ERR_INVALID_STATE and recv errors (RST, keepalive
 * abort, receive timeout) to ESP_FAIL; aa_service_run passes that through
 * and records whether a ShutdownRequest preceded the close. */
static session_end_t client_loop(int sock)
{
    /* aa_tls_t is ~16 KiB — heap, not stack. */
    aa_tls_t *tls = malloc(sizeof(*tls));
    if (!tls) {
        ESP_LOGE(TAG, "malloc tls");
        return SESSION_NONE;
    }

    if (aa_handshake_run(sock, tls) != ESP_OK) {
        ESP_LOGW(TAG, "handshake failed, dropping client");
        aa_link_status_set(AA_LINK_DISCONNECTED, "Handshake failed. Tap Connect to retry");
        free(tls);
        return SESSION_NONE;
    }

    /* Authenticated phone on TCP — the BT agent leaves the air for the
     * duration (see bt_link_set_aa_session). aa_reconnect_after_drop brings
     * it back once the session is really over; a phone that reconnects by
     * itself inside the grace window just re-arms this. */
    bt_link_set_aa_session(true, false);
    aa_link_status_set(AA_LINK_CONNECTED, "Android Auto running");

    esp_err_t err = aa_service_run(sock, tls);

    aa_tls_deinit(tls);
    free(tls);
    /* The goodbye counts even if our ShutdownResponse failed to go out — the
     * phone had already decided to leave. */
    (void)err;
    return aa_service_peer_requested_shutdown() ? SESSION_PEER_CLOSED : SESSION_LOST;
}

/* Dead-peer detection on the AA socket. Without it a phone that drops off
 * the AP without a word — out of range, 2.4 GHz interference, pocket — left
 * recv() blocked forever: nothing arrives, we send nothing unprompted, so
 * lwIP never retransmits and never times out. The head unit then sat in a
 * "live" session indefinitely: no idle screen, the BT agent held off air,
 * new phone connections queued in the listen backlog with nobody to accept
 * them, and a Connect tap (after a VESC→AA toggle) went to the dead touch
 * channel. That is the "Connect does nothing after the link dropped" from
 * the field. Only a reboot recovered it.
 *
 * TCP keepalive probes the phone's kernel after AA_KEEPALIVE_IDLE_S of
 * silence and aborts the socket after KEEPCNT unanswered probes (~14 s) —
 * works even when gearhead itself is idle. The receive timeout is the
 * backstop for a phone whose kernel still answers probes while gearhead is
 * hung: gearhead pings us every few seconds and streams video besides, so a
 * live session is never quiet this long (its own read timeout is ~25 s).
 * The send timeout bounds the ack/ping writes, which otherwise block the
 * display task on a zero window for as long as the peer stays half-alive. */
#define AA_KEEPALIVE_IDLE_S     5
#define AA_KEEPALIVE_INTVL_S    3
#define AA_KEEPALIVE_CNT        3
#define AA_RECV_TIMEOUT_S       30
#define AA_SEND_TIMEOUT_S       15

static void arm_dead_peer_detection(int sock)
{
    int yes   = 1;
    int idle  = AA_KEEPALIVE_IDLE_S;
    int intvl = AA_KEEPALIVE_INTVL_S;
    int cnt   = AA_KEEPALIVE_CNT;
    struct timeval rto = { .tv_sec = AA_RECV_TIMEOUT_S };
    struct timeval sto = { .tv_sec = AA_SEND_TIMEOUT_S };
    bool ok = true;
    ok &= setsockopt(sock, SOL_SOCKET,  SO_KEEPALIVE,  &yes,   sizeof(yes))   == 0;
    ok &= setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE,  &idle,  sizeof(idle))  == 0;
    ok &= setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl)) == 0;
    ok &= setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT,   &cnt,   sizeof(cnt))   == 0;
    ok &= setsockopt(sock, SOL_SOCKET,  SO_RCVTIMEO,   &rto,   sizeof(rto))   == 0;
    ok &= setsockopt(sock, SOL_SOCKET,  SO_SNDTIMEO,   &sto,   sizeof(sto))   == 0;
    if (!ok) {
        ESP_LOGW(TAG, "dead-peer detection: setsockopt errno %d — a silent drop "
                      "may hang the session", errno);
    } else {
        ESP_LOGI(TAG, "keepalive %d/%d/%d s, recv timeout %d s, send timeout %d s",
                 idle, intvl, cnt, AA_RECV_TIMEOUT_S, AA_SEND_TIMEOUT_S);
    }
}

/* gearhead restarts projection by itself whenever it re-runs the wireless
 * setup — the BT link bounced, the phone re-opened SPP, the agent answered
 * with WifiStartRequest — and it does so by closing the AA socket with a
 * clean FIN and connecting again within the same few ms (field logs
 * 2026-09-02 17:26 and 2026-09-04). That looks exactly like "user exited
 * Android Auto" from the socket side, and 1.3.12 answered it by kicking the
 * phone off the AP — right as the new connection was in the accept backlog,
 * so the restart died and gearhead gave up. Give the phone this long to come
 * back on its own before deciding the session is really over. */
#define RECONNECT_GRACE_MS  3000

/* Wait up to timeout_ms for a pending connection on the listen socket.
 * Returns true when accept() would not block. */
static bool wait_for_client(int listen_sock, uint32_t timeout_ms)
{
    fd_set rd;
    FD_ZERO(&rd);
    FD_SET(listen_sock, &rd);
    struct timeval tv = {
        .tv_sec  = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000,
    };
    int r = select(listen_sock + 1, &rd, NULL, NULL, &tv);
    return r > 0 && FD_ISSET(listen_sock, &rd);
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
        arm_dead_peer_detection(sock);
        aa_link_status_set(AA_LINK_CONNECTING, "Phone connected, handshake...");

        session_end_t ended = client_loop(sock);

        shutdown(sock, SHUT_RDWR);
        close(sock);
        ESP_LOGI(TAG, "client closed (%s)",
                 ended == SESSION_PEER_CLOSED ? "phone said goodbye"
                 : ended == SESSION_LOST      ? "session lost"
                                             : "no session");
        /* The verdict on what to do about it comes after the grace window
         * (aa_reconnect_after_drop); until then say what is known. */
        if (ended == SESSION_PEER_CLOSED) {
            aa_link_status_set(AA_LINK_DISCONNECTED, "Phone ended the session");
        } else if (ended == SESSION_LOST) {
            aa_link_status_set(AA_LINK_CONNECTING, "Link lost, waiting for the phone...");
        }

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

        /* Screen is back to idle — now restart the wireless flow (kick the
         * phone off the AP, bring the BT agent back on air, page) so the next
         * session can begin. Only after a real session: a client that failed
         * the handshake is not a phone we want to keep re-paging — unless the
         * agent is still off air from a previous session (the phone came back
         * inside the grace window and then failed the handshake); leaving it
         * there would mean nobody pages and Connect is ignored. And only if
         * the phone is not already knocking (see RECONNECT_GRACE_MS) —
         * kicking it then kills the very reconnect we want. */
        if (ended != SESSION_NONE || bt_link_aa_session_live()) {
            if (wait_for_client(listen_sock, RECONNECT_GRACE_MS)) {
                ESP_LOGI(TAG, "phone is reconnecting by itself — leaving the AP/BT alone");
                aa_link_status_set(AA_LINK_CONNECTING, "Phone is reconnecting...");
            } else {
                aa_reconnect_after_drop(ip, ended == SESSION_PEER_CLOSED);
            }
        }
    }
}

esp_err_t tcp_server_start(uint16_t port)
{
    static server_ctx_t ctx;
    ctx.port = port;

    /* mbedTLS handshake puts a few KiB of working state on the stack; 8 KiB is comfortable.
     * Pinned to core 1 so the recv → h264_pipe → display chain stays on the same core
     * the decoder lives on (h264_dec + openh264 helper at prio 17). Cross-core wake-ups
     * for every video packet were eating into AA throughput. */
    BaseType_t ok = xTaskCreatePinnedToCore(accept_task, "aa_tcp", 8192, &ctx, 5, NULL, 1);
    return (ok == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}
