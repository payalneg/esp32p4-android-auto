#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Synthetic VESC data source. When started, spawns a FreeRTOS task that
 * runs a scripted drive cycle and physics model, then pushes the result
 * into vesc_rt_data via vesc_rt_data_inject() at ~20 Hz. */
esp_err_t vesc_sim_start(void);
void      vesc_sim_stop(void);

#ifdef __cplusplus
}
#endif
