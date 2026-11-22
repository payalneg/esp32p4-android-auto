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

/* Conservative bounding box (inclusive, panel-native pixels) of every pixel
 * aa_overlay_draw() may read or write, for the current display_flip setting.
 * The box is a narrow strip along one edge of the portrait framebuffer: PPA
 * painted the frame with DMA (bypassing the CPU cache), and this is the only
 * region the CPU reads back while blending the HUD. Callers use it to
 * invalidate just that strip instead of the whole 768 KB frame. */
void aa_overlay_visible_panel_rect(int *col0, int *col1,
                                   int *row0, int *row1);
