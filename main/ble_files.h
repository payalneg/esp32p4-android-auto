/* BLE file manager for the P4 — browse/transfer the device filesystem from the
 * phone over the existing NotifBridge BLE link. Two characteristics on the
 * NotifBridge service: FILE-CTRL (...0009, WRITE|NOTIFY) for requests + status,
 * FILE-DATA (...000A, WRITE_NO_RSP) for raw upload bytes. See ble_files.c for
 * the wire protocol; it mirrors the Flutter side in lib/ble/file_*.dart.
 *
 * Slow filesystem work (opendir/readdir, fread for download, fwrite for upload)
 * and all BLE notifications run on a dedicated worker task — never on the
 * NimBLE host callback, which must stay responsive (same discipline as
 * ble_ota.c). */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ble_files_init(void);
void ble_files_set_link(uint16_t conn_handle, uint16_t ctrl_val_handle);
void ble_files_on_disconnect(void);

/* Routed from notif_bridge access_cb (NimBLE host task) — must not block or
 * notify; they only memcpy + enqueue. */
void ble_files_ctrl_write(const uint8_t *data, uint16_t len);
void ble_files_data_write(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif