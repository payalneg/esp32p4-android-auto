#include "splash_screen.h"

#include <unistd.h>   /* access() */

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

#include "app_fs.h"

static const char *TAG = "splash";

#define SPLASH_PATH     "/vescfs/splash.gif"   /* POSIX path on the LittleFS */
#define SPLASH_LV_PATH  "S:" SPLASH_PATH       /* LVGL FS_POSIX drive letter 'S' */

/* Bounded wait for the async LittleFS mount. A normal (already-formatted)
 * mount is fast; a first-ever boot formats (~1 min) — we never wait that long,
 * we just skip the splash (a fresh device has no splash file anyway). */
#define FS_WAIT_MS      800

/* Safety auto-hide: if the dashboard build never calls splash_screen_hide()
 * (hang, error path), the overlay still goes away on its own. Generous enough
 * to cover the normal ~5 s dashboard build. */
#define SPLASH_MAX_MS   8000

static lv_obj_t   *s_overlay;
static lv_timer_t *s_safety_timer;

/* Single point that tears the overlay down. Runs under the LVGL lock held by
 * the LVGL task (timer cb) or by splash_screen_hide() (which takes it). */
static void teardown_locked(void)
{
    if (s_safety_timer) {
        lv_timer_del(s_safety_timer);
        s_safety_timer = NULL;
    }
    if (s_overlay) {
        lv_obj_del(s_overlay);
        s_overlay = NULL;
    }
}

static void safety_timer_cb(lv_timer_t *t)
{
    (void)t;
    ESP_LOGI(TAG, "safety timeout — hiding splash");
    teardown_locked();  /* timer cb already runs under the LVGL lock */
}

void splash_screen_show(void)
{
    if (s_overlay) return;  /* already showing */

    app_fs_ensure();
    for (int waited = 0; !app_fs_ready() && waited < FS_WAIT_MS; waited += 50) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (!app_fs_ready()) {
        ESP_LOGI(TAG, "storage not ready — no splash");
        return;
    }
    if (access(SPLASH_PATH, F_OK) != 0) {
        ESP_LOGI(TAG, "no %s — no splash", SPLASH_PATH);
        return;
    }

    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGW(TAG, "display lock timeout — skipping splash");
        return;
    }

    /* Full-screen black overlay on the top layer. The top layer sits above all
     * screens and is untouched by lv_scr_load(), so it stays visible across the
     * idle→dashboard screen swap that happens while the dashboard builds. */
    s_overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_overlay);
    lv_obj_set_size(s_overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *gif = lv_gif_create(s_overlay);
    lv_gif_set_src(gif, SPLASH_LV_PATH);
    lv_obj_center(gif);

    s_safety_timer = lv_timer_create(safety_timer_cb, SPLASH_MAX_MS, NULL);

    bsp_display_unlock();
    ESP_LOGI(TAG, "splash shown (%s)", SPLASH_PATH);
}

void splash_screen_hide(void)
{
    if (!s_overlay && !s_safety_timer) return;
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGW(TAG, "display lock timeout — splash will auto-hide");
        return;  /* safety timer still pending */
    }
    teardown_locked();
    bsp_display_unlock();
    ESP_LOGI(TAG, "splash hidden");
}
