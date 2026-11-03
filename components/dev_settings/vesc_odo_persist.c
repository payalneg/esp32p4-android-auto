/*
    Copyright 2025 Super VESC Display

    Lifetime odometer mirror — see vesc_odo_persist.h for the design.

    Everything here runs on the LVGL task: the UI updater feeds
    odo_persist_update()/reads odo_persist_get_km() from the dashboard timer,
    the Settings dialog calls odo_persist_set_km(), and the trip reset hook
    fires from the same task (dashboard/charge-prompt/settings handlers). No
    locking, same as vesc_trip_persist.c.
*/

#include "vesc_odo_persist.h"

#include "esp_log.h"

#include "vesc_trip_persist.h"

static const char *TAG = "odo_persist";

/* Metres in one trip_persist kilometre. */
#define TRIP_M_PER_KM  1000.0

/* Two consecutive raw readings further apart than this are not the same
 * controller telling us where it is (a 100 ms tick covers a few metres, and
 * the VESC steps its counter in whole metres) — most likely a garbled frame.
 * Adopting one would install a nonsense lifetime total, so wait for a pair
 * that agrees. */
#define ADOPT_MAX_STEP_M  1000u

static bool     s_initialized;
static bool     s_based;         /* s_base_m is meaningful */
static bool     s_dirty;         /* a checkpoint is due */
static double   s_base_m;        /* lifetime distance when the trip read zero */
static bool     s_have_raw;      /* one raw reading seen, waiting for a second */
static uint32_t s_last_raw_m;

/* Trip resets happen mid-ride (battery swap detected, the reset button, the
 * charge prompt). Fold the finished trip into the base BEFORE trip_persist
 * zeroes, otherwise the lifetime total would drop by the whole trip: the
 * display is base + trip, so base += trip while trip is about to become 0
 * leaves the shown number exactly where it was. */
static void odo_on_trip_reset(float trip_km)
{
    if (!s_initialized) return;
    /* Before the first base exists there is nothing to fold — it will be
     * adopted from the controller's own counter on the next RT tick. */
    if (!s_based) return;
    if (!(trip_km > 0.0f)) return;          /* also rejects NaN */
    s_base_m += (double)trip_km * TRIP_M_PER_KM;
    s_dirty   = true;
    ESP_LOGI(TAG, "trip reset: folded %.2f km into the base → odo %.1f km",
             (double)trip_km, s_base_m / 1000.0);
}

void odo_persist_init(void)
{
    s_initialized = true;
    s_based       = false;
    s_dirty       = false;
    s_base_m      = 0.0;
    s_have_raw    = false;
    s_last_raw_m  = 0;

    /* Pre-reset listener (separate from trip_log's / speed_sensor's
     * post-reset callbacks, which cannot see the old trip total). */
    trip_persist_add_pre_reset_cb(odo_on_trip_reset);
    ESP_LOGI(TAG, "init (mirror is checkpointed by the trip log)");
}

void odo_persist_seed_m(uint32_t odo_m)
{
    if (!s_initialized) return;
    /* The base is whatever makes base + (the trip just resumed) equal the
     * checkpointed absolute value. Deliberately NOT "absolute - last sample's
     * trip": the checkpoint and the newest sample can belong to different
     * trips after a reset near a power cut, and this form stays right either
     * way (the trip term cancels out of the displayed total). */
    double trip_m = (double)trip_persist_get_trip_km() * TRIP_M_PER_KM;
    s_base_m   = (double)odo_m - trip_m;
    s_based    = true;
    s_dirty    = false;                      /* the log already carries it */
    s_have_raw = false;
    ESP_LOGI(TAG, "resumed: odo %.1f km (trip %.3f km → base %.1f km)",
             (double)odo_m / 1000.0, trip_m / 1000.0, s_base_m / 1000.0);
}

void odo_persist_update(uint32_t vesc_odometer_m)
{
    if (!s_initialized || s_based) return;

    /* 0 means "no odometer from this controller" (or a frame that did not
     * carry the field). Adopting it would pin the dashboard at 0 forever, so
     * leave the mirror unbased and try again on the next tick. */
    if (vesc_odometer_m == 0) return;

    /* No record in the log: this is the first boot with the mirror (or the
     * trip log is unavailable). Adopt the controller's counter — minus what
     * the trip has already counted — so the number on the dashboard does not
     * jump when the feature lands. Two readings have to agree first; see
     * ADOPT_MAX_STEP_M. */
    if (s_have_raw) {
        uint32_t step = (vesc_odometer_m > s_last_raw_m)
                            ? (vesc_odometer_m - s_last_raw_m)
                            : (s_last_raw_m - vesc_odometer_m);
        if (step <= ADOPT_MAX_STEP_M) {
            double trip_m = (double)trip_persist_get_trip_km() * TRIP_M_PER_KM;
            s_base_m = (double)vesc_odometer_m - trip_m;
            s_based  = true;
            s_dirty  = true;
            ESP_LOGI(TAG, "adopted VESC odometer: raw %.1f km, trip %.3f km → base %.1f km",
                     (double)vesc_odometer_m / 1000.0, trip_m / 1000.0, s_base_m / 1000.0);
        }
    }
    s_have_raw   = true;
    s_last_raw_m = vesc_odometer_m;
}

bool odo_persist_has_base(void)
{
    return s_initialized && s_based;
}

uint32_t odo_persist_get_m(void)
{
    if (!s_initialized || !s_based) return 0;

    double m = s_base_m + (double)trip_persist_get_trip_km() * TRIP_M_PER_KM;
    if (!(m > 0.0)) return 0;                /* negative / NaN after a wild set */
    if (m > 4294967000.0) m = 4294967000.0;
    return (uint32_t)m;
}

float odo_persist_get_km(void)
{
    return (float)odo_persist_get_m() / 1000.0f;
}

void odo_persist_set_km(float km)
{
    if (!s_initialized) return;
    if (!(km >= 0.0f)) return;               /* NaN / negative → ignore */

    double trip_m = (double)trip_persist_get_trip_km() * TRIP_M_PER_KM;
    s_base_m = (double)km * TRIP_M_PER_KM - trip_m;
    s_based  = true;
    s_dirty  = true;
    s_have_raw = false;
    ESP_LOGI(TAG, "set: odo %.1f km (trip %.3f km → base %.1f km)",
             (double)km, trip_m / 1000.0, s_base_m / 1000.0);
}

bool odo_persist_checkpoint_due(void)
{
    return s_initialized && s_based && s_dirty;
}

void odo_persist_checkpoint_taken(void)
{
    s_dirty = false;
}
