/*
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).

    Console ring for VESC script output — see vesc_lisp_console.h.
*/

#include "vesc_can/vesc_lisp_console.h"

#include "vesc_can/vesc_datatypes.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "vesc_lisp_con";

/* Without PSRAM the full ring (~40 KiB) is too much to take out of internal
 * RAM for a debug convenience — fall back to a short one. */
#define LINES_NO_PSRAM  48

typedef struct {
    uint32_t seq;
    char     kind[8];
    char     text[LISP_CONSOLE_LINE_MAX];
} con_line_t;

static con_line_t       *s_ring;
static uint32_t          s_cap;
static uint32_t          s_seq;      /* seq of the newest line, 0 = empty  */
static uint32_t          s_base;     /* nothing at or below this is served */
static uint32_t          s_dropped;  /* lines evicted from the ring        */
static SemaphoreHandle_t s_lock;

void vesc_lisp_console_init(void)
{
    if (s_ring) return;

    if (!s_lock) s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return;

    uint32_t cap = LISP_CONSOLE_LINES;
    con_line_t *ring = heap_caps_calloc(cap, sizeof(con_line_t),
                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!ring) {
        cap  = LINES_NO_PSRAM;
        ring = calloc(cap, sizeof(con_line_t));
    }
    if (!ring) {
        ESP_LOGW(TAG, "console ring alloc failed — output not captured");
        return;
    }
    s_cap  = cap;
    s_ring = ring;
    ESP_LOGI(TAG, "console ring: %u lines x %d bytes",
             (unsigned)cap, LISP_CONSOLE_LINE_MAX);
}

/* Caller holds the lock. `text` is NUL-terminated and already trimmed. */
static void push_locked(const char *kind, const char *text)
{
    if (s_seq >= s_cap) s_dropped++;      /* the slot we're about to reuse */
    con_line_t *l = &s_ring[s_seq % s_cap];
    s_seq++;
    l->seq = s_seq;
    snprintf(l->kind, sizeof l->kind, "%s", kind);
    snprintf(l->text, sizeof l->text, "%s", text);
}

/* Split a printf payload into lines. The VESC sends whatever the script
 * handed to (print ...) — that can be several lines in one packet, and
 * long lines get chopped to fit the ring rather than dropped. */
static void add_chunk(const char *kind, const char *buf, size_t len)
{
    if (!s_ring || !s_lock || !len) return;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    size_t start = 0;
    for (size_t i = 0; i <= len; i++) {
        bool eol = (i == len) || buf[i] == '\n';
        if (!eol) continue;
        /* A payload ending in '\n' must not produce a trailing blank line. */
        if (i == len && start == len) break;

        size_t end = i;
        while (end > start && (buf[end - 1] == '\r' || buf[end - 1] == '\0')) end--;

        /* Split anything longer than a ring slot instead of truncating. */
        size_t off = start;
        do {
            size_t n = end - off;
            if (n > LISP_CONSOLE_LINE_MAX - 1) n = LISP_CONSOLE_LINE_MAX - 1;
            char line[LISP_CONSOLE_LINE_MAX];
            memcpy(line, buf + off, n);
            line[n] = '\0';
            push_locked(kind, line);
            off += n;
        } while (off < end);

        start = i + 1;
    }
    xSemaphoreGive(s_lock);
}

void vesc_lisp_console_add(const char *text, const char *kind)
{
    if (!text) return;
    add_chunk(kind ? kind : "note", text, strlen(text));
}

void vesc_lisp_console_process_response(const uint8_t *data, unsigned int len)
{
    if (!s_ring || len < 2) return;

    const char *kind;
    switch (data[0]) {
    case COMM_LISP_PRINT: kind = "lisp"; break;
    case COMM_PRINT:      kind = "fw";   break;
    default:              return;
    }

    /* Payload is the text; the firmware terminates it with a NUL on some
     * paths and not on others, so cut at the first NUL if there is one. */
    const char *txt = (const char *)data + 1;
    unsigned int n  = len - 1;
    const void *nul = memchr(txt, '\0', n);
    if (nul) n = (unsigned int)((const char *)nul - txt);
    if (!n) return;

    add_chunk(kind, txt, n);
}

uint32_t vesc_lisp_console_seq(void)     { return s_seq; }
uint32_t vesc_lisp_console_dropped(void) { return s_dropped; }

uint32_t vesc_lisp_console_read(uint32_t since, uint32_t max_lines,
                                vesc_lisp_console_line_cb_t cb, void *user)
{
    if (!s_ring || !s_lock || !cb) return since;

    xSemaphoreTake(s_lock, portMAX_DELAY);

    /* Oldest line still in the ring (and still wanted — clear() moves the
     * base up instead of rewinding the sequence). */
    uint32_t oldest = (s_seq > s_cap) ? (s_seq - s_cap) : 0;
    if (oldest < s_base) oldest = s_base;
    uint32_t from   = (since > oldest) ? since : oldest;
    if (from > s_seq) from = s_seq;          /* reader ahead of us (cleared) */
    if (max_lines && s_seq - from > max_lines) from = s_seq - max_lines;

    for (uint32_t seq = from + 1; seq <= s_seq; seq++) {
        const con_line_t *l = &s_ring[(seq - 1) % s_cap];
        cb(user, l->seq, l->kind, l->text);
    }
    uint32_t last = s_seq;

    xSemaphoreGive(s_lock);
    return last;
}

void vesc_lisp_console_clear(void)
{
    if (!s_ring || !s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    /* Keep s_seq monotonic — a reader still polling with an old `since`
     * would otherwise be handed the next lines twice. Everything currently
     * stored is hidden by moving the base up instead. */
    s_base = s_seq;
    xSemaphoreGive(s_lock);
}
