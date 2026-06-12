/*
    Copyright 2025 Super VESC Display
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).
*/

#include "vesc_can/vesc_lisp_poll.h"

#include "vesc_can/buffer.h"
#include "vesc_can/comm_can.h"
#include "vesc_can/vesc_datatypes.h"

#include "esp_log.h"
#include "esp_timer.h"

#include <string.h>

static const char *TAG = "vesc_lisp";

static bool         s_active             = false;
static uint32_t     s_last_poll_ms       = 0;
static uint8_t      s_target_vesc_id     = 10;
static uint32_t     s_poll_interval_ms   = 100;

static lisp_stats_t s_stats;
static bool         s_stats_received     = false;

static inline uint32_t millis_now(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void vesc_lisp_poll_init(uint8_t target_vesc_id, uint32_t poll_interval_ms)
{
    s_active = false;
    s_last_poll_ms = 0;
    s_target_vesc_id = target_vesc_id;
    s_poll_interval_ms = poll_interval_ms ? poll_interval_ms : 100;
    memset(&s_stats, 0, sizeof(s_stats));
    s_stats_received = false;
}

void vesc_lisp_poll_start(void) { s_active = true; s_last_poll_ms = 0; }
void vesc_lisp_poll_stop(void)  { s_active = false; }

void vesc_lisp_poll_loop(void)
{
    if (!s_active) return;
    uint32_t now = millis_now();
    if (now - s_last_poll_ms < s_poll_interval_ms) return;
    s_last_poll_ms = now;

    uint8_t send_buffer[3];
    int32_t ind = 0;
    send_buffer[ind++] = COMM_LISP_GET_STATS;
    send_buffer[ind++] = 1; /* poll_all */
    /* Synced: the stats reply is the big multi-frame one (hundreds of bytes);
     * letting it overlap the RT-data poll's reply is exactly what corrupted the
     * reassembly buffer and dropped cruise/mode values. */
    comm_can_send_buffer_sync(s_target_vesc_id, send_buffer, ind, 0, 60);
}

void vesc_lisp_poll_process_response(const uint8_t *data, unsigned int len)
{
    if (len < 1) return;
    if (data[0] != COMM_LISP_GET_STATS) return;

    int32_t ind = 1;
    memset(&s_stats, 0, sizeof(s_stats));

    if (ind + 2 <= (int)len) s_stats.cpu_use   = buffer_get_float16(data, 1e2f, &ind);
    if (ind + 2 <= (int)len) s_stats.heap_use  = buffer_get_float16(data, 1e2f, &ind);
    if (ind + 2 <= (int)len) s_stats.mem_use   = buffer_get_float16(data, 1e2f, &ind);
    if (ind + 2 <= (int)len) s_stats.stack_use = buffer_get_float16(data, 1e2f, &ind);

    if (ind < (int)len) {
        buffer_get_string(data, len, s_stats.done_ctx_r, sizeof(s_stats.done_ctx_r), &ind);
    }

    s_stats.variable_count = 0;
    while (ind < (int)len && s_stats.variable_count < LISP_VAR_MAX_COUNT) {
        char name[LISP_VAR_NAME_MAX];
        int  name_len = buffer_get_string(data, len, name, sizeof(name), &ind);
        if (name_len == 0) break;
        if (ind + 4 > (int)len) {
            ESP_LOGW(TAG, "truncated value at ind=%d/%u", (int)ind, len);
            break;
        }
        double v = buffer_get_float32_auto(data, &ind);
        strncpy(s_stats.variables[s_stats.variable_count].name, name,
                sizeof(s_stats.variables[0].name) - 1);
        s_stats.variables[s_stats.variable_count].name[sizeof(s_stats.variables[0].name) - 1] = '\0';
        s_stats.variables[s_stats.variable_count].value = v;
        s_stats.variable_count++;
    }
    s_stats_received = true;
}

const lisp_stats_t *vesc_lisp_poll_get_stats(void)
{
    return s_stats_received ? &s_stats : NULL;
}

bool vesc_lisp_poll_get_variable_int(const char *name, int32_t *out)
{
    if (!s_stats_received || !name || !out) return false;
    for (uint8_t i = 0; i < s_stats.variable_count; i++) {
        if (strcmp(s_stats.variables[i].name, name) == 0) {
            *out = (int32_t)s_stats.variables[i].value;
            return true;
        }
    }
    return false;
}

bool vesc_lisp_poll_get_variable_float(const char *name, float *out)
{
    if (!s_stats_received || !name || !out) return false;
    for (uint8_t i = 0; i < s_stats.variable_count; i++) {
        if (strcmp(s_stats.variables[i].name, name) == 0) {
            *out = (float)s_stats.variables[i].value;
            return true;
        }
    }
    return false;
}
