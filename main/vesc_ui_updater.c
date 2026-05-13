#include "vesc_ui_updater.h"

#include "ble_host.h"
#include "bsp/esp-bsp.h"
#include "dev_settings.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui_mode.h"
#include "vesc_battery_calc.h"
#include "vesc_can/vesc_lisp_poll.h"
#include "vesc_can/vesc_rt_data.h"
#include "vesc_trip_persist.h"

/* GUI Guider's custom.h is generated C++-friendly but pure C. The
 * widget update functions live in Super_VESC_Display/custom/custom.c
 * which is built as part of vesc_ui. */
#include "custom.h"

static const char *TAG = "vesc_ui_upd";

static lv_timer_t *s_timer;
static bool        s_zeros_pushed;

/* Range estimator state — ported from Super_VESC_Display/src/vesc_rt_data.cpp.
 * Recalculates remaining range each 100 m or 0.1 Ah so the displayed value
 * stays stable on flat ground but tracks consumption in real time when the
 * load changes. Lives here rather than in vesc_can because the math needs
 * both trip_persist (Ah consumed, km travelled) and battery_calc (remaining
 * Ah), and the UI updater is the only consumer. */
#define RANGE_UPDATE_DISTANCE_M  100.0f
#define RANGE_UPDATE_AH          0.1f

static float s_range_last_dist_km;
static float s_range_last_ah;
static float s_range_cached_km;
static bool  s_range_initialized;

static float compute_range_km(void)
{
    float distance_km = trip_persist_get_trip_km();
    float consumed_ah = trip_persist_get_amp_hours();
    float remaining_ah = battery_calc_get_remaining_ah();

    if (distance_km < 0.1f) {
        s_range_cached_km   = 0.0f;
        s_range_initialized = false;
        return 0.0f;
    }

    float dist_delta_m = (distance_km - s_range_last_dist_km) * 1000.0f;
    float ah_delta     = consumed_ah  - s_range_last_ah;
    bool  recalc = !s_range_initialized ||
                   dist_delta_m >= RANGE_UPDATE_DISTANCE_M ||
                   ah_delta     >= RANGE_UPDATE_AH;

    if (recalc) {
        float ah_per_km = consumed_ah / distance_km;
        /* Negative / near-zero consumption (regen, sensor noise) makes the
         * range divisor blow up; cap at a sentinel large value instead so
         * the UI shows something stable rather than INF. */
        s_range_cached_km   = (ah_per_km < 0.01f) ? 999.9f
                                                  : (remaining_ah / ah_per_km);
        s_range_last_dist_km = distance_km;
        s_range_last_ah      = consumed_ah;
        s_range_initialized  = true;
    }
    return s_range_cached_km;
}

static void push_zeros_locked(void)
{
    update_speed(0.0f);
    update_current(0.0f);
    update_battery_proc(0.0f);
    update_trip(0.0f);
    update_range(0.0f);
    update_temp_fet(0.0f);
    update_temp_motor(0.0f);
    update_amp_hours(0.0f);
    update_battery_temp(0.0f);
    update_battery_voltage(0.0f);
    update_odometer(0.0f);
    update_fps(0);
}

static void push_rt_locked(void)
{
    bool fresh = vesc_rt_data_is_fresh();
    update_esc_connection_status(fresh);
    if (!fresh) return;

    const vesc_setup_values_t *rt = vesc_rt_data_get_latest();

    /* Feed trip-persist before reading any persistent counter — that way
     * trip_persist sees the latest raw VESC values and we get monotonic
     * totals back on the line below. tachometer_abs is meters; rt->amp_hours
     * already includes any in-flight regen offset from the VESC; uptime_ms
     * is the VESC's own uptime which resets on reboot. */
    trip_persist_update(rt->tachometer_abs, rt->amp_hours, rt->uptime_ms);

    update_speed(vesc_rt_data_get_speed_kmh());
    update_current(rt->current_in);
    /* battery_level is 0..1 in VESC protocol → display wants percent.
     * Smart mode uses the persistent Ah-integrating tracker; Direct mode
     * just forwards the controller's voltage-based estimate. */
    float batt_pct;
    if (settings_get_battery_calc_mode() == BATTERY_CALC_MODE_SMART) {
        batt_pct = battery_calc_get_smart_percentage(
                rt->battery_level, rt->amp_hours,
                settings_get_battery_capacity());
    } else {
        batt_pct = rt->battery_level * 100.0f;
    }
    update_battery_proc(batt_pct);
    /* Trip / Ah / uptime come from trip_persist so they survive VESC reboots
     * — the raw counters get reset to 0 by the controller, which would make
     * the dashboard appear to jump backwards. trip_persist folds those
     * resets into a running offset. */
    update_trip(trip_persist_get_trip_km());
    update_range(compute_range_km());
    update_temp_fet(rt->temp_mos);
    update_temp_motor(rt->temp_motor);
    update_amp_hours(trip_persist_get_amp_hours());
    update_battery_temp(rt->temp_mos);
    update_battery_voltage(rt->v_in);
    update_odometer(vesc_rt_data_get_odometer_km());
    update_uptime(trip_persist_get_uptime_ms());
}

