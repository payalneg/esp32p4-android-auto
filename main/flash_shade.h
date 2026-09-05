#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Backlight off while the P4 writes or erases its own flash.
 *
 * Every write/erase of the main flash stalls the MIPI-DSI scan-out — the
 * cache is held off for the duration and the panel's DMA loses its
 * framebuffer feed — so the panel shows a blue / garbage frame for as long as
 * the op runs (an NVS commit is ~100 ms, an OTA erase seconds). Flash
 * auto-suspend, which would let the fetches through, cannot be enabled on
 * this flash chip (see sdkconfig.defaults). So the stall is hidden instead:
 * the backlight goes dark just before the op and comes back a short hold
 * after the last one, once a clean frame has been scanned out.
 *
 * Interposed on the flash driver's entry points with the linker's --wrap
 * (main/CMakeLists.txt): NVS, littlefs, the trip-log partition and OTA
 * writes are all covered without touching their call sites. Reads are not
 * wrapped, and neither are writes under 1 KB: a single page program stalls
 * the panel for a dozen scan lines at most, and the small writers (trip-log
 * records every 10 s, NVS entries) would otherwise blink the screen all day.
 *
 * Arm once the display is up and the saved brightness is known; before that
 * the wrappers are pass-through. */
void flash_shade_arm(void);

#ifdef __cplusplus
}
#endif
