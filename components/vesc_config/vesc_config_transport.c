/*
 * CAN transport for the config module: builds and sends COMM_GET/SET_MCCONF/
 * APPCONF (+ defaults) and COMM_FW_VERSION to the target VESC, and routes the
 * reassembled replies (delivered on the CAN process task via
 * vesc_packet_dispatch -> vesc_config_transport_process_response) back to the
 * waiting requester.
 *
 * One request is in flight at a time. Completion is reported via the caller's
 * callback, fired either from the CAN task (reply arrived) or the esp_timer
 * task (timeout) — whichever wins the race, guarded by a spinlock.
 */
#include "vesc_config_priv.h"
#include "vesc_config/vesc_config_transport.h"

#include "vesc_can/comm_can.h"
#include "vesc_can/vesc_link.h"
#include "vesc_can/vesc_datatypes.h"
#include "vesc_can/buffer.h"
#include "dev_settings.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

#include <math.h>
#include <string.h>

static const char *TAG = "vesc_cfg_tx";

/* A correct reply (~489 B ≈ 80 CAN frames) normally lands in well under 200 ms.
 * 1.5 s cleanly distinguishes "lost a fragment" from "slow", so retries (UI
 * layer) kick in fast without false positives even at 125 kbps. */
/* One round trip to the VESC and back. Scaled for the live transport —
 * see vct_timeout_us(). */
#define VCT_TIMEOUT_MS   1500
#define VCT_MAX_TX       520             /* COMM-id byte + serialized blob (<= 489) */

typedef enum { REQ_NONE = 0, REQ_GET, REQ_SET, REQ_FW, REQ_DETECT, REQ_RAW } req_kind_t;

static portMUX_TYPE       s_lock = portMUX_INITIALIZER_UNLOCKED;
static esp_timer_handle_t s_timer = NULL;

/* Pending request state — written under s_lock. */
static volatile bool s_in_flight = false;
static req_kind_t    s_req;
static uint8_t       s_expected_cmd;       /* reply COMM id we're waiting for */
static const vc_table_t *s_table;
static vc_value_t   *s_vals_out;           /* GET target */
static vct_cb_t      s_cb;
static vct_fw_cb_t   s_fw_cb;
static vct_detect_cb_t s_detect_cb;
static vct_raw_cb_t  s_raw_cb;
static void         *s_user;

static uint8_t s_txbuf[VCT_MAX_TX];

/* Config target override for dual-head boards. 0 = use the global
 * target_vesc_id (the primary head); non-zero = configure that controller_id
 * instead. Set from the VESC config menu's "Head 1 / Head 2" selector. */
static uint8_t s_target_override = 0;

static inline uint64_t vct_timeout_us(void)
{
    return (uint64_t)vesc_link_scale_ms(VCT_TIMEOUT_MS) * 1000u;
}

static inline uint8_t cfg_target(void)
{
    return s_target_override ? s_target_override : settings_get_target_vesc_id();
}

void vesc_config_set_target(uint8_t controller_id)
{
    s_target_override = controller_id;
}

static void timeout_cb(void *arg);

void vct_init(void)
{
    if (!s_timer) {
        const esp_timer_create_args_t args = {
            .callback = timeout_cb,
            .name = "vesc_cfg_to",
        };
        esp_timer_create(&args, &s_timer);
    }
}

bool vct_busy(void)
{
    return s_in_flight;
}

/* Atomically end the in-flight request, returning the captured callback info
 * so the caller can invoke it OUTSIDE the lock. Returns false if no request
 * was in flight (lost the race — already completed). */
static bool take_completion(req_kind_t *req, vct_cb_t *cb, vct_fw_cb_t *fw_cb,
                            vct_detect_cb_t *detect_cb, vct_raw_cb_t *raw_cb, void **user)
{
    bool taken = false;
    taskENTER_CRITICAL(&s_lock);
    if (s_in_flight) {
        s_in_flight = false;
        *req = s_req;
        *cb = s_cb;
        *fw_cb = s_fw_cb;
        *detect_cb = s_detect_cb;
        *raw_cb = s_raw_cb;
        *user = s_user;
        taken = true;
    }
    taskEXIT_CRITICAL(&s_lock);
    if (taken && s_timer) {
        esp_timer_stop(s_timer);  /* no-op if already fired */
    }
    return taken;
}

