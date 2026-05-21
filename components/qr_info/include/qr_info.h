#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Build the standard Wi-Fi QR string for the SoftAP we're hosting:
 *   WIFI:T:WPA;S:<ssid>;P:<password>;H:false;;
 * Returns false if SoftAP is not running. */
bool qr_info_get_wifi_qr(char *out, size_t out_sz);

/* Build the URL where the OTA HTTP server is reachable, e.g.
 *   http://android-auto.local/
 * Uses the mDNS-published hostname so the URL is human-friendly on
 * macOS/iOS/Linux/Windows-with-Bonjour. Android Chrome does NOT resolve
 * .local — Android users fall back to the IP shown alongside the QR on
 * the idle screen. Port is omitted when 80, included otherwise. Returns
 * false if the SoftAP netif has no IP yet. */
bool qr_info_get_ota_url(char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif
