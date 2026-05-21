#include "log_capture.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

/* 1 MB of recent log lives in PSRAM. The board has 32 MB HEX PSRAM, so
 * a megabyte for log history is comfortable. The Logs UI renders only
 * the tail of this (~32 KB) at a time since lv_label can't usefully
 * scroll through a million chars, but the rest stays available for
 * future "save to SD" / dump-over-USB workflows. */
#define LOG_CAPTURE_SIZE   (1024u * 1024u)

/* Formatted log lines from ESP_LOG rarely exceed 200 bytes — but we
 * occasionally hex-dump large packets. Cap each formatted message
 * here; longer messages get tail-truncated. */
#define LOG_LINE_MAX       512

static char     *s_buf;
static size_t    s_buf_size;
static size_t    s_head;            /* next write index, wraps */
static size_t    s_total_written;   /* monotonic count of bytes accepted */
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static vprintf_like_t s_orig_vprintf;

/* esp_log's registered vprintf — runs on every ESP_LOGx call from task
 * context. ISR-context logs go through esp_rom_printf and bypass this
 * hook, so we don't need full ISR-safety on the formatting path.
 *
 * Intentionally NOT IRAM_ATTR — pinning this in IRAM shifts .text/.bss
 * boundaries enough to drop the largest contiguous internal-DRAM block
 * below the SDIO mempool threshold and panic boot ("sdio_mempool_create
 * ... no mem"). Living in flash .text is fine here: this hook is only
 * called from task context, never from an ISR. */
static int log_capture_vprintf(const char *fmt, va_list args)
{
    /* va_list is single-use: after the first consumer reads via va_arg
     * the source is in an unspecified state, so each downstream call
     * needs its own va_copy. RISC-V GCC's va_list is essentially a
     * pointer into the caller's stack frame; without two independent
     * copies the second consumer reads garbage and produces duplicate-
     * or corrupted-looking lines in the captured log. */
    va_list args_for_orig, args_for_capture;
    va_copy(args_for_orig,    args);
    va_copy(args_for_capture, args);

    /* Forward to the original first so the UART console keeps working
     * even if our ring buffer logic throws. */
    int n = s_orig_vprintf ? s_orig_vprintf(fmt, args_for_orig) : 0;
    va_end(args_for_orig);

    if (!s_buf) {
        va_end(args_for_capture);
        return n;
    }

    /* Format into a stack buffer, then push into the ring. vsnprintf
     * truncates cleanly if the line is too long. */
    char line[LOG_LINE_MAX];
    int len = vsnprintf(line, sizeof(line), fmt, args_for_capture);
    va_end(args_for_capture);
    if (len <= 0) return n;
    if (len >= (int)sizeof(line)) len = sizeof(line) - 1;

    portENTER_CRITICAL(&s_lock);
    for (int i = 0; i < len; i++) {
        s_buf[s_head] = line[i];
        s_head = (s_head + 1) % s_buf_size;
    }
    s_total_written += (size_t)len;
    portEXIT_CRITICAL(&s_lock);

    return n;
}

esp_err_t log_capture_init(void)
{
    if (s_buf) return ESP_OK;

    /* PSRAM-backed buffer — keeps internal SRAM intact for IRAM/DRAM-
     * sensitive subsystems (h264 decoder, LVGL framebuffers, etc.). */
    s_buf = heap_caps_malloc(LOG_CAPTURE_SIZE,
                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_buf) return ESP_ERR_NO_MEM;

    s_buf_size = LOG_CAPTURE_SIZE;
    s_head = 0;
    s_total_written = 0;

    s_orig_vprintf = esp_log_set_vprintf(log_capture_vprintf);
    return ESP_OK;
}

size_t log_capture_snapshot(char *out, size_t out_size)
{
    if (!s_buf || !out || out_size == 0) return 0;
    if (out_size == 1) { out[0] = '\0'; return 0; }

    portENTER_CRITICAL(&s_lock);
    size_t buf_size  = s_buf_size;
    size_t head      = s_head;
    size_t available = s_total_written < buf_size ? s_total_written : buf_size;
    size_t want      = available < out_size - 1 ? available : out_size - 1;
    /* The newest byte is at s_buf[(head - 1) % buf_size]. Walk back
     * `want` bytes from head to find the oldest we'll keep. */
    size_t start = (head + buf_size - want) % buf_size;
    for (size_t i = 0; i < want; i++) {
        out[i] = s_buf[(start + i) % buf_size];
    }
    portEXIT_CRITICAL(&s_lock);

    out[want] = '\0';
    return want;
}

