/*
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).

    Transport for the LISP-driven quick-action panel. See vesc_lisp_panel.h
    for the wire protocol. Outgoing requests are sent on the LVGL task;
    incoming UI_DESC/STATE replies are parsed on the CAN process task and
    merged into a mutex-guarded model that the renderer snapshots.
*/

#include "vesc_can/vesc_lisp_panel.h"

#include "vesc_can/buffer.h"
#include "vesc_can/comm_can.h"
#include "vesc_can/vesc_datatypes.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include <string.h>

static const char *TAG = "vesc_lisp_panel";

static uint8_t            s_target_vesc_id = 10;
static SemaphoreHandle_t  s_lock;          /* guards s_model */
static vlp_model_t        s_model;         /* authoritative model */
static bool               s_have_ui;       /* set once a UI_DESC lands */

void vesc_lisp_panel_init(uint8_t target_vesc_id)
{
    s_target_vesc_id = target_vesc_id;
    if (!s_lock) {
        s_lock = xSemaphoreCreateMutex();
    }
    if (s_lock && xSemaphoreTake(s_lock, portMAX_DELAY) == pdTRUE) {
        memset(&s_model, 0, sizeof(s_model));
        s_have_ui = false;
        xSemaphoreGive(s_lock);
    }
}

void vesc_lisp_panel_set_target(uint8_t target_vesc_id)
{
    s_target_vesc_id = target_vesc_id;
}

/* ---- outgoing ---- */

static void send_request(uint8_t msg, uint8_t ctrl_id, float value,
                         bool has_action)
{
    uint8_t buf[16];
    int32_t ind = 0;
    buf[ind++] = COMM_CUSTOM_APP_DATA;
    buf[ind++] = VLP_MAGIC0;
    buf[ind++] = VLP_MAGIC1;
    buf[ind++] = msg;
    buf[ind++] = comm_can_get_local_id();   /* reply_can_id */
    if (has_action) {
        buf[ind++] = ctrl_id;
        buffer_append_float32(buf, value, VLP_SCALE, &ind);
    }
    comm_can_send_buffer(s_target_vesc_id, buf, (unsigned int)ind, 0);
}

void vesc_lisp_panel_request_ui(void)
{
    send_request(VLP_MSG_REQ_UI, 0, 0.0f, false);
}

void vesc_lisp_panel_request_state(void)
{
    send_request(VLP_MSG_REQ_STATE, 0, 0.0f, false);
}

void vesc_lisp_panel_send_action(uint8_t ctrl_id, float value)
{
    send_request(VLP_MSG_ACTION, ctrl_id, value, true);
}

/* ---- incoming ---- */

static void parse_ui_desc(const uint8_t *data, unsigned int len, int32_t ind)
{
    vlp_model_t m;
    memset(&m, 0, sizeof(m));

    if (ind + 2 > (int)len) return;
    m.version = data[ind++];
    uint8_t count = data[ind++];

    for (uint8_t i = 0; i < count && m.count < VLP_MAX_CTRLS; i++) {
        if (ind + 2 > (int)len) break;
        vlp_ctrl_t *c = &m.ctrl[m.count];
        c->id   = data[ind++];
        c->type = data[ind++];

        buffer_get_string(data, (int)len, c->label, sizeof(c->label), &ind);

        switch (c->type) {
        case VLP_CTRL_TOGGLE:
            if (ind + 1 > (int)len) return;
            c->value = data[ind++] ? 1.0f : 0.0f;
            break;
        case VLP_CTRL_BUTTON:
            break;
        case VLP_CTRL_NUMBER:
            if (ind + 16 > (int)len) return;
            c->vmin  = buffer_get_float32(data, VLP_SCALE, &ind);
            c->vmax  = buffer_get_float32(data, VLP_SCALE, &ind);
            c->vstep = buffer_get_float32(data, VLP_SCALE, &ind);
            c->value = buffer_get_float32(data, VLP_SCALE, &ind);
            buffer_get_string(data, (int)len, c->suffix, sizeof(c->suffix), &ind);
            break;
        case VLP_CTRL_LABEL:
            if (ind + 4 > (int)len) return;
            c->value = buffer_get_float32(data, VLP_SCALE, &ind);
            buffer_get_string(data, (int)len, c->suffix, sizeof(c->suffix), &ind);
            break;
        default:
            /* Unknown control type — we can't know its tail size, so the
             * rest of the frame is unparseable. Commit what we have. */
            ESP_LOGW(TAG, "unknown ctrl type %u — truncating", c->type);
            goto commit;
        }
        m.count++;
    }

commit:
    if (s_lock && xSemaphoreTake(s_lock, portMAX_DELAY) == pdTRUE) {
        uint32_t ui = s_model.ui_epoch, st = s_model.state_epoch;
        s_model = m;
        s_model.ui_epoch    = ui + 1;
        s_model.state_epoch = st + 1;
        s_have_ui = true;
        xSemaphoreGive(s_lock);
    }
}

static void parse_state(const uint8_t *data, unsigned int len, int32_t ind)
{
    if (ind + 1 > (int)len) return;
    uint8_t count = data[ind++];

    if (!s_lock || xSemaphoreTake(s_lock, portMAX_DELAY) != pdTRUE) return;
    bool changed = false;
    for (uint8_t i = 0; i < count; i++) {
        if (ind + 5 > (int)len) break;          /* id + i32 */
        uint8_t id = data[ind++];
        float v = buffer_get_float32(data, VLP_SCALE, &ind);
        for (uint8_t k = 0; k < s_model.count; k++) {
            if (s_model.ctrl[k].id == id) {
                s_model.ctrl[k].value = v;
                changed = true;
                break;
            }
        }
    }
    if (changed) s_model.state_epoch++;
    xSemaphoreGive(s_lock);
}

void vesc_lisp_panel_process_response(const uint8_t *data, unsigned int len)
{
    if (len < 4) return;
    if (data[0] != COMM_CUSTOM_APP_DATA) return;
    if (data[1] != VLP_MAGIC0 || data[2] != VLP_MAGIC1) return;

    uint8_t msg = data[3];
    int32_t ind = 4;
    if (msg == VLP_MSG_UI_DESC) {
        parse_ui_desc(data, len, ind);
    } else if (msg == VLP_MSG_STATE) {
        parse_state(data, len, ind);
    }
}

bool vesc_lisp_panel_get_model(vlp_model_t *out)
{
    if (!out || !s_lock) return false;
    bool ok = false;
    if (xSemaphoreTake(s_lock, portMAX_DELAY) == pdTRUE) {
        if (s_have_ui) {
            *out = s_model;
            ok = true;
        }
        xSemaphoreGive(s_lock);
    }
    return ok;
}
