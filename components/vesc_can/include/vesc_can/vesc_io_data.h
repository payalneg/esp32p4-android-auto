/*
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).

    On-demand VESC IO poller for the realtime viewer screen: sends
    COMM_GET_DECODED_ADC and COMM_GET_DECODED_PPM to a target VESC over
    CAN while active, parsing the replies into a vesc_io_data_t snapshot.

    Polling is gated by vesc_io_data_set_active() so the extra CAN traffic
    only happens while the realtime screen is open — it is OFF by default.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float    adc1;          /* ADC1 decoded level (mapped, ~0..1)   */
    float    adc1_voltage;  /* ADC1 raw input voltage (V)           */
    float    adc2;          /* ADC2 decoded level                   */
    float    adc2_voltage;  /* ADC2 raw input voltage (V)           */
    float    ppm;           /* PPM/servo decoded level (-1..1)      */
    float    ppm_ms;        /* PPM pulse length (ms)                */
    uint32_t adc_rx_time;   /* esp_timer ms of last ADC reply       */
    uint32_t ppm_rx_time;   /* esp_timer ms of last PPM reply       */
} vesc_io_data_t;

/* Initialise internal state. Safe to call multiple times. */
void vesc_io_data_init(uint8_t target_vesc_id, uint32_t poll_interval_ms);

/* Enable/disable polling. OFF by default; the realtime screen turns it
 * on while visible and off when it closes. */
void vesc_io_data_set_active(bool on);
bool vesc_io_data_is_active(void);

/* Pump the poller. Call periodically from a task you already have
 * (see rt_task in vesc_rt_data.c). No-op while inactive or before the
 * interval elapses. */
void vesc_io_data_loop(void);

/* Hook from the comm_can packet handler. Filters by command byte, so
 * passing every packet is fine. */
void vesc_io_data_process_response(const uint8_t *data, unsigned int len);

/* Latest snapshot. Always non-NULL. */
const vesc_io_data_t *vesc_io_data_get_latest(void);

/* True if both ADC and PPM replies arrived within the last few seconds. */
bool vesc_io_data_is_fresh(void);

#ifdef __cplusplus
}
#endif
