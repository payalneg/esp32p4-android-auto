/*
    Copyright 2022 Benjamin Vedder benjamin@vedder.se
    Adapted from VESC firmware (GPL-3.0). Ported to ESP-IDF C.
*/

#include "vesc_can/comm_can.h"

#include "vesc_can/buffer.h"
#include "vesc_can/crc.h"

#include "driver/twai.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "vesc_can";

#define RX_BUFFER_NUM   3
/* Match PACKET_PARSER_MAX_PAYLOAD — VESC Tool's SET_MCCONF over BLE
 * arrives as PROCESS_RX_BUFFER fragments and we reassemble here before
 * handing to the packet handler. 512 was the historical FW value; recent
 * VESC FW emits configs up to ~800 B. */
#define RX_BUFFER_SIZE  1024
#define RXBUF_LEN       50

static can_status_msg   stat_msgs  [CAN_STATUS_MSGS_TO_STORE];
static can_status_msg_2 stat_msgs_2[CAN_STATUS_MSGS_TO_STORE];
static can_status_msg_3 stat_msgs_3[CAN_STATUS_MSGS_TO_STORE];
static can_status_msg_4 stat_msgs_4[CAN_STATUS_MSGS_TO_STORE];
static can_status_msg_5 stat_msgs_5[CAN_STATUS_MSGS_TO_STORE];
static can_status_msg_6 stat_msgs_6[CAN_STATUS_MSGS_TO_STORE];

static twai_timing_config_t        s_t_config = TWAI_TIMING_CONFIG_500KBITS();
static const twai_filter_config_t  s_f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static twai_general_config_t       s_g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_NC, GPIO_NUM_NC, TWAI_MODE_NORMAL);

static volatile bool s_init_done     = false;
static volatile bool s_sem_init_done = false;
static volatile bool s_stop_rx       = false;
static volatile bool s_rx_running    = false;

static SemaphoreHandle_t s_ping_sem;
static SemaphoreHandle_t s_send_mutex;
/* Completion signal for reply-expecting polls. The VESC streams its reply back
 * as a FILL_RX_BUFFER…PROCESS_RX_BUFFER sequence that we reassemble into a
 * per-sender slot keyed only by CAN id; two such replies in flight from the
 * same node clobber each other's slot → CRC mismatch and a dropped packet. We
 * avoid that not with a lock but by keeping ALL continuous polls on a single
 * task (vesc_rt_data's rt_task): it sends one request, waits on this semaphore
 * until the reply is reassembled and delivered, then fires the next. So this is
 * a "reply arrived" signal, not a mutex. (See comm_can_send_buffer_sync.) */
static SemaphoreHandle_t s_rx_done_sem;
static volatile HW_TYPE  s_ping_hw_last = HW_TYPE_VESC;

static uint8_t          s_rx_buffer[RX_BUFFER_NUM][RX_BUFFER_SIZE];
static int              s_rx_buffer_device_id[RX_BUFFER_NUM];
static volatile uint8_t s_rx_buffer_last_id;
static volatile uint8_t s_rx_buffer_response_type = 1;

static twai_message_t   s_rx_buf[RXBUF_LEN];
static volatile int     s_rx_write = 0;
static volatile int     s_rx_read  = 0;

typedef struct {
    uint8_t controller_id;
    int     can_baud_rate_kbps;
} can_config_t;

static can_config_t s_can_config;

static can_packet_handler_t s_packet_handler = NULL;

static void decode_msg(uint32_t eid, uint8_t *data8, int len);
static void process_task(void *arg);
static void rx_task(void *arg);

