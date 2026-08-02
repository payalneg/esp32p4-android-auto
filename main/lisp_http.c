#include "lisp_http.h"

#include "sdkconfig.h"

#if CONFIG_LISP_HTTP_ENABLED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "vesc_can/vesc_lisp_code.h"
#include "vesc_can/vesc_lisp_console.h"
#include "vesc_can/vesc_lisp_poll.h"

static const char *TAG = "lisp_http";

/* The editor page, gzipped at build time (see main/CMakeLists.txt). */
extern const uint8_t lisp_editor_html_gz_start[] asm("_binary_lisp_editor_html_gz_start");
extern const uint8_t lisp_editor_html_gz_end[]   asm("_binary_lisp_editor_html_gz_end");

/* Upload cap. vesc_lisp_code wraps the text in a 9-byte envelope and refuses
 * anything past LISP_MAX (120 KiB), so stay comfortably below that. */
#define CODE_MAX          (110 * 1024)
#define RX_CHUNK          4096
/* One shared PSRAM buffer for console responses — built under the lock, sent
 * after it's released so the CAN task never waits on the network. */
#define CONSOLE_JSON_MAX  (32 * 1024)
#define CONSOLE_MAX_LINES 120

enum { JOB_NONE = 0, JOB_UPLOAD, JOB_READ };

static SemaphoreHandle_t s_lock;         /* guards s_code + the job fields   */
static volatile bool     s_active;       /* a transfer we started is running */
static volatile bool     s_finished;
static volatile int      s_kind;
static volatile int      s_result;       /* vlc_result_t                     */
static volatile uint32_t s_done, s_total;
static char             *s_code;         /* PSRAM, NUL-terminated read result */
static uint32_t          s_code_len;
static char             *s_con_json;     /* PSRAM console response scratch    */

/* ---------- small helpers ---------- */

static esp_err_t send_json_err(httpd_req_t *req, int code, const char *msg)
{
    const char *status =
        (code == 400) ? "400 Bad Request"            :
        (code == 404) ? "404 Not Found"              :
        (code == 409) ? "409 Conflict"               :
        (code == 411) ? "411 Length Required"        :
        (code == 413) ? "413 Payload Too Large"      :
        (code == 503) ? "503 Service Unavailable"    :
                        "500 Internal Server Error";
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json");
    char body[192];
    int n = snprintf(body, sizeof body, "{\"error\":\"");
    for (const char *s = msg ? msg : ""; *s && n < (int)sizeof body - 8; ++s) {
        if (*s == '"' || *s == '\\') body[n++] = '\\';
        body[n++] = *s;
    }
    n += snprintf(body + n, sizeof body - n, "\"}");
    return httpd_resp_send(req, body, n);
}

/* Reads one integer query parameter; returns `def` when absent/unparsable. */
static int query_int(httpd_req_t *req, const char *key, int def)
{
    size_t qlen = httpd_req_get_url_query_len(req);
    if (qlen == 0 || qlen > 255) return def;
    char q[256], v[16];
    if (httpd_req_get_url_query_str(req, q, sizeof q) != ESP_OK) return def;
    if (httpd_query_key_value(q, key, v, sizeof v) != ESP_OK) return def;
    return atoi(v);
}

static const char *result_str(int kind, int res)
{
    if (res == VLC_OK) return kind == JOB_UPLOAD ? "Uploaded" : "Read from VESC";
    switch (res) {
    case VLC_ERR_TIMEOUT: return "VESC timeout (check CAN wiring / target id)";
    case VLC_ERR_BUSY:    return "VESC busy";
    case VLC_ERR_NOMEM:   return "Out of memory";
    case VLC_ERR_EMPTY:   return "No code stored on the VESC";
    case VLC_ERR_ARG:     return "Bad request";
    default:              return "Failed";
    }
}

/* Reads the whole request body into a fresh PSRAM buffer (NUL-terminated).
 * On failure returns NULL and sets *answered when it already sent an error
 * response (false means the socket died and there is nobody left to tell). */
