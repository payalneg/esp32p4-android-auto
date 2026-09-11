/*
    Copyright 2026 ESP32-P4 Android Auto / VESC Display (GPL-3.0).

    Dual-head helper. A dual-motor board is two VESC nodes on one CAN bus.
    The display polls the primary head (target_vesc_id) via SETUP_SELECTIVE —
    which the VESC firmware already aggregates (Ah/Wh/currents) across all CAN
    nodes whose STATUS is <100 ms fresh. Temperatures, however, are NOT
    aggregated: each node reports only its own. So the second head's temps and
    liveness are read passively from its periodic CAN STATUS_4 broadcast.

    Requires the user to enable "Send status over CAN" on the second head.
    All functions return false when the second head is disabled or its STATUS
    is stale, so callers degrade to single-head behaviour automatically.

    Over a BLE adapter link there are no broadcasts to listen to — the link is
    point-to-point — so the same temperatures are polled instead, once a
    second, through the adapter's CAN bus. Callers see no difference.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fills *temp_fet / *temp_motor (either may be NULL) from the second head's
 * latest STATUS_4 and returns true, iff the second head is enabled in
 * settings and its STATUS_4 is fresh. Otherwise returns false and leaves the
 * outputs untouched. */
bool vesc_head2_get_temps(float *temp_fet, float *temp_motor);

/* True iff the second head is enabled and currently broadcasting fresh STATUS.
 * Used by the connection-state logic ("ESC NOT CONNECTED" if any head silent). */
bool vesc_head2_is_fresh(void);

/* Poll hook for the BLE link, driven by the single VESC poll task (rt_task);
 * a no-op on CAN, where the second head broadcasts on its own, and whenever
 * the second head is disabled. Registered by main.c. */
void vesc_head2_poll_loop(void);

/* Reply hook, fed from the same dispatcher as every other VESC reply. Gates
 * on the command byte and ignores anything else. */
void vesc_head2_process_response(const uint8_t *data, unsigned int len);

#ifdef __cplusplus
}
#endif
