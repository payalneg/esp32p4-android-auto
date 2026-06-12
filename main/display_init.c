#include "display_init.h"

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "display_init";

static lv_display_t *s_display;

esp_err_t display_init(void)
{
    if (s_display) {
        return ESP_OK;
    }

    /* Rotate 90° clockwise so the 800×480 panel reads landscape (480 high
     * × 800 wide on the user side).
     *
     * DOUBLE_FULL (not TRIPLE_PARTIAL): the partial-rotate path freezes the
     * screen on P4. It hangs on TWO infinite waits, and only one is fixable
     * by a patch:
     *   1. PPA SRM rotate ISR loss — FIXED by the Espressif PPA patch
     *      (sr_macro_bk_ro_bypass in ppa_srm.c).
     *   2. DMA2D front→back copy ISR loss (lvgl_bridge_v8.c
     *      copy_unrendered_area_from_front_to_back → display_bridge_dma2d_copy_sync,
     *      portMAX_DELAY) — NOT covered by the patch, still hangs.
     * Re-tested 2026-06-12 WITH the PPA patch applied: still froze on spinbox
     * taps. So partial rendering is off the table until #2 has a fix too.
     * Cost of DOUBLE_FULL: full 800×480 software re-render on every dirty
     * frame — mitigate by deduping the UI setters, not by switching modes. */
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_90,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_FULL,
    };
    /* Pin the LVGL worker to core 0. The H.264 decoder library spawns a
     * helper task at priority 17 pinned to core 1 (CONFIG_ESP_H264_DUAL_TASK*)
     * and h264_pipe's wrapper task is also pinned to core 1; without this
     * affinity the no-affinity LVGL adapter would migrate onto core 1 and get
     * preempted by the decoder for the entire duration of a frame, blocking
     * the bsp_display_lock for hundreds of ms and stalling vesc_ui_updater. */
    cfg.lv_adapter_cfg.task_core_id = 0;
    s_display = bsp_display_start_with_config(&cfg);
    if (!s_display) {
        ESP_LOGE(TAG, "bsp_display_start failed");
        return ESP_FAIL;
    }

    /* Backlight off while the framebuffer is still uninitialised — that's
     * what causes the 1-2 s of white flash at boot. We turn it on once
     * the first frame has rendered our black background. */
    bsp_display_backlight_off();

    /* Generous timeout — LVGL task may still be holding the lock right
     * after esp_lv_adapter_start() returns. */
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl lock timeout during init");
        return ESP_FAIL;
    }

    /* Paint the LVGL active screen black so later widgets land on black,
     * not LVGL's default white. */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    bsp_display_unlock();

    /* Give LVGL a couple of render cycles to push the black background to
     * the panel before lighting the backlight — partial-rotate flush can
     * take more than one frame to settle. Otherwise we'd briefly unmask
     * whatever stale data was in the framebuffer. */
    vTaskDelay(pdMS_TO_TICKS(200));
    bsp_display_backlight_on();

    return ESP_OK;
}

struct _lv_display_t *display_get(void)
{
    return (struct _lv_display_t *)s_display;
}
