/*
    Copyright 2025 Super VESC Display
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).
*/

#include "vesc_can/vesc_rt_data.h"

#include "vesc_can/buffer.h"
#include "vesc_can/comm_can.h"
#include "vesc_can/vesc_lisp_poll.h"
#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "vesc_rt";

/* SETUP_VALUES selective request mask bits. */
#define MASK_TEMP_MOS           (1U << 0)
#define MASK_TEMP_MOTOR         (1U << 1)
#define MASK_CURRENT_MOTOR      (1U << 2)
#define MASK_CURRENT_IN         (1U << 3)
#define MASK_DUTY_NOW           (1U << 4)
#define MASK_RPM                (1U << 5)
#define MASK_SPEED              (1U << 6)
#define MASK_V_IN               (1U << 7)
#define MASK_BATTERY_LEVEL      (1U << 8)
#define MASK_AMP_HOURS          (1U << 9)
#define MASK_AMP_HOURS_CHARGED  (1U << 10)
#define MASK_WATT_HOURS         (1U << 11)
#define MASK_WATT_HOURS_CHARGED (1U << 12)
#define MASK_TACHOMETER         (1U << 13)
#define MASK_TACHOMETER_ABS     (1U << 14)
#define MASK_POSITION           (1U << 15)
#define MASK_FAULT_CODE         (1U << 16)
#define MASK_VESC_ID            (1U << 17)
#define MASK_NUM_VESCS          (1U << 18)
#define MASK_BATTERY_WH         (1U << 19)
#define MASK_ODOMETER           (1U << 20)
#define MASK_UPTIME_MS          (1U << 21)

static vesc_setup_values_t s_rt_data;
static bool                s_data_received      = false;
static bool                s_active             = false;
static uint8_t             s_target_vesc_id     = 10;
static uint32_t            s_request_interval_ms = 100;
static uint32_t            s_last_request_ms    = 0;
static TaskHandle_t        s_task_handle        = NULL;

