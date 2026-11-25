#include "debug_wifi.h"

#include "sdkconfig.h"

#if CONFIG_DEBUG_WIFI_BRIDGE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_rom_crc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#include "board.h"
#include "debug_cmds.h"
#include "log_capture.h"

static const char *TAG = "dbg_wifi";

#define MAX_CLIENTS   2
#define CMD_LINE_MAX  256
#define LOG_CHUNK     2048          /* log bytes pushed per client per pass */
#define SEND_TIMEOUT  3             /* seconds; a stalled client is dropped */
#define ACCEPT_BACKLOG 4

typedef struct {
    int    fd;                      /* -1 = slot free */
    size_t cursor;                  /* absolute log offset already sent */
    char   line[CMD_LINE_MAX];
    size_t line_len;
} client_t;

static int      s_listen_fd = -1;
static bool     s_started;
static client_t s_cli[MAX_CLIENTS];
static char     s_log_chunk[LOG_CHUNK];

/* ---- socket helpers ---- */

static bool send_all(int fd, const void *data, size_t len)
{
    const char *p = (const char *)data;
    while (len > 0) {
        int n = send(fd, p, len, 0);
        if (n <= 0) return false;
        p += n;
        len -= (size_t)n;
    }
    return true;
}

static bool send_str(int fd, const char *s)
{
    return send_all(fd, s, strlen(s));
}

static void client_close(client_t *c, const char *why)
{
    if (c->fd < 0) return;
    shutdown(c->fd, SHUT_RDWR);
    close(c->fd);
    ESP_LOGI(TAG, "client closed (%s)", why);
    c->fd = -1;
    c->cursor = 0;
    c->line_len = 0;
}

/* ---- command output sink: one client's socket ---- */

static void client_text(void *ctx, const char *s, size_t len)
{
    client_t *c = (client_t *)ctx;
    if (c->fd < 0) return;
    if (!send_all(c->fd, s, len)) {
        client_close(c, "send failed");
    }
}

/* A captured screen, raw over TCP — no base64 on a link that carries bytes
 * anyway. Logging is muted for the whole frame so no ESP_LOG line can splice
 * into the payload. */
static void client_image(void *ctx, const char *fmt, int w, int h,
                         const uint8_t *data, size_t len)
{
    client_t *c = (client_t *)ctx;
    if (c->fd < 0) return;

    esp_log_level_t prev = esp_log_level_get("*");
    esp_log_level_set("*", ESP_LOG_NONE);

    char hdr[160];
    uint32_t crc = esp_rom_crc32_le(0, data, (uint32_t)len);
    int n = snprintf(hdr, sizeof(hdr),
                     "\nSCR-BEGIN w=%d h=%d fmt=%s len=%u crc32=0x%08x\n",
                     w, h, fmt, (unsigned)len, (unsigned)crc);
    bool ok = n > 0 && n < (int)sizeof(hdr) && send_all(c->fd, hdr, (size_t)n);
    ok = ok && send_all(c->fd, data, len);
    ok = ok && send_str(c->fd, "\nSCR-END\n");

    esp_log_level_set("*", prev);

    if (!ok) {
        /* Muting covered the ESP_LOGI inside client_close; the client is gone
         * either way and the next pass reaps the slot. */
        client_close(c, "screenshot send failed");
    }
}

/* ---- log streaming ---- */

static void stream_logs(client_t *c)
{
    if (c->fd < 0) return;

    /* Drain in chunks until the reader is caught up, capped per pass so a
     * burst (a crash dump, a HEX log line) cannot starve input handling and
     * the other client. Whatever does not fit stays in the ring; if the
     * backlog ever outgrows the ring the reader is told how much it lost. */
    for (int pass = 0; pass < 8; pass++) {
        size_t dropped = 0;
        size_t n = log_capture_read(&c->cursor, s_log_chunk,
                                    sizeof(s_log_chunk), &dropped);
        if (dropped) {
            char msg[64];
            int m = snprintf(msg, sizeof(msg), "\n# %u log bytes dropped\n",
                             (unsigned)dropped);
            if (m > 0 && !send_all(c->fd, msg, (size_t)m)) {
                client_close(c, "log send failed");
                return;
            }
        }
        if (!n) return;
        if (!send_all(c->fd, s_log_chunk, n)) {
            client_close(c, "log send failed");
            return;
        }
        if (n < sizeof(s_log_chunk) - 1) return;    /* caught up */
    }
}

