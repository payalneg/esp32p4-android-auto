/*
 * Synthetic VESC controller for desk development without real hardware.
 * Runs a scripted drive cycle (idle → accelerate → cruise → regen → brake)
 * and a medium-fidelity battery model (Ri, non-linear SoC→OCV curve,
 * regenerative charging).
 *
 * It sits on comm_can's virtual bus as a VESC node, not behind it: the head
 * unit's polls arrive here as the same FILL_RX_BUFFER / PROCESS_RX_BUFFER /
 * PROCESS_SHORT_BUFFER frames a real controller would see, are reassembled,
 * answered in VESC wire format and fragmented back — after the delay a real
 * bus imposes. So everything above the driver (rt_task's serialised polls and
 * their 60 ms waits, per-id reassembly, CRC, vesc_packet_dispatch, the UI
 * updater's staleness logic) runs unchanged and on real-world timing, which is
 * the point: the old version injected data straight into vesc_rt_data at a
 * rock-steady 20 Hz and hid every timing bug the bench then met on the bike.
 *
 * Latency model (see NODE_*): a couple of ms for the controller to pick the
 * packet up and encode the answer, uniform jitter on top, the frames' own time
 * on the wire at the configured bit rate, one lost reply in a hundred (the
 * poller then eats its full timeout, as it does in the field), and every ten
 * seconds a 40 ms stall — the kind the C3 helper's exchange or a LISP GC pause
 * produces. Numbers are estimates, not measurements; tune against a captured
 * bus when we have one.
 */

#include "vesc_sim.h"

#include "dev_settings.h"
#include "vesc_can/buffer.h"
#include "vesc_can/comm_can.h"
#include "vesc_can/crc.h"
#include "vesc_can/vesc_datatypes.h"
#include "vesc_can/vesc_lisp_panel.h"
#include "vesc_can/vesc_rt_data.h"

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <math.h>
#include <string.h>

static const char *TAG = "vesc_sim";

/* ---------- Latency model ---------- */
#define NODE_LATENCY_MIN_US     2500    /* controller picks up + encodes */
#define NODE_LATENCY_JITTER_US  6000    /* uniform 0..this on top */
#define NODE_FRAME_BITS         136     /* 29-bit ext frame, 8 B, avg stuffing */
#define NODE_DROP_PERMILLE      10      /* 1 % of replies never come */
#define NODE_STALL_EVERY_MS     10000   /* periodic bus hog … */
#define NODE_STALL_US           40000   /* … adds this to one reply */
#define NODE_REPLY_SLOTS        4       /* replies in flight (polls are serialised) */
#define NODE_MAX_FRAMES         40      /* ~250 B reply */
#define NODE_REQ_MAX            64      /* longest request we answer */

#define TICK_MS              50
#define DT_SECONDS           (TICK_MS / 1000.0f)

/* Battery / vehicle constants. Sized for an e-scooter or light e-bike pack;
 * good enough to make the dashboard look alive. */
#define V_NOMINAL            50.4f   /* 14S Li-ion nominal */
#define V_MAX                54.6f
#define V_MIN                40.0f
#define R_INTERNAL           0.05f   /* Ω */
#define MASS_KG              85.0f
#define K_DRAG               0.45f
#define K_ROLL               12.0f   /* N */
#define MOTOR_KT             0.07f   /* N·m/A — lumped with gear */
#define WHEEL_RADIUS_DEFAULT 0.10f   /* m, used if settings give nothing */
#define TAU_MOTOR_TEMP_S     30.0f
#define AMBIENT_C            22.0f
#define HEAT_PER_AMP2        0.015f  /* °C/A² steady-state */

/* Drive-cycle phases. Times are seconds. */
typedef enum {
    PHASE_IDLE = 0,
    PHASE_ACCEL,
    PHASE_CRUISE,
    PHASE_DECEL_REGEN,
    PHASE_LOW_CRUISE,
    PHASE_BRAKE,
    PHASE_DWELL,
    PHASE_COUNT,
} sim_phase_t;

static const float s_phase_duration_s[PHASE_COUNT] = {
    [PHASE_IDLE]        = 5.0f,
    [PHASE_ACCEL]       = 10.0f,
    [PHASE_CRUISE]      = 30.0f,
    [PHASE_DECEL_REGEN] = 8.0f,
    [PHASE_LOW_CRUISE]  = 15.0f,
    [PHASE_BRAKE]       = 5.0f,
    [PHASE_DWELL]       = 3.0f,
};

static TaskHandle_t s_task         = NULL;
static volatile bool s_running     = false;

/* The controller's live state as the last model step left it. Written by
 * sim_task, read by node_task when it serialises a reply. */
static vesc_setup_values_t s_snap;
static portMUX_TYPE        s_snap_mux = portMUX_INITIALIZER_UNLOCKED;

/* Physical state. */
static float    s_v_ms             = 0.0f;
static float    s_soc              = 1.0f;
static float    s_ah_consumed      = 0.0f;
static float    s_ah_charged       = 0.0f;
static float    s_wh_consumed      = 0.0f;
static float    s_wh_charged       = 0.0f;
static double   s_tach_abs_m       = 0.0;  /* trip distance, monotonic */
static double   s_tach_m           = 0.0;  /* signed (same as abs for forward-only) */
static uint32_t s_odometer_m       = 0;
static float    s_motor_temp_c     = AMBIENT_C;
static float    s_mos_temp_c       = AMBIENT_C;

/* Scenario state. */
static sim_phase_t s_phase         = PHASE_IDLE;
static float       s_phase_elapsed = 0.0f;

static inline float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/* Very simple non-linear SoC → OCV-normalised mapping. Flat in the 20–80%
 * band, steeper near the edges — matches the shape of a Li-ion discharge
 * curve well enough for a dashboard. Returns 0..1 to be scaled to V_MIN..V_MAX. */
static float soc_to_ocv_norm(float soc)
{
    soc = clampf(soc, 0.0f, 1.0f);
    if (soc < 0.2f) {
        return 0.10f + (soc / 0.2f) * 0.25f;        /* 0..20%   → 0.10..0.35 */
    } else if (soc < 0.8f) {
        return 0.35f + ((soc - 0.2f) / 0.6f) * 0.45f; /* 20..80% → 0.35..0.80 */
    } else {
        return 0.80f + ((soc - 0.8f) / 0.2f) * 0.20f; /* 80..100%→ 0.80..1.00 */
    }
}

/* Throttle command from the scripted scenario, in [-1.0, +1.0].
 * Positive = motoring, negative = regen braking. */
static float scenario_throttle(sim_phase_t phase, float t)
{
    switch (phase) {
    case PHASE_IDLE:        return 0.0f;
    case PHASE_ACCEL:       return clampf(t / s_phase_duration_s[PHASE_ACCEL] * 0.6f, 0.0f, 0.6f);
    case PHASE_CRUISE:      return 0.22f;
    case PHASE_DECEL_REGEN: return -0.30f;
    case PHASE_LOW_CRUISE:  return 0.10f;
    case PHASE_BRAKE:       return -0.50f * clampf(t / s_phase_duration_s[PHASE_BRAKE], 0.0f, 1.0f);
    case PHASE_DWELL:       return 0.0f;
    default:                return 0.0f;
    }
}

