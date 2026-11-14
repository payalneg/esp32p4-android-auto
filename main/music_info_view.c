/* Music-info widget driver.
 *
 * Renders into the `dashboard_music_info_tile` that GUI Guider
 * lays out on the dashboard. Supports two adaptive layouts:
 *
 * 1. Music mode (~1:1 square album art):
 *    - Background: #000808 (seamless with dashboard background).
 *    - Left side: square album art card (120×120 on 800x480, 96×96 on 480x480).
 *      Fallback placeholder square with audio symbol when no art is available.
 *    - Right side: track title, artist name, and refined capsule transport buttons
 *      (Previous, Play/Pause toggle with signature lime accent, Next).
 *
 * 2. Video mode (~16:9 preview, e.g. YouTube):
 *    - Full-bleed background art covering the entire card tile.
 *    - Dark readability overlay across the art.
 *    - Track title and artist anchored at the bottom across full width.
 *    - Transport controls hidden.
 *
 * Polling once a tick — cheap; the LVGL task is the natural home for
 * this since strlcpy/lv_label_set_text both expect the LVGL mutex. */

#include "music_info_view.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "img_jpeg.h"

#include "notif_bridge.h"
#include "fonts/aabridge_fonts.h"

LV_FONT_DECLARE(lv_font_montserratMedium_20);
LV_FONT_DECLARE(lv_font_montserratMedium_16);

static const char *TAG = "music_info_view";

#define POLL_PERIOD_MS    400
#define TILE_W            344
#define TILE_H            136

#define ART_ALIGN16(v)    (((v) + 15) & ~15)
#define ART_MCU_W         ART_ALIGN16(TILE_W)
#define ART_MCU_H         ART_ALIGN16(TILE_H)
#define ART_BUF_SIZE      (ART_MCU_W * ART_MCU_H * 2)

static uint8_t *s_art_decoded;          /* RGB565, the decoded frame */
static uint16_t s_art_w, s_art_h;       /* what the last decode produced */

/* Art card size actually used, clamped to the tile we were attached to. */
static lv_coord_t s_tile_w = TILE_W;
static lv_coord_t s_tile_h = TILE_H;

static lv_obj_t *s_root;
static lv_obj_t *s_card;
static lv_obj_t *s_video_art_img;
static lv_obj_t *s_art_box;
static lv_obj_t *s_art_img;
static lv_obj_t *s_art_glyph;
static lv_obj_t *s_overlay;
static lv_obj_t *s_title_lbl;
static lv_obj_t *s_artist_lbl;

/* Transport controls */
static lv_obj_t *s_btn_row;
static lv_obj_t *s_btn_prev;
static lv_obj_t *s_btn_play;
static lv_obj_t *s_btn_next;
static lv_obj_t *s_lbl_play;

static lv_timer_t *s_poll;

/* Last-rendered snapshot to skip noop updates */
static char s_last_title[128];
static char s_last_artist[128];
static int  s_last_playing = -1;
static uint32_t s_last_art_hash;
static int  s_current_layout = -1; /* -1 = uninit, 0 = music, 1 = video */

static lv_img_dsc_t s_art_dsc;

static bool is_video_app(const char *pkg)
{
    if (!pkg || !pkg[0]) return false;
    char lower[64];
    size_t i;
    for (i = 0; i < sizeof(lower) - 1 && pkg[i]; i++) {
        char c = pkg[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c + ('a' - 'A'));
        lower[i] = c;
    }
    lower[i] = '\0';

    if (strstr(lower, "youtube") && !strstr(lower, "music")) return true;
    if (strstr(lower, "video")) return true;
    if (strstr(lower, "rutube")) return true;
    if (strstr(lower, "kinopoisk")) return true;
    if (strstr(lower, "netflix")) return true;
    if (strstr(lower, "vlc")) return true;
    return false;
}

static bool is_video_mode(void)
{
    const media_state_t *m = notif_bridge_get_media();
    if (s_art_w > 0 && s_art_h > 0 && m->album_art_hash != 0) {
        return ((uint32_t)s_art_w * 100 / s_art_h) >= 135;
    }
    return is_video_app(m->source_app);
}

static void btn_prev_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "user tapped media PREV");
        notif_bridge_send_cmd(NOTIF_OP_PREV, 0);
    }
}

