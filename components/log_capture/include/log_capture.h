#pragma once

#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Captures everything that goes through ESP_LOGx into a ring buffer
 * sitting in PSRAM, in addition to forwarding to the original vprintf
 * (so the UART/USB console still shows logs live). The buffer is
 * fixed-size and overwrites oldest first, so log_capture_snapshot()
 * always returns the most recent N bytes.
 *
 * Call once during early boot, after psram is initialized (i.e. inside
 * app_main, before any subsystem that needs to log on init). Safe to
 * call multiple times — second call is a no-op. */
esp_err_t log_capture_init(void);

/* Copy the most recent bytes of the log into `out`, NUL-terminated.
 * Returns the number of bytes written (excluding the NUL). If `out`
 * is smaller than what's available, the OLDEST bytes get clipped so
 * the newest log line is always at the end of `out`. Thread-safe. */
size_t   log_capture_snapshot(char *out, size_t out_size);

/* Same as log_capture_snapshot, but wraps each line in LVGL recolor
 * tags (#RRGGBB text#) based on the ESP_LOG level letter at the start
 * of the line. Renderable in an lv_label with lv_label_set_recolor=1.
 *
 *   I → info     (green)
 *   W → warning  (yellow)
 *   E → error    (red)
 *   D → debug    (light gray)
 *   V → verbose  (dim gray)
 *
 * Literal '#' characters in the log text get rewritten to '*' so the
 * recolor parser doesn't misinterpret them as tag delimiters. */
size_t   log_capture_snapshot_colorized(char *out, size_t out_size);

/* Total bytes of log captured since init, before any wrap. Useful as
 * a "did anything happen since last open" indicator. */
size_t   log_capture_total_bytes(void);

#ifdef __cplusplus
}
#endif
