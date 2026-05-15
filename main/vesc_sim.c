/*
 * Synthetic VESC controller for desk development without real hardware.
 * Runs a scripted drive cycle (idle → accelerate → cruise → regen → brake)
 * and a medium-fidelity battery model (Ri, non-linear SoC→OCV curve,
 * regenerative charging). Pushes results into vesc_rt_data at ~20 Hz.
 */

#include "vesc_sim.h"

#include "dev_settings.h"
#include "vesc_can/vesc_datatypes.h"
#include "vesc_can/vesc_rt_data.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>
#include <string.h>

static const char *TAG = "vesc_sim";

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

    vesc_rt_data_inject(&snap);
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
            const vesc_setup_values_t *rt = vesc_rt_data_get_latest();
            ESP_LOGI(TAG, "phase=%d v=%.1fkm/h Vin=%.1fV I=%.1fA SoC=%.1f%% trip=%.2fkm",
                     (int)s_phase, rt->speed * 3.6f, rt->v_in, rt->current_in,
                     rt->battery_level * 100.0f, rt->tachometer_abs / 1000.0f);
        }

        vTaskDelay(pdMS_TO_TICKS(TICK_MS));
    }

    s_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t vesc_sim_start(void)
{
    if (s_task) return ESP_OK;
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
