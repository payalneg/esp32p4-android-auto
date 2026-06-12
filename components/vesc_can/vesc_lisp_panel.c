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
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_timer.h"

#include <string.h>

static const char *TAG = "vesc_lisp_panel";

#define VLP_POLL_INTERVAL_MS  200
#define VLP_ACTION_QUEUE_LEN  8

static uint8_t            s_target_vesc_id = 10;
static SemaphoreHandle_t  s_lock;          /* guards s_model */
static vlp_model_t        s_model;         /* authoritative model */
static bool               s_have_ui;       /* set once a UI_DESC lands */

/* CAN polling state — all touched only by the poll task (poll_loop), except
 * s_enabled (set from the LVGL task on open/close) and the action queue. */
static volatile bool      s_enabled;       /* drawer open → keep polling */
static uint32_t           s_last_poll_ms;
static QueueHandle_t      s_action_q;      /* {id,value} from LVGL tap handlers */

/* Dashboard stats — polled always (not gated by the drawer); guarded by s_lock. */
static vlp_dash_t         s_dash;
static uint32_t           s_dash_last_ms;
#define VLP_DASH_INTERVAL_MS  200

typedef struct { uint8_t id; float value; } vlp_action_t;

static inline uint32_t millis_now(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void vesc_lisp_panel_init(uint8_t target_vesc_id)
{
    s_target_vesc_id = target_vesc_id;
    if (!s_lock) {
        s_lock = xSemaphoreCreateMutex();
    }
    if (!s_action_q) {
        s_action_q = xQueueCreate(VLP_ACTION_QUEUE_LEN, sizeof(vlp_action_t));
    }
    s_enabled = false;
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

/* ---- outgoing ----
 *
 * send_request runs on the single CAN poll task (via poll_loop) and waits for
 * the reply, exactly like the RT/LISP/IO polls — that is what keeps the panel's
 * big UI_DESC reply from colliding with theirs in the shared per-id reassembly
 * buffer. Tap handlers on the LVGL task never send directly; they queue an
 * action that poll_loop drains. */

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
    comm_can_send_buffer_sync(s_target_vesc_id, buf, (unsigned int)ind, 0, 60);
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
    /* Queue only — the actual send happens on the CAN poll task (poll_loop),
     * so every panel request stays on one thread. Drop silently if the queue
     * is full (8 deep) or polling isn't up yet; a lost tap is harmless. */
    if (!s_action_q) return;
    vlp_action_t a = { .id = ctrl_id, .value = value };
    xQueueSend(s_action_q, &a, 0);
}

void vesc_lisp_panel_set_enabled(bool enabled)
{
    s_enabled = enabled;
    if (enabled) s_last_poll_ms = 0;   /* poll immediately on open */
    ESP_LOGW(TAG, "panel %s (target=%u)", enabled ? "ENABLED" : "disabled", s_target_vesc_id);
}

void vesc_lisp_panel_poll_loop(void)
{
    if (!s_enabled) return;

    /* Flush any queued tap actions first so they take effect promptly. */
    if (s_action_q) {
        vlp_action_t a;
        while (xQueueReceive(s_action_q, &a, 0) == pdTRUE) {
            send_request(VLP_MSG_ACTION, a.id, a.value, true);
        }
    }

    uint32_t now = millis_now();
    if (now - s_last_poll_ms < VLP_POLL_INTERVAL_MS) return;
    s_last_poll_ms = now;

    /* Keep nudging for the UI descriptor until it lands, then poll live state. */
    if (!s_have_ui) {
        ESP_LOGW(TAG, "-> REQ_UI (no descriptor yet)");
        vesc_lisp_panel_request_ui();
    } else {
        vesc_lisp_panel_request_state();
    }
}

void vesc_lisp_panel_request_dash(void)
{
    send_request(VLP_MSG_REQ_DASH, 0, 0.0f, false);
}

void vesc_lisp_panel_dash_loop(void)
{
    /* Always-on (not gated by s_enabled): the dashboard needs cruise/profile
     * whether or not the drawer is open. Runs on the same rt_task as the other
     * polls, so its reply is serialised with them. */
    uint32_t now = millis_now();
    if (now - s_dash_last_ms < VLP_DASH_INTERVAL_MS) return;
    s_dash_last_ms = now;
    vesc_lisp_panel_request_dash();
}

bool vesc_lisp_panel_get_dash(vlp_dash_t *out)
{
    if (!out || !s_lock) return false;
    bool ok = false;
    if (xSemaphoreTake(s_lock, portMAX_DELAY) == pdTRUE) {
        *out = s_dash;
        ok = s_dash.valid;
        xSemaphoreGive(s_lock);
    }
    return ok;
}

/* ---- incoming ---- */

static void parse_ui_desc(const uint8_t *data, unsigned int len, int32_t ind)
{
    /* static, not on the stack: vlp_model_t is ~1.2 KB (ctrl[16]) and this runs
     * on the can_proc task whose stack is small — a stack frame this big
     * overflowed it and rebooted the device the moment a panel UI_DESC arrived.
     * parse_ui_desc is only ever called from can_proc, so a single static
     * scratch instance is safe. */
    static vlp_model_t m;
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
    ESP_LOGW(TAG, "UI_DESC parsed: ver=%u count=%u", m.version, m.count);
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

static void parse_dash(const uint8_t *data, unsigned int len, int32_t ind)
{
    if (ind + 16 > (int)len) return;            /* need 4 × i32 */
    int32_t ca = buffer_get_int32(data, &ind);
    float   cr = buffer_get_float32(data, VLP_SCALE, &ind);
    int32_t cp = buffer_get_int32(data, &ind);
    float   rpm = buffer_get_float32(data, VLP_SCALE, &ind);
    if (s_lock && xSemaphoreTake(s_lock, portMAX_DELAY) == pdTRUE) {
        s_dash.valid           = true;
        s_dash.cruise_active   = (ca != 0);
        s_dash.cruise_rpm      = cr;
        s_dash.current_profile = cp / 1000;     /* sent as profile × 1000 */
        s_dash.rpm_per_ms      = rpm;
        xSemaphoreGive(s_lock);
    }
    /* DIAGNOSTIC (throttled): confirm the new channel is live and what it carries. */
    {
        static uint32_t s_ms;
        uint32_t now = millis_now();
        if (now - s_ms > 2000) {
            s_ms = now;
            ESP_LOGW(TAG, "DASH rx: cruise=%d rpm=%.0f profile=%d rpm/ms=%.3f",
                     ca != 0, (double)cr, cp / 1000, (double)rpm);
        }
    }
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
    } else if (msg == VLP_MSG_DASH) {
        parse_dash(data, len, ind);
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
