/*
    Copyright 2025 Super VESC Display
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4

    Port of Super_VESC_Display/src/vesc_trip_persist.cpp. NVS via esp-idf
    nvs_flash; timing via esp_timer_get_time(). The "current VESC value +
    offset" pattern is unchanged from the original — when the VESC reboots
    and its tachometer/Ah/uptime drop, we fold the previous reading into
    the offset so the displayed totals don't jump backwards.
*/

#include "vesc_trip_persist.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "trip_persist";

#define NS                "trip_persist"
#define KEY_TRIP_TOTAL    "trip_total"
#define KEY_AH_TOTAL      "ah_total"
#define KEY_UPTIME_TOTAL  "uptime_total"

/* NVS save throttle: same 10 s the Arduino original used. */
#define SAVE_INTERVAL_US  (10ULL * 1000 * 1000)

static bool     s_initialized;
static float    s_trip_offset_meters;
static float    s_ah_offset;
static uint32_t s_uptime_offset_ms;
static float    s_current_vesc_trip;
static float    s_current_vesc_ah;
static uint32_t s_current_vesc_uptime;
static int64_t  s_last_save_us;
static bool     s_first_update = true;

/* On the very first trip_persist_update() after a boot with saved state,
 * we don't yet know the VESC's current counter values. The load step
 * parks saved totals into the *_offset variables; the first update
 * converts those into real offsets ( = saved_total - current_vesc_value ),
 * which is how the original Arduino code worked. */
static bool     s_have_saved_state;

static esp_err_t open_rw(nvs_handle_t *h) { return nvs_open(NS, NVS_READWRITE, h); }
static esp_err_t open_ro(nvs_handle_t *h) { return nvs_open(NS, NVS_READONLY,  h); }

static bool load_state(void)
{
    nvs_handle_t h;
    if (open_ro(&h) != ESP_OK) return false;

    size_t sz = sizeof(float);
    float trip_total = -1.0f;
    float ah_total   = -1.0f;
    uint32_t uptime_total = 0;
    nvs_get_blob(h, KEY_TRIP_TOTAL,   &trip_total,   &sz); sz = sizeof(float);
    nvs_get_blob(h, KEY_AH_TOTAL,     &ah_total,     &sz);
    nvs_get_u32 (h, KEY_UPTIME_TOTAL, &uptime_total);
    nvs_close(h);

    if (trip_total < 0.0f || ah_total < 0.0f) {
        ESP_LOGI(TAG, "no saved trip state");
        return false;
    }

    /* Stash the saved totals into the offset vars; the first update will
     * convert them into proper offsets relative to the live VESC counters. */
    s_trip_offset_meters = trip_total;
    s_ah_offset          = ah_total;
    s_uptime_offset_ms   = uptime_total;
    ESP_LOGI(TAG, "loaded: trip=%.2f m, Ah=%.2f, uptime=%u ms",
             trip_total, ah_total, (unsigned)uptime_total);
    return true;
}

static void save_state(void)
{
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    float trip_total    = s_current_vesc_trip   + s_trip_offset_meters;
    float ah_total      = s_current_vesc_ah     + s_ah_offset;
    uint32_t up_total   = s_current_vesc_uptime + s_uptime_offset_ms;
    nvs_set_blob(h, KEY_TRIP_TOTAL,   &trip_total, sizeof(trip_total));
    nvs_set_blob(h, KEY_AH_TOTAL,     &ah_total,   sizeof(ah_total));
    nvs_set_u32 (h, KEY_UPTIME_TOTAL, up_total);
    nvs_commit(h);
    nvs_close(h);
}

void trip_persist_init(void)
{
    s_have_saved_state = load_state();
    if (!s_have_saved_state) {
        s_trip_offset_meters = 0.0f;
        s_ah_offset          = 0.0f;
        s_uptime_offset_ms   = 0;
    }
    s_current_vesc_trip   = 0.0f;
    s_current_vesc_ah     = 0.0f;
    s_current_vesc_uptime = 0;
    s_last_save_us        = 0;
    s_first_update        = true;
    s_initialized         = true;
    ESP_LOGI(TAG, "init (have_saved=%d)", s_have_saved_state);
}

void trip_persist_update(float vesc_trip_meters,
                         float vesc_amp_hours,
                         uint32_t vesc_uptime_ms)
{
    if (!s_initialized) return;

    if (s_first_update) {
        if (s_have_saved_state) {
            /* Reinterpret the parked saved totals as offsets. saved_total
             * already includes everything pre-reboot, so the offset to add
             * to live counters is saved_total - current_vesc. */
            float saved_trip   = s_trip_offset_meters;
            float saved_ah     = s_ah_offset;
            uint32_t saved_up  = s_uptime_offset_ms;
            s_trip_offset_meters = saved_trip   - vesc_trip_meters;
            s_ah_offset          = saved_ah     - vesc_amp_hours;
            s_uptime_offset_ms   = saved_up     - vesc_uptime_ms;
            if (s_trip_offset_meters < 0.0f) s_trip_offset_meters = 0.0f;
            if (s_ah_offset          < 0.0f) s_ah_offset          = 0.0f;
            /* uint32 underflow guard: if the VESC's uptime is *higher* than
             * what we saved, the subtraction wraps to a huge number — start
             * fresh in that case. */
            if (saved_up < vesc_uptime_ms) s_uptime_offset_ms = 0;
            ESP_LOGI(TAG, "offsets: trip=%.2f m, Ah=%.2f, uptime=%u ms",
                     s_trip_offset_meters, s_ah_offset, (unsigned)s_uptime_offset_ms);
        }
        s_first_update = false;
    }

    /* Detect mid-run VESC reboot: counters that suddenly drop fold into the
     * offset so totals stay monotonic. 1 m / 0.01 Ah / 1 s tolerance keeps
     * sensor noise from triggering false detections. */
    if (vesc_trip_meters < s_current_vesc_trip - 1.0f) {
        ESP_LOGI(TAG, "VESC trip reset: %.2f → %.2f m", s_current_vesc_trip, vesc_trip_meters);
        s_trip_offset_meters += s_current_vesc_trip;
    }
    if (vesc_amp_hours < s_current_vesc_ah - 0.01f) {
        ESP_LOGI(TAG, "VESC Ah reset: %.2f → %.2f", s_current_vesc_ah, vesc_amp_hours);
        s_ah_offset += s_current_vesc_ah;
    }
    if (vesc_uptime_ms + 1000u < s_current_vesc_uptime) {
        ESP_LOGI(TAG, "VESC uptime reset: %u → %u ms",
                 (unsigned)s_current_vesc_uptime, (unsigned)vesc_uptime_ms);
        s_uptime_offset_ms += s_current_vesc_uptime;
    }

    s_current_vesc_trip   = vesc_trip_meters;
    s_current_vesc_ah     = vesc_amp_hours;
    s_current_vesc_uptime = vesc_uptime_ms;

    int64_t now_us = esp_timer_get_time();
    if (now_us - s_last_save_us >= SAVE_INTERVAL_US) {
        save_state();
        s_last_save_us = now_us;
    }
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

    nvs_handle_t h;
    if (open_rw(&h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
    ESP_LOGI(TAG, "reset complete");
}

bool trip_persist_is_initialized(void)
{
    return s_initialized;
}
