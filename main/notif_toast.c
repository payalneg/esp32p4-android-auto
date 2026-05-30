/* Heads-up notification toast.
 *
 * Lives on lv_layer_top() so it floats above the dashboard, AA video,
 * the navigation map, anything. Pulls notifications out of notif_bridge
 * by comparing inbox_seq: every accepted NEW message bumps the counter,
 * we replay everything we haven't seen yet.
 *
 * Multiple notifications coming in close together queue up: we show the
 * current one for TOAST_DWELL_MS, then slide the next one in. New
 * messages that arrive while a card is on-screen reset the dwell timer
 * AND replace the contents — there's no point holding stale Anya text
 * when Telegram already pinged.
 *
 * Why we don't use lv_timer_set_repeat_count(1): LVGL auto-deletes the
 * timer after the single fire, but we still hold the pointer and `del`
 * it again from the callback → double-free, and the next `lv_timer_reset`
 * lands on a stale handle. The dismiss timer here is a normal repeating
 * one; the callback deletes it itself after running once. */

#include "notif_toast.h"

#include <stdio.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "lvgl.h"

#include "notif_bridge.h"
#include "fonts/aabridge_fonts.h"

static const char *TAG = "notif_toast";

#define TOAST_HEIGHT     140
#define TOAST_DWELL_MS   4500
#define TOAST_SLIDE_MS   220
#define POLL_PERIOD_MS   250

#define SCREEN_W         800
#define TOAST_OFF_Y      (-TOAST_HEIGHT - 24)
#define TOAST_ON_Y       16

static lv_obj_t *s_card;
static lv_obj_t *s_icon_box;     /* blue background, fallback for letter */
static lv_obj_t *s_icon_img;     /* lv_img with the PNG when cached */
static lv_obj_t *s_icon_letter;  /* first-letter fallback when not */
static lv_obj_t *s_app_lbl;
static lv_obj_t *s_title_lbl;
static lv_obj_t *s_text_lbl;

/* Backing descriptor for the currently-shown PNG. Points into
 * notif_bridge's LRU icon cache; rebuilt on every hash change. */
static lv_img_dsc_t s_icon_dsc;
static uint32_t     s_shown_icon_hash;

static lv_timer_t *s_poll;
static lv_timer_t *s_dismiss;

static uint32_t s_last_seen_seq;
static bool s_card_visible;     /* true while card is on-screen */
static bool s_initialized;

static void show_icon_png(const uint8_t *png, size_t len)
{
    s_icon_dsc.header.always_zero = 0;
    s_icon_dsc.header.w = 0;
    s_icon_dsc.header.h = 0;
    s_icon_dsc.header.cf = LV_IMG_CF_RAW_ALPHA;
    s_icon_dsc.data = png;
    s_icon_dsc.data_size = len;
    lv_img_set_src(s_icon_img, &s_icon_dsc);
    lv_obj_center(s_icon_img);
    lv_obj_clear_flag(s_icon_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_icon_letter, LV_OBJ_FLAG_HIDDEN);
    /* PNG carries its own backdrop — turn the blue tile transparent so
     * an opaque app icon doesn't sit on a coloured square. */
    lv_obj_set_style_bg_opa(s_icon_box, LV_OPA_TRANSP, 0);
}

static void show_icon_letter(void)
{
    lv_obj_add_flag(s_icon_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_icon_letter, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_opa(s_icon_box, LV_OPA_COVER, 0);
}

static void apply_icon(const notif_msg_t *m)
{
    if (m->icon_hash == s_shown_icon_hash) return;
    s_shown_icon_hash = m->icon_hash;

    if (m->icon_hash == 0) {
        show_icon_letter();
        return;
    }
    size_t len = 0;
    const uint8_t *png = notif_bridge_get_icon(m->icon_hash, &len);
    if (png && len > 0) {
        show_icon_png(png, len);
    } else {
        /* Not in the cache yet — show the letter and nudge the phone to
         * resend the PNG. Next notification update with the same hash
         * will pick it up. */
        show_icon_letter();
        notif_bridge_send_cmd(NOTIF_OP_REQUEST_ICON, m->icon_hash);
    }
}

static void render(const notif_msg_t *m)
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
    apply_icon(m);
}

static void slide_anim(int32_t from, int32_t to, lv_anim_path_cb_t path)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_card);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_time(&a, TOAST_SLIDE_MS);
    lv_anim_set_path_cb(&a, path);
    lv_anim_set_values(&a, from, to);
    lv_anim_start(&a);
}

static void schedule_dismiss(void);   /* fwd */

static void dismiss_cb(lv_timer_t *t)
{
    (void)t;
    /* Belt-and-braces: clear our pointer before lv_timer_del so a
     * spurious double-fire (rare but possible if the timer is reset
     * concurrently from the LVGL task) doesn't free twice. */
    lv_timer_t *me = s_dismiss;
    s_dismiss = NULL;
    if (me) lv_timer_del(me);

    if (!s_card_visible) return;
    slide_anim(TOAST_ON_Y, TOAST_OFF_Y, lv_anim_path_ease_in);
    s_card_visible = false;
}

static void schedule_dismiss(void)
{
    if (s_dismiss) {
        lv_timer_reset(s_dismiss);
        return;
    }
    s_dismiss = lv_timer_create(dismiss_cb, TOAST_DWELL_MS, NULL);
    /* No repeat_count(1) — see file header for why. */
}

