/* Music-info widget driver.
 *
 * Renders into the 436×108 `dashboard_music_info_tile` that GUI Guider
 * lays out on the dashboard. Layout from back to front:
 *
 *   1. lv_img — album art zoomed to "cover" the whole tile. LVGL clips
 *      children to the parent's box, so the oversized image is cropped
 *      to 436×108 automatically.
 *   2. dark gradient strip — black at 75% alpha at the bottom, fading to
 *      transparent at the top. Keeps the title/artist legible on bright
 *      art (e.g. white Spotify covers).
 *   3. title (montserrat_32) + artist (montserrat_24) on top of the
 *      gradient, left-aligned with a bit of horizontal padding.
 *
 * Fallback when no art is in cache: hide the image, paint a flat dark
 * background, show a transport glyph next to the labels.
 *
 * Polling once a tick — cheap; the LVGL task is the natural home for
 * this since strncpy/lv_label_set_text both expect the LVGL mutex. */

#include "music_info_view.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "notif_bridge.h"
#include "fonts/aabridge_fonts.h"

static const char *TAG = "music_info_view";

#define POLL_PERIOD_MS    400
#define TILE_W            350
#define TILE_H            135
/* The phone resizes album art to exactly TILE_W×TILE_H (cover-fit + crop)
 * before sending — LVGL draws the PNG at 1:1, no zoom. */

static lv_obj_t *s_root;
static lv_obj_t *s_art_img;
static lv_obj_t *s_art_fallback_bg;
static lv_obj_t *s_overlay;
static lv_obj_t *s_art_glyph;
static lv_obj_t *s_title_lbl;
static lv_obj_t *s_artist_lbl;

static lv_timer_t *s_poll;

/* Last-rendered snapshot to skip noop label updates. */
static char s_last_title[128];
static char s_last_artist[128];
static int  s_last_playing = -1;
static uint32_t s_last_art_hash;

/* Image descriptor backed by notif_bridge's PNG cache. The cache slot
 * is held by the LRU until evicted; we drop the dsc whenever the hash
 * changes so a stale pointer never survives a refresh. */
static lv_img_dsc_t s_art_dsc;

static void show_fallback(void)
{
    /* No artwork → no rectangle. Hide the art, the dark fallback panel,
     * the readability overlay and the placeholder glyph so the tile is
     * fully transparent and inherits the dashboard's own background.
     * Only the text labels remain on top. */
    lv_obj_add_flag(s_art_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_art_fallback_bg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_art_glyph, LV_OBJ_FLAG_HIDDEN);
}

static void show_art(void)
{
    lv_obj_clear_flag(s_art_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_art_fallback_bg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_art_glyph, LV_OBJ_FLAG_HIDDEN);
}

static void apply_art(uint32_t hash)
{
    if (hash == s_last_art_hash) return;

    size_t len = 0;
    const uint8_t *png = hash ? notif_bridge_get_icon(hash, &len) : NULL;
    if (png && len > 0) {
        s_art_dsc.header.always_zero = 0;
        s_art_dsc.header.w = 0;
        s_art_dsc.header.h = 0;
        s_art_dsc.header.cf = LV_IMG_CF_RAW_ALPHA;
        s_art_dsc.data = png;
        s_art_dsc.data_size = len;
        lv_img_set_src(s_art_img, &s_art_dsc);
        show_art();
        ESP_LOGI(TAG, "art hash=0x%08X len=%u", (unsigned)hash, (unsigned)len);
    } else {
        show_fallback();
        if (hash != 0) {
            /* Cache miss — nudge the phone to resend the PNG. The next
             * media tick will deliver it and apply_art() will swap it in. */
            notif_bridge_send_cmd(NOTIF_OP_REQUEST_ART, hash);
        }
    }
    s_last_art_hash = hash;
}

static void set_container_visible(bool on)
{
    if (!s_root) return;
    /* The tileview container (`dashboard_music_info`) sits one level
     * above our root tile and ships with a dark default-theme fill.
     * Toggling its HIDDEN flag is the cheapest way to suppress that
     * fill entirely when no track is playing — custom_init() hides it
     * at boot, here we flip it on/off as media comes and goes. */
    lv_obj_t *container = lv_obj_get_parent(s_root);
    if (!container) return;
    if (on) lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);
    else    lv_obj_add_flag (container, LV_OBJ_FLAG_HIDDEN);
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    const media_state_t *m = notif_bridge_get_media();

    if (m->title[0] == 0 && m->artist[0] == 0) {
        /* No track ever pushed — keep the whole widget invisible so the
         * dashboard background shows through. */
        if (s_last_title[0] != '\0' || s_last_artist[0] != '\0') {
            lv_label_set_text(s_title_lbl, "");
            lv_label_set_text(s_artist_lbl, "");
            s_last_title[0] = '\0';
            s_last_artist[0] = '\0';
            s_last_playing = -1;
        }
        lv_obj_add_flag(s_title_lbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_artist_lbl, LV_OBJ_FLAG_HIDDEN);
        apply_art(0);
        set_container_visible(false);
        return;
    }

    /* First non-empty media — un-hide the text + the parent tileview. */
    set_container_visible(true);
    lv_obj_clear_flag(s_title_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_artist_lbl, LV_OBJ_FLAG_HIDDEN);

    if (strcmp(s_last_title, m->title) != 0) {
        lv_label_set_text(s_title_lbl, m->title);
        strncpy(s_last_title, m->title, sizeof(s_last_title) - 1);
    }
    if (strcmp(s_last_artist, m->artist) != 0) {
        lv_label_set_text(s_artist_lbl, m->artist);
        strncpy(s_last_artist, m->artist, sizeof(s_last_artist) - 1);
    }
    /* Transport state intentionally not surfaced as a glyph — the tile
     * is a read-only readout, not a control. The fallback s_art_glyph
     * stays on the static LV_SYMBOL_AUDIO note set at init time. */
    (void)s_last_playing;
    apply_art(m->album_art_hash);
}

