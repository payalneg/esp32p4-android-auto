/*
    Copyright 2025 Super VESC Display
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4

    Port of Super_VESC_Display/src/vesc_battery_calc.cpp. Arduino Preferences
    swapped for esp-idf nvs_flash; millis() swapped for esp_timer_get_time().
    Behaviour matches the original: a single ESP32 owns one battery's worth
    of state at a time. Charge/swap is detected at boot by a jump in the pack
    voltage (v_in) versus the value seen at the previous power-on — see
    battery_calc_voltage_boot_check(). The compare happens only at startup. The
    baseline rides in the trip-log records (trip_log seeds it back in its boot
    scan, while the panel is dark); the legacy NVS "last_vin" key is only read
    (upgrade path) or written when there is no trip log at all — an nvs_commit
    as the ESC comes up is a full-screen blue flash on this board.
*/

#include "vesc_battery_calc.h"

#include <math.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "vesc_trip_persist.h"
#include "dev_settings.h"   /* settings_get_battery_capacity */

static const char *TAG = "batt_calc";

#define NVS_NAMESPACE             "battery_calc"
#define KEY_REMAINING_AH          "remaining_ah"
#define KEY_LAST_VIN              "last_vin"
#define KEY_LAST_CAPACITY         "last_capacity"
#define KEY_RESET_EPOCH           "reset_epoch"

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

/* NVS write throttle. The primary persistence path is now the trip log: every
 * 10 s record carries remaining_ah + reset epoch (written into pre-erased
 * triplog sectors — no NVS page churn, no GC erase). NVS here is only the
 * fallback for when the triplog is absent/dead, so a long interval is fine.
 * (At the old 30 s cadence the 6-entry save filled a 126-entry NVS page every
 * ~10 min → a GC page-copy + 4 KB erase stalling both cores' cache that
 * often.) */
#define SAVE_INTERVAL_US          (600ULL * 1000 * 1000)

static bool     s_initialized;

/* Charge-detection baseline plumbing (see battery_calc_seed_boot_vin). */
static bool     s_triplog_alive;          /* trip log carries boot_vin forward */
static float    s_prev_boot_vin = -1.0f;  /* previous power-on's v_in, < 0 = none */
static float    s_boot_vin;               /* this boot's first valid v_in, 0 until seen */
static battery_calc_charge_cb_t s_charge_cb;   /* NULL = auto-reset on detection */
static float    s_remaining_ah;
static float    s_last_saved_capacity;
static float    s_last_net_ah;            /* last (discharged − charged) reading */
static float    s_last_controller_percent;
static bool     s_first_calculation = true;
static bool     s_capacity_changed_flag;
static int64_t  s_last_save_us;
static bool     s_vin_boot_checked;       /* voltage charge-detect runs once per boot */
/* Bumped on every reset / capacity change and stamped into trip-log records.
 * On boot the trip log offers its last record's remaining_ah as a seed; it is
 * accepted only if the record's epoch matches ours — otherwise a reset
 * happened after that record was written and the NVS value is the truth. */
static uint32_t s_reset_epoch;

static esp_err_t open_rw(nvs_handle_t *h)  { return nvs_open(NVS_NAMESPACE, NVS_READWRITE, h); }
static esp_err_t open_ro(nvs_handle_t *h)  { return nvs_open(NVS_NAMESPACE, NVS_READONLY,  h); }

/* Deferred NVS writer. nvs_commit costs ~100 ms on this board (flash suspend
 * unavailable, so a flash write stalls cache on both cores — see
 * sdkconfig.defaults), and the percentage getters run on the LVGL thread and
 * inside the AA video frame draw: committing there freezes rendering, touch
 * and the video pipe. The getters only snapshot values into this queue; a
 * low-priority task does the actual flash I/O (same pattern as trip_log). */
typedef struct {
    bool     is_state;      /* true: remaining_ah + capacity, false: v_in */
    bool     with_epoch;    /* also persist the reset epoch (reset paths only) */
    float    remaining_ah;
    float    capacity;
    float    v_in;
    uint32_t epoch;
} save_msg_t;

static QueueHandle_t     s_save_q;
/* Recursive: the getters are called from the LVGL thread AND the AA video
 * task, and the public functions nest (voltage_boot_check → reset_trip_and_ah
 * → reset). */
static SemaphoreHandle_t s_lock;

static void lock(void)   { if (s_lock) xSemaphoreTakeRecursive(s_lock, portMAX_DELAY); }
static void unlock(void) { if (s_lock) xSemaphoreGiveRecursive(s_lock); }

