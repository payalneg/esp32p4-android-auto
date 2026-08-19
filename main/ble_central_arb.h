/* Central-connect arbiter — shares NimBLE's single connect-initiator between
 * the BLE sensor clients (cadence + wheel speed).
 *
 * NimBLE allows exactly one outstanding ble_gap_connect() (and none while a
 * scan runs): a second caller gets BLE_HS_EALREADY / BLE_HS_EBUSY. Each
 * sensor client used to keep its own connect pending with BLE_HS_FOREVER
 * while the sensor sleeps, which starves any other central client forever.
 * This module owns that resource:
 *
 *   - one registered contender → identical behaviour to before: a connect
 *     pends with BLE_HS_FOREVER until the sensor wakes and advertises;
 *   - two contenders both waiting → connects are issued with a bounded
 *     window (ARB_WINDOW_MS) and rotated round-robin on every timeout, so
 *     whichever sensor wakes first gets linked within ~2 windows.
 *
 * GAP events are delivered through a thin trampoline that updates the
 * arbiter's pending state and then forwards to the owning client's callback
 * unchanged — the clients' discovery/subscribe/notify code is untouched.
 *
 * Scanning stays client-owned (settings screens are modal, so two selection
 * scans can never overlap); the scanning client must wrap the scan with
 * ble_arb_scan_suspend()/ble_arb_scan_resume() instead of cancelling the
 * pending connect itself.
 *
 * Threading: same model as the sensor clients — called from the NimBLE host
 * task and the LVGL task; tiny state under a portMUX, NimBLE calls made
 * outside the lock (NimBLE itself rejects a racing double-connect, which the
 * evaluator tolerates and retries on the next GAP event). */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "host/ble_gap.h"

#ifdef __cplusplus
extern "C" {
#endif

void ble_arb_init(void);

/* Called from ble_host's sync_cb BEFORE the clients' on_ble_sync hooks. */
void ble_arb_on_sync(uint8_t own_addr_type);

/* Register a client's GAP event callback; returns a slot id (>= 0), or -1
 * when the slot table is full. Call once, from the client's init. */
int ble_arb_register(ble_gap_event_fn *cb);

/* Ask for a connection to `peer` (copied). Idempotent — safe to call again
 * on every re-arm point; the arbiter issues the actual ble_gap_connect when
 * the initiator is free. */
void ble_arb_want_connect(int id, const ble_addr_t *peer);

/* Withdraw the request (sensor forgotten / rebinding). Cancels the in-flight
 * connect attempt if this slot owns it. */
void ble_arb_stop_connect(int id);

/* Wrap a selection scan: suspend cancels any pending connect attempt and
 * parks the arbiter; resume re-evaluates and re-issues the connect. */
void ble_arb_scan_suspend(void);
void ble_arb_scan_resume(void);

#ifdef __cplusplus
}
#endif
