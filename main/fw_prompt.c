/* "New firmware available" prompt.
 *
 * Lives on lv_layer_top(), like the notification toast, so it can float over
 * the dashboard or the navigator. The companion app owns detection — it knows
 * which firmware image is bundled in the APK and reads the running version over
 * BLE — and offers the update on the fw-prompt characteristic (see
 * notif_bridge.c). This module only renders the offer and reports the tap.
 *
 * The BLE write callback runs on the NimBLE host task; it must not touch LVGL.
 * Messages are copied into a small state struct under a critical section and a
 * timer applies them on the LVGL task. That also handles Android Auto: LVGL is
 * paused there, so the card is simply created the first time the dashboard (or
 * navigator) is back in front.
 */

#include "fw_prompt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "lvgl.h"

#include "fonts/aabridge_fonts.h"
#include "notif_bridge.h"
#include "ui_mode.h"

static const char *TAG = "fw_prompt";

#define POLL_PERIOD_MS 200

#define FW_PROMPT_COLOR_BG     0x1c1c1c
#define FW_PROMPT_COLOR_ACCENT 0x1f6feb
#define FW_PROMPT_COLOR_ERROR  0xff6b6b

typedef struct {
    uint32_t rev;
    bool     offer;
    bool     failed;
    int      progress;          /* -1 = no transfer running */
    char     current[24];
    char     available[32];
    char     title[96];
    char     body[192];
    char     now[40];
    char     later[40];
    char     skip[40];
    char     updating[40];
    char     error[128];
} fw_prompt_state_t;

static fw_prompt_state_t s_state;
static portMUX_TYPE      s_mux = portMUX_INITIALIZER_UNLOCKED;

/* Version the rider has skipped on this power cycle. The phone remembers it
 * across runs too, but keeping it here stops a second phone (or a re-offer in
 * the same session) from asking again. */
static char s_skipped[32];

static uint32_t s_applied_rev;
static bool     s_visible;
static bool     s_initialized;

static lv_obj_t *s_backdrop;
static lv_obj_t *s_card;
static lv_obj_t *s_title_lbl;
static lv_obj_t *s_body_lbl;
static lv_obj_t *s_ver_lbl;
static lv_obj_t *s_status_lbl;
static lv_obj_t *s_bar;
static lv_obj_t *s_btn_row;
static lv_obj_t *s_btn_now;
static lv_obj_t *s_btn_later;
static lv_obj_t *s_btn_skip;

/* ---------- state transfer (BLE task → LVGL task) ---------- */

static void snapshot(fw_prompt_state_t *out)
{
    portENTER_CRITICAL(&s_mux);
    *out = s_state;
    portEXIT_CRITICAL(&s_mux);
}

