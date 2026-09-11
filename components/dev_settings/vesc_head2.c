/*
    Copyright 2026 ESP32-P4 Android Auto / VESC Display (GPL-3.0).

    See vesc_head2.h. Thin settings-gated reader over the CAN STATUS_4 table
    kept by comm_can.c. STATUS_4.rx_time is stamped with xTaskGetTickCount(),
    so freshness is computed in ticks.
*/

#include "vesc_head2.h"

#include "dev_settings.h"
#include "vesc_can/buffer.h"
#include "vesc_can/comm_can.h"
#include "vesc_can/vesc_datatypes.h"
#include "vesc_can/vesc_link.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Display/liveness window. The VESC-side aggregation window is a tight 100 ms,
 * but for showing temps and the alive flag a few missed 50 Hz frames are fine —
 * 1 s avoids flicker if the bus hiccups. */
#define HEAD2_FRESH_MS 1000u

/* ---- BLE polling ----
 *
 * COMM_GET_VALUES_SELECTIVE with just the two temperature bits: the smallest
 * request that answers the same question STATUS_4 answers on CAN. It is the
 * only place in the firmware that sends command 50 (the dashboard polls 51,
 * SETUP_SELECTIVE), so the reply needs no further disambiguation — which
 * matters, because a point-to-point link matches replies by arrival order. */
#define HEAD2_MASK_TEMP_FET   (1U << 0)
#define HEAD2_MASK_TEMP_MOTOR (1U << 1)
#define HEAD2_POLL_MS         1000u

static float   s_polled_fet;
static float   s_polled_motor;
static int64_t s_polled_us;          /* 0 = never answered */
static int64_t s_last_poll_us;

void vesc_head2_poll_loop(void)
{
    if (vesc_link_get_mode() != VESC_LINK_MODE_BLE) return;
    if (!settings_get_second_head_enabled()) return;

    int64_t now = esp_timer_get_time();
    if (s_last_poll_us && (now - s_last_poll_us) < (int64_t)HEAD2_POLL_MS * 1000) {
        return;
    }
    s_last_poll_us = now;

    uint8_t buf[8];
    int32_t ind = 0;
    buf[ind++] = COMM_GET_VALUES_SELECTIVE;
    buffer_append_uint32(buf, HEAD2_MASK_TEMP_FET | HEAD2_MASK_TEMP_MOTOR, &ind);
    vesc_link_send_sync(settings_get_second_head_id(), buf, (unsigned int)ind, 0,
                        vesc_link_sync_timeout_ms());
}

void vesc_head2_process_response(const uint8_t *data, unsigned int len)
{
    if (len < 9 || data[0] != COMM_GET_VALUES_SELECTIVE) return;

    int32_t  ind  = 1;
    uint32_t mask = buffer_get_uint32(data, &ind);
    if (!(mask & HEAD2_MASK_TEMP_FET) || !(mask & HEAD2_MASK_TEMP_MOTOR)) return;

    s_polled_fet   = buffer_get_float16(data, 1e1f, &ind);
    s_polled_motor = buffer_get_float16(data, 1e1f, &ind);
    s_polled_us    = esp_timer_get_time();
}

bool vesc_head2_get_temps(float *temp_fet, float *temp_motor)
{
    if (!settings_get_second_head_enabled()) return false;

    if (vesc_link_get_mode() == VESC_LINK_MODE_BLE) {
        if (s_polled_us == 0) return false;
        if ((esp_timer_get_time() - s_polled_us) / 1000 > HEAD2_FRESH_MS) return false;
        if (temp_fet)   *temp_fet   = s_polled_fet;
        if (temp_motor) *temp_motor = s_polled_motor;
        return true;
    }

    can_status_msg_4 *m = comm_can_get_status_msg_4_id(settings_get_second_head_id());
    if (!m) return false;

    uint32_t age_ticks = xTaskGetTickCount() - m->rx_time;
    if (age_ticks * portTICK_PERIOD_MS > HEAD2_FRESH_MS) return false;

    if (temp_fet)   *temp_fet   = m->temp_fet;
    if (temp_motor) *temp_motor = m->temp_motor;
    return true;
}

bool vesc_head2_is_fresh(void)
{
    return vesc_head2_get_temps(NULL, NULL);
}