static void save_task(void *arg)
{
    (void)arg;
    save_msg_t m;
    for (;;) {
        if (xQueueReceive(s_save_q, &m, portMAX_DELAY) != pdTRUE) continue;
        nvs_handle_t h;
        if (open_rw(&h) != ESP_OK) {
            ESP_LOGW(TAG, "open RW failed — state not saved");
            continue;
        }
        if (m.is_state) {
            nvs_set_blob(h, KEY_REMAINING_AH,  &m.remaining_ah, sizeof(m.remaining_ah));
            nvs_set_blob(h, KEY_LAST_CAPACITY, &m.capacity,     sizeof(m.capacity));
        } else {
            nvs_set_blob(h, KEY_LAST_VIN, &m.v_in, sizeof(m.v_in));
        }
        if (m.with_epoch) nvs_set_u32(h, KEY_RESET_EPOCH, m.epoch);
        nvs_commit(h);
        nvs_close(h);
        ESP_LOGD(TAG, "saved: %s", m.is_state ? "state" : "vin");
    }
}

/* Enqueue-only; called with s_lock held. If the queue is momentarily full the
 * save is skipped — the next throttled tick retries with fresher values. */
static void save_state(bool with_epoch)
{
    save_msg_t m = { .is_state     = true,
                     .with_epoch   = with_epoch,
                     .remaining_ah = s_remaining_ah,
                     .capacity     = s_last_saved_capacity,
                     .epoch        = s_reset_epoch };
    if (!s_save_q || xQueueSend(s_save_q, &m, 0) != pdTRUE)
        ESP_LOGW(TAG, "save queue unavailable — state save skipped");
}

static void save_vin(float v_in)
{
    save_msg_t m = { .is_state = false, .v_in = v_in };
    if (!s_save_q || xQueueSend(s_save_q, &m, 0) != pdTRUE)
        ESP_LOGW(TAG, "save queue unavailable — vin save skipped");
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
    nvs_get_u32(h, KEY_RESET_EPOCH, &s_reset_epoch);   /* stays 0 if never saved */
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
    if (!s_lock) s_lock = xSemaphoreCreateRecursiveMutex();
    if (!s_save_q) {
        s_save_q = xQueueCreate(4, sizeof(save_msg_t));
        if (!s_save_q || xTaskCreate(save_task, "batt_sav", 4096, NULL, 2, NULL) != pdPASS) {
            ESP_LOGE(TAG, "save task/queue init failed — battery state won't persist");
            if (s_save_q) { vQueueDelete(s_save_q); s_save_q = NULL; }
        }
    }

    lock();
    s_initialized           = load_state();
    s_last_net_ah           = 0.0f;
    s_first_calculation     = true;
    s_capacity_changed_flag = false;
    s_last_save_us          = 0;
    s_vin_boot_checked      = false;
    unlock();
    ESP_LOGI(TAG, "init (initialized=%d)", s_initialized);
}

void battery_calc_reset(float current_battery_percent, float battery_capacity)
{
    lock();
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
    /* New epoch: trip-log records written before this reset must not seed
     * remaining_ah on the next boot — the NVS value saved here wins. */
    s_reset_epoch++;
    save_state(true);
    unlock();
    ESP_LOGI(TAG, "reset: %.1f%% = %.2f Ah of %.1f Ah (epoch %u)",
             current_battery_percent, s_remaining_ah, battery_capacity,
             (unsigned)s_reset_epoch);
}

static float smart_percentage_locked(float controller_battery_level,
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
            /* Start the periodic-save clock from the first ESC data, not
             * from boot: with s_last_save_us == 0 an ESC that shows up more
             * than SAVE_INTERVAL after power-on would trigger an nvs_commit
             * (blue flash) on its very first tick. */
            s_last_save_us            = esp_timer_get_time();
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
        save_state(false);
        s_last_save_us = now_us;
    }
    return pct;
}

float battery_calc_get_smart_percentage(float controller_battery_level,
                                        float controller_amp_hours,
                                        float controller_amp_hours_charged,
                                        float battery_capacity)
{
    lock();
    float pct = smart_percentage_locked(controller_battery_level,
                                        controller_amp_hours,
                                        controller_amp_hours_charged,
                                        battery_capacity);
    unlock();
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
    lock();
    s_capacity_changed_flag = true;
    unlock();
    ESP_LOGI(TAG, "capacity change flagged");
}

void battery_calc_reset_trip_and_ah(void)
{
    trip_persist_reset();
    lock();
    /* Smart % is consumption-based (remaining_ah / capacity); zeroing the
     * consumed capacity treats the pack as full again, so the percentage
     * returns to 100 %. battery_calc_reset() always lands at full pack.
     * (No visible effect in Direct mode, where % is the controller's
     * voltage-based estimate, not this tracker.) */
    float cap = (s_last_saved_capacity > 0.1f) ? s_last_saved_capacity
                                               : settings_get_battery_capacity();
    battery_calc_reset(s_last_controller_percent, cap);
    unlock();
    ESP_LOGI(TAG, "reset_trip_and_ah → trip + battery to full (%.1f Ah)", cap);
}

