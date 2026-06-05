/*
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).

    LISP code transfer + control over CAN. Implements the multi-step
    upload (stop -> erase -> chunked write -> run) and read-back of the
    LISP program stored on a target VESC, plus a start/stop control.

    A single dedicated worker task runs one operation at a time, blocking
    on per-step acks delivered by vesc_lisp_code_process_response() (called
    on the CAN process task). Progress and completion are reported via
    callbacks INVOKED FROM THE WORKER TASK — a UI caller must marshal those
    onto its own (e.g. LVGL) thread.

    NOTE: the exact COMM_LISP_{WRITE,ERASE,READ}_CODE wire format and the
    on-flash "packed" header ([int32 code_len][int16 num_imports][code\0])
    follow the documented VESC firmware / VESC Tool CodeLoader layout. It
    must be validated against the target firmware (e.g. by sniffing a VESC
    Tool upload through the BLE NUS bridge). See vesc_lisp_code.c.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VLC_OK = 0,
    VLC_ERR_BUSY,
    VLC_ERR_TIMEOUT,
    VLC_ERR_NOMEM,
    VLC_ERR_ARG,
    VLC_ERR_EMPTY,     /* read: no code stored on the VESC */
} vlc_result_t;

/* Progress during upload/read: bytes done out of total. */
typedef void (*vlc_progress_cb_t)(void *user, uint32_t done, uint32_t total);
/* Upload finished. */
typedef void (*vlc_upload_done_cb_t)(void *user, vlc_result_t res);
/* Read finished. `code` is a NUL-terminated string valid ONLY for the
 * duration of the callback (copy it before returning). NULL on error. */
typedef void (*vlc_read_done_cb_t)(void *user, vlc_result_t res,
                                   const char *code, uint32_t len);

/* Spawns the worker task + creates sync primitives. Safe to call repeatedly. */
void vesc_lisp_code_init(uint8_t target_vesc_id);
void vesc_lisp_code_set_target(uint8_t target_vesc_id);

bool vesc_lisp_code_busy(void);

/* Upload `code` (len bytes, no NUL needed), erase+write to the VESC, and
 * optionally start it running afterwards. `code` is copied internally, so
 * the caller may free it on return. Returns false immediately if busy /
 * invalid / out of memory; otherwise runs async and reports via callbacks. */
bool vesc_lisp_code_upload(const char *code, uint32_t len, bool run_after,
                           vlc_progress_cb_t progress, vlc_upload_done_cb_t done,
                           void *user);

/* Read the currently-stored LISP code from the VESC. */
bool vesc_lisp_code_read(vlc_progress_cb_t progress, vlc_read_done_cb_t done,
                         void *user);

/* Start (run=true) or stop (run=false) the stored script. Fire-and-forget. */
void vesc_lisp_code_set_running(bool run);

/* Hook from the comm_can packet handler. Filters by command byte. */
void vesc_lisp_code_process_response(const uint8_t *data, unsigned int len);

#ifdef __cplusplus
}
#endif
