/*
    Copyright 2025 Super VESC Display

    Lifetime odometer that lives on the display instead of on the controller.

    The VESC keeps its own `odometer` counter and only writes it to its flash
    every so often, so a key-off straight after a ride regularly loses the last
    few hundred metres — and there is nothing on the display side to recover
    it from. This module keeps a mirror that is checkpointed into the raw trip
    log (components/trip_log) every ~10 s, exactly like the trip totals, so a
    power cut costs at most the last sample interval.

    At runtime the mirror is "base + trip": `base` is the lifetime distance at
    the moment the current trip read zero, and the trip distance comes from
    trip_persist, which already folds VESC reboots and bad frames. A trip reset
    folds the finished trip into `base` (see the pre-reset hook in
    vesc_trip_persist) so the displayed total never steps backwards, and
    odo_persist_set_km() re-bases it when the rider edits the number.

    The checkpoint stores the ABSOLUTE odometer, not the base. That matters on
    the one boot that follows a trip reset near a power cut: the log can end up
    with a checkpoint from the old trip next to a sample from the new one (or
    the other way round), and a base+sample pair from two different trips would
    add the finished trip twice or drop it. An absolute value is correct in
    either order — the boot only uses it to re-derive `base`, and the trip
    distance cancels out of the displayed total.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void odo_persist_init(void);

/* Restore from the newest odometer checkpoint of the raw trip log. Must be
 * called AFTER trip_persist_seed_totals(), because the base is derived from
 * the trip total that was just resumed. Without it the first VESC reading
 * becomes the base (first boot after this feature landed). */
void odo_persist_seed_m(uint32_t odo_m);

/* Feed the controller's own `odometer` (metres) once per RT tick. Only a
 * plausible pair of readings from the very first boot with no record is used —
 * to derive the base — after that the mirror runs on trip_persist. A zero
 * reading (no odometer in this firmware, or a short frame) is ignored rather
 * than adopted. */
void odo_persist_update(uint32_t vesc_odometer_m);

/* True once the mirror has a base (seeded from the log, adopted from the VESC,
 * or set by the rider). No checkpoint is written before that, so a boot with
 * no odometer data cannot lock in a zero. */
bool odo_persist_has_base(void);

/* Lifetime distance, base + current trip — this is what the trip log
 * checkpoints. */
uint32_t odo_persist_get_m(void);
float    odo_persist_get_km(void);

/* Re-base the mirror so the dashboard shows exactly `km` right now (Settings →
 * Odometer). Persisted on the next checkpoint. */
void odo_persist_set_km(float km);

/* Checkpoint bookkeeping for the trip logger: a set/reset/first-base marks the
 * state dirty, and the logger clears the flag once it has queued a record. */
bool odo_persist_checkpoint_due(void);
void odo_persist_checkpoint_taken(void);

#ifdef __cplusplus
}
#endif