static void timeout_cb(void *arg)
{
    (void)arg;
    req_kind_t req; vct_cb_t cb; vct_fw_cb_t fw_cb; vct_detect_cb_t detect_cb;
    vct_raw_cb_t raw_cb; void *user;
    if (take_completion(&req, &cb, &fw_cb, &detect_cb, &raw_cb, &user)) {
        ESP_LOGW(TAG, "request (kind %d) timed out", (int)req);
        if (req == REQ_FW) {
            if (fw_cb) fw_cb(0, 0, false, user);
        } else if (req == REQ_DETECT) {
            if (detect_cb) detect_cb(VCT_DETECT_TIMEOUT, user);
        } else if (req == REQ_RAW) {
            if (raw_cb) raw_cb(0, NULL, 0, user);
        } else if (cb) {
            cb(VC_ERR_TIMEOUT, user);
        }
    }
}

/* Begin a request: store state + start the timeout. Returns false if busy. */
static bool begin(req_kind_t req, uint8_t expected_cmd, const vc_table_t *t,
                  vc_value_t *vals_out, vct_cb_t cb, vct_fw_cb_t fw_cb,
                  vct_detect_cb_t detect_cb, vct_raw_cb_t raw_cb,
                  void *user, uint32_t timeout_us)
{
    taskENTER_CRITICAL(&s_lock);
    if (s_in_flight) {
        taskEXIT_CRITICAL(&s_lock);
        return false;
    }
    s_in_flight    = true;
    s_req          = req;
    s_expected_cmd = expected_cmd;
    s_table        = t;
    s_vals_out     = vals_out;
    s_cb           = cb;
    s_fw_cb        = fw_cb;
    s_detect_cb    = detect_cb;
    s_raw_cb       = raw_cb;
    s_user         = user;
    taskEXIT_CRITICAL(&s_lock);

    if (s_timer) {
        esp_timer_start_once(s_timer, timeout_us);
    }
    return true;
}

static uint8_t get_cmd(const vc_table_t *t, bool defaults)
{
    if (t->kind == 0) {
        return defaults ? COMM_GET_MCCONF_DEFAULT : COMM_GET_MCCONF;
    }
    return defaults ? COMM_GET_APPCONF_DEFAULT : COMM_GET_APPCONF;
}

static uint8_t set_cmd(const vc_table_t *t)
{
    return (t->kind == 0) ? COMM_SET_MCCONF : COMM_SET_APPCONF;
}

vc_result_t vct_get(const vc_table_t *t, bool defaults, vc_value_t *vals_out,
                    vct_cb_t cb, void *user)
{
    uint8_t cmd = get_cmd(t, defaults);
    if (!begin(REQ_GET, cmd, t, vals_out, cb, NULL, NULL, NULL, user, vct_timeout_us())) {
        return VC_ERR_BUSY;
    }
    uint8_t buf = cmd;
    vesc_link_send(cfg_target(), &buf, 1, 0);
    ESP_LOGI(TAG, "GET %s%s sent", t->kind ? "appconf" : "mcconf",
             defaults ? " (default)" : "");
    return VC_OK;
}

vc_result_t vct_set(const vc_table_t *t, const vc_value_t *vals,
                    vct_cb_t cb, void *user)
{
    uint8_t cmd = set_cmd(t);
    s_txbuf[0] = cmd;
    size_t n = vesc_config_serialize(t, vals, s_txbuf + 1, sizeof(s_txbuf) - 1);
    if (n == 0) {
        ESP_LOGE(TAG, "serialize overflow");
        return VC_ERR_INTERNAL;
    }
    if (!begin(REQ_SET, cmd, t, NULL, cb, NULL, NULL, NULL, user, vct_timeout_us())) {
        return VC_ERR_BUSY;
    }
    vesc_link_send(cfg_target(), s_txbuf, (unsigned)(n + 1), 0);
    ESP_LOGI(TAG, "SET %s sent (%u B)", t->kind ? "appconf" : "mcconf",
             (unsigned)(n + 1));
    return VC_OK;
}

vc_result_t vct_probe_fw(vct_fw_cb_t cb, void *user)
{
    if (!begin(REQ_FW, COMM_FW_VERSION, NULL, NULL, NULL, cb, NULL, NULL, user, vct_timeout_us())) {
        return VC_ERR_BUSY;
    }
    uint8_t buf = COMM_FW_VERSION;
    vesc_link_send(cfg_target(), &buf, 1, 0);
    ESP_LOGI(TAG, "FW_VERSION probe sent to id %u", cfg_target());
    return VC_OK;
}