static void copy_str(char *dst, size_t dst_sz, const char *src)
{
    if (!src) src = "";
    size_t n = strlen(src);
    if (n >= dst_sz) n = dst_sz - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* Split a newline-separated message in place. A stray '\r' from CRLF writers
 * is trimmed so comparisons and labels stay clean. */
static int split_lines(char *buf, char *lines[], int max)
{
    int n = 0;
    char *p = buf;
    while (p && n < max) {
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';
        size_t len = strlen(p);
        if (len && p[len - 1] == '\r') p[len - 1] = '\0';
        lines[n++] = p;
        p = nl ? nl + 1 : NULL;
    }
    return n;
}

void fw_prompt_handle_text(char *text)
{
    if (!text || !text[0]) return;
    char *lines[10];
    const int n = split_lines(text, lines, 10);
    if (n == 0) return;

    if (strcmp(lines[0], "OFFER") == 0) {
        if (n < 3) return;
        portENTER_CRITICAL(&s_mux);
        if (strcmp(lines[2], s_skipped) == 0) {
            portEXIT_CRITICAL(&s_mux);
            return;
        }
        const bool same = s_state.offer &&
                          strcmp(s_state.available, lines[2]) == 0;
        /* A transfer is running — a repeated offer must not put the buttons
         * back under the rider's finger. */
        if (same && s_state.progress >= 0) {
            portEXIT_CRITICAL(&s_mux);
            return;
        }
        copy_str(s_state.current, sizeof(s_state.current), lines[1]);
        copy_str(s_state.available, sizeof(s_state.available), lines[2]);
        copy_str(s_state.title, sizeof(s_state.title), n > 3 ? lines[3] : NULL);
        copy_str(s_state.body, sizeof(s_state.body), n > 4 ? lines[4] : NULL);
        copy_str(s_state.now, sizeof(s_state.now), n > 5 ? lines[5] : NULL);
        copy_str(s_state.later, sizeof(s_state.later), n > 6 ? lines[6] : NULL);
        copy_str(s_state.skip, sizeof(s_state.skip), n > 7 ? lines[7] : NULL);
        copy_str(s_state.updating, sizeof(s_state.updating),
                 n > 8 ? lines[8] : NULL);
        if (!s_state.title[0]) copy_str(s_state.title, sizeof(s_state.title),
                                        "Firmware update available");
        if (!s_state.now[0]) copy_str(s_state.now, sizeof(s_state.now), "Update");
        if (!s_state.later[0]) copy_str(s_state.later, sizeof(s_state.later), "Later");
        if (!s_state.skip[0]) copy_str(s_state.skip, sizeof(s_state.skip), "Skip");
        if (!s_state.updating[0])
            copy_str(s_state.updating, sizeof(s_state.updating), "Updating");
        s_state.offer = true;
        s_state.failed = false;
        s_state.progress = -1;
        s_state.error[0] = '\0';
        s_state.rev++;
        portEXIT_CRITICAL(&s_mux);
        ESP_LOGI(TAG, "offer %s -> %s", lines[1], lines[2]);
        return;
    }

    if (strcmp(lines[0], "PROGRESS") == 0) {
        if (n < 2) return;
        int pct = atoi(lines[1]);
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        portENTER_CRITICAL(&s_mux);
        if (!s_state.offer) {
            portEXIT_CRITICAL(&s_mux);
            return;
        }
        s_state.progress = pct;
        s_state.failed = false;
        s_state.rev++;
        portEXIT_CRITICAL(&s_mux);
        return;
    }

    if (strcmp(lines[0], "FAILED") == 0) {
        portENTER_CRITICAL(&s_mux);
        if (!s_state.offer) {
            portEXIT_CRITICAL(&s_mux);
            return;
        }
        s_state.failed = true;
        s_state.progress = -1;
        copy_str(s_state.error, sizeof(s_state.error),
                 n > 1 && lines[1][0] ? lines[1] : "Update failed");
        s_state.rev++;
        portEXIT_CRITICAL(&s_mux);
        return;
    }
}

void fw_prompt_on_disconnect(void)
{
    portENTER_CRITICAL(&s_mux);
    s_state.offer = false;
    s_state.progress = -1;
    s_state.failed = false;
    s_state.rev++;
    portEXIT_CRITICAL(&s_mux);
}

/* ---------- UI ---------- */

static void hide_card(void)
{
    if (!s_backdrop) return;
    lv_obj_add_flag(s_backdrop, LV_OBJ_FLAG_HIDDEN);
    s_visible = false;
}

static void on_now(lv_event_t *e)
{
    (void)e;
    fw_prompt_state_t v;
    snapshot(&v);
    if (!v.offer || v.progress >= 0) return;
    notif_bridge_send_fw_choice(1);
    portENTER_CRITICAL(&s_mux);
    s_state.progress = 0;
    s_state.failed = false;
    s_state.rev++;
    portEXIT_CRITICAL(&s_mux);
}

static void on_later(lv_event_t *e)
{
    (void)e;
    notif_bridge_send_fw_choice(0);
    portENTER_CRITICAL(&s_mux);
    s_state.offer = false;
    s_state.rev++;
    portEXIT_CRITICAL(&s_mux);
    hide_card();
}

static void on_skip(lv_event_t *e)
{
    (void)e;
    fw_prompt_state_t v;
    snapshot(&v);
    notif_bridge_send_fw_choice(2);
    portENTER_CRITICAL(&s_mux);
    copy_str(s_skipped, sizeof(s_skipped), v.available);
    s_state.offer = false;
    s_state.rev++;
    portEXIT_CRITICAL(&s_mux);
    hide_card();
    ESP_LOGI(TAG, "skipping %s", v.available);
}

static lv_obj_t *make_button(lv_obj_t *parent, lv_color_t bg, lv_color_t fg,
                             lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_height(btn, 52);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, 52);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_pad_hor(btn, 18, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_color(lbl, fg, 0);
    lv_obj_set_style_text_font(lbl, &aabridge_font_24, 0);
    lv_obj_center(lbl);
    return btn;
}

static void apply_state(const fw_prompt_state_t *v)
{
    lv_label_set_text(s_title_lbl, v->title);
    lv_label_set_text(s_body_lbl, v->body);

    char ver[80];
    snprintf(ver, sizeof(ver), "%s  ->  %s", v->current, v->available);
    lv_label_set_text(s_ver_lbl, ver);

    lv_obj_t *now_lbl = lv_obj_get_child(s_btn_now, 0);
    lv_obj_t *later_lbl = lv_obj_get_child(s_btn_later, 0);
    lv_obj_t *skip_lbl = lv_obj_get_child(s_btn_skip, 0);
    if (now_lbl)   lv_label_set_text(now_lbl, v->now);
    if (later_lbl) lv_label_set_text(later_lbl, v->later);
    if (skip_lbl)  lv_label_set_text(skip_lbl, v->skip);

    if (v->failed) {
        lv_obj_clear_flag(s_status_lbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(s_status_lbl,
                                    lv_color_hex(FW_PROMPT_COLOR_ERROR), 0);
        lv_label_set_text(s_status_lbl, v->error);
        lv_obj_add_flag(s_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_btn_row, LV_OBJ_FLAG_HIDDEN);
    } else if (v->progress >= 0) {
        lv_obj_clear_flag(s_status_lbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(s_status_lbl, lv_color_white(), 0);
        char status[64];
        snprintf(status, sizeof(status), "%s %d%%", v->updating, v->progress);
        lv_label_set_text(s_status_lbl, status);
        lv_obj_clear_flag(s_bar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_value(s_bar, v->progress, LV_ANIM_OFF);
        lv_obj_add_flag(s_btn_row, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_status_lbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_btn_row, LV_OBJ_FLAG_HIDDEN);
    }
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    fw_prompt_state_t v;
    snapshot(&v);
    if (v.rev == s_applied_rev) return;

    if (!v.offer) {
        if (s_visible) hide_card();
        s_applied_rev = v.rev;
        return;
    }

    /* Android Auto draws straight to the panel and LVGL is paused; the card
     * would not be seen or touched. Keep the offer pending until the dashboard
     * or navigator is the live screen again. */
    if (!s_visible) {
        if (ui_mode_get() == UI_MODE_AA) return;
        lv_obj_clear_flag(s_backdrop, LV_OBJ_FLAG_HIDDEN);
        s_visible = true;
    }
    apply_state(&v);
    s_applied_rev = v.rev;
}

esp_err_t fw_prompt_init(void)
{
    if (s_initialized) return ESP_OK;
    if (lv_disp_get_default() == NULL) return ESP_ERR_INVALID_STATE;
    if (bsp_display_lock(1000) != ESP_OK) return ESP_FAIL;

    s_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_backdrop, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(s_backdrop, 0, 0);
    lv_obj_set_style_bg_color(s_backdrop, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_backdrop, LV_OPA_60, 0);
    lv_obj_set_style_border_width(s_backdrop, 0, 0);
    lv_obj_set_style_radius(s_backdrop, 0, 0);
    lv_obj_set_style_pad_all(s_backdrop, 0, 0);
    lv_obj_clear_flag(s_backdrop, LV_OBJ_FLAG_SCROLLABLE);
    /* Swallow taps so the dashboard behind the dialog does not react. */
    lv_obj_add_flag(s_backdrop, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_backdrop, LV_OBJ_FLAG_HIDDEN);

    s_card = lv_obj_create(s_backdrop);
    lv_obj_set_width(s_card, LV_PCT(90));
    lv_obj_set_height(s_card, LV_SIZE_CONTENT);
    lv_obj_center(s_card);
    lv_obj_set_style_bg_color(s_card, lv_color_hex(FW_PROMPT_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_card, lv_color_hex(FW_PROMPT_COLOR_ACCENT), 0);
    lv_obj_set_style_border_width(s_card, 2, 0);
    lv_obj_set_style_radius(s_card, 18, 0);
    lv_obj_set_style_pad_all(s_card, 20, 0);
    lv_obj_set_style_pad_row(s_card, 12, 0);
    lv_obj_set_flex_flow(s_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_card, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_card, LV_OBJ_FLAG_CLICKABLE);

    s_title_lbl = lv_label_create(s_card);
    lv_obj_set_width(s_title_lbl, LV_PCT(100));
    lv_label_set_long_mode(s_title_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_title_lbl, &aabridge_font_32, 0);
    lv_label_set_text(s_title_lbl, "");

    s_ver_lbl = lv_label_create(s_card);
    lv_obj_set_width(s_ver_lbl, LV_PCT(100));
    lv_label_set_long_mode(s_ver_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_ver_lbl,
                                lv_color_hex(0x7ab7ff), 0);
    lv_obj_set_style_text_font(s_ver_lbl, &aabridge_font_24, 0);
    lv_label_set_text(s_ver_lbl, "");

    s_body_lbl = lv_label_create(s_card);
    lv_obj_set_width(s_body_lbl, LV_PCT(100));
    lv_label_set_long_mode(s_body_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_body_lbl,
                                lv_color_hex(0xbfbfbf), 0);
    lv_obj_set_style_text_font(s_body_lbl, &aabridge_font_24, 0);
    lv_label_set_text(s_body_lbl, "");

    s_status_lbl = lv_label_create(s_card);
    lv_obj_set_width(s_status_lbl, LV_PCT(100));
    lv_label_set_long_mode(s_status_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_status_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_status_lbl, &aabridge_font_24, 0);
    lv_label_set_text(s_status_lbl, "");
    lv_obj_add_flag(s_status_lbl, LV_OBJ_FLAG_HIDDEN);

    s_bar = lv_bar_create(s_card);
    lv_obj_set_size(s_bar, LV_PCT(100), 14);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar,
                              lv_color_hex(FW_PROMPT_COLOR_ACCENT),
                              LV_PART_INDICATOR);
    lv_obj_add_flag(s_bar, LV_OBJ_FLAG_HIDDEN);

    s_btn_row = lv_obj_create(s_card);
    lv_obj_set_width(s_btn_row, LV_PCT(100));
    lv_obj_set_height(s_btn_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(s_btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_btn_row, 0, 0);
    lv_obj_set_style_pad_all(s_btn_row, 0, 0);
    lv_obj_set_style_pad_column(s_btn_row, 12, 0);
    lv_obj_set_style_pad_row(s_btn_row, 8, 0);
    /* Wraps on the 480x480 dashboard-only board; one row on the 800-wide
     * panels. */
    lv_obj_set_flex_flow(s_btn_row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(s_btn_row, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_btn_row, LV_OBJ_FLAG_SCROLLABLE);

    s_btn_later = make_button(s_btn_row, lv_color_hex(0x3a3a3a),
                              lv_color_white(), on_later);
    s_btn_skip = make_button(s_btn_row, lv_color_hex(0x3a3a3a),
                             lv_color_white(), on_skip);
    s_btn_now = make_button(s_btn_row, lv_color_hex(FW_PROMPT_COLOR_ACCENT),
                            lv_color_white(), on_now);
    /* Update is the primary action, so it sits last (rightmost). */

    lv_timer_create(poll_cb, POLL_PERIOD_MS, NULL);

    bsp_display_unlock();
    s_initialized = true;
    ESP_LOGI(TAG, "firmware prompt ready");
    return ESP_OK;
}
