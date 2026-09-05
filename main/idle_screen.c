#include "idle_screen.h"

#include <stdbool.h>
#include <stdio.h>

#include "aa_reconnect.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "idle_screen";

static lv_obj_t *s_root;
static lv_obj_t *s_title;
static lv_obj_t *s_subtitle;
static lv_obj_t *s_connect_btn;
static lv_obj_t *s_info;
static char      s_line1[40];
static char      s_line2[80];
static char      s_info_txt[80];
static uint32_t  s_title_color = 0xffffff;
static bool      s_initialized;

/* Apply the cached strings + colour. Caller holds the display lock. */
static void apply_cached(void)
{
    lv_label_set_text(s_title, s_line1);
    lv_obj_set_style_text_color(s_title, lv_color_hex(s_title_color), 0);
    lv_label_set_text(s_subtitle, s_line2);
    lv_label_set_text(s_info, s_info_txt);
}

static void connect_btn_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "Connect button tapped — restarting the AA link");
    aa_reconnect_manual();
}

esp_err_t idle_screen_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl lock timeout");
        return ESP_FAIL;
    }

    s_root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_root, lv_color_hex(0x0a0a0a), 0);
    lv_obj_set_style_pad_all(s_root, 24, 0);
    lv_obj_set_flex_flow(s_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_root, 20, 0);
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_root, LV_OBJ_FLAG_HIDDEN);

    s_title = lv_label_create(s_root);
    lv_label_set_text(s_title, "");
    lv_obj_set_style_text_color(s_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_32, 0);

    s_subtitle = lv_label_create(s_root);
    lv_label_set_text(s_subtitle, "");
    lv_obj_set_style_text_color(s_subtitle, lv_color_hex(0xa0a0a0), 0);

    /* AP / IP / port reference line — dimmer than the step detail above. */
    s_info = lv_label_create(s_root);
    lv_label_set_text(s_info, "");
    lv_obj_set_style_text_color(s_info, lv_color_hex(0x606060), 0);

    /* "Connect" button — hidden by default, unhidden once we reach the
     * "Waiting for phone" state. Sits below the subtitle in the column. */
    s_connect_btn = lv_btn_create(s_root);
    lv_obj_set_style_pad_hor(s_connect_btn, 32, 0);
    lv_obj_set_style_pad_ver(s_connect_btn, 14, 0);
    lv_obj_set_style_bg_color(s_connect_btn, lv_color_hex(0x1f6feb), 0);
    lv_obj_set_style_radius(s_connect_btn, 8, 0);
    lv_obj_add_event_cb(s_connect_btn, connect_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_connect_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *btn_lbl = lv_label_create(s_connect_btn);
    lv_label_set_text(btn_lbl, "Connect");
    lv_obj_set_style_text_color(btn_lbl, lv_color_white(), 0);
    lv_obj_center(btn_lbl);

    bsp_display_unlock();
    s_initialized = true;
    return ESP_OK;
}

void idle_screen_show(const char *line1, const char *line2)
{
    snprintf(s_line1, sizeof(s_line1), "%s", line1 ? line1 : "");
    snprintf(s_line2, sizeof(s_line2), "%s", line2 ? line2 : "");
    s_title_color = 0xffffff;
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(200) == ESP_OK) {
        apply_cached();
        lv_obj_clear_flag(s_root, LV_OBJ_FLAG_HIDDEN);
        bsp_display_unlock();
    }
}

void idle_screen_set_state(idle_screen_state_t state, const char *detail)
{
    const char *title;
    uint32_t    color;
    switch (state) {
    case IDLE_STATE_CONNECTING: title = "Connecting...";  color = 0xffc043; break;
    case IDLE_STATE_CONNECTED:  title = "Connected";      color = 0x4cd964; break;
    default:                    title = "Disconnected";   color = 0xff6b6b; break;
    }
    snprintf(s_line1, sizeof(s_line1), "%s", title);
    snprintf(s_line2, sizeof(s_line2), "%s", detail ? detail : "");
    s_title_color = color;
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(200) == ESP_OK) {
        apply_cached();
        bsp_display_unlock();
    }
}

void idle_screen_set_info(const char *info)
{
    snprintf(s_info_txt, sizeof(s_info_txt), "%s", info ? info : "");
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(200) == ESP_OK) {
        lv_label_set_text(s_info, s_info_txt);
        bsp_display_unlock();
    }
}

void idle_screen_refresh(void)
{
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(200) == ESP_OK) {
        apply_cached();
        lv_obj_clear_flag(s_root, LV_OBJ_FLAG_HIDDEN);
        /* The video sink wrote the last AA frame straight to the panel
         * framebuffer; LVGL only redraws dirty regions, so without this
         * the user would see a frozen frame instead of the idle text. */
        lv_obj_invalidate(lv_scr_act());
        bsp_display_unlock();
    }
}

void idle_screen_hide(void)
{
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(200) == ESP_OK) {
        lv_obj_add_flag(s_root, LV_OBJ_FLAG_HIDDEN);
        bsp_display_unlock();
    }
}

void idle_screen_set_connect_visible(bool visible)
{
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(200) == ESP_OK) {
        if (visible) {
            lv_obj_clear_flag(s_connect_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_connect_btn, LV_OBJ_FLAG_HIDDEN);
        }
        bsp_display_unlock();
    }
}