static void sim_step(uint32_t uptime_ms)
{
    /* Advance phase clock and roll over to next phase. */
    s_phase_elapsed += DT_SECONDS;
    if (s_phase_elapsed >= s_phase_duration_s[s_phase]) {
        s_phase_elapsed = 0.0f;
        s_phase = (sim_phase_t)((s_phase + 1) % PHASE_COUNT);
    }

    float throttle = scenario_throttle(s_phase, s_phase_elapsed);

    /* Settings the user can change at runtime. */
    float    capacity_ah  = settings_get_battery_capacity();
    float    power_max_kw = settings_get_power_max_kw();
    uint16_t wheel_mm     = settings_get_wheel_diameter_mm();
    uint8_t  poles        = settings_get_motor_poles();

    if (capacity_ah  < 0.5f) capacity_ah  = 20.0f;
    if (power_max_kw < 0.1f) power_max_kw = 1.5f;
    if (wheel_mm     < 50)   wheel_mm     = 200;
    if (poles        < 2)    poles        = 14;

    float wheel_r_m = wheel_mm / 2000.0f;

    /* Motor current request, bounded by configured power envelope. */
    float v_in_prev = V_MIN + (V_MAX - V_MIN) * soc_to_ocv_norm(s_soc);
    float max_i     = (power_max_kw * 1000.0f) / fmaxf(v_in_prev, V_MIN);
    float i_mot     = throttle * max_i;

    /* Forces. */
    float f_drag = K_DRAG * s_v_ms * s_v_ms + (s_v_ms > 0.05f ? K_ROLL : 0.0f);
    float f_mot  = i_mot * MOTOR_KT * 30.0f;   /* scaled torque-to-force gear factor */
    float accel  = (f_mot - (s_v_ms > 0.05f ? f_drag : 0.0f)) / MASS_KG;

    s_v_ms += accel * DT_SECONDS;
    if (s_v_ms < 0.0f) s_v_ms = 0.0f;

    /* If the wheels aren't moving, regen can't pull current. */
    if (s_v_ms <= 0.01f && i_mot < 0.0f) i_mot = 0.0f;

    /* Charge/discharge accounting. */
    float d_ah = i_mot * DT_SECONDS / 3600.0f;
    if (i_mot > 0.0f) s_ah_consumed += d_ah;
    else              s_ah_charged  += -d_ah;

    float net_ah  = s_ah_consumed - s_ah_charged;
    s_soc         = clampf(1.0f - net_ah / capacity_ah, 0.0f, 1.0f);

    float v_oc    = V_MIN + (V_MAX - V_MIN) * soc_to_ocv_norm(s_soc);
    float v_in    = v_oc - i_mot * R_INTERNAL;
    v_in          = clampf(v_in, V_MIN - 5.0f, V_MAX + 1.0f);

    float power_w = i_mot * v_in;
    float d_wh    = power_w * DT_SECONDS / 3600.0f;
    if (i_mot > 0.0f) s_wh_consumed += d_wh;
    else              s_wh_charged  += -d_wh;

    /* Distances. */
    double d_m = (double)s_v_ms * DT_SECONDS;
    s_tach_m     += d_m;
    s_tach_abs_m += d_m;
    s_odometer_m += (uint32_t)d_m;  /* coarse; meter-level resolution is fine for UI */

    /* Mechanical signals. */
    float wheel_circ = 2.0f * (float)M_PI * wheel_r_m;
    float wheel_rps  = (wheel_circ > 0.001f) ? (s_v_ms / wheel_circ) : 0.0f;
    float erpm       = wheel_rps * 60.0f * (poles / 2.0f);
    float duty       = clampf(v_in > 1.0f ? (i_mot * R_INTERNAL + 0.0f) / v_in + throttle * 0.5f
                                          : 0.0f,
                              -1.0f, 1.0f);

    /* Thermal: RC-filter motor temperature toward steady-state for this load. */
    float target_motor = AMBIENT_C + HEAT_PER_AMP2 * i_mot * i_mot * 6.0f;
    s_motor_temp_c += (target_motor - s_motor_temp_c) * (DT_SECONDS / TAU_MOTOR_TEMP_S);
    s_mos_temp_c   += (target_motor * 0.6f + AMBIENT_C * 0.4f - s_mos_temp_c)
                        * (DT_SECONDS / TAU_MOTOR_TEMP_S);

    /* Assemble snapshot. */
    vesc_setup_values_t snap;
    memset(&snap, 0, sizeof(snap));
    snap.temp_mos          = s_mos_temp_c;
    snap.temp_motor        = s_motor_temp_c;
    snap.current_motor     = i_mot;
    snap.current_in        = i_mot * 0.95f;   /* slight conversion loss */
    snap.duty_now          = duty;
    snap.rpm               = erpm;
    snap.speed             = s_v_ms;
    snap.v_in              = v_in;
    snap.battery_level     = s_soc;
    snap.amp_hours         = s_ah_consumed;
    snap.amp_hours_charged = s_ah_charged;
    snap.watt_hours        = s_wh_consumed;
    snap.watt_hours_charged= s_wh_charged;
    snap.tachometer        = (float)s_tach_m;
    snap.tachometer_abs    = (float)s_tach_abs_m;
    snap.position          = 0.0f;
    snap.fault_code        = 0;
    snap.vesc_id           = settings_get_target_vesc_id();
    snap.num_vescs         = 1;
    snap.battery_wh        = capacity_ah * V_NOMINAL;
    snap.odometer          = s_odometer_m;
    snap.uptime_ms         = uptime_ms;

    portENTER_CRITICAL(&s_snap_mux);
    s_snap = snap;
    portEXIT_CRITICAL(&s_snap_mux);
}

