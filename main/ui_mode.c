#include "ui_mode.h"

#include <stdatomic.h>

#include "bsp/esp-bsp.h"
#include "display_video.h"
#include "esp_log.h"
#include "lvgl.h"
#include "touch_input.h"
#include "vesc_ui.h"

static const char *TAG = "ui_mode";

static atomic_int s_mode = UI_MODE_VESC;
static lv_obj_t  *s_aa_screen;
static bool       s_inited;

esp_err_t ui_mode_init(void)
{
    if (s_inited) return ESP_OK;
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl lock timeout");
        return ESP_FAIL;
    }
    s_aa_screen = lv_scr_act();
    vesc_ui_init();
    lv_obj_t *vesc = vesc_ui_get_screen();
    if (vesc) lv_scr_load(vesc);
    bsp_display_unlock();
    s_inited = true;
    return ESP_OK;
}

ui_mode_t ui_mode_get(void) { return atomic_load(&s_mode); }

void ui_mode_set(ui_mode_t mode)
{
    if (!s_inited) return;
    if (atomic_load(&s_mode) == mode) return;
    if (bsp_display_lock(200) != ESP_OK) return;
    if (mode == UI_MODE_VESC) {
        lv_obj_t *vesc = vesc_ui_get_screen();
        if (vesc) {
            lv_scr_load(vesc);
            /* The video sink wrote the last AA frame straight to the
             * panel framebuffer (bypassing LVGL). LVGL only redraws
             * dirty regions, so without an explicit invalidate the user
             * sees the stale video frame until something on the
             * dashboard changes. */
            lv_obj_invalidate(vesc);
        }
        /* Resume LVGL worker now if the video sink had paused it —
         * otherwise the dashboard wouldn't repaint until the next
         * decoded frame triggers the resume from inside show_yuv420. */
        display_video_yield_panel();
    } else {
        if (s_aa_screen) lv_scr_load(s_aa_screen);
    }
    bsp_display_unlock();
    atomic_store(&s_mode, mode);
    /* Tell touch_input which consumer should see GT911 events — LVGL when
     * the VESC dashboard is up, AA when the phone is the active screen. */
    touch_input_set_mode(mode == UI_MODE_VESC ? TOUCH_MODE_LVGL : TOUCH_MODE_AA);
    ESP_LOGI(TAG, "switched to %s", mode == UI_MODE_VESC ? "VESC" : "AA");
}

void ui_mode_toggle(void)
{
    ui_mode_set(ui_mode_get() == UI_MODE_AA ? UI_MODE_VESC : UI_MODE_AA);
}