static bool set_timing_for_speed(int can_speed_kbps)
{
    switch (can_speed_kbps) {
    case 125:
        s_t_config = (twai_timing_config_t)TWAI_TIMING_CONFIG_125KBITS();
        return true;
    case 250:
        s_t_config = (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS();
        return true;
    case 500:
        s_t_config = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
        return true;
    case 1000:
        s_t_config = (twai_timing_config_t)TWAI_TIMING_CONFIG_1MBITS();
        return true;
    default:
        ESP_LOGW(TAG, "invalid CAN speed %d kbps, defaulting to 500", can_speed_kbps);
        s_t_config = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
        s_can_config.can_baud_rate_kbps = 500;
        return false;
    }
}

esp_err_t comm_can_start(int pin_tx, int pin_rx,
                         uint8_t controller_id, int can_speed_kbps)
{
    for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
        stat_msgs  [i].id = -1;
        stat_msgs_2[i].id = -1;
        stat_msgs_3[i].id = -1;
        stat_msgs_4[i].id = -1;
        stat_msgs_5[i].id = -1;
        stat_msgs_6[i].id = -1;
    }
    for (int i = 0; i < RX_BUFFER_NUM; i++) {
        s_rx_buffer_device_id[i] = -1;
    }

    s_can_config.controller_id      = controller_id;
    s_can_config.can_baud_rate_kbps = can_speed_kbps;
    set_timing_for_speed(can_speed_kbps);

    if (!s_sem_init_done) {
        s_ping_sem    = xSemaphoreCreateBinary();
        s_send_mutex  = xSemaphoreCreateMutex();
        s_rx_done_sem = xSemaphoreCreateBinary();
        /* 4096: the dispatch chain fans out to every *_process_response handler
         * (RT data, LISP stats, IO, config, LISP code, quick-action panel); a
         * couple of them parse multi-hundred-byte frames. 3072 was marginal. */
        xTaskCreatePinnedToCore(process_task, "can_proc", 4096, NULL, 8, NULL, 0);
        s_sem_init_done = true;
    }

    /* A 1 KB SET_MCCONF fragments into ~150 CAN frames (FILL_RX_BUFFER
     * @7B + FILL_RX_BUFFER_LONG @6B + PROCESS_RX_BUFFER). At 500 kbit/s
     * one frame is ~280 µs → the burst flushes in ~42 ms, but bursts
     * arrive faster than that from VESC Tool over BLE. tx_queue_len 20
     * overflowed and twai_transmit silently timed out mid-fragment →
     * VESC saw a hole in the reassembly buffer and dropped the whole
     * write. 200 holds a full burst with headroom. */
    s_g_config.tx_queue_len = 200;
    s_g_config.rx_queue_len = 100;
    s_g_config.tx_io        = (gpio_num_t)pin_tx;
    s_g_config.rx_io        = (gpio_num_t)pin_rx;

    esp_err_t err = twai_driver_install(&s_g_config, &s_t_config, &s_f_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }
    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_start failed: %s", esp_err_to_name(err));
        twai_driver_uninstall();
        return err;
    }

    s_stop_rx = false;
    xTaskCreatePinnedToCore(rx_task, "can_rx", 3072, NULL, 8, NULL, 0);

    s_init_done = true;
    ESP_LOGI(TAG, "TWAI up: TX=%d RX=%d ID=%u %d kbps",
             pin_tx, pin_rx, controller_id, s_can_config.can_baud_rate_kbps);
    return ESP_OK;
}

void comm_can_stop(void)
{
    if (!s_init_done) return;

    xSemaphoreTake(s_send_mutex, portMAX_DELAY);
    s_init_done = false;
    xSemaphoreGive(s_send_mutex);

    s_stop_rx = true;
    while (s_rx_running) {
        vTaskDelay(1);
    }

    twai_stop();
    twai_driver_uninstall();
}

esp_err_t comm_can_reinit(uint8_t controller_id, int can_speed_kbps)
{
    ESP_LOGI(TAG, "reinit ID=%u speed=%d kbps", controller_id, can_speed_kbps);
    int prev_tx = s_g_config.tx_io;
    int prev_rx = s_g_config.rx_io;

    comm_can_stop();
    vTaskDelay(pdMS_TO_TICKS(100));

    s_can_config.controller_id      = controller_id;
    s_can_config.can_baud_rate_kbps = can_speed_kbps;
    set_timing_for_speed(can_speed_kbps);

    esp_err_t err = twai_driver_install(&s_g_config, &s_t_config, &s_f_config);
    if (err != ESP_OK) return err;
    err = twai_start();
    if (err != ESP_OK) {
        twai_driver_uninstall();
        return err;
    }

    s_stop_rx = false;
    xTaskCreatePinnedToCore(rx_task, "can_rx", 3072, NULL, 8, NULL, 0);
    s_init_done = true;
    ESP_LOGI(TAG, "reinit ok TX=%d RX=%d ID=%u %d kbps",
             prev_tx, prev_rx, controller_id, s_can_config.can_baud_rate_kbps);
    return ESP_OK;
}

