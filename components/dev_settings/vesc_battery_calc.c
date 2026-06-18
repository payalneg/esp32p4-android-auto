/*
    Copyright 2025 Super VESC Display
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4

    Port of Super_VESC_Display/src/vesc_battery_calc.cpp. Arduino Preferences
    swapped for esp-idf nvs_flash; millis() swapped for esp_timer_get_time().
    Behaviour matches the original: a single ESP32 owns one battery's worth
    of state at a time. Charge/swap is detected at boot by a > +5 % jump in the
    pack voltage (v_in) versus the value saved at the previous power-on — see
    battery_calc_voltage_boot_check(). The save/compare happens only at startup.
*/

#include "vesc_battery_calc.h"

#include <math.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "vesc_trip_persist.h"
#include "dev_settings.h"   /* settings_get_battery_capacity */

static const char *TAG = "batt_calc";

#define NVS_NAMESPACE             "battery_calc"
#define KEY_REMAINING_AH          "remaining_ah"
#define KEY_LAST_VIN              "last_vin"
#define KEY_LAST_CAPACITY         "last_capacity"

/* Charge/swap heuristic: compare the pack voltage saved at the previous
 * power-on against this boot's voltage. A jump up of more than this (percent,
 * relative) means the pack was topped up or swapped while we were off, so the
 * trip is rolled over. Voltage (not the controller %) is the signal because it
 * is available in both Direct and Smart modes and doesn't lean on the
 * controller's own state-of-charge estimate. 1 % relative ≈ a few tenths of a
 * volt on a typical pack — enough to clear normal rest-vs-sag jitter between
 * power-ons while still catching a top-up. */
#define VIN_CHARGE_PCT_THRESHOLD  1.0f

/* Below this the v_in reading is "no valid pack voltage yet" (ESC not really
 * up): skip the boot check and retry on the next tick. */
#define VIN_VALID_MIN             1.0f

/* NVS write throttle. Flushing every tick would wear flash and hurt
 * latency for no benefit. 30 s matches the Arduino original. */
#define SAVE_INTERVAL_US          (30ULL * 1000 * 1000)

static bool     s_initialized;
static float    s_remaining_ah;
static float    s_last_saved_capacity;
static float    s_last_net_ah;            /* last (discharged − charged) reading */
static float    s_last_controller_percent;
static bool     s_first_calculation = true;
static bool     s_capacity_changed_flag;
static int64_t  s_last_save_us;
static bool     s_vin_boot_checked;       /* voltage charge-detect runs once per boot */

static esp_err_t open_rw(nvs_handle_t *h)  { return nvs_open(NVS_NAMESPACE, NVS_READWRITE, h); }
static esp_err_t open_ro(nvs_handle_t *h)  { return nvs_open(NVS_NAMESPACE, NVS_READONLY,  h); }

static void save_state(void)
{
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) {
        ESP_LOGW(TAG, "open RW failed — state not saved");
        return;
    }
    nvs_set_blob(h, KEY_REMAINING_AH,  &s_remaining_ah,        sizeof(s_remaining_ah));
    nvs_set_blob(h, KEY_LAST_CAPACITY, &s_last_saved_capacity, sizeof(s_last_saved_capacity));
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGD(TAG, "state saved: %.2f Ah of %.1f Ah", s_remaining_ah, s_last_saved_capacity);
}

static void save_vin(float v_in)
{
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_blob(h, KEY_LAST_VIN, &v_in, sizeof(v_in));
    nvs_commit(h);
    nvs_close(h);
}

static bool load_state(void)
{
    nvs_handle_t h;
    if (open_ro(&h) != ESP_OK) return false;

    size_t sz = sizeof(float);
    float rem = -1.0f;
    float cap = -1.0f;
    nvs_get_blob(h, KEY_REMAINING_AH,  &rem, &sz); sz = sizeof(float);
    nvs_get_blob(h, KEY_LAST_CAPACITY, &cap, &sz);
    nvs_close(h);

    if (rem < 0.0f) {
        ESP_LOGI(TAG, "no saved battery state");
        return false;
    }
    s_remaining_ah        = rem;
    s_last_saved_capacity = cap;
    ESP_LOGI(TAG, "loaded: %.2f Ah of %.1f Ah", s_remaining_ah, s_last_saved_capacity);
    return true;
}

