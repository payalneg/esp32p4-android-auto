/*
    Copyright 2025 Super VESC Display
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).

    Periodic COMM_LISP_GET_STATS poll. Disabled unless
    CONFIG_VESC_CAN_LISP_POLL_ENABLE=y. Optional — only useful when
    the VESC has a Lisp app loaded that exports variables.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LISP_VAR_NAME_MAX  64
#define LISP_VAR_MAX_COUNT 32

typedef struct {
    char   name[LISP_VAR_NAME_MAX];
    double value;
} lisp_variable_t;

typedef struct {
    double          cpu_use;
    double          heap_use;
    double          mem_use;
    double          stack_use;
    char            done_ctx_r[128];
    lisp_variable_t variables[LISP_VAR_MAX_COUNT];
    uint8_t         variable_count;
} lisp_stats_t;

void vesc_lisp_poll_init(uint8_t target_vesc_id, uint32_t poll_interval_ms);
void vesc_lisp_poll_start(void);
void vesc_lisp_poll_stop(void);
/* Pumped from vesc_rt_data's CAN polling task — one task drives both
 * pollers so we don't double the FreeRTOS overhead for a single CAN bus. */
void vesc_lisp_poll_loop(void);

/* Ask for ONE COMM_LISP_GET_STATS without turning the periodic poll on.
 * Callable from any task: it only raises a flag, the request itself goes
 * out from vesc_lisp_poll_once_loop() on the CAN task (that serialisation
 * is what keeps the big multi-frame reply from colliding with the RT poll).
 * Used by the web LISP editor while its Stats tab is open — the bus stays
 * quiet the rest of the time. */
void vesc_lisp_poll_request_once(void);
void vesc_lisp_poll_once_loop(void);

void vesc_lisp_poll_process_response(const uint8_t *data, unsigned int len);

const lisp_stats_t *vesc_lisp_poll_get_stats(void);
bool vesc_lisp_poll_get_variable_int(const char *name, int32_t *out);
bool vesc_lisp_poll_get_variable_float(const char *name, float *out);

#ifdef __cplusplus
}
#endif
