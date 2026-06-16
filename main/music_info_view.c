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
#include "esp_heap_caps.h"
#include "driver/jpeg_decode.h"

#include "notif_bridge.h"
#include "fonts/aabridge_fonts.h"

static const char *TAG = "music_info_view";

#define POLL_PERIOD_MS    400
/* Visible art tile dimensions. Widget is sized down to ART_MCU_W on
 * the width axis so it stays equal-to-or-narrower than the decoded
 * frame — lv_img tiles a smaller-than-widget source and we'd see
 * adjacent edge tiles instead of clean crop. */
#define TILE_W            344
#define TILE_H            136

/* JPEG hardware output is RGB565 = 2 bytes/px. The P4 decoder rounds
 * OUTPUT up to 16 on BOTH axes — any 344×136 input becomes 352×144 in
 * the output buffer (right 8 cols and bottom 8 rows are padding). The
 * buffer is sized to that 16×16-aligned frame; the lv_img widget at
 * TILE_W×TILE_H stays smaller-or-equal and crops the visible area to
 * the real decoded pixels. */
#define ART_MCU_W         352
#define ART_MCU_H         144
#define ART_BUF_SIZE      (ART_MCU_W * ART_MCU_H * 2)

static jpeg_decoder_handle_t s_jpgd_handle;
static uint8_t *s_art_decoded;          /* RGB565, 350×135 visible inside 352×144 */
static uint8_t *s_jpeg_scratch;         /* DMA-aligned input copy */
static size_t   s_jpeg_scratch_cap;

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

static bool ensure_jpeg_scratch(size_t need)
{
    if (need <= s_jpeg_scratch_cap) return true;
    if (s_jpeg_scratch) heap_caps_free(s_jpeg_scratch);
    /* JPEG hardware reads via DMA — input must be cache-aligned. PSRAM
     * is fine, the engine internally bursts through cache. */
    size_t alloc = (need + 1023) & ~1023u;
    s_jpeg_scratch = heap_caps_aligned_calloc(64, 1, alloc,
                                              MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (!s_jpeg_scratch) {
        s_jpeg_scratch_cap = 0;
        return false;
    }
    s_jpeg_scratch_cap = alloc;
    return true;
}

static bool decode_art_jpeg(const uint8_t *src, size_t len)
{
    if (!s_jpgd_handle || !s_art_decoded) return false;
    if (!ensure_jpeg_scratch(len)) return false;
    memcpy(s_jpeg_scratch, src, len);

    jpeg_decode_cfg_t cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        /* BGR matches what the ST7701 / LVGL RGB565 pipeline expects on
         * this board — RGB order would surface as red/blue swap. */
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
    };
    uint32_t out_size = 0;
    esp_err_t r = jpeg_decoder_process(s_jpgd_handle, &cfg,
                                       s_jpeg_scratch, len,
                                       s_art_decoded, ART_BUF_SIZE,
                                       &out_size);
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "jpeg decode failed: %s", esp_err_to_name(r));
        return false;
    }
    return true;
}