static bool load_vin(float *out)
{
    nvs_handle_t h;
    if (open_ro(&h) != ESP_OK) return false;
    size_t sz = sizeof(float);
    float v = -1.0f;
    nvs_get_blob(h, KEY_LAST_VIN, &v, &sz);
    nvs_close(h);
    if (v < VIN_VALID_MIN) return false;
    *out = v;
    return true;
}

void battery_calc_init(void)
{
    s_initialized           = load_state();
    s_last_net_ah           = 0.0f;
    s_first_calculation     = true;
    s_capacity_changed_flag = false;
    s_last_save_us          = 0;
    s_vin_boot_checked      = false;
    ESP_LOGI(TAG, "init (initialized=%d)", s_initialized);
}

void battery_calc_reset(float current_battery_percent, float battery_capacity)
{
    /* Original semantics: reset always lands at full pack, not at the
     * controller %. The whole point of smart calc is to be more accurate
     * than the voltage-based controller estimate, so treating reset as
     * "we know it's full" matches user intent (capacity change / charge). */
    s_remaining_ah            = battery_capacity;
    s_last_saved_capacity     = battery_capacity;
    s_last_controller_percent = current_battery_percent;
    s_last_net_ah             = 0.0f;
    s_first_calculation       = true;
    s_capacity_changed_flag   = false;
    s_initialized             = true;
    save_state();
    ESP_LOGI(TAG, "reset: %.1f%% = %.2f Ah of %.1f Ah",
             current_battery_percent, s_remaining_ah, battery_capacity);
}

float battery_calc_get_smart_percentage(float controller_battery_level,
                                        float controller_amp_hours,
                                        float controller_amp_hours_charged,
                                        float battery_capacity)
{
    if (battery_capacity <= 0.0f) {
        ESP_LOGW(TAG, "invalid capacity %.1f — falling back to direct", battery_capacity);
        return controller_battery_level * 100.0f;
    }

    /* Net energy actually pulled from the pack = discharged − regenerated.
     * Both VESC counters are monotonic; a regen burst grows amp_hours_charged,
     * which shrinks net so the delta below goes negative and credits Ah back
     * into the estimate. Tracking net (not gross discharge) keeps the gauge
     * honest on anything with active braking. */
    float net_ah = controller_amp_hours - controller_amp_hours_charged;

    float current_controller_percent = controller_battery_level * 100.0f;

    /* Capacity change → start over at full. */
    if (s_capacity_changed_flag ||
        (s_initialized && s_last_saved_capacity > 0.0f &&
         fabsf(s_last_saved_capacity - battery_capacity) > 0.1f)) {
        ESP_LOGI(TAG, "capacity changed %.1f → %.1f Ah — resetting",
                 s_last_saved_capacity, battery_capacity);
        battery_calc_reset(current_controller_percent, battery_capacity);
        s_capacity_changed_flag = false;
        return current_controller_percent;
    }

    /* First call after boot: continue from the NVS remaining-Ah estimate, or
     * seed from the controller if we have no saved state. Charge/swap is no
     * longer detected here — battery_calc_voltage_boot_check() owns that, keyed
     * off the pack voltage and run once per boot in both Direct and Smart. */
    if (!s_initialized || s_first_calculation) {
        if (s_initialized) {
            ESP_LOGI(TAG, "continuing: %.2f Ah remain (controller now %.1f%%)",
                     s_remaining_ah, current_controller_percent);
            s_last_controller_percent = current_controller_percent;
            /* Seed the net-Ah baseline to the current reading so the first
             * delta below is ~0. Without this, s_last_net_ah is still 0 (set
             * by init/reset) and the fall-through subtracts the WHOLE absolute
             * counter in one tick — e.g. 15 Ah pack with net at 1.95 Ah would
             * jump straight to 13.05 Ah / 87 %. */
            s_last_net_ah             = net_ah;
            s_first_calculation       = false;
        } else {
            /* No saved state — seed from controller. */
            battery_calc_reset(current_controller_percent, battery_capacity);
            return current_controller_percent;
        }
    }

    s_last_controller_percent = current_controller_percent;

    /* Integrate net Ah since the last call: delta of (discharged − charged),
     * not the absolute value. A regen burst shrinks net → consumed goes
     * negative → Ah is credited back to the remaining estimate. */
    float consumed = net_ah - s_last_net_ah;
    s_last_net_ah = net_ah;
    s_remaining_ah -= consumed;

    if (s_remaining_ah < 0.0f)              s_remaining_ah = 0.0f;
    if (s_remaining_ah > battery_capacity)  s_remaining_ah = battery_capacity;

    float pct = (s_remaining_ah / battery_capacity) * 100.0f;
    if (pct < 0.0f)   pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;

    int64_t now_us = esp_timer_get_time();
    if (now_us - s_last_save_us >= SAVE_INTERVAL_US) {
        save_state();
        s_last_save_us = now_us;
    }
    return pct;
}