/* ---- command input ---- */

static void run_line(client_t *c, char *line)
{
    /* Trim CR/LF and surrounding blanks. The tokeniser below is the same
     * "split on spaces" the hosts use; no quoting, no escapes — both ends are
     * scripts and the commands take bare words and numbers. */
    while (*line == ' ' || *line == '\t') line++;
    size_t len = strlen(line);
    while (len && (line[len - 1] == '\r' || line[len - 1] == '\n' ||
                   line[len - 1] == ' ' || line[len - 1] == '\t')) {
        line[--len] = '\0';
    }
    if (!len) return;

    char *argv[16];
    int argc = 0;
    for (char *p = line; *p && argc < (int)(sizeof(argv) / sizeof(argv[0])); ) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
    }

    debug_out_t out = { .ctx = c, .text = client_text, .image = client_image };
    int rc = debug_cmds_run(argc, argv, &out);

    if (c->fd >= 0) {
        char tail[32];
        int n = (rc == 0) ? snprintf(tail, sizeof(tail), "# done\n")
                          : snprintf(tail, sizeof(tail), "# err %d\n", rc);
        if (n > 0 && !send_all(c->fd, tail, (size_t)n)) {
            client_close(c, "reply failed");
        }
    }
}

static void read_input(client_t *c)
{
    char buf[128];
    int n = recv(c->fd, buf, sizeof(buf), 0);
    if (n == 0) { client_close(c, "peer closed"); return; }
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            ESP_LOGW(TAG, "recv failed (errno %d)", errno);
            client_close(c, "recv error");
        }
        return;
    }

    for (int i = 0; i < n; i++) {
        char ch = buf[i];
        if (ch == '\n') {
            c->line[c->line_len] = '\0';
            run_line(c, c->line);     /* may drop the client mid-command */
            c->line_len = 0;
            if (c->fd < 0) return;
            continue;
        }
        if (c->line_len + 1 < sizeof(c->line)) {
            c->line[c->line_len++] = ch;
        } else {
            /* Overlong line: drop it rather than splice two commands. */
            c->line_len = 0;
            if (c->fd >= 0) send_str(c->fd, "# err line too long\n");
        }
    }
}

/* ---- accept + loop ---- */

static void accept_clients(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_t *c = &s_cli[i];
        if (c->fd >= 0) continue;

        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int fd = accept(s_listen_fd, (struct sockaddr *)&peer, &peer_len);
        if (fd < 0) return;

        int one = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
        struct timeval sto = { .tv_sec = SEND_TIMEOUT };
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &sto, sizeof(sto));

        char ip[INET_ADDRSTRLEN] = "?";
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
        ESP_LOGI(TAG, "client %s:%u connected", ip,
                 (unsigned)ntohs(peer.sin_port));

        c->fd = fd;
        c->line_len = 0;

        /* Replay the tail of the log so a client that connects after boot still
         * sees how it got here — the whole point when chasing a boot problem
         * with no serial cable attached. */
        size_t total = log_capture_total_bytes();
        size_t backlog = (size_t)CONFIG_DEBUG_WIFI_BRIDGE_BACKLOG_KB * 1024u;
        c->cursor = total > backlog ? total - backlog : 0;

        char hdr[256];
        const esp_app_desc_t *app = esp_app_get_description();
        int n = snprintf(hdr, sizeof(hdr),
                         "# %s debug bridge — board %s, firmware %s\n"
                         "# live log follows; send a command line to run it "
                         "(help lists them)\n"
                         "# screenshot frame: SCR-BEGIN line, len raw bytes, "
                         "SCR-END\n",
                         BOARD_MODEL_ID, BOARD_MODEL_NAME,
                         app ? app->version : "?");
        if (n > 0 && !send_all(fd, hdr, (size_t)n)) {
            client_close(c, "banner failed");
        }
        return;     /* one accept per pass keeps the log stream flowing */
    }
}

