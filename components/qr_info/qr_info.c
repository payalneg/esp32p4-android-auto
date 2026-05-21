/* Connection-info helpers for the on-screen QR code page.
 *
 * Kept in its own component (rather than `main/`) to break what would
 * otherwise be a circular link dep: vesc_ui's QR screen needs these
 * helpers, but `main` already privately requires vesc_ui. Standalone
 * component → vesc_ui REQUIRES it, no cycle. */

#include "qr_info.h"

#include <stdio.h>
#include <string.h>

#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "sdkconfig.h"

static bool ap_gateway_ip(char *ip_out, size_t ip_sz)
{
    esp_netif_t *ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (!ap) return false;
    esp_netif_ip_info_t info = {0};
    if (esp_netif_get_ip_info(ap, &info) != ESP_OK) return false;
    if (info.ip.addr == 0) return false;
    snprintf(ip_out, ip_sz, IPSTR, IP2STR(&info.ip));
    return true;
}

bool qr_info_get_wifi_qr(char *out, size_t out_sz)
{
    /* Pull SSID/password straight from the active WiFi AP config —
     * works whether or not main/wifi_manager exposes them. */
    wifi_config_t cfg = {0};
    if (esp_wifi_get_config(WIFI_IF_AP, &cfg) != ESP_OK) return false;
    if (cfg.ap.ssid_len == 0 && cfg.ap.ssid[0] == '\0') return false;

    char ssid[33];
    size_t slen = cfg.ap.ssid_len ? cfg.ap.ssid_len : strnlen((char *)cfg.ap.ssid, 32);
    if (slen > 32) slen = 32;
    memcpy(ssid, cfg.ap.ssid, slen);
    ssid[slen] = '\0';

    char pass[65];
    strlcpy(pass, (const char *)cfg.ap.password, sizeof(pass));

    const char *auth = (cfg.ap.authmode == WIFI_AUTH_OPEN || pass[0] == '\0')
                       ? "nopass" : "WPA";
    int n = snprintf(out, out_sz,
                     "WIFI:T:%s;S:%s;P:%s;H:false;;",
                     auth, ssid, pass);
    return n > 0 && (size_t)n < out_sz;
}

bool qr_info_get_ota_url(char *out, size_t out_sz)
{
    /* Gate on AP-up so a stale URL isn't shown after a failed Wi-Fi bring-up. */
    char ip[16];
    if (!ap_gateway_ip(ip, sizeof(ip))) return false;
#ifdef CONFIG_OTA_HTTP_PORT
    int port = CONFIG_OTA_HTTP_PORT;
#else
    int port = 80;
#endif
    /* Lowercase URL — some QR scanners only treat lowercase "http://" as a
     * tappable URL even though the RFC says scheme+host are case-insensitive.
     * Tradeoff: byte mode QR is ~30% larger than alphanumeric (which requires
     * all-uppercase), but the user-experience win of "scan → tap → open" beats
     * the slight extra density.
     *
     * Host is mDNS-published name (see main/mdns_advertise.c) so the URL is
     * human-friendly on macOS/iOS/Linux/Windows-with-Bonjour. Android Chrome
     * does NOT resolve .local — Android users have to fall back to the IP
     * shown alongside the QR on the idle screen. */
    int n = (port == 80)
            ? snprintf(out, out_sz, "http://%s.local",    AA_MDNS_HOSTNAME)
            : snprintf(out, out_sz, "http://%s.local:%d", AA_MDNS_HOSTNAME, port);
    return n > 0 && (size_t)n < out_sz;
}
