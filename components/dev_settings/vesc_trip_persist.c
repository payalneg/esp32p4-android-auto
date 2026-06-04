/*
    Copyright 2025 Super VESC Display
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4

    Trip / Ah / uptime running totals with the "current VESC value + offset"
    pattern: when the VESC reboots and its tachometer/Ah/uptime drop, we fold the
    previous reading into the offset so displayed totals stay continuous.

    Persistence is NO LONGER here. The raw trip log (main/trip_log.c) writes a
    record every 10 s carrying these totals; on boot it calls
    trip_persist_seed_totals() to resume. This keeps the periodic flash writes in
    one place (the raw log, whose erases are kept off the ride) and the flash I/O
    on a dedicated writer task rather than the LVGL thread.
*/

#include "vesc_trip_persist.h"

#include "esp_log.h"

static const char *TAG = "trip_persist";

static bool     s_initialized;
static float    s_trip_offset_meters;
static float    s_ah_offset;
static uint32_t s_uptime_offset_ms;
static float    s_current_vesc_trip;
static float    s_current_vesc_ah;
static uint32_t s_current_vesc_uptime;
static bool     s_first_update = true;
static bool     s_have_saved_state;
static void   (*s_reset_cb)(void);

void trip_persist_init(void)
{
    s_trip_offset_meters  = 0.0f;
    s_ah_offset           = 0.0f;
    s_uptime_offset_ms    = 0;
    s_current_vesc_trip   = 0.0f;
    s_current_vesc_ah     = 0.0f;
    s_current_vesc_uptime = 0;
    s_first_update        = true;
    s_have_saved_state    = false;
    s_initialized         = true;
    ESP_LOGI(TAG, "init (totals resumed from the raw trip log)");
}

void trip_persist_seed_totals(float trip_total_m, float ah_total, uint32_t uptime_total_ms)
{
    if (trip_total_m < 0.0f || ah_total < 0.0f) return;
    /* Park the saved totals into the offset vars; the first update converts them
     * into proper offsets relative to the live VESC counters. */
    s_trip_offset_meters = trip_total_m;
    s_ah_offset          = ah_total;
    s_uptime_offset_ms   = uptime_total_ms;
    s_have_saved_state   = true;
    s_first_update       = true;
    ESP_LOGI(TAG, "seeded: trip=%.2f m, Ah=%.2f, uptime=%u ms",
             trip_total_m, ah_total, (unsigned)uptime_total_ms);
}

void trip_persist_update(float vesc_trip_meters,
                         float vesc_amp_hours,
                         uint32_t vesc_uptime_ms)
{
    if (!s_initialized) return;

    if (s_first_update) {
        if (s_have_saved_state) {
            float saved_trip   = s_trip_offset_meters;
            float saved_ah     = s_ah_offset;
            uint32_t saved_up  = s_uptime_offset_ms;
            s_trip_offset_meters = saved_trip - vesc_trip_meters;
            s_ah_offset          = saved_ah   - vesc_amp_hours;
            s_uptime_offset_ms   = saved_up   - vesc_uptime_ms;
            if (s_trip_offset_meters < 0.0f) s_trip_offset_meters = 0.0f;
            if (s_ah_offset          < 0.0f) s_ah_offset          = 0.0f;
            if (saved_up < vesc_uptime_ms) s_uptime_offset_ms = 0;
            ESP_LOGI(TAG, "offsets: trip=%.2f m, Ah=%.2f, uptime=%u ms",
                     s_trip_offset_meters, s_ah_offset, (unsigned)s_uptime_offset_ms);
        }
        s_first_update = false;
    }

    /* Detect mid-run VESC reboot: counters that suddenly drop fold into the
     * offset so totals stay monotonic. */
    if (vesc_trip_meters < s_current_vesc_trip - 1.0f) {
        s_trip_offset_meters += s_current_vesc_trip;
    }
    if (vesc_amp_hours < s_current_vesc_ah - 0.01f) {
        s_ah_offset += s_current_vesc_ah;
    }
    if (vesc_uptime_ms + 1000u < s_current_vesc_uptime) {
        s_uptime_offset_ms += s_current_vesc_uptime;
    }

    s_current_vesc_trip   = vesc_trip_meters;
    s_current_vesc_ah     = vesc_amp_hours;
    s_current_vesc_uptime = vesc_uptime_ms;
}

float trip_persist_get_trip_km(void)
{
    if (!s_initialized) return 0.0f;
    return (s_current_vesc_trip + s_trip_offset_meters) / 1000.0f;
}

float trip_persist_get_amp_hours(void)
{
    if (!s_initialized) return 0.0f;
    return s_current_vesc_ah + s_ah_offset;
}

uint32_t trip_persist_get_uptime_ms(void)
{
    if (!s_initialized) return 0;
    return s_current_vesc_uptime + s_uptime_offset_ms;
}

void trip_persist_reset(void)
{
    s_trip_offset_meters  = 0.0f;
    s_ah_offset           = 0.0f;
    s_uptime_offset_ms    = 0;
    s_current_vesc_trip   = 0.0f;
    s_current_vesc_ah     = 0.0f;
    s_current_vesc_uptime = 0;
    s_first_update        = true;
    s_have_saved_state    = false;
    ESP_LOGI(TAG, "reset complete");

    if (s_reset_cb) s_reset_cb();   /* roll the trip logger over to a new trip */
}

void trip_persist_set_reset_cb(void (*cb)(void))
{
    s_reset_cb = cb;
}

bool trip_persist_is_initialized(void)
{
    return s_initialized;
}
