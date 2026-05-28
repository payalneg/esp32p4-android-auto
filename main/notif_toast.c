#include "notif_toast.h"

#include <stdio.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "lvgl.h"

#include "notif_bridge.h"

static const char *TAG = "notif_toast";

#define TOAST_HEIGHT     96
#define TOAST_DWELL_MS   4000
#define TOAST_SLIDE_MS   220
#define POLL_PERIOD_MS   400

static lv_obj_t *s_card;
static lv_obj_t *s_icon_letter;
static lv_obj_t *s_app_lbl;
static lv_obj_t *s_title_lbl;
static lv_obj_t *s_text_lbl;
static lv_timer_t *s_poll;
static lv_timer_t *s_dismiss;

static uint32_t s_last_seen_id;
static bool s_initialized;

static void slide_out(void)
{
    if (!s_card || lv_obj_has_flag(s_card, LV_OBJ_FLAG_HIDDEN)) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_card);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_time(&a, TOAST_SLIDE_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_values(&a, 12, -TOAST_HEIGHT - 20);
    lv_anim_start(&a);
}

static void dismiss_cb(lv_timer_t *t)
{
    (void)t;
    slide_out();
    lv_timer_del(s_dismiss);
    s_dismiss = NULL;
}

static void slide_in(const notif_msg_t *m)
{
    char letter[5] = "?";
    if (m->app_name[0]) {
        letter[0] = (char)((m->app_name[0] >= 'a' && m->app_name[0] <= 'z')
                           ? m->app_name[0] - 32 : m->app_name[0]);
        letter[1] = '\0';
    }
    lv_label_set_text(s_icon_letter, letter);
    lv_label_set_text(s_app_lbl, m->app_name);
    lv_label_set_text(s_title_lbl, m->title[0] ? m->title : m->app_name);
    lv_label_set_text(s_text_lbl, m->text);

    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_HIDDEN);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_card);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_time(&a, TOAST_SLIDE_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_values(&a, -TOAST_HEIGHT - 20, 12);
    lv_anim_start(&a);

    if (s_dismiss) {
        lv_timer_reset(s_dismiss);
    } else {
        s_dismiss = lv_timer_create(dismiss_cb, TOAST_DWELL_MS, NULL);
        lv_timer_set_repeat_count(s_dismiss, 1);
    }
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    notif_msg_t buf[1];
    if (notif_bridge_recent(buf, 1) == 0) return;
    if (buf[0].id == s_last_seen_id) return;
    s_last_seen_id = buf[0].id;
    if (buf[0].removed) return;
    slide_in(&buf[0]);
}

esp_err_t notif_toast_init(void)
{
    if (s_initialized) return ESP_OK;
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl lock timeout");
        return ESP_FAIL;
    }

    /* lv_layer_top() always renders above the active screen — what we
     * need for a global "ping" overlay that survives screen swaps. */
    s_card = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_card, LV_PCT(90), TOAST_HEIGHT);
    lv_obj_set_x(s_card, (800 - (int)(800 * 0.9)) / 2);
    lv_obj_set_y(s_card, -TOAST_HEIGHT - 20);
    lv_obj_set_style_bg_color(s_card, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_bg_opa(s_card, LV_OPA_90, 0);
    lv_obj_set_style_border_color(s_card, lv_color_hex(0x1f6feb), 0);
    lv_obj_set_style_border_width(s_card, 1, 0);
    lv_obj_set_style_radius(s_card, 12, 0);
    lv_obj_set_style_pad_all(s_card, 10, 0);
    lv_obj_set_flex_flow(s_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(s_card, 12, 0);
    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_card, LV_OBJ_FLAG_HIDDEN);

    /* Icon placeholder — first letter of the app name. Once an actual PNG
     * decoder is enabled in LVGL, swap this for an lv_image fed from
     * notif_bridge_get_icon(). */
    lv_obj_t *icon = lv_obj_create(s_card);
    lv_obj_set_size(icon, 56, 56);
    lv_obj_set_style_bg_color(icon, lv_color_hex(0x1f6feb), 0);
    lv_obj_set_style_radius(icon, 12, 0);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
    s_icon_letter = lv_label_create(icon);
    lv_obj_set_style_text_color(s_icon_letter, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_icon_letter, &lv_font_montserrat_22, 0);
    lv_label_set_text(s_icon_letter, "?");
    lv_obj_center(s_icon_letter);

    lv_obj_t *col = lv_obj_create(s_card);
    lv_obj_set_size(col, LV_PCT(80), LV_PCT(100));
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, 2, 0);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    s_app_lbl = lv_label_create(col);
    lv_obj_set_style_text_color(s_app_lbl, lv_color_hex(0x808080), 0);
    lv_label_set_text(s_app_lbl, "");

    s_title_lbl = lv_label_create(col);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_title_lbl, &lv_font_montserrat_22, 0);
    lv_label_set_text(s_title_lbl, "");

    s_text_lbl = lv_label_create(col);
    lv_obj_set_style_text_color(s_text_lbl, lv_color_hex(0xc0c0c0), 0);
    lv_label_set_long_mode(s_text_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(s_text_lbl, LV_PCT(100));
    lv_label_set_text(s_text_lbl, "");

    s_poll = lv_timer_create(poll_cb, POLL_PERIOD_MS, NULL);

    bsp_display_unlock();
    s_initialized = true;
    ESP_LOGI(TAG, "toast overlay ready");
    return ESP_OK;
}
