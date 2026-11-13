#pragma once

/* Battery pack monitor backed by a JK-BMS over BLE.
 *
 * Owns:
 *   - the persisted BLE address of the pack (NVS namespace "jkbms"), written
 *     by a dedicated low-priority task so pairing from the LVGL screen never
 *     blocks the display on a flash commit;
 *   - the glue between ble_bms_client (radio + decoded frames) and
 *     vesc_battery_calc (the "where does the dashboard % come from" switch).
 *
 * Currents and power for the ride still come from the VESC. This module only
 * supplies the pack's own SOC / remaining Ah / full Ah (and the detail values
 * shown on the battery screen), plus the remaining-range input. */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BMS_BATTERY_MAX_CELLS 32

typedef struct {
    bool     bound;
    bool     connected;
    bool     scanning;
    bool     fresh;            /* valid status frame within the last 10 s */
    bool     has_data;

    float    soc;              /* percent, NAN when unknown */
    float    remaining_ah;
    float    full_ah;
    float    voltage;
    float    current;          /* A, positive = charging */
    float    power;
    float    temp1, temp2, temp_mos;
    float    cell_min, cell_max, cell_delta;
    int      cell_min_index, cell_max_index;
    int      cell_count;
    float    cells[BMS_BATTERY_MAX_CELLS];
    uint32_t cycles;
    float    soh;
    bool     charging_mos, discharging_mos, balancing, heating;
    char     model[20];
    uint32_t age_ms;
} bms_telem_t;

typedef void (*bms_scan_cb_t)(const uint8_t addr[6], uint8_t addr_type,
                              const char *name, int8_t rssi);

/* Load the bound pack from NVS, bind it, and register the battery-source
 * callback. Call from app_main after ble_host_init(), next to
 * speed_sensor_init(). */
void bms_battery_init(void);

void bms_battery_scan_start(void);
void bms_battery_scan_stop(void);
void bms_battery_set_scan_cb(bms_scan_cb_t cb);

/* Pair with a pack (persists the address) / forget it. */
void bms_battery_select(const uint8_t addr[6], uint8_t addr_type);
void bms_battery_forget(void);
bool bms_battery_is_bound(void);

void bms_battery_get(bms_telem_t *out);

#ifdef __cplusplus
}
#endif
