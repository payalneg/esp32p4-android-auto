#include "ui_mode.h"

#include <stdatomic.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "dev_settings.h"
#include "esp_log.h"
#include "lvgl.h"
#include "sdkconfig.h"
#include "touch_input.h"
#include "vesc_ui.h"
#if CONFIG_AA_ENABLE
#include "display_video.h"
#endif
#if CONFIG_NAV_ENABLE
#include "ble_nav.h"
#include "nav_screen.h"
#endif

static const char *TAG = "ui_mode";

static atomic_int s_mode = UI_MODE_VESC;
static bool       s_inited;
#if CONFIG_AA_ENABLE
static lv_obj_t  *s_aa_screen;   /* what lv_scr_act() was before the dashboard */
#endif

esp_err_t ui_mode_init(void)
{
    if (s_inited) return ESP_OK;
    /* No panel means no screens to build on. lv_obj_create(NULL) puts the
     * object on the DEFAULT display and dereferences it, so a BSP that failed
     * to bring the LCD up turned every boot into a null-pointer panic here —
     * a reboot loop on a device whose only way out is the screen that just
     * failed. Refuse instead: main.c already treats a non-OK return as "UI
     * disabled", and the rest of the firmware (VESC link, logging, OTA) keeps
     * running and can be reached over the wire. */
    if (lv_disp_get_default() == NULL) {
        ESP_LOGE(TAG, "no LVGL display registered — UI disabled");
        return ESP_ERR_INVALID_STATE;
    }
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl lock timeout");
        return ESP_FAIL;
    }
#if CONFIG_AA_ENABLE
    s_aa_screen = lv_scr_act();
#endif
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

#if !CONFIG_AA_ENABLE && !CONFIG_NAV_ENABLE
/* Neither phone screen is compiled in: UI_MODE_VESC is the only mode there is.
 * The setter stays (vesc_ui_updater and the header are shared with the boards
 * that do have one) but has nothing to switch to, so it just reports the
 * attempt. */
static ui_mode_t phone_mode(void) { return UI_MODE_VESC; }

void ui_mode_set(ui_mode_t mode)
{
    if (mode != UI_MODE_VESC) {
        ESP_LOGW(TAG, "ui_mode_set(%s) ignored — no Android Auto / navigator in this build",
                 ui_mode_name(mode));
    }
}
#else
/* Which full-screen source the gesture brings up. A board with both honours
 * the Settings choice and falls back to Android Auto if the navigator screen
 * could not allocate its buffers; the dashboard-only board has just the
 * navigator, so there the choice does not arise. */
static ui_mode_t phone_mode(void)
{
#if CONFIG_NAV_ENABLE
    if (nav_screen_get() != NULL
#if CONFIG_AA_ENABLE
        && settings_get_phone_screen() == 1
#endif
       ) {
        return UI_MODE_NAV;
    }
#endif
#if CONFIG_AA_ENABLE
    return UI_MODE_AA;
#else
    /* Navigator did not come up and there is no projection to fall back on. */
    return UI_MODE_VESC;
#endif
}

void ui_mode_set(ui_mode_t mode)
{
    if (!s_inited) return;
#if CONFIG_NAV_ENABLE
    if (mode == UI_MODE_NAV && nav_screen_get() == NULL) {
#if CONFIG_AA_ENABLE
        mode = UI_MODE_AA;
#else
        mode = UI_MODE_VESC;
#endif
    }
#endif
#if !CONFIG_AA_ENABLE
    if (mode == UI_MODE_AA) {
        ESP_LOGW(TAG, "ui_mode_set(AA) ignored — no Android Auto in this build");
        return;
    }
#endif
    if (atomic_load(&s_mode) == mode) return;
    ESP_LOGW(TAG, "ui_mode_set(%s) called from task=%s",
             ui_mode_name(mode), pcTaskGetName(NULL));
    if (mode == UI_MODE_VESC || mode == UI_MODE_NAV) {
        /* Flip the mode flag BEFORE we let LVGL start rendering the VESC
         * dashboard. On the AA boards the decoder thread
         * (display_video_show_yuv420) checks ui_mode_get() at the top and
         * takes the non-AA early-out, which resumes the worker and drops the
         * frame. If we did this in the opposite order — load+invalidate the
         * screen → resume LVGL → store mode — a decoder pass that read
         * mode==AA before our store would dive into the AA branch and call
         * esp_lv_adapter_pause(2000) just as the worker started a full-screen
         * redraw of ~1700 GUI Guider widgets. That redraw is heavy enough on
         * 800×480 that pause() times out, leaving the AA path stuck in
         * 2 s/frame mode after every AA→VESC→AA toggle. */
        atomic_store(&s_mode, mode);
#if CONFIG_NAV_ENABLE
        nav_screen_set_active(mode == UI_MODE_NAV);
#endif
        if (bsp_display_lock(200) != ESP_OK) return;
        lv_obj_t *scr = vesc_ui_get_screen();
#if CONFIG_NAV_ENABLE
        if (mode == UI_MODE_NAV) scr = nav_screen_get();
#endif
        if (scr) {
            lv_scr_load(scr);
            /* The video sink wrote the last AA frame straight to the
             * panel framebuffer (bypassing LVGL). LVGL only redraws
             * dirty regions, so without an explicit invalidate the user
             * sees the stale video frame until something on the
             * dashboard changes. */
            lv_obj_invalidate(scr);
        }
#if CONFIG_AA_ENABLE
        /* Resume LVGL worker now if the video sink had paused it —
         * otherwise the dashboard wouldn't repaint until the next
         * decoded frame triggers the resume from inside show_yuv420.
         * Idempotent — if the decoder already raced us through the
         * non-AA branch above and resumed, this is a no-op. */
        display_video_yield_panel();
#endif
        bsp_display_unlock();
    }
#if CONFIG_AA_ENABLE
    else {
        /* AA: load the (mostly empty) AA screen FIRST so when we flip the
         * mode flag the next decoder pass sees a worker with nothing heavy
         * to redraw, and esp_lv_adapter_pause completes immediately. */
#if CONFIG_NAV_ENABLE
        nav_screen_set_active(false);
#endif
        if (bsp_display_lock(200) != ESP_OK) return;
        if (s_aa_screen) lv_scr_load(s_aa_screen);
        bsp_display_unlock();
        atomic_store(&s_mode, mode);
    }
#endif
    /* Tell touch_input which consumer should see GT911 events — LVGL when
     * the VESC dashboard or the navigator picture is up, AA when the phone
     * is the active screen. */
    touch_input_set_mode(mode == UI_MODE_AA ? TOUCH_MODE_AA : TOUCH_MODE_LVGL);
#if CONFIG_NAV_ENABLE
    /* The phone only streams tiles while the navigator screen is the one
     * being looked at — tell it which way this went. */
    ble_nav_notify_state();
#endif
    ESP_LOGI(TAG, "switched to %s", ui_mode_name(mode));
}

#endif /* CONFIG_AA_ENABLE || CONFIG_NAV_ENABLE */

void ui_mode_toggle(void)
{
    ui_mode_set(ui_mode_get() == UI_MODE_VESC ? phone_mode() : UI_MODE_VESC);
}