/* ======================= Virtual VESC node ======================= */

/* One request the head unit sent us, reassembled. */
typedef struct {
    uint8_t  sender;            /* HU's controller id — where the reply goes */
    uint8_t  send;              /* VESC "send" flag: 0 = reply over CAN */
    uint16_t len;
    uint8_t  data[NODE_REQ_MAX];
} node_req_t;

typedef struct {
    uint32_t eid;
    uint8_t  len;
    uint8_t  data[8];
} node_frame_t;

/* A reply waiting for its delivery time. Frames are pushed into comm_can's
 * receive ring from the esp_timer task — the ring's single producer. */
typedef struct {
    node_frame_t       frames[NODE_MAX_FRAMES];
    int                n;
    esp_timer_handle_t timer;
    volatile bool      busy;
} node_slot_t;

static QueueHandle_t s_req_q;
static TaskHandle_t  s_node_task;
static node_slot_t   s_slots[NODE_REPLY_SLOTS];
static portMUX_TYPE  s_slot_mux = portMUX_INITIALIZER_UNLOCKED;

/* Reassembly of the HU's fragmented requests. One buffer: there is exactly
 * one peer on this bus. */
static uint8_t  s_rx_asm[512];
static uint32_t s_stat_req, s_stat_dropped, s_stat_unanswered;
static int64_t  s_last_stall_us;

static uint8_t node_id(void)
{
    return settings_get_target_vesc_id();
}

static uint32_t rand_below(uint32_t n)
{
    return n ? (esp_random() % n) : 0;
}

/* Head unit → node. Called from comm_can_transmit_eid under its send mutex on
 * whatever task is polling, so: parse, queue, return. */
void vesc_sim_can_tx(uint32_t eid, const uint8_t *d, uint8_t len)
{
    uint8_t       id  = (uint8_t)(eid & 0xFF);
    CAN_PACKET_ID cmd = (CAN_PACKET_ID)(eid >> 8);
    if (id != node_id() && id != 255) return;   /* another node's traffic */
    if (!s_req_q) return;

    node_req_t r = { 0 };
    switch (cmd) {
    case CAN_PACKET_FILL_RX_BUFFER:
        if (len >= 1 && d[0] + (len - 1) <= sizeof(s_rx_asm)) {
            memcpy(s_rx_asm + d[0], d + 1, len - 1);
        }
        return;
    case CAN_PACKET_FILL_RX_BUFFER_LONG: {
        if (len < 2) return;
        unsigned off = ((unsigned)d[0] << 8) | d[1];
        if (off + (len - 2) <= sizeof(s_rx_asm)) {
            memcpy(s_rx_asm + off, d + 2, len - 2);
        }
        return;
    }
    case CAN_PACKET_PROCESS_RX_BUFFER: {
        if (len < 6) return;
        unsigned plen = ((unsigned)d[2] << 8) | d[3];
        unsigned crc  = ((unsigned)d[4] << 8) | d[5];
        if (plen > sizeof(s_rx_asm) || crc16(s_rx_asm, plen) != crc) {
            ESP_LOGW(TAG, "node: CRC/len mismatch on %u-byte request", plen);
            return;
        }
        r.sender = d[0];
        r.send   = d[1];
        r.len    = (uint16_t)(plen > NODE_REQ_MAX ? NODE_REQ_MAX : plen);
        memcpy(r.data, s_rx_asm, r.len);
        break;
    }
    case CAN_PACKET_PROCESS_SHORT_BUFFER:
        if (len < 3) return;
        r.sender = d[0];
        r.send   = d[1];
        r.len    = (uint16_t)(len - 2);
        memcpy(r.data, d + 2, r.len);
        break;
    case CAN_PACKET_PING:
        /* Answered like a real node: PONG carries our id + hardware type.
         * Routed through the queue so the ring keeps its single producer. */
        r.sender  = d[0];
        r.send    = 0xFF;                   /* marker: PONG, see node_task */
        r.len     = 0;
        break;
    default:
        return;                             /* SET_CURRENT etc. — accepted, no reply */
    }
    if (xQueueSend(s_req_q, &r, 0) != pdTRUE) {
        ESP_LOGW(TAG, "node: request queue full");
    }
}