void battery_calc_voltage_boot_check(float v_in)
{
    /* Runs once per boot. The first valid reading is compared against the pack
     * voltage seen at the previous power-on; a jump up beyond the threshold
     * means the battery was charged or swapped while we were off → roll the
     * trip over. Either way, this boot's voltage becomes the next boot's
     * baseline. Independent of the Direct/Smart percentage path.
     *
     * Baseline source: the newest trip-log record (seeded in trip_log's boot
     * scan, panel still dark). The legacy NVS key is read only when no record
     * carries a baseline yet (first boot after the update / empty log), and
     * written only when there is no trip log at all — otherwise the commit
     * would land right here, seconds after the panel lit up (blue flash). */
    if (s_vin_boot_checked) return;
    if (v_in < VIN_VALID_MIN) return;   /* ESC not really up yet — retry next tick */

    lock();
    if (s_vin_boot_checked) { unlock(); return; }   /* lost the race — already done */

    float saved = 0.0f;
    bool  have_saved = false;
    if (s_triplog_alive && s_prev_boot_vin >= VIN_VALID_MIN) {
        saved = s_prev_boot_vin;
        have_saved = true;
    } else {
        have_saved = load_vin(&saved);
    }
    bool  ask = false;          /* hand the decision to s_charge_cb after unlock */
    float ask_pct = 0.0f;
    if (have_saved) {
        float change_pct = (v_in - saved) / saved * 100.0f;
        if (change_pct > VIN_CHARGE_PCT_THRESHOLD) {
            if (s_charge_cb) {
                ESP_LOGI(TAG, "charge detected at boot: pack %.1f V → %.1f V (+%.1f%%) — asking",
                         saved, v_in, change_pct);
                ask = true;
                ask_pct = change_pct;
            } else {
                ESP_LOGI(TAG, "charge detected at boot: pack %.1f V → %.1f V (+%.1f%%) — reset trip",
                         saved, v_in, change_pct);
                battery_calc_reset_trip_and_ah();
            }
        } else {
            ESP_LOGI(TAG, "no charge at boot: pack %.1f V → %.1f V (%+.1f%%) — keep trip",
                     saved, v_in, change_pct);
        }
    } else {
        ESP_LOGI(TAG, "no saved pack voltage — baseline set to %.1f V", v_in);
    }

    s_boot_vin = v_in;         /* trip_log stamps it into every record from now on */
    if (!s_triplog_alive) {
        save_vin(v_in);        /* no trip log to carry it → legacy NVS baseline */
    }
    s_vin_boot_checked = true;
    unlock();

    /* Outside the lock: the UI callback may build widgets and later call
     * battery_calc_reset_trip_and_ah() from a button handler. */
    if (ask) s_charge_cb(saved, v_in, ask_pct);
}

void battery_calc_set_charge_detected_cb(battery_calc_charge_cb_t cb)
{
    lock();
    s_charge_cb = cb;
    unlock();
}

float battery_calc_get_remaining_ah(void)
{
    lock();
    float rem = s_remaining_ah;
    unlock();
    return rem;
}

void battery_calc_seed_boot_vin(bool triplog_alive, float prev_boot_vin)
{
    lock();
    s_triplog_alive = triplog_alive;
    s_prev_boot_vin = prev_boot_vin;
    unlock();
    ESP_LOGI(TAG, "boot-vin baseline: %s, prev power-on %.1f V",
             triplog_alive ? "trip log" : "NVS (no trip log)", prev_boot_vin);
}

float battery_calc_get_boot_vin(void)
{
    return s_boot_vin;
}

void battery_calc_get_persist(float *remaining_ah, uint32_t *epoch)
{
    lock();
    if (remaining_ah) *remaining_ah = s_remaining_ah;
    if (epoch)        *epoch        = s_reset_epoch;
    unlock();
}

void battery_calc_seed_remaining(float remaining_ah, uint32_t epoch)
{
    lock();
    if (!s_initialized) {
        /* No NVS state at all → no capacity/epoch baseline to validate the
         * seed against; let the first calculation seed from the controller. */
        unlock();
        ESP_LOGI(TAG, "seed skipped — no saved state");
        return;
    }
    if (epoch != s_reset_epoch) {
        /* A reset/capacity change happened after this record was written;
         * the NVS value saved by that reset is the fresher truth. */
        unlock();
        ESP_LOGI(TAG, "seed skipped — epoch %u != %u (reset since)",
                 (unsigned)epoch, (unsigned)s_reset_epoch);
        return;
    }
    if (remaining_ah < 0.0f ||
        (s_last_saved_capacity > 0.0f && remaining_ah > s_last_saved_capacity + 0.01f)) {
        unlock();
        ESP_LOGW(TAG, "seed skipped — implausible %.2f Ah", remaining_ah);
        return;
    }
    s_remaining_ah = remaining_ah;
    unlock();
    ESP_LOGI(TAG, "seeded from trip log: %.2f Ah remain (epoch %u)",
             remaining_ah, (unsigned)epoch);
}
