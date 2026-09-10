#pragma once

#include "sdkconfig.h"

#define MODE_BT_CLASSIC       1
#define MODE_WIRELESS_HELPER  2

#ifndef CONNECTION_MODE
#define CONNECTION_MODE MODE_WIRELESS_HELPER
#endif

/* Real AA head unit listens on 5288. Wireless Helper APK has this port
 * hardcoded — it grabs only the IP from mDNS and ignores the published port. */
#define AA_TCP_PORT            5288
/* Hostname/instance the device answers to over mDNS. Comes from Kconfig
 * (CONFIG_WEB_MDNS_HOSTNAME) because the dashboard-only ESP32-S3 board is not
 * an "android-auto" host — see main/Kconfig.projbuild. components/qr_info gets
 * the same value injected through its CMakeLists. */
#define AA_MDNS_HOSTNAME       CONFIG_WEB_MDNS_HOSTNAME
#if CONFIG_AA_ENABLE
#define AA_MDNS_INSTANCE_NAME  "ESP32-P4 Android Auto"
#else
#define AA_MDNS_INSTANCE_NAME  "Super VESC Display"
#endif
/* Wireless Helper APK browses for _aawireless._tcp; this is the de-facto
 * service type used by hostapd-based AA dongles. */
#define AA_MDNS_SERVICE_TYPE   "_aawireless"
#define AA_MDNS_PROTO          "_tcp"

/* Audio service advertisement. Gearhead 1.7 on the Nothing A142 hard-rejects
 * a head unit without audio channels — observed: SD response 168 bytes (no
 * audio), ~950 ms later recv_decrypted ERR_INVALID_STATE and the client
 * closes the TCP socket. Has to stay 1 for gearhead to project at all.
 * Toggling this off only makes sense if we ever find a phone/version pair
 * that doesn't require audio. */
#define ENABLE_AUDIO           1

/* Per-stream audio control. Per openauto's ServiceFactory the three audio
 * AVChannels are independent: System (type 2 — UI beeps) is mandatory,
 * Speech (type 1 — nav/voice prompts, low-bandwidth 16 kHz mono) and
 * Media (type 3 — music/podcasts, 48 kHz stereo, the heavy one) can be
 * dropped individually without breaking projection.
 *
 * We don't have an audio sink and the user wants media to play through
 * the phone's headphones anyway, so drop Media to avoid streaming ~1 MB/min
 * of audio bytes that compete with the video path on TCP. Phone routes
 * Media back to its own AudioManager when the channel isn't advertised. */
#define ENABLE_AUDIO_MEDIA     0  /* 48 kHz stereo music — dropped */
#define ENABLE_AUDIO_SPEECH    0  /* 16 kHz mono nav prompts / Google Assistant —
                                     dropped so navigation TTS stays on the phone
                                     headphones instead of getting ack'd into the
                                     void on our head unit. */
#define ENABLE_AUDIO_SYSTEM    1  /* UI beeps — gearhead requires this */

/* Dashboard wall clock (cur_time_label + Time row in Settings + VBAT
 * AUTO-mode routing for LP domain). Set to 0 if the CR2032 on H8 is
 * draining too fast — disabling shuts off the LP-on-VBAT routing so
 * the LP domain runs from VDDA only, eliminating any battery drain
 * path through the chip. Coin cell still leaks ~30 µA via R52 → EN
 * which is unavoidable in hardware. */
#define ENABLE_WALL_CLOCK      0