/* ---- reply fragmenting: the sending half of VESC's comm_can_send_buffer,
 * with this node as the sender and the HU as the destination. */
static void slot_push(node_slot_t *sl, uint32_t eid, const uint8_t *d, uint8_t len)
{
    if (sl->n >= NODE_MAX_FRAMES) return;
    node_frame_t *f = &sl->frames[sl->n++];
    f->eid = eid;
    f->len = len;
    memcpy(f->data, d, len);
}

static void slot_fragment(node_slot_t *sl, uint8_t dst, const uint8_t *data,
                          unsigned len, uint8_t send)
{
    uint8_t  me = node_id();
    uint8_t  b[8];
    if (len <= 6) {
        b[0] = me; b[1] = send;
        memcpy(b + 2, data, len);
        slot_push(sl, dst | ((uint32_t)CAN_PACKET_PROCESS_SHORT_BUFFER << 8), b, (uint8_t)(len + 2));
        return;
    }
    unsigned end_a = 0;
    for (unsigned i = 0; i < len; i += 7) {
        if (i > 255) break;
        end_a = i + 7;
        uint8_t n = (uint8_t)((i + 7 <= len) ? 7 : len - i);
        b[0] = (uint8_t)i;
        memcpy(b + 1, data + i, n);
        slot_push(sl, dst | ((uint32_t)CAN_PACKET_FILL_RX_BUFFER << 8), b, (uint8_t)(n + 1));
    }
    for (unsigned i = end_a; i < len; i += 6) {
        uint8_t n = (uint8_t)((i + 6 <= len) ? 6 : len - i);
        b[0] = (uint8_t)(i >> 8); b[1] = (uint8_t)(i & 0xFF);
        memcpy(b + 2, data + i, n);
        slot_push(sl, dst | ((uint32_t)CAN_PACKET_FILL_RX_BUFFER_LONG << 8), b, (uint8_t)(n + 2));
    }
    unsigned short crc = crc16(data, len);
    b[0] = me; b[1] = send;
    b[2] = (uint8_t)(len >> 8); b[3] = (uint8_t)(len & 0xFF);
    b[4] = (uint8_t)(crc >> 8); b[5] = (uint8_t)(crc & 0xFF);
    slot_push(sl, dst | ((uint32_t)CAN_PACKET_PROCESS_RX_BUFFER << 8), b, 6);
}

static void slot_deliver_cb(void *arg)
{
    node_slot_t *sl = (node_slot_t *)arg;
    for (int i = 0; i < sl->n; i++) {
        comm_can_virtual_rx(sl->frames[i].eid, sl->frames[i].data, sl->frames[i].len);
    }
    sl->busy = false;
}

static node_slot_t *slot_take(void)
{
    node_slot_t *sl = NULL;
    portENTER_CRITICAL(&s_slot_mux);
    for (int i = 0; i < NODE_REPLY_SLOTS; i++) {
        if (!s_slots[i].busy) { s_slots[i].busy = true; sl = &s_slots[i]; break; }
    }
    portEXIT_CRITICAL(&s_slot_mux);
    if (sl) sl->n = 0;
    return sl;
}

