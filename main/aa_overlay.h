#pragma once

#include <stdint.h>

/* Paint a translucent VESC HUD onto a panel-native RGB565 framebuffer.
 * Two compact widgets sit in the AA bottom-bar gaps (the empty zones
 * between mic and the app icon cluster, and between the cluster and the
 * clock): "64 KM/H" on the left, a small battery icon + "78%" on the right.
 *
 * The framebuffer is assumed to already hold a 90° CW-rotated AA video
 * frame (480 wide × 800 tall in panel-native coords; the user looks at it
 * in landscape — 800 wide × 480 tall). Coordinates are mapped from
 * user-landscape to panel-native inside this function.
 *
 * Pulls values from cockpit_get_speed_value / cockpit_get_battery_proc_value
 * so the HUD always agrees with the dashboard (demo mode included).
 *
 * Pure CPU; ~1-2 ms per call on ESP32-P4 @400 MHz. Caller is responsible
 * for cache flushing the buffer afterwards before handing it to the LCD
 * DMA. */
void aa_overlay_draw(uint16_t *fb);

/* Non-zero key that changes exactly when the rendered overlay would: speed,
 * battery %, cruise flag, units. Lets the display stage skip the ~9 ms
 * redraw while nothing it shows has moved. */
uint32_t aa_overlay_content_key(void);
