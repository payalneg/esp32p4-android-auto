#include "ota_screen.h"

#include <stdio.h>

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ota_screen";

static lv_obj_t *s_root;
static lv_obj_t *s_title;
static lv_obj_t *s_subtitle;
static lv_obj_t *s_status;
static lv_obj_t *s_bar;
static lv_obj_t *s_pct;
static bool s_initialized;

esp_err_t ota_screen_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

#if !CONFIG_C6_OTA_DISPLAY_PROGRESS
    ESP_LOGI(TAG, "display progress disabled — OTA will be log-only");
    return ESP_OK;
#endif

    if (!bsp_display_start()) {
        ESP_LOGE(TAG, "bsp_display_start failed");
        return ESP_FAIL;
    }
    bsp_display_backlight_on();

    if (!bsp_display_lock(0)) {
        return ESP_FAIL;
    }

    s_root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(s_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_root, lv_color_hex(0x0a0a0a), 0);
    lv_obj_set_style_pad_all(s_root, 24, 0);
    lv_obj_set_flex_flow(s_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_root, LV_OBJ_FLAG_HIDDEN);

    s_title = lv_label_create(s_root);
    lv_label_set_text(s_title, "Updating Wi-Fi co-processor");
    lv_obj_set_style_text_color(s_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_pad_bottom(s_title, 12, 0);

    s_subtitle = lv_label_create(s_root);
    lv_label_set_text(s_subtitle, "Don't power off");
    lv_obj_set_style_text_color(s_subtitle, lv_color_hex(0xa0a0a0), 0);
    lv_obj_set_style_pad_bottom(s_subtitle, 32, 0);

    s_bar = lv_bar_create(s_root);
    lv_obj_set_size(s_bar, LV_PCT(80), 20);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x3aa3ff), LV_PART_INDICATOR);

    s_pct = lv_label_create(s_root);
    lv_label_set_text(s_pct, "0%");
    lv_obj_set_style_text_color(s_pct, lv_color_white(), 0);
    lv_obj_set_style_pad_top(s_pct, 12, 0);

    s_status = lv_label_create(s_root);
    lv_label_set_text(s_status, "");
    lv_obj_set_style_text_color(s_status, lv_color_hex(0x808080), 0);

    bsp_display_unlock();
    s_initialized = true;
    return ESP_OK;
}

void ota_screen_show(const char *subtitle)
{
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(0)) {
        if (subtitle) {
            lv_label_set_text(s_subtitle, subtitle);
        }
        lv_obj_clear_flag(s_root, LV_OBJ_FLAG_HIDDEN);
        bsp_display_unlock();
    }
}

void ota_screen_set_progress(uint32_t done, uint32_t total)
{
    if (!s_initialized || !total) {
        return;
    }
    int pct = (int)((uint64_t)done * 100 / total);
    if (pct > 100) pct = 100;
    if (bsp_display_lock(0)) {
        lv_bar_set_value(s_bar, pct, LV_ANIM_OFF);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", pct);
        lv_label_set_text(s_pct, buf);
        bsp_display_unlock();
    }
}

void ota_screen_set_status(const char *line)
{
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(0)) {
        lv_label_set_text(s_status, line ? line : "");
        bsp_display_unlock();
    }
}

void ota_screen_hide(void)
{
    if (!s_initialized) {
        return;
    }
    if (bsp_display_lock(0)) {
        lv_obj_add_flag(s_root, LV_OBJ_FLAG_HIDDEN);
        bsp_display_unlock();
    }
}