static void wifi_debug_task(void *arg)
{
    (void)arg;

    while (true) {
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(s_listen_fd, &rd);
        int maxfd = s_listen_fd;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (s_cli[i].fd >= 0) {
                FD_SET(s_cli[i].fd, &rd);
                if (s_cli[i].fd > maxfd) maxfd = s_cli[i].fd;
            }
        }

        struct timeval tv = { .tv_sec = 0, .tv_usec = 100000 };
        int r = select(maxfd + 1, &rd, NULL, NULL, &tv);
        if (r < 0 && errno != EINTR) {
            ESP_LOGW(TAG, "select errno %d", errno);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (r > 0 && FD_ISSET(s_listen_fd, &rd)) {
            accept_clients();
        }

        /* Always push what is new before reading, so a client that keeps
         * typing still sees the log interleaved the way the console shows it. */
        for (int i = 0; i < MAX_CLIENTS; i++) {
            stream_logs(&s_cli[i]);
            if (s_cli[i].fd < 0) continue;
            if (r > 0 && FD_ISSET(s_cli[i].fd, &rd)) read_input(&s_cli[i]);
        }
    }
}

/* ---- start ---- */

static void log_endpoint(uint16_t port)
{
    esp_netif_t *n = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (!n) n = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip = { 0 };
    if (n && esp_netif_get_ip_info(n, &ip) == ESP_OK && ip.ip.addr) {
        ESP_LOGI(TAG, "debug bridge on " IPSTR ":%u — from a laptop: "
                      "python scripts/wifi_debug.py -H " IPSTR " logs",
                 IP2STR(&ip.ip), (unsigned)port, IP2STR(&ip.ip));
    } else {
        ESP_LOGW(TAG, "debug bridge on :%u (no IP yet — Wi-Fi still coming up?)",
                 (unsigned)port);
    }
}

esp_err_t debug_wifi_start(void)
{
    if (s_started) return ESP_ERR_INVALID_STATE;

    for (int i = 0; i < MAX_CLIENTS; i++) s_cli[i].fd = -1;

    s_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (s_listen_fd < 0) {
        ESP_LOGE(TAG, "socket() failed errno %d", errno);
        return ESP_FAIL;
    }

    int one = 1;
    setsockopt(s_listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(CONFIG_DEBUG_WIFI_BRIDGE_PORT),
    };
    if (bind(s_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        ESP_LOGE(TAG, "bind() :%u failed errno %d",
                 (unsigned)CONFIG_DEBUG_WIFI_BRIDGE_PORT, errno);
        close(s_listen_fd);
        s_listen_fd = -1;
        return ESP_FAIL;
    }
    if (listen(s_listen_fd, ACCEPT_BACKLOG) != 0) {
        ESP_LOGE(TAG, "listen() failed errno %d", errno);
        close(s_listen_fd);
        s_listen_fd = -1;
        return ESP_FAIL;
    }

    debug_cmds_init();

    /* The command set includes a screenshot: lv_snapshot + the JPEG encoder run
     * on this task, so give it the same 8 KiB the serial REPL uses. */
    BaseType_t ok = xTaskCreate(wifi_debug_task, "dbg_wifi", 8192, NULL, 3, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "task create failed");
        close(s_listen_fd);
        s_listen_fd = -1;
        return ESP_ERR_NO_MEM;
    }

    s_started = true;
    log_endpoint(CONFIG_DEBUG_WIFI_BRIDGE_PORT);
    return ESP_OK;
}

#else  /* !CONFIG_DEBUG_WIFI_BRIDGE */

esp_err_t debug_wifi_start(void)
{
    return ESP_OK;
}

#endif /* CONFIG_DEBUG_WIFI_BRIDGE */
