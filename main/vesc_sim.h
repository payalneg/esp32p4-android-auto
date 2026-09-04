#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Synthetic VESC controller. vesc_sim_start() spawns the drive-cycle /
 * physics model and a VESC node that answers the head unit's polls in wire
 * format with realistic bus latency. Pair with
 * comm_can_start_virtual(ctrl_id, vesc_sim_can_tx) — the node is the far end
 * of that virtual bus. */
esp_err_t vesc_sim_start(void);
void      vesc_sim_stop(void);

/* comm_can_virtual_tx_t: every frame the head unit transmits lands here. */
void vesc_sim_can_tx(uint32_t eid, const uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif
