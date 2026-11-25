#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Debug bridge over Wi-Fi: a plain TCP server that
 *
 *   - streams the ESP_LOG history and everything logged from then on
 *     (components/log_capture's PSRAM ring) to every connected client, so the
 *     board can be watched with no cable attached — the case that matters when
 *     the phone owns the USB port, e.g. while flashing firmware over it, or
 *     when the unit is buried behind the dash;
 *   - takes the same commands the serial bridge registers (screenshot, tap,
 *     swipe, tasks, lvtimers, uimode, navstat...) and runs them through
 *     debug_cmds.c, the one implementation both transports share.
 *
 * Wire protocol (scripts/wifi_debug.py speaks it):
 *   server -> client, on connect: a few "# " banner lines, then the last
 *     CONFIG_DEBUG_WIFI_BRIDGE_BACKLOG_KB of the log, then live log bytes;
 *   server -> client, for a screenshot: "SCR-BEGIN w= h= fmt= len= crc32="
 *     line, then exactly len raw bytes, then "\nSCR-END\n" (logging muted for
 *     the transfer, like the serial bridge's base64 framing);
 *   client -> server: one command per line; after it completes the server
 *     sends "# done" ("# err <code>" for a failing command).
 *
 * Call once, after the network is up (wifi_manager_wait_ready). No-op stub
 * unless CONFIG_DEBUG_WIFI_BRIDGE; a second call returns ESP_ERR_INVALID_STATE. */
esp_err_t debug_wifi_start(void);

#ifdef __cplusplus
}
#endif
