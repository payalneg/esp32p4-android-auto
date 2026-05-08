#pragma once

#include <stdint.h>

/* Paint a translucent VESC HUD (speed left-middle, battery right-middle)
 * onto a panel-native RGB565 framebuffer. The framebuffer is assumed to
 * already hold a 90° CW-rotated AA video frame (480 wide × 800 tall in
 * panel-native coords; the user looks at it in landscape — 800 wide ×
 * 480 tall). Coordinates are mapped from user-landscape to panel-native
 * inside this function.
 *
 * Reads vesc_rt_data; if the snapshot isn't fresh, draws "--" placeholders
 * so the user can tell the head unit is alive but the VESC is silent.
 *
 * Pure CPU; ~1-2 ms per call on ESP32-P4 @400 MHz. Caller is responsible
 * for cache flushing the buffer afterwards before handing it to the LCD
 * DMA. */
void aa_overlay_draw(uint16_t *fb);