/* Wire time of one frame at the configured bit rate. */
static uint32_t frame_us(void)
{
    int kbps = (int)settings_get_can_speed();
    if (kbps <= 0) kbps = 500;
    return (uint32_t)(NODE_FRAME_BITS * 1000UL / (uint32_t)kbps);
}

static void slot_schedule(node_slot_t *sl, uint32_t extra_us)
{
    uint32_t delay = NODE_LATENCY_MIN_US + rand_below(NODE_LATENCY_JITTER_US)
                   + (uint32_t)sl->n * frame_us() + extra_us;
    if (esp_timer_start_once(sl->timer, delay) != ESP_OK) {
        slot_deliver_cb(sl);                /* timer busy?? deliver now */
    }
}

/* ---- reply bodies ---- */

static unsigned build_values_setup(uint8_t *buf, uint8_t cmd, uint32_t mask)
{
    vesc_setup_values_t v;
    portENTER_CRITICAL(&s_snap_mux);
    v = s_snap;
    portEXIT_CRITICAL(&s_snap_mux);

    int32_t ind = 0;
    buf[ind++] = cmd;
    if (cmd == COMM_GET_VALUES_SETUP_SELECTIVE) {
        buffer_append_uint32(buf, mask, &ind);
    }
    /* Field order = VESC commands.c COMM_GET_VALUES_SETUP = the parser in
     * vesc_rt_data_process_response. Keep the three in step. */
    if (mask & (1U << 0))  buffer_append_float16(buf, v.temp_mos, 1e1f, &ind);
    if (mask & (1U << 1))  buffer_append_float16(buf, v.temp_motor, 1e1f, &ind);
    if (mask & (1U << 2))  buffer_append_float32(buf, v.current_motor, 1e2f, &ind);
    if (mask & (1U << 3))  buffer_append_float32(buf, v.current_in, 1e2f, &ind);
    if (mask & (1U << 4))  buffer_append_float16(buf, v.duty_now, 1e3f, &ind);
    if (mask & (1U << 5))  buffer_append_float32(buf, v.rpm, 1e0f, &ind);
    if (mask & (1U << 6))  buffer_append_float32(buf, v.speed, 1e3f, &ind);
    if (mask & (1U << 7))  buffer_append_float16(buf, v.v_in, 1e1f, &ind);
    if (mask & (1U << 8))  buffer_append_float16(buf, v.battery_level, 1e3f, &ind);
    if (mask & (1U << 9))  buffer_append_float32(buf, v.amp_hours, 1e4f, &ind);
    if (mask & (1U << 10)) buffer_append_float32(buf, v.amp_hours_charged, 1e4f, &ind);
    if (mask & (1U << 11)) buffer_append_float32(buf, v.watt_hours, 1e4f, &ind);
    if (mask & (1U << 12)) buffer_append_float32(buf, v.watt_hours_charged, 1e4f, &ind);
    if (mask & (1U << 13)) buffer_append_float32(buf, v.tachometer, 1e3f, &ind);
    if (mask & (1U << 14)) buffer_append_float32(buf, v.tachometer_abs, 1e3f, &ind);
    if (mask & (1U << 15)) buffer_append_float32(buf, v.position, 1e6f, &ind);
    if (mask & (1U << 16)) buf[ind++] = v.fault_code;
    if (mask & (1U << 17)) buf[ind++] = v.vesc_id;
    if (mask & (1U << 18)) buf[ind++] = v.num_vescs;
    if (mask & (1U << 19)) buffer_append_float32(buf, v.battery_wh, 1e3f, &ind);
    if (mask & (1U << 20)) buffer_append_uint32(buf, v.odometer, &ind);
    if (mask & (1U << 21)) buffer_append_uint32(buf, v.uptime_ms, &ind);
    return (unsigned)ind;
}