static void btn_play_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        const media_state_t *m = notif_bridge_get_media();
        notif_op_t op = m->is_playing ? NOTIF_OP_PAUSE : NOTIF_OP_PLAY;
        ESP_LOGI(TAG, "user tapped media %s", op == NOTIF_OP_PAUSE ? "PAUSE" : "PLAY");
        notif_bridge_send_cmd(op, 0);
        if (s_lbl_play) {
            lv_label_set_text(s_lbl_play, (op == NOTIF_OP_PLAY) ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        }
    }
}

static void btn_next_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "user tapped media NEXT");
        notif_bridge_send_cmd(NOTIF_OP_NEXT, 0);
    }
}

#if defined(LV_SIMSHIM)
static void sim_overlay_click_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "sim: tapped video preview -> NEXT");
        notif_bridge_send_cmd(NOTIF_OP_NEXT, 0);
    }
}
#endif

static void apply_layout(bool is_video, bool force)
{
    int mode = is_video ? 1 : 0;
    if (!force && s_current_layout == mode) return;
    s_current_layout = mode;

    lv_coord_t pad_v = (s_tile_h < 100) ? 5 : ((s_tile_h < 120) ? 6 : 8);
    lv_coord_t pad_h = (s_tile_w < 280) ? 6 : 8;
    lv_coord_t max_art = s_tile_h - 2 * pad_v;
    if (max_art > 120) max_art = 120;

    lv_coord_t art_size = max_art;
    if (s_tile_w < 280) {
        lv_coord_t narrow_cap = (s_tile_h < 100) ? 76 : 96;
        if (art_size > narrow_cap) art_size = narrow_cap;
    }
    lv_coord_t art_rad = (art_size >= 110) ? 14 : ((art_size >= 85) ? 10 : 8);

    if (is_video) {
        /* Video Mode (16:9, e.g. YouTube):
         * Full-bleed background art, dark overlay, title & artist anchored
         * along the bottom across full width. Transport controls hidden. */
        if (s_btn_row) lv_obj_add_flag(s_btn_row, LV_OBJ_FLAG_HIDDEN);
        if (s_art_box) lv_obj_add_flag(s_art_box, LV_OBJ_FLAG_HIDDEN);
        if (s_card) {
            lv_obj_set_style_bg_opa(s_card, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(s_card, 0, 0);
        }
        if (s_video_art_img) {
            lv_obj_clear_flag(s_video_art_img, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_overlay) {
            lv_obj_set_size(s_overlay, s_tile_w, s_tile_h);
            lv_obj_align(s_overlay, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_bg_opa(s_overlay, LV_OPA_30, 0);
            lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
        }

        lv_coord_t v_pad_h = (s_tile_w < 280) ? 14 : 28;
        lv_coord_t lbl_w = s_tile_w - v_pad_h;
        lv_coord_t title_bot_y, artist_bot_y;
        if (s_tile_h < 100) {
            title_bot_y  = -22;
            artist_bot_y = -2;
        } else if (s_tile_h < 120) {
            title_bot_y  = -25;
            artist_bot_y = -3;
        } else {
            title_bot_y  = -28;
            artist_bot_y = -4;
        }

        if (s_title_lbl) {
            lv_obj_set_width(s_title_lbl, lbl_w);
            lv_obj_align(s_title_lbl, LV_ALIGN_BOTTOM_MID, 0, title_bot_y);
            lv_obj_set_style_text_align(s_title_lbl, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_move_foreground(s_title_lbl);
        }
        if (s_artist_lbl) {
            lv_obj_set_width(s_artist_lbl, lbl_w);
            lv_obj_align(s_artist_lbl, LV_ALIGN_BOTTOM_MID, 0, artist_bot_y);
            lv_obj_set_style_text_align(s_artist_lbl, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_move_foreground(s_artist_lbl);
        }
    } else {
        /* Music Mode (1:1 square cover):
         * - Background fill: #000808 (matches dashboard background)
         * - LEFT side: square album art or fallback placeholder
         * - RIGHT side: track title, artist name, and transport buttons */
        if (s_video_art_img) lv_obj_add_flag(s_video_art_img, LV_OBJ_FLAG_HIDDEN);
        if (s_overlay) lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);

        if (s_card) {
            lv_obj_set_style_bg_color(s_card, lv_color_hex(0x000808), 0);
            lv_obj_set_style_bg_opa(s_card, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(s_card, 1, 0);
            lv_obj_set_style_border_color(s_card, lv_color_hex(0x0c1c1e), 0);
        }

        /* Cover square on the LEFT */
        if (s_art_box) {
            lv_obj_set_size(s_art_box, art_size, art_size);
            lv_obj_align(s_art_box, LV_ALIGN_LEFT_MID, pad_h, 0);
            lv_obj_set_style_radius(s_art_box, art_rad, 0);
            lv_obj_clear_flag(s_art_box, LV_OBJ_FLAG_HIDDEN);
        }

        /* Available width for the RIGHT column */
        lv_coord_t gap_x = (s_tile_w < 280) ? ((s_tile_h < 100) ? 8 : 10) : 14;
        lv_coord_t left_pad = pad_h + art_size + gap_x;
        lv_coord_t right_w  = s_tile_w - left_pad - pad_h;
        if (right_w < 100) right_w = 100;

        lv_coord_t title_y, artist_y, btn_y;
        if (s_tile_h < 100) {
            title_y  = 3;
            artist_y = 25;
            btn_y    = 50;
        } else if (s_tile_h < 120) {
            title_y  = 6;
            artist_y = 30;
            btn_y    = 60;
        } else {
            title_y  = 6;
            artist_y = 32;
            btn_y    = 72;
        }

        if (s_title_lbl) {
            lv_obj_set_width(s_title_lbl, right_w);
            lv_obj_align(s_title_lbl, LV_ALIGN_TOP_LEFT, left_pad, title_y);
            lv_obj_set_style_text_align(s_title_lbl, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_move_foreground(s_title_lbl);
        }
        if (s_artist_lbl) {
            lv_obj_set_width(s_artist_lbl, right_w);
            lv_obj_align(s_artist_lbl, LV_ALIGN_TOP_LEFT, left_pad, artist_y);
            lv_obj_set_style_text_align(s_artist_lbl, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_move_foreground(s_artist_lbl);
        }

        if (s_btn_row) {
            lv_obj_set_width(s_btn_row, right_w);
            lv_obj_align(s_btn_row, LV_ALIGN_TOP_LEFT, left_pad, btn_y);
            lv_obj_clear_flag(s_btn_row, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(s_btn_row);
        }
    }
}

static void show_fallback(void)
{
    if (s_art_img) lv_obj_add_flag(s_art_img, LV_OBJ_FLAG_HIDDEN);
    if (s_video_art_img) lv_obj_add_flag(s_video_art_img, LV_OBJ_FLAG_HIDDEN);
    if (s_overlay) lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    if (s_current_layout == 1) {
        if (s_art_box) lv_obj_add_flag(s_art_box, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (s_art_box) lv_obj_clear_flag(s_art_box, LV_OBJ_FLAG_HIDDEN);
        if (s_art_glyph) lv_obj_clear_flag(s_art_glyph, LV_OBJ_FLAG_HIDDEN);
    }
}

static void show_art(void)
{
    if (s_current_layout == 1) {
        if (s_video_art_img) lv_obj_clear_flag(s_video_art_img, LV_OBJ_FLAG_HIDDEN);
        if (s_overlay) lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
        if (s_art_box) lv_obj_add_flag(s_art_box, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (s_video_art_img) lv_obj_add_flag(s_video_art_img, LV_OBJ_FLAG_HIDDEN);
        if (s_overlay) lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
        if (s_art_box) lv_obj_clear_flag(s_art_box, LV_OBJ_FLAG_HIDDEN);
        if (s_art_img) lv_obj_clear_flag(s_art_img, LV_OBJ_FLAG_HIDDEN);
        if (s_art_glyph) lv_obj_add_flag(s_art_glyph, LV_OBJ_FLAG_HIDDEN);
    }
}

static bool decode_art_jpeg(const uint8_t *src, size_t len)
{
    if (!s_art_decoded) return false;
    return img_jpeg_decode_rgb565(src, len, s_art_decoded, ART_BUF_SIZE,
                                  &s_art_w, &s_art_h);
}

static void apply_art(uint32_t hash)
{
    if (hash == s_last_art_hash) return;

    size_t len = 0;
    const uint8_t *bytes = hash ? notif_bridge_get_icon(hash, &len) : NULL;
    if (bytes && len > 0 && decode_art_jpeg(bytes, len)) {
        s_art_dsc.header.always_zero = 0;
        s_art_dsc.header.w = s_art_w;
        s_art_dsc.header.h = s_art_h;
        s_art_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
        s_art_dsc.data = s_art_decoded;
        s_art_dsc.data_size = (uint32_t)s_art_w * s_art_h * 2u;
        lv_img_cache_invalidate_src(&s_art_dsc);

        bool is_video = is_video_mode();
        apply_layout(is_video, true);

        lv_coord_t pad_v = (s_tile_h < 100) ? 5 : ((s_tile_h < 120) ? 6 : 8);
        lv_coord_t max_art = s_tile_h - 2 * pad_v;
        if (max_art > 120) max_art = 120;

        lv_coord_t art_size = max_art;
        if (s_tile_w < 280) {
            lv_coord_t narrow_cap = (s_tile_h < 100) ? 76 : 96;
            if (art_size > narrow_cap) art_size = narrow_cap;
        }

        if (s_art_img && s_art_w > 0 && s_art_h > 0) {
            lv_img_set_src(s_art_img, &s_art_dsc);
            lv_img_set_pivot(s_art_img, s_art_w / 2, s_art_h / 2);
            uint16_t zw = (art_size * 256) / s_art_w;
            uint16_t zh = (art_size * 256) / s_art_h;
            uint16_t zoom = (zw > zh) ? zw : zh;
            lv_img_set_zoom(s_art_img, zoom);
            lv_obj_center(s_art_img);
        }
        if (s_video_art_img && s_art_w > 0 && s_art_h > 0) {
            lv_img_set_src(s_video_art_img, &s_art_dsc);
            lv_img_set_pivot(s_video_art_img, s_art_w / 2, s_art_h / 2);
            uint16_t zw = (s_tile_w * 256) / s_art_w;
            uint16_t zh = (s_tile_h * 256) / s_art_h;
            uint16_t zoom_v = (zw > zh) ? zw : zh;
            lv_img_set_zoom(s_video_art_img, zoom_v);
            lv_obj_center(s_video_art_img);
        }

        show_art();
        ESP_LOGI(TAG, "art hash=0x%08X jpeg_len=%u -> %ux%u rgb565 (%s)",
                 (unsigned)hash, (unsigned)len,
                 (unsigned)s_art_w, (unsigned)s_art_h,
                 is_video ? "video 16:9" : "music 1:1");
    } else if (bytes && len > 0) {
        ESP_LOGW(TAG, "art hash=0x%08X len=%u -- decode rejected",
                 (unsigned)hash, (unsigned)len);
        return;
    } else {
        bool is_video = is_video_mode();
        apply_layout(is_video, true);
        show_fallback();
        if (hash != 0) {
            notif_bridge_send_cmd(NOTIF_OP_REQUEST_ART, hash);
        }
    }
    s_last_art_hash = hash;
}

static void set_container_visible(bool on)
{
    if (!s_root) return;
    lv_obj_t *container = lv_obj_get_parent(s_root);
    if (!container || container == lv_scr_act() || lv_obj_get_parent(container) == NULL) return;
    if (on) lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);
    else    lv_obj_add_flag (container, LV_OBJ_FLAG_HIDDEN);
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_root || lv_obj_get_screen(s_root) != lv_scr_act()) {
        return;
    }
    const media_state_t *m = notif_bridge_get_media();

    if (m->title[0] == 0 && m->artist[0] == 0) {
        if (s_last_title[0] != '\0' || s_last_artist[0] != '\0') {
            if (s_title_lbl) lv_label_set_text(s_title_lbl, "");
            if (s_artist_lbl) lv_label_set_text(s_artist_lbl, "");
            s_last_title[0] = '\0';
            s_last_artist[0] = '\0';
            s_last_playing = -1;
        }
        if (s_title_lbl) lv_obj_add_flag(s_title_lbl, LV_OBJ_FLAG_HIDDEN);
        if (s_artist_lbl) lv_obj_add_flag(s_artist_lbl, LV_OBJ_FLAG_HIDDEN);
        if (s_btn_row) lv_obj_add_flag(s_btn_row, LV_OBJ_FLAG_HIDDEN);
        apply_art(0);
        set_container_visible(false);
        return;
    }

    set_container_visible(true);
    if (s_title_lbl) lv_obj_clear_flag(s_title_lbl, LV_OBJ_FLAG_HIDDEN);
    if (s_artist_lbl) lv_obj_clear_flag(s_artist_lbl, LV_OBJ_FLAG_HIDDEN);

    if (strcmp(s_last_title, m->title) != 0) {
        if (s_title_lbl) lv_label_set_text(s_title_lbl, m->title);
        strncpy(s_last_title, m->title, sizeof(s_last_title) - 1);
        s_last_title[sizeof(s_last_title) - 1] = '\0';
    }
    if (strcmp(s_last_artist, m->artist) != 0) {
        if (s_artist_lbl) lv_label_set_text(s_artist_lbl, m->artist);
        strncpy(s_last_artist, m->artist, sizeof(s_last_artist) - 1);
        s_last_artist[sizeof(s_last_artist) - 1] = '\0';
    }

    if (s_last_playing != (int)m->is_playing) {
        s_last_playing = m->is_playing ? 1 : 0;
        if (s_lbl_play) {
            lv_label_set_text(s_lbl_play, m->is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        }
    }

    bool is_video = is_video_mode();
    apply_layout(is_video, false);

    if (!is_video && s_btn_row) {
        lv_obj_clear_flag(s_btn_row, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_btn_row);
    }

    apply_art(m->album_art_hash);
}

static esp_err_t init_jpeg_pipeline(void)
{
    if (s_art_decoded) return ESP_OK;
    esp_err_t r = img_jpeg_init();
    if (r != ESP_OK) return r;
    s_art_decoded = img_jpeg_alloc_rgb565(ART_BUF_SIZE);
    if (!s_art_decoded) {
        ESP_LOGE(TAG, "no PSRAM for %u-byte art buffer", (unsigned)ART_BUF_SIZE);
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "jpeg pipeline ready, %u-byte rgb565 framebuffer",
             (unsigned)ART_BUF_SIZE);
    return ESP_OK;
}

esp_err_t music_info_view_attach(lv_obj_t *parent)
{
    if (!parent) return ESP_ERR_INVALID_ARG;
    if (s_root) return ESP_OK;
    s_root = parent;

    init_jpeg_pipeline();

    lv_obj_update_layout(s_root);
    lv_coord_t avail_w = lv_obj_get_width(s_root);
    lv_coord_t avail_h = lv_obj_get_height(s_root);
    s_tile_w = TILE_W;
    s_tile_h = TILE_H;
    if (avail_w > 16 && avail_w < TILE_W) s_tile_w = avail_w;
    if (avail_h > 16 && avail_h < TILE_H) s_tile_h = avail_h;

    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_root, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_root, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_root, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(s_root, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_root, 0, LV_PART_MAIN);

    lv_coord_t pad_v = (s_tile_h < 100) ? 5 : ((s_tile_h < 120) ? 6 : 8);
    lv_coord_t pad_h = (s_tile_w < 280) ? 6 : 8;
    lv_coord_t max_art = s_tile_h - 2 * pad_v;
    if (max_art > 120) max_art = 120;

    lv_coord_t art_size = max_art;
    if (s_tile_w < 280) {
        lv_coord_t narrow_cap = (s_tile_h < 100) ? 76 : 96;
        if (art_size > narrow_cap) art_size = narrow_cap;
    }
    lv_coord_t art_rad = (art_size >= 110) ? 14 : ((art_size >= 85) ? 10 : 8);
    lv_coord_t card_rad = (s_tile_h < 110) ? 14 : 18;

    /* Centered card container */
    s_card = lv_obj_create(s_root);
    lv_obj_set_size(s_card, s_tile_w, s_tile_h);
    lv_obj_align(s_card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_card, lv_color_hex(0x000808), 0);
    lv_obj_set_style_bg_opa(s_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_card, 1, 0);
    lv_obj_set_style_border_color(s_card, lv_color_hex(0x0c1c1e), 0);
    lv_obj_set_style_radius(s_card, card_rad, 0);
    lv_obj_set_style_clip_corner(s_card, true, 0);
    lv_obj_set_style_pad_all(s_card, 0, 0);
    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_SCROLLABLE);

    /* Video art image (fills entire card in video mode) */
    s_video_art_img = lv_img_create(s_card);
    lv_obj_set_size(s_video_art_img, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(s_video_art_img);
    lv_obj_add_flag(s_video_art_img, LV_OBJ_FLAG_HIDDEN);

    /* Readability overlay (video mode only) */
    s_overlay = lv_obj_create(s_card);
    lv_obj_set_size(s_overlay, s_tile_w, s_tile_h);
    lv_obj_align(s_overlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_30, 0);
    lv_obj_set_style_border_width(s_overlay, 0, 0);
    lv_obj_set_style_radius(s_overlay, card_rad, 0);
    lv_obj_set_style_clip_corner(s_overlay, true, 0);
    lv_obj_set_style_pad_all(s_overlay, 0, 0);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
#if defined(LV_SIMSHIM)
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_overlay, sim_overlay_click_cb, LV_EVENT_CLICKED, NULL);
#else
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_CLICKABLE);
#endif
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);

    /* Square art container on left (music mode) */
    s_art_box = lv_obj_create(s_card);
    lv_obj_set_size(s_art_box, art_size, art_size);
    lv_obj_align(s_art_box, LV_ALIGN_LEFT_MID, pad_h, 0);
    lv_obj_set_style_bg_color(s_art_box, lv_color_hex(0x071516), 0);
    lv_obj_set_style_border_width(s_art_box, 1, 0);
    lv_obj_set_style_border_color(s_art_box, lv_color_hex(0x10282a), 0);
    lv_obj_set_style_radius(s_art_box, art_rad, 0);
    lv_obj_set_style_clip_corner(s_art_box, true, 0);
    lv_obj_set_style_pad_all(s_art_box, 0, 0);
    lv_obj_clear_flag(s_art_box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    /* Music album art image (inside square box) */
    s_art_img = lv_img_create(s_art_box);
    lv_obj_set_size(s_art_img, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(s_art_img);
    lv_obj_add_flag(s_art_img, LV_OBJ_FLAG_HIDDEN);

    /* Fallback audio glyph (inside square box) */
    s_art_glyph = lv_label_create(s_art_box);
    lv_obj_set_style_text_color(s_art_glyph, lv_color_hex(0xb5ff00), 0);
    lv_obj_set_style_text_font(s_art_glyph, (art_size < 80) ? &lv_font_montserratMedium_16 : &lv_font_montserratMedium_20, 0);
    lv_label_set_text(s_art_glyph, LV_SYMBOL_AUDIO);
    lv_obj_center(s_art_glyph);

    /* Text labels */
    s_title_lbl = lv_label_create(s_card);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_title_lbl, &aabridge_font_24, 0);
    lv_label_set_long_mode(s_title_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(s_title_lbl, "—");

    s_artist_lbl = lv_label_create(s_card);
    lv_obj_set_style_text_color(s_artist_lbl, lv_color_hex(0x8ca0a2), 0);
    lv_obj_set_style_text_font(s_artist_lbl, &aabridge_font_24, 0);
    lv_label_set_long_mode(s_artist_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(s_artist_lbl, "");

    /* Transport button row */
    s_btn_row = lv_obj_create(s_card);
    lv_coord_t btn_gap, side_w, play_w, play_h;
    const lv_font_t *btn_font;
    if (s_tile_h < 100) {
        btn_gap   = 10;
        side_w    = 24;
        play_w    = 30;
        play_h    = 30;
        btn_font  = &lv_font_montserratMedium_16;
    } else if (s_tile_h < 120) {
        btn_gap   = 14;
        side_w    = 28;
        play_w    = 34;
        play_h    = 34;
        btn_font  = &lv_font_montserratMedium_20;
    } else {
        btn_gap   = (s_tile_w < 280) ? 14 : 20;
        side_w    = (s_tile_w < 280) ? 30 : 36;
        play_w    = (s_tile_w < 280) ? 36 : 42;
        play_h    = (s_tile_w < 280) ? 36 : 42;
        btn_font  = &lv_font_montserratMedium_20;
    }

    lv_obj_set_height(s_btn_row, play_h + 4);
    lv_obj_set_style_bg_opa(s_btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_btn_row, 0, 0);
    lv_obj_set_style_pad_all(s_btn_row, 0, 0);
    lv_obj_set_style_pad_column(s_btn_row, btn_gap, 0);
    lv_obj_clear_flag(s_btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(s_btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_btn_row, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Prev button (frameless icon, centered) */
    s_btn_prev = lv_btn_create(s_btn_row);
    lv_obj_set_size(s_btn_prev, side_w, play_h);
    lv_obj_set_style_bg_opa(s_btn_prev, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(s_btn_prev, LV_OPA_30, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(s_btn_prev, lv_color_hex(0x183c40), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(s_btn_prev, 0, 0);
    lv_obj_set_style_shadow_width(s_btn_prev, 0, 0);
    lv_obj_set_style_radius(s_btn_prev, 8, 0);
    lv_obj_set_style_pad_all(s_btn_prev, 0, 0);
    lv_obj_add_event_cb(s_btn_prev, btn_prev_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_prev = lv_label_create(s_btn_prev);
    lv_obj_set_style_text_color(lbl_prev, lv_color_hex(0x8ca0a2), 0);
    lv_obj_set_style_text_color(lbl_prev, lv_color_hex(0xb5ff00), LV_STATE_PRESSED);
    lv_obj_set_style_text_font(lbl_prev, btn_font, 0);
    lv_label_set_text(lbl_prev, LV_SYMBOL_PREV);
    lv_obj_center(lbl_prev);

    /* Play/Pause button (rounded-square outline with signature lime accent) */
    s_btn_play = lv_btn_create(s_btn_row);
    lv_obj_set_size(s_btn_play, play_w, play_h);
    lv_obj_set_style_bg_opa(s_btn_play, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(s_btn_play, LV_OPA_30, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(s_btn_play, lv_color_hex(0xb5ff00), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(s_btn_play, 2, 0);
    lv_obj_set_style_border_color(s_btn_play, lv_color_hex(0xb5ff00), 0);
    lv_obj_set_style_border_opa(s_btn_play, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_btn_play, (play_h < 34) ? 7 : 9, 0);
    lv_obj_set_style_shadow_width(s_btn_play, 0, 0);
    lv_obj_set_style_pad_all(s_btn_play, 0, 0);
    lv_obj_add_event_cb(s_btn_play, btn_play_cb, LV_EVENT_CLICKED, NULL);

    s_lbl_play = lv_label_create(s_btn_play);
    lv_obj_set_style_text_color(s_lbl_play, lv_color_hex(0xb5ff00), 0);
    lv_obj_set_style_text_font(s_lbl_play, btn_font, 0);
    lv_label_set_text(s_lbl_play, LV_SYMBOL_PLAY);
    lv_obj_center(s_lbl_play);

    /* Next button (frameless icon, centered) */
    s_btn_next = lv_btn_create(s_btn_row);
    lv_obj_set_size(s_btn_next, side_w, play_h);
    lv_obj_set_style_bg_opa(s_btn_next, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(s_btn_next, LV_OPA_30, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(s_btn_next, lv_color_hex(0x183c40), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(s_btn_next, 0, 0);
    lv_obj_set_style_shadow_width(s_btn_next, 0, 0);
    lv_obj_set_style_radius(s_btn_next, 8, 0);
    lv_obj_set_style_pad_all(s_btn_next, 0, 0);
    lv_obj_add_event_cb(s_btn_next, btn_next_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_next = lv_label_create(s_btn_next);
    lv_obj_set_style_text_color(lbl_next, lv_color_hex(0x8ca0a2), 0);
    lv_obj_set_style_text_color(lbl_next, lv_color_hex(0xb5ff00), LV_STATE_PRESSED);
    lv_obj_set_style_text_font(lbl_next, btn_font, 0);
    lv_label_set_text(lbl_next, LV_SYMBOL_NEXT);
    lv_obj_center(lbl_next);

    s_current_layout = -1;
    apply_layout(false, true);

    s_poll = lv_timer_create(poll_cb, POLL_PERIOD_MS, NULL);
    poll_cb(s_poll);

    ESP_LOGI(TAG, "attached to %p (tile %dx%d)", parent, s_tile_w, s_tile_h);
    return ESP_OK;
}

void music_info_view_detach(void)
{
    if (s_poll) { lv_timer_del(s_poll); s_poll = NULL; }
    s_root            = NULL;
    s_card            = NULL;
    s_video_art_img   = NULL;
    s_art_box         = NULL;
    s_art_img         = NULL;
    s_art_glyph       = NULL;
    s_overlay         = NULL;
    s_title_lbl       = NULL;
    s_artist_lbl      = NULL;
    s_btn_row         = NULL;
    s_btn_prev        = NULL;
    s_btn_play        = NULL;
    s_btn_next        = NULL;
    s_lbl_play        = NULL;
    s_last_title[0]   = '\0';
    s_last_artist[0]  = '\0';
    s_last_playing    = -1;
    s_last_art_hash   = 0;
    s_current_layout  = -1;
}