/* LVGL recolor tag prefix per ESP_LOG level letter. Picks one of five
 * colors based on the first non-space character of each log line. The
 * tag is "#RRGGBB " (7 bytes + space), closed with a single '#'. */
static const char *level_color_tag(char level)
{
    switch (level) {
        case 'E': return "#FF4444 ";  /* error    — red    */
        case 'W': return "#FFB02E ";  /* warning  — orange */
        case 'I': return "#B6FF2E ";  /* info     — green  */
        case 'D': return "#AAAAAA ";  /* debug    — light gray */
        case 'V': return "#666666 ";  /* verbose  — dim gray   */
        default:  return NULL;        /* no recognized level — uncolored */
    }
}

size_t log_capture_snapshot_colorized(char *out, size_t out_size)
{
    if (!out || out_size < 2) {
        if (out && out_size) out[0] = '\0';
        return 0;
    }

    /* Pull raw bytes into the start of the output buffer, then walk
     * through it line by line, rewriting in place into a colorized
     * stream. Allocate scratch for the rewrite to avoid in-place
     * shifting (the colorized version is always >= original size). */
    size_t raw_cap = out_size > 1024 ? out_size / 2 : out_size - 1;
    if (raw_cap < 64) raw_cap = out_size - 1;

    /* Use heap_caps_malloc → fall back to malloc — PSRAM is fine for
     * the scratch buffer since this is called from a UI event handler,
     * not a hot path. */
    char *raw = heap_caps_malloc(raw_cap + 1,
                                 MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!raw) raw = malloc(raw_cap + 1);
    if (!raw) {
        out[0] = '\0';
        return 0;
    }
    size_t raw_len = log_capture_snapshot(raw, raw_cap + 1);

    /* Walk raw[] line by line. Each line begins after a '\n' (or at
     * raw[0] for the first). We may have started mid-line because of
     * the ring-buffer wrap; treat that as no recognized level. */
    size_t op = 0;
    size_t ip = 0;
    bool   first_line = true;
    while (ip < raw_len && op < out_size - 1) {
        /* Find end of this line. */
        size_t line_end = ip;
        while (line_end < raw_len && raw[line_end] != '\n') line_end++;
        size_t line_len = line_end - ip;
        bool has_newline = (line_end < raw_len);

        /* Detect level letter at the start of the line: the format is
         * "X (timestamp) tag: msg" where X is I/W/E/D/V. Skip if the
         * first line of a wrapped snapshot doesn't fit that pattern. */
        char level = '\0';
        if (!first_line || ip == 0) {
            /* Trim leading whitespace before the level char. */
            size_t scan = ip;
            while (scan < line_end && (raw[scan] == ' ' || raw[scan] == '\r'))
                scan++;
            if (scan < line_end &&
                (raw[scan] == 'I' || raw[scan] == 'W' || raw[scan] == 'E' ||
                 raw[scan] == 'D' || raw[scan] == 'V') &&
                scan + 2 < line_end && raw[scan + 1] == ' ' &&
                raw[scan + 2] == '(') {
                level = raw[scan];
            }
        }
        first_line = false;

        const char *tag = level_color_tag(level);

        /* Write opening tag (if any). */
        if (tag) {
            size_t taglen = strlen(tag);
            if (op + taglen >= out_size - 1) break;
            memcpy(out + op, tag, taglen);
            op += taglen;
        }

        /* Copy the line, replacing any literal '#' with '*' so the
         * recolor parser doesn't choke. */
        for (size_t i = 0; i < line_len; i++) {
            if (op >= out_size - 2) break;
            char c = raw[ip + i];
            out[op++] = (c == '#') ? '*' : c;
        }

        /* Close tag, then ALWAYS emit '\n' so LVGL recolor never sees
         * adjacent tags (close '#' followed immediately by next open
         * '#' would trigger LVGL's lv_txt.c PAR-state quirk where '#'
         * after '#' renders as a literal char and the rest of the line
         * loses its color). Also defends against ESP-IDF log calls
         * that don't end with '\n' (rare, but possible). */
        if (tag) {
            if (op >= out_size - 1) break;
            out[op++] = '#';
        }
        if (op >= out_size - 1) break;
        out[op++] = '\n';

        ip = line_end + (has_newline ? 1 : 0);
    }
    out[op] = '\0';

    free(raw);
    return op;
}

size_t log_capture_total_bytes(void)
{
    portENTER_CRITICAL(&s_lock);
    size_t total = s_total_written;
    portEXIT_CRITICAL(&s_lock);
    return total;
}
