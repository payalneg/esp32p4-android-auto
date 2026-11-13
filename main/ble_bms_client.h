#pragma once

/* BLE central / GATT-client for a JK-BMS (Jikong) battery.
 *
 * Service 0xFFE0, characteristic 0xFFE1 (some units expose separate write-only
 * and notify-only instances of the same UUID). After subscribing we request
 * status frames with the fixed 20-byte command
 *
 *     AA 55 90 EB 96 ... crc      cell info / SOC / Ah / voltage / current
 *     AA 55 90 EB 97 ... crc      device info / model
 *
 * and reassemble the 300-byte replies, which the BMS splits across several
 * ATT notifications. Validation is the JK 8-bit sum checksum.
 *
 * This module owns only the radio link and the decoded snapshot. Persistence
 * of the bound address and the battery-source integration live in
 * bms_battery.c.
 *
 * Connection lifecycle mirrors ble_speed_client: bind an address -> request a
 * connection through ble_central_arb (the initiator is shared with the other
 * central clients) -> discover/subscribe -> poll; on disconnect re-arm.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_BMS_MAX_CELLS 32

/* Live pack state (safe from any task; backed by a spinlock). */
typedef struct {
    bool     bound;
    bool     connected;        /* link up + subscribed */
    bool     scanning;
    bool     fresh;            /* a valid cell-info frame within the last 10 s */
    bool     has_data;         /* at least one valid frame since boot */

    float    soc;              /* 0..100 */
    float    remaining_ah;
    float    full_ah;
    float    voltage;          /* V */
    float    current;          /* A, positive = charging */
    float    power;            /* W, positive = charging */
    float    temp1, temp2;     /* deg C, NAN when absent */
    float    temp_mos;         /* deg C, NAN when absent */
    float    cell_min, cell_max, cell_delta;   /* V */
    int      cell_min_index, cell_max_index;   /* 1-based, 0 = unknown */
    int      cell_count;
    float    cells[BLE_BMS_MAX_CELLS];

    uint32_t cycles;
    float    soh;              /* percent, NAN when absent */
    bool     charging_mos;
    bool     discharging_mos;
    bool     balancing;
    bool     heating;

    char     model[20];        /* from the device-info frame, "" until read */
    uint32_t age_ms;           /* since the last valid status frame */
    uint32_t frames;           /* valid status frames since boot */
} ble_bms_state_t;

/* Called on the NimBLE host task for each JK-BMS advertiser found. addr is
 * 6 bytes in NimBLE native (little-endian) order. */
typedef void (*ble_bms_scan_cb_t)(const uint8_t addr[6], uint8_t addr_type,
                                  const char *name, int8_t rssi);

/* Called once from ble_host_init() before the host task starts (after
 * ble_arb_init). */
void ble_bms_client_init(void);

/* Called from ble_host's on_sync_cb once the stack is up. Re-arms a connect to
 * a previously-bound BMS, if any. */
void ble_bms_on_ble_sync(uint8_t own_addr_type);

/* Start / stop a selection scan (auto-stops after a few seconds). */
void ble_bms_scan_start(void);
void ble_bms_scan_stop(void);
void ble_bms_set_scan_cb(ble_bms_scan_cb_t cb);

/* Bind to a specific BMS and (re)connect. Does NOT persist — bms_battery.c
 * owns NVS. addr is 6 bytes native order. */
void ble_bms_bind(const uint8_t addr[6], uint8_t addr_type);

/* Forget the bound BMS: disconnect and stop reconnecting. */
void ble_bms_forget(void);

bool ble_bms_is_bound(void);

/* Copy the current pack state. */
void ble_bms_get(ble_bms_state_t *out);

#ifdef __cplusplus
}
#endif