void comm_can_transmit_eid(uint32_t id, const uint8_t *data, uint8_t len)
{
    if (!s_init_done) return;
    if (len > 8) len = 8;

    twai_message_t tx_msg = {0};
    tx_msg.extd            = 1;
    tx_msg.identifier      = id;
    tx_msg.data_length_code = len;
    memcpy(tx_msg.data, data, len);

    xSemaphoreTake(s_send_mutex, portMAX_DELAY);
    if (!s_init_done) {
        xSemaphoreGive(s_send_mutex);
        return;
    }
    twai_transmit(&tx_msg, pdMS_TO_TICKS(5));
    xSemaphoreGive(s_send_mutex);
}

void comm_can_send_buffer(uint8_t controller_id, const uint8_t *data,
                          unsigned int len, uint8_t send)
{
    uint8_t send_buffer[8];

    if (len <= 6) {
        uint32_t ind = 0;
        send_buffer[ind++] = s_can_config.controller_id;
        send_buffer[ind++] = send;
        memcpy(send_buffer + ind, data, len);
        ind += len;
        comm_can_transmit_eid(controller_id |
                              ((uint32_t)CAN_PACKET_PROCESS_SHORT_BUFFER << 8),
                              send_buffer, ind);
        return;
    }

    unsigned int end_a = 0;
    for (unsigned int i = 0; i < len; i += 7) {
        if (i > 255) break;
        end_a = i + 7;

        uint8_t send_len = 7;
        send_buffer[0] = (uint8_t)i;

        if ((i + 7) <= len) {
            memcpy(send_buffer + 1, data + i, send_len);
        } else {
            send_len = (uint8_t)(len - i);
            memcpy(send_buffer + 1, data + i, send_len);
        }
        comm_can_transmit_eid(controller_id |
                              ((uint32_t)CAN_PACKET_FILL_RX_BUFFER << 8),
                              send_buffer, send_len + 1);
    }

    for (unsigned int i = end_a; i < len; i += 6) {
        uint8_t send_len = 6;
        send_buffer[0] = (uint8_t)(i >> 8);
        send_buffer[1] = (uint8_t)(i & 0xFF);

        if ((i + 6) <= len) {
            memcpy(send_buffer + 2, data + i, send_len);
        } else {
            send_len = (uint8_t)(len - i);
            memcpy(send_buffer + 2, data + i, send_len);
        }
        comm_can_transmit_eid(controller_id |
                              ((uint32_t)CAN_PACKET_FILL_RX_BUFFER_LONG << 8),
                              send_buffer, send_len + 2);
    }

    uint32_t ind = 0;
    send_buffer[ind++] = s_can_config.controller_id;
    send_buffer[ind++] = send;
    send_buffer[ind++] = (uint8_t)(len >> 8);
    send_buffer[ind++] = (uint8_t)(len & 0xFF);
    unsigned short crc = crc16(data, len);
    send_buffer[ind++] = (uint8_t)(crc >> 8);
    send_buffer[ind++] = (uint8_t)(crc & 0xFF);

    comm_can_transmit_eid(controller_id |
                          ((uint32_t)CAN_PACKET_PROCESS_RX_BUFFER << 8),
                          send_buffer, ind);
}

void comm_can_send_buffer_sync(uint8_t controller_id, const uint8_t *data,
                               unsigned int len, uint8_t send,
                               uint32_t reply_timeout_ms)
{
    if (!s_init_done || !s_rx_done_sem) {
        comm_can_send_buffer(controller_id, data, len, send);
        return;
    }
    /* Call ONLY from the single CAN poll task (vesc_rt_data's rt_task) — that
     * single-threadedness is what serialises requests; there is no lock here.
     * Drain any stale completion, send, then wait (bounded) for this request's
     * reply to be reassembled and delivered before returning, so the caller
     * won't fire the next poll while this reply is still streaming in. */
    xSemaphoreTake(s_rx_done_sem, 0);
    comm_can_send_buffer(controller_id, data, len, send);
    xSemaphoreTake(s_rx_done_sem, pdMS_TO_TICKS(reply_timeout_ms));
}

uint8_t comm_can_get_local_id(void)
{
    return s_can_config.controller_id;
}