esp_err_t music_info_view_attach(lv_obj_t *parent)
{
    if (!parent) return ESP_ERR_INVALID_ARG;
    if (s_root) return ESP_OK;
    s_root = parent;

    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_root, 0, LV_PART_MAIN);
    /* GUI Guider's tile ships with a default white bg + light border.
     * That's what shows up as the "rectangle on an empty dashboard"
     * the moment we hide every child. Strip the fill, the border and
     * the shadow so an empty widget is fully transparent — the corners
     * still come out rounded because each chrome child (art / overlay
     * / fallback bg) carries its own 18 px radius + clip_corner. */
    lv_obj_set_style_bg_opa(s_root, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_root, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_root, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(s_root, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_root, 0, LV_PART_MAIN);

    /* Flat dark background — kept around for show_art() path so the
     * art has an opaque neighbour, but hidden at boot so the widget is
     * fully transparent until a track lands. */
    s_art_fallback_bg = lv_obj_create(s_root);
    lv_obj_set_size(s_art_fallback_bg, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(s_art_fallback_bg, 0, 0);
    lv_obj_set_style_bg_color(s_art_fallback_bg, lv_color_hex(0x181818), 0);
    lv_obj_set_style_border_width(s_art_fallback_bg, 0, 0);
    lv_obj_set_style_radius(s_art_fallback_bg, 18, 0);
    lv_obj_set_style_clip_corner(s_art_fallback_bg, true, 0);
    lv_obj_clear_flag(s_art_fallback_bg,
                      LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_art_fallback_bg, LV_OBJ_FLAG_HIDDEN);

    /* Album art — sits at the very bottom of the z-order so the overlay
     * and labels render on top. Phone-side already shaped it to the
     * art size, so no zoom/crop on the LVGL side. Centered horizontally
     * inside the tile — the tile is wider than the art so an equal gap
     * shows on both sides instead of pinning it to the left edge. */
    s_art_img = lv_img_create(s_root);
    lv_obj_set_size(s_art_img, TILE_W, TILE_H);
    lv_obj_align(s_art_img, LV_ALIGN_CENTER, 0, 0);
    /* Rounded corners — LVGL v8 clips the image to the radius if
     * clip_corner is set on the same object. Matches the overlay's 18 px
     * so they look like one card. */
    lv_obj_set_style_radius(s_art_img, 18, 0);
    lv_obj_set_style_clip_corner(s_art_img, true, 0);
    lv_obj_add_flag(s_art_img, LV_OBJ_FLAG_HIDDEN);

    /* Readability overlay — solid black at 50% alpha. Sized and centered
     * to match the album art (not the whole tile), so when the tile is
     * wider than the art we don't paint a dark band on the bare edges.
     * LVGL v8 has no per-channel gradient-opacity style; a flat
     * translucent layer evenly dims any album art and keeps the
     * title/artist legible. */
    s_overlay = lv_obj_create(s_root);
    lv_obj_set_size(s_overlay, TILE_W, TILE_H);
    lv_obj_align(s_overlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(s_overlay, 0, 0);
    lv_obj_set_style_radius(s_overlay, 18, 0);
    lv_obj_set_style_clip_corner(s_overlay, true, 0);
    lv_obj_set_style_pad_all(s_overlay, 0, 0);
    lv_obj_clear_flag(s_overlay,
                      LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);

    /* Transport glyph — only shown when there's no album art. Sits in
     * the lower-left so it doesn't fight the title position. */
    s_art_glyph = lv_label_create(s_root);
    lv_obj_set_style_text_color(s_art_glyph, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_art_glyph, &aabridge_font_32, 0);
    lv_label_set_text(s_art_glyph, LV_SYMBOL_AUDIO);
    lv_obj_align(s_art_glyph, LV_ALIGN_LEFT_MID, 14, 0);

    /* Title — bottom-left, full tile width minus padding. Long Russian
     * titles ("Where Is My Mind? (из фильма «Бойцовский клуб»)") get
     * dot-truncated so the artist line still fits. */
    /* LVGL v8 has no text-shadow style; legibility on bright art is
     * carried by the bottom black band created above. */
    s_title_lbl = lv_label_create(s_root);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_title_lbl, &aabridge_font_32, 0);
    lv_obj_set_style_text_align(s_title_lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(s_title_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s_title_lbl, TILE_W - 28);
    /* Labels track the centered art band — outer offset matches the
     * (tile - art) / 2 gap plus a small inner padding, so they begin at
     * the art's left edge instead of clinging to the tile's. */
    lv_obj_align(s_title_lbl, LV_ALIGN_BOTTOM_MID, 0, -32);
    lv_label_set_text(s_title_lbl, "—");

    s_artist_lbl = lv_label_create(s_root);
    lv_obj_set_style_text_color(s_artist_lbl, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_style_text_font(s_artist_lbl, &aabridge_font_32, 0);
    lv_obj_set_style_text_align(s_artist_lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(s_artist_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s_artist_lbl, TILE_W - 28);
    lv_obj_align(s_artist_lbl, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_label_set_text(s_artist_lbl, "");

    s_poll = lv_timer_create(poll_cb, POLL_PERIOD_MS, NULL);
    poll_cb(s_poll);

    ESP_LOGI(TAG, "attached to %p (tile %dx%d)", parent, TILE_W, TILE_H);
    return ESP_OK;
}
