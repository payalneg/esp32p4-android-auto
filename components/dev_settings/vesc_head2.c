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

/* On a BLE link these temperatures are polled instead of arriving as 50 Hz
 * broadcasts, and the dashboard gates ALL of its data on the second head
 * being fresh — so the definition of "fresh" here decides whether the screen
 * stays up.
 *
 * A wall-clock window does not work, because the poll period is not a
 * constant: the poll runs on the single VESC task, whose cycle stretches by up
 * to the reply timeout (CONFIG_VESC_BLE_SYNC_TIMEOUT_MS, 300 ms) for every
 * request in it that goes unanswered. A controller without the LISP script
 * never answers the dashboard's CUSTOM_APP_DATA poll (300 ms, every 400 ms);
 * the quick-action drawer adds another; the addressing probe blocks the task
 * for up to 1.2 s a pass while unresolved. Under that load a 1000 ms gate
 * comes round every 1.4–2.5 s, so a window of "three poll periods" (3 s)
 * tolerated one lost reply at best and none at all on a scriptless
 * controller with the drawer open: the flag flipped, and the whole dashboard
 * went blank with it.
 *
 * So freshness is counted in polls, not milliseconds: the head is stale once
 * HEAD2_MAX_MISSES consecutive polls have gone out without an answer in
 * between, however long the cycle took — 3.9 s at the nominal 1.3 s spacing,
 * ~7.5 s at the worst stretched one. A wall-clock ceiling remains only as a
 * backstop for polls not being sent at all (link down, task stalled); it is
 * set above anything the miss rule can take so it never pre-empts it, and in
 * those cases the primary poll's own 5 s window has already pulled the
 * "ESC NOT CONNECTED" banner anyway. */
#define HEAD2_MAX_MISSES       3u
#define HEAD2_FRESH_BLE_MAX_MS 10000u

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
/* Polls sent since the last answer, minus the one that may still be in
 * flight: bumped when a poll goes out and the previous one is still
 * unanswered, cleared by every reply. */
static volatile uint8_t s_misses;
static volatile bool    s_pending;   /* a poll is out, no reply yet */

void vesc_head2_poll_loop(void)
{
    if (vesc_link_get_mode() != VESC_LINK_MODE_BLE) return;
    if (!settings_get_second_head_enabled()) return;

    int64_t now = esp_timer_get_time();
    if (s_last_poll_us && (now - s_last_poll_us) < (int64_t)HEAD2_POLL_MS * 1000) {
        return;
    }
    s_last_poll_us = now;

    if (s_pending && s_misses < 255) s_misses++;
    s_pending = true;

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
    s_misses       = 0;
    s_pending      = false;
}

bool vesc_head2_get_temps(float *temp_fet, float *temp_motor)
{
    if (!settings_get_second_head_enabled()) return false;

    if (vesc_link_get_mode() == VESC_LINK_MODE_BLE) {
        if (s_polled_us == 0) return false;
        if (s_misses >= HEAD2_MAX_MISSES) return false;
        if ((esp_timer_get_time() - s_polled_us) / 1000 > HEAD2_FRESH_BLE_MAX_MS) return false;
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