bool comm_can_ping(uint8_t controller_id, HW_TYPE *hw_type)
{
    if (!s_init_done) return false;

    uint8_t buffer[1];
    buffer[0] = s_can_config.controller_id;
    comm_can_transmit_eid(controller_id |
                          ((uint32_t)CAN_PACKET_PING << 8), buffer, 1);

    bool ret = xSemaphoreTake(s_ping_sem, pdMS_TO_TICKS(10)) == pdTRUE;
    if (ret && hw_type) *hw_type = s_ping_hw_last;
    return ret;
}

void comm_can_set_duty(uint8_t controller_id, float duty)
{
    int32_t ind = 0;
    uint8_t  buffer[4];
    buffer_append_int32(buffer, (int32_t)(duty * 100000.0f), &ind);
    comm_can_transmit_eid(controller_id |
                          ((uint32_t)CAN_PACKET_SET_DUTY << 8), buffer, ind);
}

void comm_can_set_current(uint8_t controller_id, float current)
{
    int32_t ind = 0;
    uint8_t  buffer[4];
    buffer_append_int32(buffer, (int32_t)(current * 1000.0f), &ind);
    comm_can_transmit_eid(controller_id |
                          ((uint32_t)CAN_PACKET_SET_CURRENT << 8), buffer, ind);
}

void comm_can_set_current_brake(uint8_t controller_id, float current)
{
    int32_t ind = 0;
    uint8_t  buffer[4];
    buffer_append_int32(buffer, (int32_t)(current * 1000.0f), &ind);
    comm_can_transmit_eid(controller_id |
                          ((uint32_t)CAN_PACKET_SET_CURRENT_BRAKE << 8), buffer, ind);
}

void comm_can_set_rpm(uint8_t controller_id, float rpm)
{
    int32_t ind = 0;
    uint8_t  buffer[4];
    buffer_append_int32(buffer, (int32_t)rpm, &ind);
    comm_can_transmit_eid(controller_id |
                          ((uint32_t)CAN_PACKET_SET_RPM << 8), buffer, ind);
}

can_status_msg *comm_can_get_status_msg_id(int id)
{
    for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
        if (stat_msgs[i].id == id) return &stat_msgs[i];
    }
    return NULL;
}
can_status_msg_2 *comm_can_get_status_msg_2_id(int id)
{
    for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
        if (stat_msgs_2[i].id == id) return &stat_msgs_2[i];
    }
    return NULL;
}
can_status_msg_3 *comm_can_get_status_msg_3_id(int id)
{
    for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
        if (stat_msgs_3[i].id == id) return &stat_msgs_3[i];
    }
    return NULL;
}
can_status_msg_4 *comm_can_get_status_msg_4_id(int id)
{
    for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
        if (stat_msgs_4[i].id == id) return &stat_msgs_4[i];
    }
    return NULL;
}
can_status_msg_5 *comm_can_get_status_msg_5_id(int id)
{
    for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
        if (stat_msgs_5[i].id == id) return &stat_msgs_5[i];
    }
    return NULL;
}
can_status_msg_6 *comm_can_get_status_msg_6_id(int id)
{
    for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
        if (stat_msgs_6[i].id == id) return &stat_msgs_6[i];
    }
    return NULL;
}

void comm_can_set_packet_handler(can_packet_handler_t handler)
{
    s_packet_handler = handler;
}