static void present(const notif_msg_t *m)
{
    render(m);
    if (s_card_visible) {
        /* Card already on-screen — just refresh content and bump the
         * dwell timer, no slide animation. */
        schedule_dismiss();
        return;
    }
    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_y(s_card, TOAST_OFF_Y);
    slide_anim(TOAST_OFF_Y, TOAST_ON_Y, lv_anim_path_ease_out);
    s_card_visible = true;
    schedule_dismiss();
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    uint32_t seq = notif_bridge_inbox_seq();
    if (seq == s_last_seen_seq) return;

    /* New stuff landed since last tick. Replay the freshest one — if
     * many came in at once we'd just queue them, but in practice 250 ms
     * is much shorter than the slide+dwell cycle so showing the latest
     * is the same UX as Mi Fit. */
    notif_msg_t buf[1];
    if (notif_bridge_recent(buf, 1) > 0 && !buf[0].removed) {
        present(&buf[0]);
    }
    s_last_seen_seq = seq;
}

esp_err_t notif_toast_init(void)
{
    if (s_initialized) return ESP_OK;
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl lock timeout");
        return ESP_FAIL;
    }

    s_card = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_card, LV_PCT(92), TOAST_HEIGHT);
    lv_obj_set_x(s_card, (SCREEN_W - (int)(SCREEN_W * 0.92)) / 2);
    lv_obj_set_y(s_card, TOAST_OFF_Y);
    lv_obj_set_style_bg_color(s_card, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_bg_opa(s_card, LV_OPA_90, 0);
    lv_obj_set_style_border_color(s_card, lv_color_hex(0x1f6feb), 0);
    lv_obj_set_style_border_width(s_card, 2, 0);
    lv_obj_set_style_radius(s_card, 14, 0);
    lv_obj_set_style_pad_all(s_card, 14, 0);
    lv_obj_set_flex_flow(s_card, LV_FLEX_FLOW_ROW);
    /* Vertically center children — the 72 px icon would otherwise hug
     * the top of the 140 px card and leave a lopsided gap below. */
    lv_obj_set_flex_align(s_card, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s_card, 16, 0);
    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_card, LV_OBJ_FLAG_HIDDEN);

    /* Icon slot — blue square serves as both letter-fallback background
     * and host for the lv_img widget when an actual PNG is in the cache.
     * Children share the same center via lv_obj_center / alignment. */
    s_icon_box = lv_obj_create(s_card);
    /* Tile matches the 72 px PNG the phone sends — lv_img would
     * otherwise tile a smaller-than-box source rather than scale. */
    lv_obj_set_size(s_icon_box, 72, 72);
    lv_obj_set_style_bg_color(s_icon_box, lv_color_hex(0x1f6feb), 0);
    lv_obj_set_style_radius(s_icon_box, 18, 0);
    lv_obj_set_style_border_width(s_icon_box, 0, 0);
    lv_obj_set_style_pad_all(s_icon_box, 0, 0);
    lv_obj_set_style_clip_corner(s_icon_box, true, 0);
    lv_obj_clear_flag(s_icon_box,
                      LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    s_icon_img = lv_img_create(s_icon_box);
    /* DO NOT lv_obj_set_size() here — lv_img falls back to tiling when
     * the widget is larger than the source PNG (the 72 px icon ends up
     * as one main tile plus three partial edge tiles inside the 96 px
     * box). Let lv_img self-size from the PNG header, center it, and
     * scale-fit to the box via lv_img_set_zoom() in show_icon_png(). */
    lv_obj_center(s_icon_img);
    lv_obj_add_flag(s_icon_img, LV_OBJ_FLAG_HIDDEN);

    s_icon_letter = lv_label_create(s_icon_box);
    lv_obj_set_style_text_color(s_icon_letter, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_icon_letter, &aabridge_font_48, 0);
    lv_label_set_text(s_icon_letter, "?");
    lv_obj_center(s_icon_letter);

    lv_obj_t *col = lv_obj_create(s_card);
    lv_obj_set_size(col, LV_PCT(80), LV_PCT(100));
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, 4, 0);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    s_app_lbl = lv_label_create(col);
    lv_obj_set_style_text_color(s_app_lbl, lv_color_hex(0x9a9a9a), 0);
    lv_obj_set_style_text_font(s_app_lbl, &aabridge_font_24, 0);
    lv_label_set_text(s_app_lbl, "");

    /* Sender (title) is the "from whom" line — smaller now so the
     * actual message gets visual priority. Body text bumped to the
     * larger font so a glance is enough to read. */
    s_title_lbl = lv_label_create(col);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_title_lbl, &aabridge_font_24, 0);
    lv_label_set_long_mode(s_title_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s_title_lbl, LV_PCT(100));
    lv_label_set_text(s_title_lbl, "");

    s_text_lbl = lv_label_create(col);
    lv_obj_set_style_text_color(s_text_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_text_lbl, &aabridge_font_24, 0);
    lv_label_set_long_mode(s_text_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s_text_lbl, LV_PCT(100));
    lv_label_set_text(s_text_lbl, "");

    /* Initialise seq baseline so we don't replay history that landed
     * before init (e.g. test notifications fired while the dashboard
     * was still building). */
    s_last_seen_seq = notif_bridge_inbox_seq();

    s_poll = lv_timer_create(poll_cb, POLL_PERIOD_MS, NULL);

    bsp_display_unlock();
    s_initialized = true;
    ESP_LOGI(TAG, "toast overlay ready");
    return ESP_OK;
}