static char *recv_body(httpd_req_t *req, uint32_t max_len, uint32_t *out_len,
                       bool *answered)
{
    *answered = true;
    int remaining = req->content_len;
    if (remaining <= 0) {
        send_json_err(req, 411, "empty body");
        return NULL;
    }
    if ((uint32_t)remaining > max_len) {
        send_json_err(req, 413, "too large");
        return NULL;
    }
    char *buf = heap_caps_malloc(remaining + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) buf = malloc(remaining + 1);
    if (!buf) {
        send_json_err(req, 500, "out of memory");
        return NULL;
    }
    int off = 0;
    while (off < remaining) {
        int want = remaining - off;
        if (want > RX_CHUNK) want = RX_CHUNK;
        int n = httpd_req_recv(req, buf + off, want);
        if (n == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (n <= 0) {
            free(buf);
            *answered = false;   /* socket is gone — nothing to answer */
            return NULL;
        }
        off += n;
    }
    buf[off] = '\0';
    if (out_len) *out_len = (uint32_t)off;
    return buf;
}

/* ---------- async job plumbing (callbacks run on the vesc_lisp_code task) ---------- */

static void prog_cb(void *u, uint32_t done, uint32_t total)
{
    (void)u;
    s_done  = done;
    s_total = total;
}

static void upload_done_cb(void *u, vlc_result_t res)
{
    (void)u;
    s_result   = res;
    s_finished = true;
    s_active   = false;
}

static void read_done_cb(void *u, vlc_result_t res, const char *code, uint32_t len)
{
    (void)u;
    if (res == VLC_OK && code && len) {
        char *c = heap_caps_malloc(len + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (c) {
            memcpy(c, code, len);
            c[len] = '\0';
            xSemaphoreTake(s_lock, portMAX_DELAY);
            free(s_code);
            s_code     = c;
            s_code_len = len;
            xSemaphoreGive(s_lock);
        } else {
            res = VLC_ERR_NOMEM;
        }
    }
    s_result   = res;
    s_finished = true;
    s_active   = false;
}

/* ---------- GET /lisp ---------- */

static esp_err_t page_handler(httpd_req_t *req)
{
    const size_t len = lisp_editor_html_gz_end - lisp_editor_html_gz_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    return httpd_resp_send(req, (const char *)lisp_editor_html_gz_start, len);
}

/* ---------- GET /lisp/api/state ---------- */

static esp_err_t state_handler(httpd_req_t *req)
{
    bool busy = s_active || vesc_lisp_code_busy();

    xSemaphoreTake(s_lock, portMAX_DELAY);
    uint32_t code_len = s_code_len;
    xSemaphoreGive(s_lock);

    char body[1024];
    int n = snprintf(body, sizeof body,
        "{\"busy\":%s,\"mine\":%s,\"kind\":%d,\"done\":%u,\"total\":%u,"
        "\"finished\":%s,\"result\":%d,\"msg\":\"%s\",\"codeLen\":%u,"
        "\"codeMax\":%u,\"replMax\":%d,\"consoleSeq\":%u,\"consoleDropped\":%u",
        busy ? "true" : "false",
        s_active ? "true" : "false",
        s_kind,
        (unsigned)s_done, (unsigned)s_total,
        s_finished ? "true" : "false",
        s_result,
        s_kind ? result_str(s_kind, s_result) : "",
        (unsigned)code_len, (unsigned)CODE_MAX, VESC_LISP_REPL_MAX,
        (unsigned)vesc_lisp_console_seq(),
        (unsigned)vesc_lisp_console_dropped());

    const lisp_stats_t *st = vesc_lisp_poll_get_stats();
    if (st && n < (int)sizeof body - 256) {
        n += snprintf(body + n, sizeof body - n,
            ",\"stats\":{\"cpu\":%.1f,\"heap\":%.1f,\"mem\":%.1f,\"stack\":%.1f,"
            "\"vars\":[", st->cpu_use, st->heap_use, st->mem_use, st->stack_use);
        for (uint8_t i = 0; i < st->variable_count; i++) {
            if (n > (int)sizeof body - 96) break;
            n += snprintf(body + n, sizeof body - n, "%s{\"n\":\"%s\",\"v\":%.4f}",
                          i ? "," : "", st->variables[i].name,
                          st->variables[i].value);
        }
        n += snprintf(body + n, sizeof body - n, "]}");
    }
    n += snprintf(body + n, sizeof body - n, "}");

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, body, n);
}

/* ---------- POST /lisp/api/read ---------- */

static esp_err_t read_handler(httpd_req_t *req)
{
    if (s_active || vesc_lisp_code_busy()) {
        return send_json_err(req, 409, "a transfer is already running");
    }
    s_kind = JOB_READ; s_finished = false; s_result = VLC_OK;
    s_done = 0; s_total = 0; s_active = true;
    if (!vesc_lisp_code_read(prog_cb, read_done_cb, NULL)) {
        s_active = false;
        return send_json_err(req, 503, "CAN not ready or out of memory");
    }
    httpd_resp_set_status(req, "202 Accepted");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---------- GET /lisp/api/code ---------- */

static esp_err_t code_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    /* Sent with the lock held: the buffer belongs to whoever read it last and
     * a concurrent read would free it under us. The only task that can block
     * on this is the transfer worker finishing its NEXT read, which is
     * harmless (it just stays "busy" a moment longer). */
    xSemaphoreTake(s_lock, portMAX_DELAY);
    esp_err_t e = s_code ? httpd_resp_send(req, s_code, s_code_len)
                         : httpd_resp_send(req, "", 0);
    xSemaphoreGive(s_lock);
    return e;
}

/* ---------- POST /lisp/api/upload?run=0|1 ---------- */

static esp_err_t upload_handler(httpd_req_t *req)
{
    bool run_after = query_int(req, "run", 1) != 0;

    if (s_active || vesc_lisp_code_busy()) {
        return send_json_err(req, 409, "a transfer is already running");
    }

    uint32_t len = 0;
    bool answered = false;
    char *code = recv_body(req, CODE_MAX, &len, &answered);
    if (!code) return answered ? ESP_OK : ESP_FAIL;

    s_kind = JOB_UPLOAD; s_finished = false; s_result = VLC_OK;
    s_done = 0; s_total = 0; s_active = true;

    /* vesc_lisp_code_upload copies the text into its own blob, so the receive
     * buffer goes back to the heap right away. */
    bool started = vesc_lisp_code_upload(code, len, run_after,
                                         prog_cb, upload_done_cb, NULL);
    free(code);
    if (!started) {
        s_active = false;
        return send_json_err(req, 503, "busy, out of memory, or script too large");
    }
    ESP_LOGI(TAG, "upload started: %u bytes, run_after=%d", (unsigned)len, run_after);
    httpd_resp_set_status(req, "202 Accepted");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---------- POST /lisp/api/run?run=0|1 ---------- */

static esp_err_t run_handler(httpd_req_t *req)
{
    if (s_active || vesc_lisp_code_busy()) {
        return send_json_err(req, 409, "a transfer is running");
    }
    bool run = query_int(req, "run", 1) != 0;
    vesc_lisp_code_set_running(run);
    vesc_lisp_console_add(run ? "-- start requested" : "-- stop requested", "note");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---------- POST /lisp/api/repl ---------- */

static esp_err_t repl_handler(httpd_req_t *req)
{
    if (s_active || vesc_lisp_code_busy()) {
        return send_json_err(req, 409, "a transfer is running");
    }

    uint32_t len = 0;
    bool answered = false;
    char *expr = recv_body(req, VESC_LISP_REPL_MAX, &len, &answered);
    if (!expr) return answered ? ESP_OK : ESP_FAIL;

    /* Trim trailing whitespace so a stray newline from the input box doesn't
     * eat into the (short) single-transfer budget. */
    while (len && (expr[len - 1] == '\n' || expr[len - 1] == '\r' ||
                   expr[len - 1] == ' '  || expr[len - 1] == '\t')) {
        expr[--len] = '\0';
    }

    bool ok = len && vesc_lisp_code_repl(expr, len);
    if (ok) {
        char note[VESC_LISP_REPL_MAX + 16];
        snprintf(note, sizeof note, "> %s", expr);
        vesc_lisp_console_add(note, "note");
    }
    free(expr);

    if (!ok) return send_json_err(req, 503, "cannot send (busy or empty)");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---------- GET /lisp/api/console?since=N ---------- */

typedef struct {
    char    *buf;
    size_t   cap;
    size_t   len;
    uint32_t last_seq;
    bool     first;
    bool     full;
} con_sink_t;

static void sink_raw(con_sink_t *s, const char *txt)
{
    size_t n = strlen(txt);
    if (s->len + n >= s->cap) { s->full = true; return; }
    memcpy(s->buf + s->len, txt, n);
    s->len += n;
}

static void sink_escaped(con_sink_t *s, const char *txt)
{
    for (const unsigned char *p = (const unsigned char *)txt; *p; p++) {
        if (s->len + 8 >= s->cap) { s->full = true; return; }
        if (*p == '"' || *p == '\\') {
            s->buf[s->len++] = '\\';
            s->buf[s->len++] = (char)*p;
        } else if (*p < 0x20) {
            s->len += snprintf(s->buf + s->len, s->cap - s->len, "\\u%04x", *p);
        } else {
            s->buf[s->len++] = (char)*p;
        }
    }
}

static void console_line_cb(void *user, uint32_t seq, const char *kind,
                            const char *text)
{
    con_sink_t *s = user;
    if (s->full) return;
    sink_raw(s, s->first ? "{\"s\":" : ",{\"s\":");
    char num[16];
    snprintf(num, sizeof num, "%u", (unsigned)seq);
    sink_raw(s, num);
    sink_raw(s, ",\"k\":\"");
    sink_escaped(s, kind);
    sink_raw(s, "\",\"t\":\"");
    sink_escaped(s, text);
    sink_raw(s, "\"}");
    if (s->full) return;      /* truncated — don't claim this line was sent */
    s->first    = false;
    s->last_seq = seq;
}

static esp_err_t console_handler(httpd_req_t *req)
{
    if (!s_con_json) return send_json_err(req, 500, "console buffer missing");

    uint32_t since = (uint32_t)query_int(req, "since", 0);

    xSemaphoreTake(s_lock, portMAX_DELAY);
    con_sink_t sink = {
        .buf = s_con_json, .cap = CONSOLE_JSON_MAX - 64, .len = 0,
        .last_seq = since, .first = true, .full = false,
    };
    sink_raw(&sink, "{\"lines\":[");
    /* The ring's own lock is held only inside this call; the JSON is built
     * into PSRAM here and pushed to the socket after we're out. */
    vesc_lisp_console_read(since, CONSOLE_MAX_LINES, console_line_cb, &sink);
    sink.cap = CONSOLE_JSON_MAX;      /* the tail always fits */
    sink_raw(&sink, "],\"seq\":");
    char tail[96];
    snprintf(tail, sizeof tail, "%u,\"dropped\":%u,\"more\":%s}",
             (unsigned)sink.last_seq, (unsigned)vesc_lisp_console_dropped(),
             (sink.last_seq < vesc_lisp_console_seq()) ? "true" : "false");
    sink_raw(&sink, tail);

    httpd_resp_set_type(req, "application/json");
    esp_err_t e = httpd_resp_send(req, s_con_json, sink.len);
    xSemaphoreGive(s_lock);
    return e;
}

/* ---------- POST /lisp/api/console/clear ---------- */

static esp_err_t console_clear_handler(httpd_req_t *req)
{
    vesc_lisp_console_clear();
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---------- POST /lisp/api/stats ---------- */

static esp_err_t stats_handler(httpd_req_t *req)
{
    /* One-shot COMM_LISP_GET_STATS; the answer shows up in /lisp/api/state a
     * poll or two later. Cheap enough to call while the Stats tab is open. */
    vesc_lisp_poll_request_once();
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---------- registration ---------- */

esp_err_t lisp_http_register(httpd_handle_t server)
{
    if (!server) {
        ESP_LOGE(TAG, "no server handle — call after ota_http_start()");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_lock) return ESP_ERR_INVALID_STATE;

    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    s_con_json = heap_caps_malloc(CONSOLE_JSON_MAX,
                                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_con_json) {
        ESP_LOGW(TAG, "console JSON buffer alloc failed — console disabled");
    }
    /* Idempotent; also covers builds where CAN never came up (emulator mode)
     * so the page still loads and just shows an empty console. */
    vesc_lisp_console_init();

    static const httpd_uri_t routes[] = {
        { .uri = "/lisp",                  .method = HTTP_GET,
          .handler = page_handler,          .user_ctx = NULL },
        { .uri = "/lisp/api/state",        .method = HTTP_GET,
          .handler = state_handler,         .user_ctx = NULL },
        { .uri = "/lisp/api/code",         .method = HTTP_GET,
          .handler = code_handler,          .user_ctx = NULL },
        { .uri = "/lisp/api/console",      .method = HTTP_GET,
          .handler = console_handler,       .user_ctx = NULL },
        { .uri = "/lisp/api/read",         .method = HTTP_POST,
          .handler = read_handler,          .user_ctx = NULL },
        { .uri = "/lisp/api/upload",       .method = HTTP_POST,
          .handler = upload_handler,        .user_ctx = NULL },
        { .uri = "/lisp/api/run",          .method = HTTP_POST,
          .handler = run_handler,           .user_ctx = NULL },
        { .uri = "/lisp/api/repl",         .method = HTTP_POST,
          .handler = repl_handler,          .user_ctx = NULL },
        { .uri = "/lisp/api/console/clear", .method = HTTP_POST,
          .handler = console_clear_handler, .user_ctx = NULL },
        { .uri = "/lisp/api/stats",        .method = HTTP_POST,
          .handler = stats_handler,         .user_ctx = NULL },
    };
    for (size_t i = 0; i < sizeof routes / sizeof routes[0]; ++i) {
        esp_err_t e = httpd_register_uri_handler(server, &routes[i]);
        if (e != ESP_OK) {
            ESP_LOGE(TAG, "register %s: %s", routes[i].uri, esp_err_to_name(e));
            return e;
        }
    }
    ESP_LOGI(TAG, "web LISP editor attached at /lisp (%u byte page)",
             (unsigned)(lisp_editor_html_gz_end - lisp_editor_html_gz_start));
    return ESP_OK;
}

#else  /* !CONFIG_LISP_HTTP_ENABLED */

esp_err_t lisp_http_register(httpd_handle_t server)
{
    (void)server;
    return ESP_OK;
}

#endif