static void apply_art(uint32_t hash)
{
    if (hash == s_last_art_hash) return;

    size_t len = 0;
    const uint8_t *bytes = hash ? notif_bridge_get_icon(hash, &len) : NULL;
    if (bytes && len > 0 && decode_art_jpeg(bytes, len)) {
        s_art_dsc.header.always_zero = 0;
        s_art_dsc.header.w = ART_MCU_W;
        s_art_dsc.header.h = ART_MCU_H;
        s_art_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
        s_art_dsc.data = s_art_decoded;
        s_art_dsc.data_size = ART_BUF_SIZE;
        /* dsc pointer is reused per track — drop any cached decode of
         * the previous bitmap before we hand LVGL the new pixels. */
        lv_img_cache_invalidate_src(&s_art_dsc);
        lv_img_set_src(s_art_img, &s_art_dsc);
        show_art();
        ESP_LOGI(TAG, "art hash=0x%08X jpeg_len=%u → %dx%d rgb565",
                 (unsigned)hash, (unsigned)len, ART_MCU_W, ART_MCU_H);
    } else if (bytes && len > 0) {
        /* Decode failed — keep whatever was on screen and don't update
         * s_last_art_hash so we retry on the next tick. */
        ESP_LOGW(TAG, "art hash=0x%08X len=%u — decode rejected",
                 (unsigned)hash, (unsigned)len);
        return;
    } else {
        show_fallback();
        if (hash != 0) {
            /* Cache miss — nudge the phone to resend the bytes. The
             * next media tick will deliver it. */
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
    /* The music tile lives on the dashboard screen. Don't update it while a
     * different screen is active — in DIRECT (partial) render mode writing an
     * off-screen widget invalidates its region and bleeds onto the active
     * screen. On return the dashboard repaints and the next poll catches up. */
    if (!s_root || lv_obj_get_screen(s_root) != lv_scr_act()) {
        return;
    }
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

static esp_err_t init_jpeg_pipeline(void)
{
    if (s_jpgd_handle) return ESP_OK;
    jpeg_decode_engine_cfg_t cfg = {
        .intr_priority = 0,
        .timeout_ms    = 200,
    };
    esp_err_t r = jpeg_new_decoder_engine(&cfg, &s_jpgd_handle);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "jpeg engine init failed: %s", esp_err_to_name(r));
        return r;
    }
    s_art_decoded = heap_caps_aligned_calloc(64, 1, ART_BUF_SIZE,
                                             MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (!s_art_decoded) {
        ESP_LOGE(TAG, "no PSRAM for %u-byte art buffer", (unsigned)ART_BUF_SIZE);
        jpeg_del_decoder_engine(s_jpgd_handle);
        s_jpgd_handle = NULL;
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
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_20, 0);
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
    lv_obj_set_style_text_font(s_title_lbl, &aabridge_font_24, 0);
    lv_obj_set_style_text_align(s_title_lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(s_title_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s_title_lbl, TILE_W - 28);
    /* Bottom-anchored offsets sized to font 24 (≈28 px line height) —
     * artist sits 4 px from the bottom, title 28 px above that. */
    lv_obj_align(s_title_lbl, LV_ALIGN_BOTTOM_MID, 0, -28);
    lv_label_set_text(s_title_lbl, "—");

    s_artist_lbl = lv_label_create(s_root);
    lv_obj_set_style_text_color(s_artist_lbl, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_style_text_font(s_artist_lbl, &aabridge_font_24, 0);
    lv_obj_set_style_text_align(s_artist_lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(s_artist_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s_artist_lbl, TILE_W - 28);
    lv_obj_align(s_artist_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_label_set_text(s_artist_lbl, "");

    s_poll = lv_timer_create(poll_cb, POLL_PERIOD_MS, NULL);
    poll_cb(s_poll);

    ESP_LOGI(TAG, "attached to %p (tile %dx%d)", parent, TILE_W, TILE_H);
    return ESP_OK;
}

void music_info_view_detach(void)
{
    /* Called before the dashboard screen that hosts our tile is torn down (a
     * theme switch). Stop the poller and drop every widget reference — the
     * objects are children of the about-to-be-deleted screen, so LVGL frees
     * them with it; we must not lv_obj_del them here. The JPEG pipeline
     * (decoder + PSRAM frame buffer) is screen-independent and intentionally
     * kept, so a re-attach is cheap. After this, music_info_view_attach() will
     * rebuild the tile on the next theme's widget. */
    if (s_poll) { lv_timer_del(s_poll); s_poll = NULL; }
    s_root            = NULL;
    s_art_img         = NULL;
    s_art_fallback_bg = NULL;
    s_overlay         = NULL;
    s_art_glyph       = NULL;
    s_title_lbl       = NULL;
    s_artist_lbl      = NULL;
    /* Force the next poll to re-push the current track to the fresh widgets. */
    s_last_title[0]  = '\0';
    s_last_artist[0] = '\0';
    s_last_playing   = -1;
}