static void decode_msg(uint32_t eid, uint8_t *data8, int len)
{
    int32_t ind = 0;
    uint8_t crc_low, crc_high, commands_send;

    uint8_t       id  = eid & 0xFF;
    CAN_PACKET_ID cmd = (CAN_PACKET_ID)(eid >> 8);

    /* Messages addressed to us (or broadcast). */
    if (id == 255 || id == s_can_config.controller_id) {
        switch (cmd) {
        case CAN_PACKET_FILL_RX_BUFFER: {
            int buf_ind = -1;
            int offset  = data8[0];
            data8++; len--;

            for (int i = 0; i < RX_BUFFER_NUM; i++) {
                if (s_rx_buffer_device_id[i] == id) { buf_ind = i; break; }
            }
            if (buf_ind < 0 && offset == 0) {
                for (int i = 0; i < RX_BUFFER_NUM; i++) {
                    if (s_rx_buffer_device_id[i] == -1) {
                        buf_ind = i;
                        s_rx_buffer_device_id[i] = id;
                        break;
                    }
                }
            }
            if (buf_ind < 0) break;
            if (offset + len <= RX_BUFFER_SIZE) {
                memcpy(s_rx_buffer[buf_ind] + offset, data8, len);
            }
        } break;

        case CAN_PACKET_FILL_RX_BUFFER_LONG: {
            int buf_ind = -1;
            int offset  = (int)data8[0] << 8;
            offset |= data8[1];
            data8 += 2; len -= 2;

            for (int i = 0; i < RX_BUFFER_NUM; i++) {
                if (s_rx_buffer_device_id[i] == id) { buf_ind = i; break; }
            }
            if (buf_ind < 0 && offset == 0) {
                for (int i = 0; i < RX_BUFFER_NUM; i++) {
                    if (s_rx_buffer_device_id[i] == -1) {
                        buf_ind = i;
                        s_rx_buffer_device_id[i] = id;
                        break;
                    }
                }
            }
            if (buf_ind < 0) break;
            if ((offset + len) <= RX_BUFFER_SIZE) {
                memcpy(s_rx_buffer[buf_ind] + offset, data8, len);
            }
        } break;

        case CAN_PACKET_PROCESS_RX_BUFFER: {
            ind = 0;
            uint8_t last_id = data8[ind++];
            commands_send   = data8[ind++];

            if (commands_send == 0 || commands_send == 3) {
                s_rx_buffer_last_id = last_id;
            }
            s_rx_buffer_response_type = (commands_send == 3) ? 0 : 1;

            int rxbuf_len = (int)data8[ind++] << 8;
            rxbuf_len    |= (int)data8[ind++];
            if (rxbuf_len > RX_BUFFER_SIZE) break;

            int buf_ind = -1;
            for (int i = 0; i < RX_BUFFER_NUM; i++) {
                if (s_rx_buffer_device_id[i] == id) { buf_ind = i; break; }
            }
            if (buf_ind < 0) break;

            crc_high = data8[ind++];
            crc_low  = data8[ind++];

            if (crc16(s_rx_buffer[buf_ind], rxbuf_len) ==
                ((unsigned short)crc_high << 8 | (unsigned short)crc_low)) {
                if (s_packet_handler) {
                    s_packet_handler(s_rx_buffer[buf_ind], rxbuf_len);
                }
            } else {
                ESP_LOGW(TAG, "PROCESS_RX_BUFFER CRC mismatch (id=%u len=%d)", id, rxbuf_len);
            }
            s_rx_buffer_device_id[buf_ind] = -1;
            /* Transaction finished (delivered or CRC-dropped) — wake any waiter
             * so the next reply-expecting poll can go out (comm_can_send_buffer_sync). */
            if (s_rx_done_sem) xSemaphoreGive(s_rx_done_sem);
        } break;

        case CAN_PACKET_PROCESS_SHORT_BUFFER: {
            ind = 0;
            uint8_t last_id = data8[ind++];
            commands_send   = data8[ind++];
            if (commands_send == 0 || commands_send == 3) {
                s_rx_buffer_last_id = last_id;
            }
            s_rx_buffer_response_type = (commands_send == 3) ? 0 : 1;

            if (s_packet_handler) {
                s_packet_handler(data8 + ind, len - ind);
            }
            if (s_rx_done_sem) xSemaphoreGive(s_rx_done_sem);
        } break;

        case CAN_PACKET_PING: {
            uint8_t buffer[2];
            buffer[0] = s_can_config.controller_id;
            buffer[1] = HW_TYPE_CUSTOM_MODULE;
            uint32_t pong_id = data8[0] | ((uint32_t)CAN_PACKET_PONG << 8);
            comm_can_transmit_eid(pong_id, buffer, 2);
        } break;

        case CAN_PACKET_PONG:
            xSemaphoreGive(s_ping_sem);
            s_ping_hw_last = (len >= 2) ? (HW_TYPE)data8[1] : HW_TYPE_VESC_BMS;
            break;

        default:
            break;
        }
    }

    /* Broadcast STATUS_* frames — record latest per controller ID. */
    switch (cmd) {
    case CAN_PACKET_STATUS:
        for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
            can_status_msg *t = &stat_msgs[i];
            if (t->id == id || t->id == -1) {
                ind = 0;
                t->id      = id;
                t->rx_time = xTaskGetTickCount();
                t->rpm     = (float)buffer_get_int32(data8, &ind);
                t->current = (float)buffer_get_int16(data8, &ind) / 10.0f;
                t->duty    = (float)buffer_get_int16(data8, &ind) / 1000.0f;
                break;
            }
        }
        break;
    case CAN_PACKET_STATUS_2:
        for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
            can_status_msg_2 *t = &stat_msgs_2[i];
            if (t->id == id || t->id == -1) {
                ind = 0;
                t->id                = id;
                t->rx_time           = xTaskGetTickCount();
                t->amp_hours         = (float)buffer_get_int32(data8, &ind) / 1e4f;
                t->amp_hours_charged = (float)buffer_get_int32(data8, &ind) / 1e4f;
                break;
            }
        }
        break;
    case CAN_PACKET_STATUS_3:
        for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
            can_status_msg_3 *t = &stat_msgs_3[i];
            if (t->id == id || t->id == -1) {
                ind = 0;
                t->id                 = id;
                t->rx_time            = xTaskGetTickCount();
                t->watt_hours         = (float)buffer_get_int32(data8, &ind) / 1e4f;
                t->watt_hours_charged = (float)buffer_get_int32(data8, &ind) / 1e4f;
                break;
            }
        }
        break;
    case CAN_PACKET_STATUS_4:
        for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
            can_status_msg_4 *t = &stat_msgs_4[i];
            if (t->id == id || t->id == -1) {
                ind = 0;
                t->id          = id;
                t->rx_time     = xTaskGetTickCount();
                t->temp_fet    = (float)buffer_get_int16(data8, &ind) / 10.0f;
                t->temp_motor  = (float)buffer_get_int16(data8, &ind) / 10.0f;
                t->current_in  = (float)buffer_get_int16(data8, &ind) / 10.0f;
                t->pid_pos_now = (float)buffer_get_int16(data8, &ind) / 50.0f;
                break;
            }
        }
        break;
    case CAN_PACKET_STATUS_5:
        for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
            can_status_msg_5 *t = &stat_msgs_5[i];
            if (t->id == id || t->id == -1) {
                ind = 0;
                t->id         = id;
                t->rx_time    = xTaskGetTickCount();
                t->tacho_value = buffer_get_int32(data8, &ind);
                t->v_in       = (float)buffer_get_int16(data8, &ind) / 1e1f;
                break;
            }
        }
        break;
    case CAN_PACKET_STATUS_6:
        for (int i = 0; i < CAN_STATUS_MSGS_TO_STORE; i++) {
            can_status_msg_6 *t = &stat_msgs_6[i];
            if (t->id == id || t->id == -1) {
                ind = 0;
                t->id      = id;
                t->rx_time = xTaskGetTickCount();
                t->adc_1   = buffer_get_float16(data8, 1e3f, &ind);
                t->adc_2   = buffer_get_float16(data8, 1e3f, &ind);
                t->adc_3   = buffer_get_float16(data8, 1e3f, &ind);
                t->ppm     = buffer_get_float16(data8, 1e3f, &ind);
                break;
            }
        }
        break;
    default:
        break;
    }
}

static void rx_task(void *arg)
{
    (void)arg;
    s_rx_running = true;
    while (!s_stop_rx) {
        twai_message_t msg;
        if (twai_receive(&msg, pdMS_TO_TICKS(10)) == ESP_OK) {
            int next_write = s_rx_write + 1;
            if (next_write >= RXBUF_LEN) next_write = 0;
            if (next_write != s_rx_read) {
                s_rx_buf[s_rx_write] = msg;
                s_rx_write = next_write;
            }
        }
    }
    s_rx_running = false;
    vTaskDelete(NULL);
}

static void process_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
        while (s_rx_read != s_rx_write) {
            twai_message_t *msg = &s_rx_buf[s_rx_read];
            int next_read = s_rx_read + 1;
            if (next_read >= RXBUF_LEN) next_read = 0;
            s_rx_read = next_read;

            if (msg->extd) {
                decode_msg(msg->identifier, msg->data, msg->data_length_code);
            }
        }
    }
}
