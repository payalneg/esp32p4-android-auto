#include "ui_mode.h"

#include <stdatomic.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "ble_nav.h"
#include "dev_settings.h"
#include "display_video.h"
#include "esp_log.h"
#include "lvgl.h"
#include "nav_screen.h"
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

const char *ui_mode_name(ui_mode_t mode)
{
    switch (mode) {
        case UI_MODE_AA:   return "AA";
        case UI_MODE_VESC: return "VESC";
        case UI_MODE_NAV:  return "NAV";
        default:           return "?";
    }
}

/* Which full-screen source the gesture brings up, honouring the Settings
 * choice and falling back to Android Auto if the navigator screen could not
 * allocate its framebuffers. */
static ui_mode_t phone_mode(void)
{
    if (settings_get_phone_screen() == 1 && nav_screen_get() != NULL) {
        return UI_MODE_NAV;
    }
    return UI_MODE_AA;
}

void ui_mode_set(ui_mode_t mode)
{
    if (!s_inited) return;
    if (mode == UI_MODE_NAV && nav_screen_get() == NULL) mode = UI_MODE_AA;
    if (atomic_load(&s_mode) == mode) return;
    ESP_LOGW(TAG, "ui_mode_set(%s) called from task=%s",
             ui_mode_name(mode), pcTaskGetName(NULL));
    if (mode == UI_MODE_VESC || mode == UI_MODE_NAV) {
        /* Flip the mode flag BEFORE we let LVGL start rendering the VESC
         * dashboard. Decoder thread (display_video_show_yuv420) checks
         * ui_mode_get() at the top and takes the non-AA early-out, which
         * resumes the worker and drops the frame. If we did this in the
         * opposite order — load+invalidate the screen → resume LVGL → store
         * mode — a decoder pass that read mode==AA before our store would dive
         * into the AA branch and call esp_lv_adapter_pause(2000) just as
         * the worker started a full-screen redraw of ~1700 GUI Guider
         * widgets. That redraw is heavy enough on 800×480 that pause()
         * times out, leaving the AA path stuck in 2 s/frame mode after
         * every AA→VESC→AA toggle. */
        atomic_store(&s_mode, mode);
        nav_screen_set_active(mode == UI_MODE_NAV);
        if (bsp_display_lock(200) != ESP_OK) return;
        lv_obj_t *scr = (mode == UI_MODE_NAV) ? nav_screen_get()
                                              : vesc_ui_get_screen();
        if (scr) {
            lv_scr_load(scr);
            /* The video sink wrote the last AA frame straight to the
             * panel framebuffer (bypassing LVGL). LVGL only redraws
             * dirty regions, so without an explicit invalidate the user
             * sees the stale video frame until something on the
             * dashboard changes. */
            lv_obj_invalidate(scr);
        }
        /* Resume LVGL worker now if the video sink had paused it —
         * otherwise the dashboard wouldn't repaint until the next
         * decoded frame triggers the resume from inside show_yuv420.
         * Idempotent — if the decoder already raced us through the
         * non-AA branch above and resumed, this is a no-op. */
        display_video_yield_panel();
        bsp_display_unlock();
    } else {
        /* AA: load the (mostly empty) AA screen FIRST so when we flip the
         * mode flag the next decoder pass sees a worker with nothing heavy
         * to redraw, and esp_lv_adapter_pause completes immediately. */
        nav_screen_set_active(false);
        if (bsp_display_lock(200) != ESP_OK) return;
        if (s_aa_screen) lv_scr_load(s_aa_screen);
        bsp_display_unlock();
        atomic_store(&s_mode, mode);
    }
    /* Tell touch_input which consumer should see GT911 events — LVGL when
     * the VESC dashboard or the navigator picture is up, AA when the phone
     * is the active screen. */
    touch_input_set_mode(mode == UI_MODE_AA ? TOUCH_MODE_AA : TOUCH_MODE_LVGL);
    /* The phone only streams frames while the navigator screen is the one
     * being looked at — tell it which way this went. */
    ble_nav_notify_state();
    ESP_LOGI(TAG, "switched to %s", ui_mode_name(mode));
}

void ui_mode_toggle(void)
{
    ui_mode_set(ui_mode_get() == UI_MODE_VESC ? phone_mode() : UI_MODE_VESC);
}