float battery_calc_display_percentage(float controller_battery_level,
                                      float controller_amp_hours,
                                      float controller_amp_hours_charged)
{
    /* Single source of truth for "the battery number on screen": honours the
     * Direct vs Smart setting so the cockpit, AA HUD and trip log all agree.
     * Direct forwards the controller's voltage-based estimate; Smart uses the
     * net-Ah-integrating tracker against the configured capacity. */
    if (settings_get_battery_calc_mode() == BATTERY_CALC_MODE_SMART) {
        return battery_calc_get_smart_percentage(controller_battery_level,
                                                 controller_amp_hours,
                                                 controller_amp_hours_charged,
                                                 settings_get_battery_capacity());
    }
    return controller_battery_level * 100.0f;
}

bool battery_calc_is_initialized(void)
{
    return s_initialized;
}

void battery_calc_capacity_changed(void)
{
    s_capacity_changed_flag = true;
    ESP_LOGI(TAG, "capacity change flagged");
}

void battery_calc_reset_trip_and_ah(void)
{
    trip_persist_reset();
    /* Smart % is consumption-based (remaining_ah / capacity); zeroing the
     * consumed capacity treats the pack as full again, so the percentage
     * returns to 100 %. battery_calc_reset() always lands at full pack.
     * (No visible effect in Direct mode, where % is the controller's
     * voltage-based estimate, not this tracker.) */
    float cap = (s_last_saved_capacity > 0.1f) ? s_last_saved_capacity
                                               : settings_get_battery_capacity();
    battery_calc_reset(s_last_controller_percent, cap);
    ESP_LOGI(TAG, "reset_trip_and_ah → trip + battery to full (%.1f Ah)", cap);
}

void battery_calc_voltage_boot_check(float v_in)
{
    /* Runs once per boot. The first valid reading is compared against the pack
     * voltage stored at the previous power-on; a jump up beyond the threshold
     * means the battery was charged or swapped while we were off → roll the
     * trip over. Either way, this boot's voltage becomes the next boot's
     * baseline. Independent of the Direct/Smart percentage path. */
    if (s_vin_boot_checked) return;
    if (v_in < VIN_VALID_MIN) return;   /* ESC not really up yet — retry next tick */

    float saved = 0.0f;
    if (load_vin(&saved)) {
        float change_pct = (v_in - saved) / saved * 100.0f;
        if (change_pct > VIN_CHARGE_PCT_THRESHOLD) {
            ESP_LOGI(TAG, "charge detected at boot: pack %.1f V → %.1f V (+%.1f%%) — reset trip",
                     saved, v_in, change_pct);
            battery_calc_reset_trip_and_ah();
        } else {
            ESP_LOGI(TAG, "no charge at boot: pack %.1f V → %.1f V (%+.1f%%) — keep trip",
                     saved, v_in, change_pct);
        }
    } else {
        ESP_LOGI(TAG, "no saved pack voltage — baseline set to %.1f V", v_in);
    }

    save_vin(v_in);            /* baseline for the next power-on */
    s_vin_boot_checked = true;
}

float battery_calc_get_remaining_ah(void)
{
    return s_remaining_ah;
}
