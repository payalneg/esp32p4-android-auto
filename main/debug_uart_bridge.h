#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Debug bridge over the UART0 console (esp_console REPL). Registers commands a
 * host PC drives for UI test automation / visual analysis:
 *
 *   screenshot [--fmt jpeg|rgb565] [--quality N]
 *       Capture lv_scr_act() via lv_snapshot and stream it back as a
 *       base64-framed JPEG (HW-encoded) or raw RGB565. Logging is muted for
 *       the duration of the transfer so the binary payload isn't spliced by
 *       interleaved ESP_LOG lines. See scripts/uart_debug.py for the host side.
 *
 *   tap <x> <y>
 *   swipe <x1> <y1> <x2> <y2> [ms]
 *   touchdown <x> <y> / touchup / touchmove <x> <y>
 *       Inject synthetic LVGL pointer events (touch_input_inject) in landscape
 *       coords 0..799 x 0..479.
 *
 * The bridge owns UART0 stdin (the REPL reads it). Call once, after the display
 * and touch_input are up. No-op stub unless CONFIG_DEBUG_UART_BRIDGE.
 * Idempotent — returns ESP_ERR_INVALID_STATE on a second call. */
esp_err_t debug_uart_bridge_init(void);

#ifdef __cplusplus
}
#endif
