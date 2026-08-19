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
static bool     s_uptime_supported;   /* VESC reports uptime_ms (sticky once seen) */
static void   (*s_reset_cb)(void);
static void   (*s_reset_cb2)(void);   /* second listener (BLE trip reset) */

/* A VESC uptime that went back by more than this is a controller reboot, not
 * jitter between two CAN replies. */
#define REBOOT_UPTIME_SLACK_MS  1000u

/* Fallback reboot test for VESC firmware that doesn't report uptime: the
 * tachometer must have RESTARTED (dropped to near zero), not merely dipped —
 * a dip on its own is a garbled/out-of-order CAN reply. */
#define REBOOT_TACH_RESTART_M   1000.0f

/* Clear every running-total field to a clean slate. Shared by init and reset:
 * both start counting from zero relative to the live VESC counters on the next
 * update. */
static void zero_state(void)
{
    s_trip_offset_meters  = 0.0f;
    s_ah_offset           = 0.0f;
    s_uptime_offset_ms    = 0;
    s_current_vesc_trip   = 0.0f;
    s_current_vesc_ah     = 0.0f;
    s_current_vesc_uptime = 0;
    s_first_update        = true;
    s_have_saved_state    = false;
    /* s_uptime_supported is a property of the connected VESC, not of the trip —
     * deliberately NOT cleared here so a reset mid-ride doesn't fall back to
     * the weaker tachometer-only reboot test. */
}

void trip_persist_init(void)
{
    zero_state();
    s_initialized = true;
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

    if (vesc_uptime_ms > 0u) s_uptime_supported = true;

    if (s_first_update) {
        if (s_have_saved_state) {
            float saved_trip   = s_trip_offset_meters;
            float saved_ah     = s_ah_offset;
            uint32_t saved_up  = s_uptime_offset_ms;
            /* offset = (saved total) - (raw counter right now). It is EXPECTED
             * to come out negative: the VESC keeps counting while we are off,
             * so its raw counters are normally far ahead of the trip we are
             * resuming. These three used to be clamped at 0 "to be safe", which
             * made the dashboard display the VESC's raw counters instead of the
             * trip — a 10 km ride showed 255 km / 6 h whenever the display
             * rebooted while the controller stayed powered. Unsigned wrap on
             * the uptime offset is intentional and cancels out in the getter. */
            s_trip_offset_meters = saved_trip - vesc_trip_meters;
            s_ah_offset          = saved_ah   - vesc_amp_hours;
            s_uptime_offset_ms   = saved_up   - vesc_uptime_ms;
            ESP_LOGI(TAG, "offsets: trip=%.2f m, Ah=%.2f, uptime=%u ms (raw tach=%.0f m, up=%u ms)",
                     s_trip_offset_meters, s_ah_offset, (unsigned)s_uptime_offset_ms,
                     vesc_trip_meters, (unsigned)vesc_uptime_ms);
        } else {
            /* No saved baseline — fresh boot with an empty log, or the first
             * tick right after trip_persist_reset(). The VESC counters are
             * monotonic, so baseline the offset to their current value: the
             * displayed totals start at 0 and count up from here. Without this
             * the totals snap to the VESC's lifetime counters (the "reset
             * doesn't stick" bug). */
            s_trip_offset_meters = -vesc_trip_meters;
            s_ah_offset          = -vesc_amp_hours;
            s_uptime_offset_ms   = (uint32_t)(0u - vesc_uptime_ms);   /* unsigned wrap → display 0 */
            ESP_LOGI(TAG, "baselined to zero: tach=%.0f m, Ah=%.2f, uptime=%u ms",
                     vesc_trip_meters, vesc_amp_hours, (unsigned)vesc_uptime_ms);
        }
        s_first_update = false;
        /* Adopt the raw counters as the baseline in the same tick so the reboot
         * test below never compares this first reading against zeros. */
        s_current_vesc_trip   = vesc_trip_meters;
        s_current_vesc_ah     = vesc_amp_hours;
        s_current_vesc_uptime = vesc_uptime_ms;
        return;
    }

    /* Mid-run VESC reboot: the raw counters restart at zero, so fold the last
     * pre-reboot readings into the offsets and the displayed totals stay
     * continuous.
     *
     * The reboot is decided by ONE signal — the VESC's uptime going backwards —
     * because that is the event that resets all three counters at once. Folding
     * each counter on its own dip (the old behaviour) meant a single garbled or
     * out-of-order CAN reply added the VESC's entire raw tachometer (hundreds
     * of km) to the trip, permanently. */
    bool rebooted;
    if (s_uptime_supported) {
        rebooted = (vesc_uptime_ms + REBOOT_UPTIME_SLACK_MS < s_current_vesc_uptime);
    } else {
        /* No uptime from this VESC: require the tachometer to have restarted
         * near zero rather than merely dipped. */
        rebooted = (vesc_trip_meters < s_current_vesc_trip - 1.0f) &&
                   (vesc_trip_meters < REBOOT_TACH_RESTART_M);
    }

    if (rebooted) {
        ESP_LOGI(TAG, "VESC reboot: folding tach=%.0f m, Ah=%.2f, uptime=%u ms",
                 s_current_vesc_trip, s_current_vesc_ah,
                 (unsigned)s_current_vesc_uptime);
        s_trip_offset_meters += s_current_vesc_trip;
        s_ah_offset          += s_current_vesc_ah;
        s_uptime_offset_ms   += s_current_vesc_uptime;

        s_current_vesc_trip   = vesc_trip_meters;
        s_current_vesc_ah     = vesc_amp_hours;
        s_current_vesc_uptime = vesc_uptime_ms;
        return;
    }

    /* No reboot → the raw counters are monotonic on the controller, so a value
     * that came back smaller is a bad frame (the shared per-id CAN reassembly
     * buffer can hand us a short/interleaved reply). Hold the last good reading
     * instead of letting the dashboard jump backwards. */
    if (vesc_trip_meters   > s_current_vesc_trip)   s_current_vesc_trip   = vesc_trip_meters;
    if (vesc_amp_hours     > s_current_vesc_ah)     s_current_vesc_ah     = vesc_amp_hours;
    if (vesc_uptime_ms     > s_current_vesc_uptime) s_current_vesc_uptime = vesc_uptime_ms;
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
    zero_state();
    ESP_LOGI(TAG, "reset complete");

    if (s_reset_cb) s_reset_cb();   /* roll the trip logger over to a new trip */
    if (s_reset_cb2) s_reset_cb2(); /* zero the BLE wheel-sensor trip */
}

void trip_persist_set_reset_cb(void (*cb)(void))
{
    s_reset_cb = cb;
}

void trip_persist_add_reset_cb(void (*cb)(void))
{
    s_reset_cb2 = cb;
}
