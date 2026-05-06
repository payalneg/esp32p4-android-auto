#include "idle_screen.h"

#include <stdbool.h>
#include <stdio.h>

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "idle_screen";

static lv_obj_t *s_root;
static lv_obj_t *s_title;
static lv_obj_t *s_subtitle;
static char      s_line1[40];
static char      s_line2[80];
static bool      s_initialized;

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
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_root, LV_OBJ_FLAG_HIDDEN);

    s_title = lv_label_create(s_root);
    lv_label_set_text(s_title, "");
    lv_obj_set_style_text_color(s_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_pad_bottom(s_title, 12, 0);

    s_subtitle = lv_label_create(s_root);
    lv_label_set_text(s_subtitle, "");
    lv_obj_set_style_text_color(s_subtitle, lv_color_hex(0xa0a0a0), 0);

    bsp_display_unlock();
    s_initialized = true;
    return ESP_OK;
}

void idle_screen_show(const char *line1, const char *line2)
{
    snprintf(s_line1, sizeof(s_line1), "%s", line1 ? line1 : "");
    snprintf(s_line2, sizeof(s_line2), "%s", line2 ? line2 : "");
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(200) == ESP_OK) {
        lv_label_set_text(s_title, s_line1);
        lv_label_set_text(s_subtitle, s_line2);
        lv_obj_clear_flag(s_root, LV_OBJ_FLAG_HIDDEN);
        bsp_display_unlock();
    }
}

void idle_screen_refresh(void)
{
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(200) == ESP_OK) {
        lv_label_set_text(s_title, s_line1);
        lv_label_set_text(s_subtitle, s_line2);
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
