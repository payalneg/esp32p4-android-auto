#include "touch_input.h"

#include <stdatomic.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "touch_input";

/* GT911 docs warn that reads faster than ~10 ms can return stale/duplicate
 * frames. 20 ms = 50 Hz which is well above touch perception (~30 Hz is
 * already smooth) and lines up with the BSP example's polling cadence. */
#define POLL_INTERVAL_MS  20

/* Coordinate mapping: panel native is 480×800 portrait. The video pipeline
 * (display_video.c, DISPLAY_CPU_YUV_FULL path) writes panel pixel (dx, dy)
 * from AA pixel (sx=dy, sy=479-dx). Inverting: panel touch (tx, ty) shows AA
 * pixel (ax=ty, ay=479-tx). AA frame is 800×480 landscape, which matches the
 * touch_screen_config we advertise in InputChannel (width=800, height=480). */
#define PANEL_NATIVE_W  480
#define PANEL_NATIVE_H  800
#define AA_W            800
#define AA_H            480

static TaskHandle_t      s_task;
static touch_send_fn     s_cb;
static void             *s_cb_ctx;
static atomic_bool       s_stop_requested;

static void poll_task(void *arg)
{
    (void)arg;
    esp_lcd_touch_handle_t tp = bsp_display_get_touch_handle();
    if (!tp) {
        ESP_LOGE(TAG, "no touch handle from BSP — task exiting");
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    bool was_pressed = false;
    uint16_t last_x = 0, last_y = 0;

    while (!atomic_load(&s_stop_requested)) {
        if (esp_lcd_touch_read_data(tp) == ESP_OK) {
            uint16_t tx[1], ty[1], strength[1];
            uint8_t  cnt = 0;
            bool pressed = esp_lcd_touch_get_coordinates(tp, tx, ty, strength,
                                                         &cnt, 1);
            uint64_t ts = (uint64_t)esp_timer_get_time();

            if (pressed && cnt > 0) {
                /* GT911 returns native panel coords (0..479, 0..799). Map to
                 * AA landscape. Clamp defensively in case the chip ever
                 * reports an over-range value. */
                uint16_t panel_x = tx[0] < PANEL_NATIVE_W ? tx[0] : PANEL_NATIVE_W - 1;
                uint16_t panel_y = ty[0] < PANEL_NATIVE_H ? ty[0] : PANEL_NATIVE_H - 1;
                uint16_t aa_x = panel_y;
                uint16_t aa_y = (PANEL_NATIVE_W - 1) - panel_x;
                if (aa_x >= AA_W) aa_x = AA_W - 1;
                if (aa_y >= AA_H) aa_y = AA_H - 1;

                touch_action_t action;
                if (!was_pressed) {
                    action = TOUCH_ACTION_PRESS;
                } else if (aa_x != last_x || aa_y != last_y) {
                    action = TOUCH_ACTION_DRAG;
                } else {
                    /* Same position, still touching — phone doesn't need an
                     * event for this. Skip to avoid spamming the input
                     * channel at 50 Hz with redundant DRAGs. */
                    goto next;
                }

                if (s_cb) s_cb(ts, action, aa_x, aa_y, s_cb_ctx);
                last_x = aa_x;
                last_y = aa_y;
                was_pressed = true;
            } else if (was_pressed) {
                /* Phone needs the final position with the RELEASE — openauto
                 * sends the last (x, y) we saw, not (0, 0). */
                if (s_cb) s_cb(ts, TOUCH_ACTION_RELEASE, last_x, last_y, s_cb_ctx);
                was_pressed = false;
            }
        }
next:
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }

    s_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t touch_input_start(touch_send_fn cb, void *ctx)
{
    if (s_task) {
        /* Already running. Update the callback in case the new caller has
         * different state (e.g. fresh socket on reconnect). */
        s_cb = cb;
        s_cb_ctx = ctx;
        return ESP_OK;
    }
    if (!bsp_display_get_touch_handle()) {
        ESP_LOGE(TAG, "BSP touch not initialised — call bsp_display_start first");
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_stop_requested, false);
    s_cb = cb;
    s_cb_ctx = ctx;
    BaseType_t ok = xTaskCreate(poll_task, "touch_input", 4096, NULL, 5, &s_task);
    if (ok != pdPASS) {
        s_cb = NULL;
        s_cb_ctx = NULL;
        s_task = NULL;
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "polling started (%d ms)", POLL_INTERVAL_MS);
    return ESP_OK;
}

void touch_input_stop(void)
{
    if (!s_task) return;
    atomic_store(&s_stop_requested, true);
    /* Task self-deletes on the next poll cycle (within POLL_INTERVAL_MS). We
     * don't join — the AA service tears down on its own thread and we want
     * to return promptly. The cb will see at most one more event after this
     * call; the new aa_service_run will install a fresh cb anyway. */
    s_cb = NULL;
    s_cb_ctx = NULL;
}
