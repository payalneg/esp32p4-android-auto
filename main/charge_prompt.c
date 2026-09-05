/* See charge_prompt.h. */
#include "charge_prompt.h"

#include <stdio.h>

#include "esp_log.h"
#include "lvgl.h"
#include "vesc_battery_calc.h"

static const char *TAG = "charge_prompt";

/* Unanswered prompt closes as "Keep" after this. */
#define CHARGE_PROMPT_TIMEOUT_MS  (30 * 1000)

/* Same palette as the trip-statistics dialogs (custom/trip_statistics.c). */
#define COL_PANEL  0x12181C
#define COL_BTN    0x2a3440
#define COL_RED    0xFF3B30
#define COL_TEXT   0xFFFFFF
#define COL_DIM    0x8A9499

static lv_obj_t   *s_modal;     /* full-screen backdrop; NULL = no prompt */
static lv_timer_t *s_timeout;

static void close_prompt(void)
{
    if (s_timeout) { lv_timer_del(s_timeout); s_timeout = NULL; }
    if (s_modal)   { lv_obj_del(s_modal);     s_modal   = NULL; }
}

static void reset_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    close_prompt();
    ESP_LOGI(TAG, "rider chose Reset — trip + battery counter to full");
    battery_calc_reset_trip_and_ah();
}

static void keep_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    close_prompt();
    ESP_LOGI(TAG, "rider chose Keep — trip continues");
}

static void timeout_cb(lv_timer_t *t)
{
    /* Delete ourselves here (allowed inside the callback) rather than via
     * repeat_count=1 — see the note in notif_toast.c about dangling timer
     * pointers. close_prompt() then only has the widget tree to drop. */
    lv_timer_del(t);
    s_timeout = NULL;
    ESP_LOGI(TAG, "no answer in %d s — keeping the trip", CHARGE_PROMPT_TIMEOUT_MS / 1000);
    close_prompt();
}

/* Flat rounded button with a centred label (trip-stats look). */
static lv_obj_t *make_btn(lv_obj_t *parent, uint32_t bg, const char *text,
                          lv_event_cb_t cb)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, 220, 64);
    lv_obj_set_style_radius(b, 8, 0);
    lv_obj_set_style_border_width(b, 0, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);

    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_center(l);

    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

/* battery_calc callback. Runs on the LVGL thread (the updater lv_timer, lock
 * held), no battery_calc lock held — safe to build widgets directly. */
static void on_charge_detected(float prev_v, float now_v, float change_pct)
{
    if (s_modal) return;   /* already asking */
    ESP_LOGI(TAG, "pack %.1f V -> %.1f V (+%.1f%%) — asking the rider",
             prev_v, now_v, change_pct);

    /* Dimmed, click-blocking backdrop above everything. */
    s_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_modal, 800, 480);
    lv_obj_set_pos(s_modal, 0, 0);
    lv_obj_set_style_bg_color(s_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_modal, LV_OPA_60, 0);
    lv_obj_set_style_border_width(s_modal, 0, 0);
    lv_obj_set_style_radius(s_modal, 0, 0);
    lv_obj_clear_flag(s_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_modal, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *box = lv_obj_create(s_modal);
    lv_obj_set_size(box, 560, 260);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(box);
    lv_label_set_text(title, "Battery charged?");
    lv_obj_set_style_text_color(title, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* ASCII only — Montserrat has no arrow glyph, so "->" not U+2192. */
    lv_obj_t *body = lv_label_create(box);
    lv_label_set_text_fmt(body,
                          "Pack voltage %.1f V -> %.1f V (+%.0f%%)\n"
                          "Reset trip and battery counter?",
                          prev_v, now_v, change_pct);
    lv_obj_set_style_text_color(body, lv_color_hex(COL_DIM), 0);
    lv_obj_set_style_text_font(body, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 62);

    lv_obj_t *yes = make_btn(box, COL_RED, "Reset", reset_cb);
    lv_obj_align(yes, LV_ALIGN_BOTTOM_LEFT, 12, -12);

    lv_obj_t *no = make_btn(box, COL_BTN, "Keep", keep_cb);
    lv_obj_align(no, LV_ALIGN_BOTTOM_RIGHT, -12, -12);

    s_timeout = lv_timer_create(timeout_cb, CHARGE_PROMPT_TIMEOUT_MS, NULL);
}

void charge_prompt_init(void)
{
    battery_calc_set_charge_detected_cb(on_charge_detected);
}