static inline uint32_t millis_now(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void vesc_rt_data_init(uint8_t target_vesc_id, uint32_t poll_interval_ms)
{
    memset(&s_rt_data, 0, sizeof(s_rt_data));
    s_data_received       = false;
    s_active              = false;
    s_target_vesc_id      = target_vesc_id;
    s_request_interval_ms = poll_interval_ms ? poll_interval_ms : 100;
    s_last_request_ms     = 0;
    ESP_LOGI(TAG, "init target=%u interval=%u ms", target_vesc_id,
             (unsigned)s_request_interval_ms);
}

void vesc_rt_data_start(void)
{
    s_active           = true;
    s_last_request_ms  = 0;
    ESP_LOGI(TAG, "polling started");
}

void vesc_rt_data_stop(void)
{
    s_active = false;
    ESP_LOGI(TAG, "polling stopped");
}

void vesc_rt_data_request(void)
{
    uint32_t mask =
        MASK_AMP_HOURS |
        MASK_TEMP_MOS |
        MASK_TEMP_MOTOR |
        MASK_CURRENT_IN |
        MASK_BATTERY_LEVEL |
        MASK_SPEED |
        MASK_V_IN |
        MASK_WATT_HOURS |
        MASK_WATT_HOURS_CHARGED |
        MASK_TACHOMETER_ABS |
        MASK_BATTERY_WH |
        MASK_ODOMETER |
        MASK_UPTIME_MS;

    uint8_t  send_buffer[10];
    int32_t  ind = 0;
    send_buffer[ind++] = COMM_GET_VALUES_SETUP_SELECTIVE;
    buffer_append_uint32(send_buffer, mask, &ind);

    /* send=0: VESC will reply over CAN (PROCESS_*_BUFFER) */
    comm_can_send_buffer(s_target_vesc_id, send_buffer, ind, 0);
}

void vesc_rt_data_process_response(const uint8_t *data, unsigned int len)
{
    if (len < 1) return;

    uint8_t cmd = data[0];
    if (cmd != COMM_GET_VALUES_SETUP && cmd != COMM_GET_VALUES_SETUP_SELECTIVE) {
        return;
    }

    int32_t  ind  = 1;
    uint32_t mask = 0xFFFFFFFFu;
    if (cmd == COMM_GET_VALUES_SETUP_SELECTIVE) {
        if (len < 5) {
            ESP_LOGW(TAG, "selective response too short (%u)", len);
            return;
        }
        mask = buffer_get_uint32(data, &ind);
    }

    if ((mask & MASK_TEMP_MOS) && ind + 2 <= (int)len)
        s_rt_data.temp_mos = buffer_get_float16(data, 1e1f, &ind);
    if ((mask & MASK_TEMP_MOTOR) && ind + 2 <= (int)len)
        s_rt_data.temp_motor = buffer_get_float16(data, 1e1f, &ind);
    if ((mask & MASK_CURRENT_MOTOR) && ind + 4 <= (int)len)
        s_rt_data.current_motor = buffer_get_float32(data, 1e2f, &ind);
    if ((mask & MASK_CURRENT_IN) && ind + 4 <= (int)len)
        s_rt_data.current_in = buffer_get_float32(data, 1e2f, &ind);
    if ((mask & MASK_DUTY_NOW) && ind + 2 <= (int)len)
        s_rt_data.duty_now = buffer_get_float16(data, 1e3f, &ind);
    if ((mask & MASK_RPM) && ind + 4 <= (int)len)
        s_rt_data.rpm = buffer_get_float32(data, 1e0f, &ind);
    if ((mask & MASK_SPEED) && ind + 4 <= (int)len)
        s_rt_data.speed = buffer_get_float32(data, 1e3f, &ind);
    if ((mask & MASK_V_IN) && ind + 2 <= (int)len)
        s_rt_data.v_in = buffer_get_float16(data, 1e1f, &ind);
    if ((mask & MASK_BATTERY_LEVEL) && ind + 2 <= (int)len)
        s_rt_data.battery_level = buffer_get_float16(data, 1e3f, &ind);
    if ((mask & MASK_AMP_HOURS) && ind + 4 <= (int)len)
        s_rt_data.amp_hours = buffer_get_float32(data, 1e4f, &ind);
    if ((mask & MASK_AMP_HOURS_CHARGED) && ind + 4 <= (int)len)
        s_rt_data.amp_hours_charged = buffer_get_float32(data, 1e4f, &ind);
    if ((mask & MASK_WATT_HOURS) && ind + 4 <= (int)len)
        s_rt_data.watt_hours = buffer_get_float32(data, 1e4f, &ind);
    if ((mask & MASK_WATT_HOURS_CHARGED) && ind + 4 <= (int)len)
        s_rt_data.watt_hours_charged = buffer_get_float32(data, 1e4f, &ind);
    if ((mask & MASK_TACHOMETER) && ind + 4 <= (int)len)
        s_rt_data.tachometer = buffer_get_float32(data, 1e3f, &ind);
    if ((mask & MASK_TACHOMETER_ABS) && ind + 4 <= (int)len)
        s_rt_data.tachometer_abs = buffer_get_float32(data, 1e3f, &ind);
    if ((mask & MASK_POSITION) && ind + 4 <= (int)len)
        s_rt_data.position = buffer_get_float32(data, 1e6f, &ind);
    if ((mask & MASK_FAULT_CODE) && ind + 1 <= (int)len)
        s_rt_data.fault_code = data[ind++];
    if ((mask & MASK_VESC_ID) && ind + 1 <= (int)len)
        s_rt_data.vesc_id = data[ind++];
    if ((mask & MASK_NUM_VESCS) && ind + 1 <= (int)len)
        s_rt_data.num_vescs = data[ind++];
    if ((mask & MASK_BATTERY_WH) && ind + 4 <= (int)len)
        s_rt_data.battery_wh = buffer_get_float32(data, 1e3f, &ind);
    if ((mask & MASK_ODOMETER) && ind + 4 <= (int)len)
        s_rt_data.odometer = buffer_get_uint32(data, &ind);
    if ((mask & MASK_UPTIME_MS) && ind + 4 <= (int)len)
        s_rt_data.uptime_ms = buffer_get_uint32(data, &ind);

    s_rt_data.rx_time = millis_now();
    s_data_received   = true;

    static uint32_t log_counter = 0;
    if (++log_counter >= 20) {
        log_counter = 0;
        float speed_kmh   = s_rt_data.speed * 3.6f;
        float odometer_km = s_rt_data.odometer / 1000.0f;
        float power_w     = s_rt_data.current_in * s_rt_data.v_in;
        ESP_LOGI(TAG, "v=%.1fV i=%.1fA P=%.0fW spd=%.1fkm/h odo=%.1fkm",
                 s_rt_data.v_in, s_rt_data.current_in, power_w,
                 speed_kmh, odometer_km);
    }
}

void vesc_rt_data_inject(const vesc_setup_values_t *src)
{
    if (!src) return;
    s_rt_data = *src;
    s_rt_data.rx_time = millis_now();
    s_data_received   = true;
}

const vesc_setup_values_t *vesc_rt_data_get_latest(void) { return &s_rt_data; }

bool vesc_rt_data_is_fresh(void)
{
    if (!s_data_received) return false;
    uint32_t age = millis_now() - s_rt_data.rx_time;
    return age < 5000;
}

float vesc_rt_data_get_speed_kmh(void)
{
    return s_rt_data.speed * 3.6f;
}

float vesc_rt_data_get_trip_km(void)
{
    return s_rt_data.tachometer_abs / 1000.0f;
}

float vesc_rt_data_get_odometer_km(void)
{
    return s_rt_data.odometer / 1000.0f;
}

float vesc_rt_data_get_efficiency_whkm(void)
{
    float wh_consumed = s_rt_data.watt_hours - s_rt_data.watt_hours_charged;
    float distance_km = s_rt_data.tachometer_abs / 1000.0f;
    if (distance_km < 0.01f) return 0.0f;
    return wh_consumed / distance_km;
}

float vesc_rt_data_get_amp_hours(void)    { return s_rt_data.amp_hours; }
uint32_t vesc_rt_data_get_uptime_ms(void) { return s_rt_data.uptime_ms; }

void vesc_rt_data_loop(void)
{
    if (!s_active) return;
    uint32_t now = millis_now();
    if (now - s_last_request_ms >= s_request_interval_ms) {
        vesc_rt_data_request();
        s_last_request_ms = now;
    }
}

/* Single CAN-polling task: drives both RT data (~100 ms cycle) and the
 * optional LISP stats poll (~100 ms cycle). Each _loop() function has its
 * own internal interval gate, so calling both at 50 Hz here is fine — the
 * actual `comm_can_send_buffer()` cadence is owned by the loops, not us.
 * Keeping them on one task halves FreeRTOS overhead and serialises CAN
 * writes onto a single context (no contention on the TWAI driver). */
static void rt_task(void *arg)
{
    (void)arg;
    for (;;) {
        vesc_rt_data_loop();
#if CONFIG_VESC_CAN_LISP_POLL_ENABLE
        vesc_lisp_poll_loop();
#endif
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t vesc_rt_data_start_task(void)
{
    if (s_task_handle) return ESP_OK;
    BaseType_t r = xTaskCreatePinnedToCore(rt_task, "vesc_rt", 3072, NULL, 5, &s_task_handle, 0);
    return r == pdPASS ? ESP_OK : ESP_FAIL;
}
