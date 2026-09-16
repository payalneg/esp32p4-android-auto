/* Capture the live panel to a JPEG file.
 *
 * What the rider was looking at when something went wrong is the one piece of
 * a problem report that cannot be reconstructed from a log: "the speed was
 * blank" and "the speed read 0" are different faults with the same words. The
 * companion app asks for a shot over the file channel (ble_files) and attaches
 * it next to the logs.
 *
 * The same picture the UART debug bridge sends, written to a file instead of
 * the console — both go through lv_snapshot and the board's JPEG encoder.
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Render the active screen and write it to `path` as JPEG.
 *
 * Runs on the caller's task and takes the LVGL lock for the snapshot itself,
 * so do not call it from the LVGL worker. The frame buffer (panel area x 2
 * bytes) and the encoder output are allocated in PSRAM for the duration and
 * freed before returning — a shot is not free, but it costs nothing between
 * shots.
 *
 * Returns ESP_OK, or ESP_ERR_NO_MEM / ESP_ERR_TIMEOUT / ESP_FAIL with the
 * reason logged. `quality` is 1..100; 80 is a good picture at about a tenth of
 * the raw size.
 */
esp_err_t screen_capture_to_file(const char *path, int quality);

/* Where the app looks for the shot it just asked for. On the LittleFS
 * partition, so it survives a reboot and can be picked up later. */
#define SCREEN_CAPTURE_PATH "/vescfs/report-screen.jpg"

/* The head unit's own log, dumped beside the picture by the same request. The
 * app attaches both, plus its own screenshot and journal, so a report carries
 * what the panel showed AND what it was saying at the time. */
#define REPORT_LOG_PATH     "/vescfs/report-log.txt"

/* Dump the in-PSRAM log ring to REPORT_LOG_PATH. Same caveats as above: not
 * from the LVGL worker, and it needs the app filesystem mounted. */
esp_err_t report_log_to_file(void);

#ifdef __cplusplus
}
#endif
