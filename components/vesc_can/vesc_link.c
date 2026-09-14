/*
    Copyright 2026 ESP32-P4 Android Auto / VESC Display (GPL-3.0).

    See vesc_link.h. Thin dispatcher: the CAN branch calls comm_can directly
    (same component, no indirection worth paying for), the BLE branch goes
    through the ops table the backend in main/ registers at boot.
*/

#include "vesc_can/vesc_link.h"

#include "vesc_can/comm_can.h"

#include "esp_log.h"

#include <stdio.h>

static const char *TAG = "vesc_link";

/* CAN reply budget. Was a literal 60 at every poll site; a CAN round trip is
 * a few ms, so this is already generous. */
#define CAN_SYNC_TIMEOUT_MS 60

static vesc_link_mode_t           s_mode = VESC_LINK_MODE_CAN;
static const vesc_link_ops_t     *s_ble;
static vesc_link_packet_handler_t s_handler;

void vesc_link_register_ble(const vesc_link_ops_t *ops)
{
    s_ble = ops;
}

void vesc_link_set_mode(vesc_link_mode_t mode)
{
    if (mode == VESC_LINK_MODE_BLE && !s_ble) {
        ESP_LOGE(TAG, "BLE mode requested but no backend registered — staying on CAN");
        return;
    }
    if (s_mode == mode) return;
    s_mode = mode;
    ESP_LOGW(TAG, "VESC link is now %s", mode == VESC_LINK_MODE_BLE ? "BLE" : "CAN");
}

vesc_link_mode_t vesc_link_get_mode(void)
{
    return s_mode;
}

bool vesc_link_is_up(void)
{
    if (s_mode == VESC_LINK_MODE_BLE) {
        return s_ble && s_ble->is_up && s_ble->is_up();
    }
    return comm_can_is_running();
}

void vesc_link_send(uint8_t target, const uint8_t *data, unsigned int len,
                    uint8_t send_mode)
{
    if (s_mode == VESC_LINK_MODE_BLE) {
        if (s_ble && s_ble->send) s_ble->send(target, data, len, send_mode);
        return;
    }
    comm_can_send_buffer(target, data, len, send_mode);
}

void vesc_link_send_sync(uint8_t target, const uint8_t *data, unsigned int len,
                         uint8_t send_mode, uint32_t timeout_ms)
{
    if (s_mode == VESC_LINK_MODE_BLE) {
        if (s_ble && s_ble->send_sync) {
            s_ble->send_sync(target, data, len, send_mode, timeout_ms);
        }
        return;
    }
    comm_can_send_buffer_sync(target, data, len, send_mode, timeout_ms);
}

uint8_t vesc_link_reply_id(void)
{
    if (s_mode == VESC_LINK_MODE_BLE) return VESC_LINK_REPLY_ID_BLE;
    return comm_can_get_local_id();
}

uint32_t vesc_link_sync_timeout_ms(void)
{
    if (s_mode == VESC_LINK_MODE_BLE) return CONFIG_VESC_BLE_SYNC_TIMEOUT_MS;
    return CAN_SYNC_TIMEOUT_MS;
}

uint32_t vesc_link_scale_ms(uint32_t base_ms)
{
    if (s_mode != VESC_LINK_MODE_BLE) return base_ms;
    return (base_ms * (uint32_t)CONFIG_VESC_BLE_INTERVAL_PCT) / 100u;
}

void vesc_link_set_packet_handler(vesc_link_packet_handler_t handler)
{
    s_handler = handler;
    /* The CAN decode task keeps its own hook; route it through here so both
     * transports land on the same fan-out. */
    comm_can_set_packet_handler(vesc_link_deliver);
}

void vesc_link_deliver(const uint8_t *data, unsigned int len)
{
    if (s_handler) s_handler(data, len);
}

bool vesc_link_health_text(char *buf, size_t n)
{
    if (!buf || n == 0) return false;

    if (s_mode == VESC_LINK_MODE_BLE) {
        if (s_ble && s_ble->health_text) return s_ble->health_text(buf, n);
        snprintf(buf, n, "BLE: not ready");
        return true;
    }

    uint32_t err = 0, rec = 0;
    if (!comm_can_get_bus_health(&err, &rec)) {
        snprintf(buf, n, "CAN errors: (bus down)");
        return true;
    }
    if (rec) {
        snprintf(buf, n, "CAN errors: %lu\nBus-off recoveries: %lu",
                 (unsigned long)err, (unsigned long)rec);
    } else {
        snprintf(buf, n, "CAN errors: %lu", (unsigned long)err);
    }
    return true;
}