static unsigned build_fw_version(uint8_t *buf)
{
    static const char hw[] = "SIM VESC";
    int32_t ind = 0;
    buf[ind++] = COMM_FW_VERSION;
    buf[ind++] = 6;                                 /* pretend 6.05 */
    buf[ind++] = 5;
    memcpy(buf + ind, hw, sizeof(hw)); ind += (int32_t)sizeof(hw);
    for (int i = 0; i < 12; i++) buf[ind++] = (uint8_t)(0xA0 + i);   /* uuid */
    buf[ind++] = 0;                                 /* pairing_done */
    buf[ind++] = 0;                                 /* test fw */
    buf[ind++] = HW_TYPE_VESC;
    buf[ind++] = 0;                                 /* custom configs */
    buf[ind++] = 0;                                 /* phase filters */
    return (unsigned)ind;
}

/* Build the answer to one request into buf; 0 = this node does not answer
 * (like a controller without the matching feature — the poller times out). */
static unsigned build_reply(const node_req_t *r, uint8_t *buf)
{
    if (r->len < 1) return 0;
    switch (r->data[0]) {
    case COMM_GET_VALUES_SETUP:
        return build_values_setup(buf, COMM_GET_VALUES_SETUP, 0xFFFFFFFFu);
    case COMM_GET_VALUES_SETUP_SELECTIVE: {
        if (r->len < 5) return 0;
        int32_t ind = 1;
        uint32_t mask = buffer_get_uint32(r->data, &ind);
        return build_values_setup(buf, COMM_GET_VALUES_SETUP_SELECTIVE, mask);
    }
    case COMM_FW_VERSION:
        return build_fw_version(buf);
    case COMM_GET_DECODED_ADC: {
        /* Throttle voltage tracks the model's duty; brake idle. */
        float thr = 0.85f + fmaxf(s_snap.duty_now, 0.0f) * 3.0f;
        int32_t ind = 0;
        buf[ind++] = COMM_GET_DECODED_ADC;
        buffer_append_float32(buf, fmaxf(s_snap.duty_now, 0.0f), 1e6f, &ind);
        buffer_append_float32(buf, thr, 1e6f, &ind);
        buffer_append_float32(buf, 0.0f, 1e6f, &ind);
        buffer_append_float32(buf, 0.82f, 1e6f, &ind);
        return (unsigned)ind;
    }
    case COMM_GET_DECODED_PPM: {
        int32_t ind = 0;
        buf[ind++] = COMM_GET_DECODED_PPM;
        buffer_append_float32(buf, 0.0f, 1e6f, &ind);
        buffer_append_float32(buf, 1.5f, 1e6f, &ind);
        return (unsigned)ind;
    }
    case COMM_CUSTOM_APP_DATA:
        /* The dashboard's cruise/profile poll, served the way main.lisp does:
         * fixed 4 × i32 layout (see parse_dash). Everything else the panel
         * speaks (UI_DESC, STATE, actions, PAS set) stays unanswered here. */
        if (r->len >= 4 && r->data[1] == VLP_MAGIC0 && r->data[2] == VLP_MAGIC1 &&
            r->data[3] == VLP_MSG_REQ_DASH) {
            int32_t ind = 0;
            buf[ind++] = COMM_CUSTOM_APP_DATA;
            buf[ind++] = VLP_MAGIC0;
            buf[ind++] = VLP_MAGIC1;
            buf[ind++] = VLP_MSG_DASH;
            buffer_append_int32(buf, 0, &ind);                          /* cruise off */
            buffer_append_float32(buf, 0.0f, VLP_SCALE, &ind);         /* cruise rpm */
            buffer_append_int32(buf, 2 * 1000, &ind);                   /* profile 2 */
            buffer_append_float32(buf, s_snap.rpm / 1000.0f, VLP_SCALE, &ind);
            return (unsigned)ind;
        }
        return 0;
    default:
        return 0;
    }
}

