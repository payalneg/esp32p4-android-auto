/* Connection-interval boost for bulk transfers over the phone's BLE link.
 *
 * The phone writes one chunk per ATT round trip, so on a bulk transfer the
 * connection interval *is* the throughput: Android's default is around 45 ms,
 * and it refuses anything below 11.25 ms. Both bulk users of the link (the
 * firmware update in ble_ota.c and the navigator frames in ble_nav.c) ask for
 * the fast interval from this side; the app asks from its side too, because
 * either end's request may be ignored.
 *
 * Nested: the last caller to release drops the link back to a balanced
 * interval, so an OTA started while frames are streaming does not lose the
 * boost when the frame path releases it. */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Ask for (on = true) or release (on = false) the fast interval on `conn`.
 *
 * `suspend_scan` additionally parks the sensor-connect arbiter, whose idle
 * initiator scan runs at 100 % duty and shares the radio with this link. Right
 * for a transfer measured in minutes and then done (OTA); wrong for something
 * that runs for a whole ride (frames), where a cadence or speed sensor waking
 * up must still be able to connect. */
void ble_link_boost_request(uint16_t conn_handle, bool on, bool suspend_scan);

/* Drop all outstanding requests — the link is gone. */
void ble_link_boost_reset(void);

#ifdef __cplusplus
}
#endif
