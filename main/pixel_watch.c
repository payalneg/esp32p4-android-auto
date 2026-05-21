/* Pixel watcher — detects whether the light-blue screen flashes seen
 * during CAN activity are caused by something writing into our PSRAM
 * framebuffer (software bug) or by the MIPI-DSI link/panel itself
 * glitching (hardware: EMI / power supply dip / sync loss).
 *
 * We can't read back from the panel — DSI is one-way. But the panel's
 * source-of-truth is the framebuffer in PSRAM. So we poll pixel (0,0)
 * of each LVGL framebuffer at 50 Hz, expect the dashboard's near-black
 * background (RGB565 ~ 0x0021), and ESP_LOGW any deviation.
 *
 * Verdict after running with CAN active:
 *   - log fires DURING blue flash  → software is writing blue to FB
 *   - log silent DURING blue flash → FB is fine, panel/DSI is glitching
 *     (hardware fix required: decoupling near CAN transceiver, ferrite
 *     on CAN line, common-mode choke, separate DSI from CAN routing)
 */

#include "pixel_watch.h"

#include "bsp/esp-bsp.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "pixwatch";

/* Pixels are RGB565. Panel-native upper-left corner sits in the dashboard
 * statusbar area, which is the dashboard bg color 0x07090A. Converted to
 * RGB565: ((7>>3)<<11) | ((9>>2)<<5) | (10>>3) = 0 | (2<<5) | 1 = 0x0041.
 * LV_COLOR_16_SWAP is enabled in our lv_conf, so the byte order in
 * memory is little-endian-swapped to 0x4100. */
#define EXPECTED_PIXEL_BG   0x4100u
/* "Anything notably non-dark" — green channel > 32 (half of 6-bit range)
 * AND blue channel > 16. The light blue we're hunting (0x64C8FF) maps to
 * RGB565 ~ 0x665F (byte-swapped 0x5F66): G6 = 50, B5 = 31. Plenty of
 * margin above ambient text/widget colors on the dashboard. */
static inline bool looks_like_blue_flash(uint16_t px)
{
    /* Undo LV_COLOR_16_SWAP first. */
    uint16_t native = (px << 8) | (px >> 8);
    uint8_t g6 = (native >> 5) & 0x3F;
    uint8_t b5 = native & 0x1F;
    return g6 > 32 && b5 > 16;
}

static void pixel_watch_task(void *arg)
{
    void *fb0 = NULL;
    void *fb1 = NULL;

    /* Wait for the panel / FBs to be ready. bsp_display_start has run by
     * the time app_main spawns us, but esp_lcd_dpi_panel_get_frame_buffer
     * can still return NULL pointers if a flush hasn't happened yet. */
    while (!fb0) {
        esp_lcd_panel_handle_t panel = bsp_display_get_panel_handle();
        if (panel) {
            esp_lcd_dpi_panel_get_frame_buffer(panel, 2, &fb0, &fb1);
        }
        if (!fb0) vTaskDelay(pdMS_TO_TICKS(200));
    }
    ESP_LOGI(TAG, "watching FBs: fb0=%p fb1=%p", fb0, fb1);

    /* Cache line size — we only read 2 bytes, but esp_cache_msync requires
     * the address and length to be aligned. Smallest valid alignment for
     * PSRAM cache on ESP32-P4 is 64 bytes. Just sync a single line per FB
     * containing pixel (0,0). */
    const size_t LINE = 64;
    uint16_t last_logged_p0 = EXPECTED_PIXEL_BG;
    uint16_t last_logged_p1 = EXPECTED_PIXEL_BG;

    for (;;) {
        /* Invalidate CPU cache for the first line of each FB so we read
         * what the DMA actually sees in PSRAM, not stale cached bytes. */
        esp_cache_msync(fb0, LINE,
                        ESP_CACHE_MSYNC_FLAG_DIR_M2C |
                        ESP_CACHE_MSYNC_FLAG_INVALIDATE);
        if (fb1) {
            esp_cache_msync(fb1, LINE,
                            ESP_CACHE_MSYNC_FLAG_DIR_M2C |
                            ESP_CACHE_MSYNC_FLAG_INVALIDATE);
        }

        uint16_t p0 = *(volatile uint16_t *)fb0;
        uint16_t p1 = fb1 ? *(volatile uint16_t *)fb1 : 0;

        if (looks_like_blue_flash(p0) && p0 != last_logged_p0) {
            ESP_LOGW(TAG, "fb0 (0,0) = 0x%04X — looks like blue flash", p0);
            last_logged_p0 = p0;
        } else if (!looks_like_blue_flash(p0)) {
            last_logged_p0 = EXPECTED_PIXEL_BG;
        }
        if (fb1 && looks_like_blue_flash(p1) && p1 != last_logged_p1) {
            ESP_LOGW(TAG, "fb1 (0,0) = 0x%04X — looks like blue flash", p1);
            last_logged_p1 = p1;
        } else if (fb1 && !looks_like_blue_flash(p1)) {
            last_logged_p1 = EXPECTED_PIXEL_BG;
        }

        vTaskDelay(pdMS_TO_TICKS(20));  /* 50 Hz poll */
    }
}

esp_err_t pixel_watch_start(void)
{
    BaseType_t ok = xTaskCreatePinnedToCore(
        pixel_watch_task, "pixwatch", 3072, NULL, 1, NULL, 0);
    return (ok == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}
