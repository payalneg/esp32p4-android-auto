/*
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).

    Console ring for the VESC's script output — COMM_LISP_PRINT (the
    script's own `(print ...)`) and COMM_PRINT (plain commands_printf()).
    Both arrive unsolicited on the CAN bus, addressed to our controller id,
    so the only thing needed to see them is to stop dropping them.

    A running script can print inside a 100 Hz loop, so this is a bounded
    ring: newest lines win, older ones are dropped and counted. Readers
    (the web editor's /lisp/api/console) poll with the sequence number of
    the last line they saw; the gap between that and `dropped` tells them
    how much they missed.

    Lines are written from the CAN process task and read from the httpd
    task — everything below takes an internal mutex.
*/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LISP_CONSOLE_LINES     192
#define LISP_CONSOLE_LINE_MAX  200

/* Idempotent. Allocates the ring (PSRAM when available) + mutex. Safe to
 * call before comm_can is up; process_response() is a no-op until then. */
void vesc_lisp_console_init(void);

/* Hook from the comm_can packet handler. Filters by command byte. */
void vesc_lisp_console_process_response(const uint8_t *data, unsigned int len);

/* Push a line from our own side (status notes shown inline with the VESC
 * output, e.g. "-- repl: (+ 1 2)"). `kind` is echoed back to readers. */
void vesc_lisp_console_add(const char *text, const char *kind);

/* Sequence number of the newest line (0 = nothing ever printed). */
uint32_t vesc_lisp_console_seq(void);
/* Lines evicted before any reader saw them are NOT counted here — this is
 * the total number of lines that fell out of the ring. */
uint32_t vesc_lisp_console_dropped(void);

/* Iterate the lines newer than `since`, oldest first, up to `max_lines`.
 * The callback runs with the ring locked: copy, don't block. Returns the
 * sequence number of the last line passed to the callback (== `since` when
 * there was nothing new). */
typedef void (*vesc_lisp_console_line_cb_t)(void *user, uint32_t seq,
                                            const char *kind, const char *text);
uint32_t vesc_lisp_console_read(uint32_t since, uint32_t max_lines,
                                vesc_lisp_console_line_cb_t cb, void *user);

/* Forget everything. Sequence numbers keep counting up (readers polling
 * with an old `since` must not be handed the same lines twice). */
void vesc_lisp_console_clear(void);

#ifdef __cplusplus
}
#endif
