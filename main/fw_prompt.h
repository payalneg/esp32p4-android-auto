#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* "New firmware available" prompt on lv_layer_top().
 *
 * The companion app compares the firmware bundled in the APK with the version
 * reported on the OTA-info characteristic and, when the APK is newer, offers it
 * on the fw-prompt characteristic (NotifBridge ...000E). The card this module
 * builds sits over the dashboard / navigator, with Update / Later / Skip
 * buttons. A tap is sent back to the phone, which flashes the image over BLE;
 * progress and failures arrive as further writes on the same characteristic.
 *
 * The BLE callbacks run on the NimBLE host task, so nothing here touches LVGL
 * from them: a message is copied under a short critical section and an LVGL
 * timer builds/updates the card on the LVGL task. In Android Auto mode LVGL is
 * paused, so the card waits until the dashboard is up again. */
esp_err_t fw_prompt_init(void);

/* Newline-separated message from the phone:
 *   OFFER\n<current>\n<available>\n<title>\n<body>\n<now>\n<later>\n<skip>\n<updating>
 *   PROGRESS\n<0..100>
 *   FAILED\n<text>
 * The buffer is parsed in place, so it must be writable (the NimBLE write
 * callback hands over its flatten buffer). Safe to call from that task. */
void fw_prompt_handle_text(char *text);

/* Link dropped — an update can no longer run, so take the card away. */
void fw_prompt_on_disconnect(void);

#ifdef __cplusplus
}
#endif