/* Pull cruise-control state out of the periodic COMM_LISP_GET_STATS reply.
 * The Lisp app on the VESC publishes these names (see main.lisp in the
 * Super_VESC_Display repo). Variables stay absent until the first valid
 * response arrives — the getters return false in that case and the
 * dashboard keeps showing CC off, which is the right default. */
static void push_cruise_locked(void)
{
    /* Current ride profile (eco / normal / sport, etc.) — Lisp publishes
     * `current-profile` as an int; the dashboard maps the index to a label
     * via update_mode_text. Independent of CC state, so update before the
     * CC block. */
    int32_t current_profile = 0;
    if (vesc_lisp_poll_get_variable_int("current-profile", &current_profile)) {
        update_mode_text((uint8_t)current_profile);
    }

    int32_t cc_active = 0;
    if (vesc_lisp_poll_get_variable_int("cruise-active", &cc_active)) {
        update_cruise_control_status(cc_active != 0);

        /* Speed text only matters when CC is engaged. Conversion mirrors
         * the original Arduino impl: speed_kmh = rpm / rpm-per-ms * 3.6.
         * rpm-per-ms is also Lisp-published and depends on motor poles /
         * wheel diameter, so we let the VESC tell us instead of duplicating
         * the math here. */
        if (cc_active) {
            float cc_rpm    = 0.0f;
            float rpm_per_ms = 0.0f;
            if (vesc_lisp_poll_get_variable_float("cruise-rpm",  &cc_rpm) &&
                vesc_lisp_poll_get_variable_float("rpm-per-ms",  &rpm_per_ms) &&
                rpm_per_ms > 0.1f) {
                update_cruise_speed(cc_rpm / rpm_per_ms * 3.6f);
            }
        }
    }
}

/* Runs inside lv_timer_handler() with the LVGL lock already held — zero
 * contention with the render task (it's the same task). 10 Hz is plenty
 * for a dashboard; numeric fields don't benefit from higher refresh. */
static void updater_lv_timer_cb(lv_timer_t *t)
{
    (void)t;
    /* BT icon reflects the real NimBLE host state regardless of demo —
     * cockpit_demo_tick no longer touches it, so no writer conflict.
     * update_ble_status dedups internally. */
    update_ble_status(ble_host_is_connected());

    /* Demo mode (Settings → Demo mode) drives the rest of the setters
     * from cockpit_demo_tick. Skip the RT pump so it doesn't overwrite
     * demo values with zeros every 100 ms. When demo turns off, force
     * zeros once so stale demo numbers don't linger. */
    if (dashboard_demo_is_active()) {
        s_zeros_pushed = false;
        return;
    }
    if (!s_zeros_pushed) {
        push_zeros_locked();
        s_zeros_pushed = true;
    }
    push_rt_locked();
    push_cruise_locked();
}

esp_err_t vesc_ui_updater_start(void)
{
    if (s_timer) return ESP_OK;
    if (bsp_display_lock(2000) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl lock timeout — cannot register update timer");
        return ESP_FAIL;
    }
    s_timer = lv_timer_create(updater_lv_timer_cb, 100, NULL);
    bsp_display_unlock();
    if (!s_timer) {
        ESP_LOGE(TAG, "lv_timer_create failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}