static void node_task(void *arg)
{
    (void)arg;
    node_req_t r;
    uint8_t    reply[256];
    for (;;) {
        if (xQueueReceive(s_req_q, &r, portMAX_DELAY) != pdTRUE) continue;
        s_stat_req++;

        node_slot_t *sl = slot_take();
        if (!sl) {                          /* all in flight — behave like a lost frame */
            s_stat_dropped++;
            continue;
        }

        if (r.send == 0xFF) {               /* PONG */
            uint8_t b[2] = { node_id(), HW_TYPE_VESC };
            slot_push(sl, r.sender | ((uint32_t)CAN_PACKET_PONG << 8), b, 2);
            slot_schedule(sl, 0);
            continue;
        }

        unsigned n = (r.send == 0) ? build_reply(&r, reply) : 0;
        if (n == 0) {
            sl->busy = false;
            if (r.send == 0) s_stat_unanswered++;
            continue;
        }
        if (rand_below(1000) < NODE_DROP_PERMILLE) {
            s_stat_dropped++;
            sl->busy = false;
            continue;                       /* HU waits out its timeout */
        }

        uint32_t extra = 0;
        int64_t  now   = esp_timer_get_time();
        if (now - s_last_stall_us > (int64_t)NODE_STALL_EVERY_MS * 1000) {
            s_last_stall_us = now;
            extra = NODE_STALL_US;
        }
        slot_fragment(sl, r.sender, reply, n, 1);
        slot_schedule(sl, extra);
    }
}

static esp_err_t node_init(void)
{
    if (s_req_q) return ESP_OK;
    s_req_q = xQueueCreate(8, sizeof(node_req_t));
    if (!s_req_q) return ESP_ERR_NO_MEM;
    for (int i = 0; i < NODE_REPLY_SLOTS; i++) {
        const esp_timer_create_args_t a = {
            .callback = slot_deliver_cb,
            .arg      = &s_slots[i],
            .name     = "vesc_sim_rx",
        };
        if (esp_timer_create(&a, &s_slots[i].timer) != ESP_OK) return ESP_FAIL;
    }
    s_last_stall_us = esp_timer_get_time();
    /* Above rt_task (5) so a queued request is answered as soon as the poller
     * blocks; below can_proc (8) so delivery never starves the decoder. */
    if (xTaskCreatePinnedToCore(node_task, "vesc_node", 4096, NULL, 6, &s_node_task, 0) != pdPASS) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "virtual VESC node id=%u: latency %u+0..%u us, %u us/frame, "
                  "%u/1000 replies dropped, %u ms stall every %u s",
             node_id(), NODE_LATENCY_MIN_US, NODE_LATENCY_JITTER_US, frame_us(),
             NODE_DROP_PERMILLE, NODE_STALL_US / 1000, NODE_STALL_EVERY_MS / 1000);
    return ESP_OK;
}

static void sim_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "drive cycle started — capacity=%.1fAh power_max=%.1fkW",
             settings_get_battery_capacity(), settings_get_power_max_kw());

    uint32_t log_counter = 0;
    int64_t  t0          = esp_timer_get_time();

    while (s_running) {
        uint32_t uptime_ms = (uint32_t)((esp_timer_get_time() - t0) / 1000);
        sim_step(uptime_ms);

        if (++log_counter >= 40) {  /* every ~2 s */
            log_counter = 0;
            ESP_LOGI(TAG, "phase=%d v=%.1fkm/h Vin=%.1fV I=%.1fA SoC=%.1f%% trip=%.2fkm "
                          "| node: %lu req, %lu dropped, %lu timeouts seen by HU",
                     (int)s_phase, s_snap.speed * 3.6f, s_snap.v_in, s_snap.current_in,
                     s_snap.battery_level * 100.0f, s_snap.tachometer_abs / 1000.0f,
                     (unsigned long)s_stat_req, (unsigned long)s_stat_dropped,
                     (unsigned long)s_stat_unanswered);
        }

        vTaskDelay(pdMS_TO_TICKS(TICK_MS));
    }

    s_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t vesc_sim_start(void)
{
    if (s_task) return ESP_OK;
    esp_err_t e = node_init();
    if (e != ESP_OK) return e;
    s_running = true;
    BaseType_t r = xTaskCreatePinnedToCore(sim_task, "vesc_sim", 4096, NULL, 4, &s_task, 0);
    if (r != pdPASS) {
        s_running = false;
        ESP_LOGE(TAG, "failed to spawn sim task");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void vesc_sim_stop(void)
{
    s_running = false;
}
