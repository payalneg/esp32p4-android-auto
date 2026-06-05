/*
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).
*/

#include "vesc_can/vesc_io_data.h"

#include "vesc_can/buffer.h"
#include "vesc_can/comm_can.h"
#include "vesc_can/vesc_datatypes.h"

#include "esp_log.h"
#include "esp_timer.h"

#include <string.h>

static const char *TAG = "vesc_io";

static vesc_io_data_t s_io;
static bool           s_active           = false;
static uint8_t        s_target_vesc_id   = 10;
static uint32_t       s_poll_interval_ms = 150;
static uint32_t       s_last_poll_ms     = 0;

static inline uint32_t millis_now(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void vesc_io_data_init(uint8_t target_vesc_id, uint32_t poll_interval_ms)
{
    memset(&s_io, 0, sizeof(s_io));
    s_active           = false;
    s_target_vesc_id   = target_vesc_id;
    s_poll_interval_ms = poll_interval_ms ? poll_interval_ms : 150;
    s_last_poll_ms     = 0;
    ESP_LOGI(TAG, "init target=%u interval=%u ms", target_vesc_id,
             (unsigned)s_poll_interval_ms);
}

void vesc_io_data_set_active(bool on)
{
    s_active       = on;
    s_last_poll_ms = 0; /* poll immediately on enable */
}

bool vesc_io_data_is_active(void) { return s_active; }

static void send_cmd(uint8_t cmd)
{
    uint8_t send_buffer[1];
    send_buffer[0] = cmd;
    /* send=0: VESC replies over CAN (PROCESS_*_BUFFER) */
    comm_can_send_buffer(s_target_vesc_id, send_buffer, 1, 0);
}

void vesc_io_data_loop(void)
{
    if (!s_active) return;
    uint32_t now = millis_now();
    if (now - s_last_poll_ms < s_poll_interval_ms) return;
    s_last_poll_ms = now;

    send_cmd(COMM_GET_DECODED_ADC);
    send_cmd(COMM_GET_DECODED_PPM);
}

void vesc_io_data_process_response(const uint8_t *data, unsigned int len)
{
    if (len < 1) return;

    int32_t ind = 1;
    switch (data[0]) {
    case COMM_GET_DECODED_ADC:
        /* [adc1_decoded][adc1_voltage][adc2_decoded][adc2_voltage], int32/1e6 */
        if (ind + 16 > (int)len) return;
        s_io.adc1         = buffer_get_float32(data, 1e6f, &ind);
        s_io.adc1_voltage = buffer_get_float32(data, 1e6f, &ind);
        s_io.adc2         = buffer_get_float32(data, 1e6f, &ind);
        s_io.adc2_voltage = buffer_get_float32(data, 1e6f, &ind);
        s_io.adc_rx_time  = millis_now();
        break;
    case COMM_GET_DECODED_PPM:
        /* [ppm_decoded][pulse_len_ms], int32/1e6 */
        if (ind + 8 > (int)len) return;
        s_io.ppm         = buffer_get_float32(data, 1e6f, &ind);
        s_io.ppm_ms      = buffer_get_float32(data, 1e6f, &ind);
        s_io.ppm_rx_time = millis_now();
        break;
    default:
        break;
    }
}

const vesc_io_data_t *vesc_io_data_get_latest(void) { return &s_io; }

bool vesc_io_data_is_fresh(void)
{
    uint32_t now = millis_now();
    if (s_io.adc_rx_time == 0 || s_io.ppm_rx_time == 0) return false;
    return (now - s_io.adc_rx_time < 3000) && (now - s_io.ppm_rx_time < 3000);
}