vc_result_t vct_detect_foc(bool detect_can, double max_power_loss, double min_current_in,
                           double max_current_in, double openloop_rpm, double sl_erpm,
                           vct_detect_cb_t cb, void *user)
{
    int32_t ind = 0;
    s_txbuf[ind++] = COMM_DETECT_APPLY_ALL_FOC;
    s_txbuf[ind++] = detect_can ? 1 : 0;
    buffer_append_int32(s_txbuf, (int32_t)llround(max_power_loss * 1e3), &ind);
    buffer_append_int32(s_txbuf, (int32_t)llround(min_current_in  * 1e3), &ind);
    buffer_append_int32(s_txbuf, (int32_t)llround(max_current_in  * 1e3), &ind);
    buffer_append_int32(s_txbuf, (int32_t)llround(openloop_rpm    * 1e3), &ind);
    buffer_append_int32(s_txbuf, (int32_t)llround(sl_erpm         * 1e3), &ind);
    /* ~120 s: detection spins the motor through several measurement phases. */
    if (!begin(REQ_DETECT, COMM_DETECT_APPLY_ALL_FOC, NULL, NULL, NULL, NULL,
               cb, NULL, user, 120000u * 1000u)) {
        return VC_ERR_BUSY;
    }
    vesc_link_send(cfg_target(), s_txbuf, (unsigned)ind, 0);
    ESP_LOGW(TAG, "DETECT_APPLY_ALL_FOC sent (max_loss=%.0f W) — motor will spin",
             max_power_loss);
    return VC_OK;
}

vc_result_t vct_request_raw(uint8_t cmd, const uint8_t *body, unsigned int body_len,
                            unsigned int timeout_ms, vct_raw_cb_t cb, void *user)
{
    if (1u + body_len > sizeof s_txbuf) {
        return VC_ERR_INTERNAL;
    }
    s_txbuf[0] = cmd;
    if (body_len && body) {
        memcpy(s_txbuf + 1, body, body_len);
    }
    if (!begin(REQ_RAW, cmd, NULL, NULL, NULL, NULL, NULL, cb, user,
               (uint32_t)timeout_ms * 1000u)) {
        return VC_ERR_BUSY;
    }
    vesc_link_send(cfg_target(), s_txbuf, 1u + body_len, 0);
    ESP_LOGW(TAG, "raw detect cmd %u sent (%u B) — motor may spin", cmd, 1u + body_len);
    return VC_OK;
}

bool vesc_config_transport_process_response(const uint8_t *data, unsigned int len)
{
    if (len < 1) {
        return false;
    }
    uint8_t cmd = data[0];

    /* Match against the in-flight request without committing yet. */
    bool mine;
    req_kind_t req;
    const vc_table_t *table;
    vc_value_t *vals_out;
    taskENTER_CRITICAL(&s_lock);
    mine = s_in_flight && cmd == s_expected_cmd;
    req = s_req;
    table = s_table;
    vals_out = s_vals_out;
    taskEXIT_CRITICAL(&s_lock);
    if (!mine) {
        return false;
    }

    /* Parse the payload (still tolerant of a concurrent timeout: take_completion
     * below decides who actually invokes the callback). */
    vc_result_t res = VC_OK;
    uint8_t fw_major = 0, fw_minor = 0;
    int detect_result = 0;
    if (req == REQ_FW) {
        if (len >= 3) {
            fw_major = data[1];
            fw_minor = data[2];
        } else {
            res = VC_ERR_INTERNAL;
        }
    } else if (req == REQ_GET) {
        /* blob starts after the COMM-id byte */
        if (!vesc_config_deserialize(table, data + 1, len - 1, vals_out)) {
            res = VC_ERR_SIGNATURE;
        }
    } else if (req == REQ_DETECT) {
        int32_t ind = 1;  /* skip COMM id */
        detect_result = (len >= 3) ? (int)buffer_get_int16(data, &ind) : VCT_DETECT_TIMEOUT;
    } /* REQ_RAW: payload handed to the cb below; REQ_SET: ack only */

    req_kind_t fin_req; vct_cb_t cb; vct_fw_cb_t fw_cb; vct_detect_cb_t detect_cb;
    vct_raw_cb_t raw_cb; void *user;
    if (take_completion(&fin_req, &cb, &fw_cb, &detect_cb, &raw_cb, &user)) {
        if (fin_req == REQ_FW) {
            if (fw_cb) fw_cb(fw_major, fw_minor, res == VC_OK, user);
        } else if (fin_req == REQ_DETECT) {
            if (detect_cb) detect_cb(detect_result, user);
        } else if (fin_req == REQ_RAW) {
            if (raw_cb) raw_cb(1, data + 1, len - 1, user);   /* payload after COMM id */
        } else if (cb) {
            cb(res, user);
        }
    }
    return true;
}
