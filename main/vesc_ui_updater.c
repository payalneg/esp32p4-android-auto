#include "vesc_ui_updater.h"

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui_mode.h"
#include "vesc_can/vesc_rt_data.h"

/* GUI Guider's custom.h is generated C++-friendly but pure C. The
 * widget update functions live in Super_VESC_Display/custom/custom.c
 * which is built as part of vesc_ui. */
#include "custom.h"

static const char *TAG = "vesc_ui_upd";

static lv_timer_t *s_timer;
static bool        s_zeros_pushed;

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

    update_speed(vesc_rt_data_get_speed_kmh());
    update_current(rt->current_in);
    /* battery_level is 0..1 in VESC protocol → display wants percent. */
    update_battery_proc(rt->battery_level * 100.0f);
    update_trip(vesc_rt_data_get_trip_km());
    /* No range estimator yet — shown as Wh/km efficiency placeholder. */
    update_range(0.0f);
    update_temp_fet(rt->temp_mos);
    update_temp_motor(rt->temp_motor);
    update_amp_hours(vesc_rt_data_get_amp_hours());
    update_battery_temp(rt->temp_mos);
    update_battery_voltage(rt->v_in);
    update_odometer(vesc_rt_data_get_odometer_km());
    update_uptime(vesc_rt_data_get_uptime_ms());
}

/* Runs inside lv_timer_handler() with the LVGL lock already held — zero
 * contention with the render task (it's the same task). 10 Hz is plenty
 * for a dashboard; numeric fields don't benefit from higher refresh. */
static void updater_lv_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_zeros_pushed) {
        push_zeros_locked();
        s_zeros_pushed = true;
    }
    push_rt_locked();
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
