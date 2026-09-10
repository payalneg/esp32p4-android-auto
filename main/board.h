#pragma once

#include "sdkconfig.h"

/* Compile-time board identity.
 *
 * The active board is chosen by the Kconfig `choice BOARD_MODEL`
 * (CONFIG_BOARD_WAVESHARE_43 / CONFIG_BOARD_JC4880P443C /
 * CONFIG_BOARD_S3_TOUCH_LCD_4), set per build via the
 * sdkconfig.defaults.<board> overlay — see scripts/build_board.sh.
 *
 * BOARD_MODEL_ID is the short slug the firmware advertises to the companion app
 * (BLE OTA-info characteristic + GET /info) so the app can pick the matching
 * bundled firmware image during over-WiFi OTA. Keep these strings in sync with
 * the asset names in flutter-application/ (esp32p4_android_auto-<id>.bin) and
 * the release artifact names in scripts/release.sh. */
#if CONFIG_BOARD_JC4880P443C
#define BOARD_MODEL_ID    "jc4880"
#define BOARD_MODEL_NAME  "Guition JC4880P443C"
#elif CONFIG_BOARD_S3_TOUCH_LCD_4
#define BOARD_MODEL_ID    "s3touch4"
#define BOARD_MODEL_NAME  "Waveshare ESP32-S3-Touch-LCD-4"
#else
#define BOARD_MODEL_ID    "waveshare"
#define BOARD_MODEL_NAME  "Waveshare ESP32-P4 4.3\""
#endif
