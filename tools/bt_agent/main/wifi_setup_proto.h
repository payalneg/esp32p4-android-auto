#pragma once

#include <stdint.h>
#include <stddef.h>

/* AA Wireless setup messages — protobuf wire format borrowed from
 * `WirelessAndroidAutoDongle/.../proto/`. We hand-encode/parse since the
 * messages are tiny (a handful of strings + varints) and pulling in nanopb
 * for this would be overkill. */

/* Message type identifiers used in the 4-byte SPP frame header. */
typedef enum {
    AA_WSM_INVALID                = -1,
    AA_WSM_WIFI_START_REQUEST     = 1,
    AA_WSM_WIFI_INFO_REQUEST      = 2,
    AA_WSM_WIFI_INFO_RESPONSE     = 3,
    AA_WSM_WIFI_VERSION_REQUEST   = 4,
    AA_WSM_WIFI_VERSION_RESPONSE  = 5,
    AA_WSM_WIFI_CONNECT_STATUS    = 6,
    AA_WSM_WIFI_START_RESPONSE    = 7,
} aa_wsm_id_t;

/* SecurityMode enum — matches WifiInfoResponse.proto. */
typedef enum {
    AA_WSEC_UNKNOWN          = 0,
    AA_WSEC_OPEN             = 1,
    AA_WSEC_WEP_64           = 2,
    AA_WSEC_WEP_128          = 3,
    AA_WSEC_WPA_PERSONAL     = 4,
    AA_WSEC_WPA2_PERSONAL    = 8,
    AA_WSEC_WPA_WPA2_PERSONAL = 12,
} aa_wifi_security_t;

/* AccessPointType — STATIC = phone joins our existing network,
 * DYNAMIC = HU spun up an AP just for this session. */
typedef enum {
    AA_WAPT_STATIC  = 0,
    AA_WAPT_DYNAMIC = 1,
} aa_wifi_apt_t;

/* WifiStartRequest{ ip_address, port } — encode into out, return bytes used. */
size_t aa_wsm_build_start_request(uint8_t *out, size_t cap,
                                  const char *ip, uint32_t port);

/* WifiInfoResponse{ ssid, key, bssid, security_mode, access_point_type } —
 * encode into out, return bytes used. */
size_t aa_wsm_build_info_response(uint8_t *out, size_t cap,
                                  const char *ssid, const char *key,
                                  const char *bssid,
                                  aa_wifi_security_t security,
                                  aa_wifi_apt_t apt);
