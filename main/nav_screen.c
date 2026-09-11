#include "nav_screen.h"

#include "nav_map.h"
#include "nav_route.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_cache.h"
#include "esp_private/esp_cache_private.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "dev_settings.h"
#include "fonts/aabridge_fonts.h"
#include "speed_sensor.h"
#include "vesc_battery_calc.h"
#include "vesc_can/vesc_rt_data.h"
#include "vesc_can/vesc_lisp_panel.h"

static const char *TAG = "nav_screen";

/* How often the LVGL side looks for a committed frame. Frames arrive around
 * once a second; a short tick only costs a flag read and keeps the "waiting"
 * text honest. */
#define TICK_PERIOD_MS 50

/* Ticks the just-retired front buffer stays off limits after a swap. LVGL
 * renders the image into the panel framebuffer asynchronously (and twice, once
 * per framebuffer in DOUBLE_DIRECT), so handing it straight back to the BLE
 * worker's DMA would tear the picture being drawn. One 50 ms tick is enough
 * — a full-screen image flushes in about 7 ms in this render mode — and it
 * lets the map move at the eight frames a second the head unit can compose. */
#define SWAP_COOLDOWN_TICKS 1

static lv_obj_t *s_screen;
static lv_obj_t *s_img;
static lv_obj_t *s_status_box;
static lv_obj_t *s_status_lbl;
static lv_obj_t *s_speed_lbl;
static lv_obj_t *s_speed_unit;
static lv_obj_t *s_batt_lbl;
static lv_obj_t *s_cc_img;
static lv_obj_t *s_turn_box;
static lv_obj_t *s_turn_arrow;
static lv_obj_t *s_turn_dist;
static lv_obj_t *s_turn_unit;
static lv_obj_t *s_remain_lbl;
static lv_obj_t *s_pick_box;
static lv_obj_t *s_pick_lbl;
static lv_timer_t *s_pick_timer;
static nav_screen_dest_cb_t s_dest_cb;
static nav_screen_zoom_cb_t s_zoom_cb;
static nav_screen_search_cb_t s_search_cb;

/* Typing on the panel. The keyboard and the results live in one overlay that
 * covers the map; the map keeps composing underneath, which costs nothing and
 * means closing the overlay never shows a stale frame. */
static lv_obj_t *s_find_box;
static lv_obj_t *s_find_ta;
static lv_obj_t *s_find_kb;
static lv_obj_t *s_find_list;
static lv_obj_t *s_find_hint;
static lv_timer_t *s_query_timer;

/* What the phone last found. Written by the BLE worker, drained on the LVGL
 * tick — nothing here may touch LVGL from another task. */
static nav_search_hit_t s_hits[NAV_SEARCH_MAX];
static atomic_int  s_nhits;
static atomic_bool s_hits_fresh;
static lv_obj_t *s_zoom_lbl;
static int32_t s_pick_lat_e7, s_pick_lon_e7;
static lv_timer_t *s_tick;

static uint16_t *s_fb[2];
static size_t    s_fb_bytes;
static int       s_front;          /* index of the buffer lv_img points at */
static lv_img_dsc_t s_dsc;

/* Written by the BLE worker, read by the LVGL tick (and the other way round
 * for the cooldown) — one flag each way, so plain atomics are enough. */
static atomic_bool s_pending;
static atomic_int  s_cooldown;

static atomic_bool s_active;
static atomic_bool s_phone;
static atomic_bool s_streaming;

static atomic_uint s_frames;
static _Atomic int64_t s_last_frame_us;

/* Text currently on the placeholder, so a tick that changes nothing does not
 * touch LVGL at all. */
static const char *s_status_text;

/* The same two readouts Android Auto gets painted over its video, in the same
 * typeface: Antonio 64 for the number, Antonio 22 for the unit. Over there
 * aa_overlay.c has to draw glyphs into the framebuffer by hand because AA
 * bypasses LVGL; the map is an ordinary LVGL image, so ordinary labels do it.
 *
 * The plate behind each readout replaces the halo the hand-drawn version
 * uses — the ground under it is anything from a pale road to a dark park. */
extern const lv_font_t lv_font_Antonio_Regular_64;
extern const lv_font_t lv_font_Antonio_Regular_22;
LV_IMG_DECLARE(_cruise_control_alpha_38x38);

#define HUD_PLATE   0x101418
/* How long the "go here?" prompt waits for an answer. */
#define PICK_TIMEOUT_MS 12000
#define CC_COLOUR   0x33FF66

/* A transparent row that lays its children out left to right, so a readout is
 * just "big number" + "small unit" without any arithmetic. */
static lv_obj_t *hud_row_create(lv_obj_t *parent, lv_align_t align, int x, int y)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(row, align, x, y);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(row, lv_color_hex(HUD_PLATE), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_60, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_pad_hor(row, 10, 0);
    /* The grey has to clear the glyphs, not sit on them: Antonio's digits
     * reach the top of their line box, so a two-pixel pad looked like the
     * plate had been cut off. */
    lv_obj_set_style_pad_ver(row, 8, 0);
    lv_obj_set_style_pad_column(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

static lv_obj_t *hud_text(lv_obj_t *row, const lv_font_t *font, uint32_t colour)
{
    lv_obj_t *lbl = lv_label_create(row);
    lv_obj_set_style_text_color(lbl, lv_color_hex(colour), 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_label_set_text(lbl, "");
    return lbl;
}

/* Speed and charge, read the way aa_overlay.c reads them: straight from the
 * VESC RT poller rather than the cockpit cache, honouring the BLE wheel
 * sensor choice and the km/miles toggle, with cruise from the LISP panel. */
static void refresh_hud(void)
{
    if (!s_speed_lbl || !s_batt_lbl) return;

    const bool ble_src = speed_source_is_ble();
    float kmh = ble_src ? speed_sensor_get_kmh() : 0.0f;
    int batt = -1;
    if (vesc_rt_data_is_fresh()) {
        const vesc_setup_values_t *rt = vesc_rt_data_get_latest();
        if (!ble_src) kmh = vesc_rt_data_get_speed_kmh();
        const float pct = battery_calc_display_percentage(
            rt->battery_level, rt->amp_hours, rt->amp_hours_charged);
        batt = (int)(pct + 0.5f);
        if (batt < 0) batt = 0;
        if (batt > 99) batt = 99;
    }
    if (kmh < 0) kmh = -kmh;
    const bool imperial = settings_get_use_imperial();
    if (imperial) kmh *= 0.621371f;
    int shown = (int)(kmh + 0.5f);
    if (shown < 0) shown = 0;
    if (shown > 999) shown = 999;

    char buf[16];
    snprintf(buf, sizeof buf, "%d", shown);
    if (strcmp(buf, lv_label_get_text(s_speed_lbl)) != 0) {
        lv_label_set_text(s_speed_lbl, buf);
    }
    const char *unit = imperial ? "MPH" : "KM/H";
    if (strcmp(unit, lv_label_get_text(s_speed_unit)) != 0) {
        lv_label_set_text(s_speed_unit, unit);
    }

    if (batt < 0) snprintf(buf, sizeof buf, "--");
    else          snprintf(buf, sizeof buf, "%d", batt);
    if (strcmp(buf, lv_label_get_text(s_batt_lbl)) != 0) {
        lv_label_set_text(s_batt_lbl, buf);
    }

    /* Cruise comes from the LISP dash packet, which has its own pump — it can
     * report cruise while an RT poll has just gapped. */
    vlp_dash_t dash;
    const bool cc = vesc_lisp_panel_get_dash(&dash) && dash.cruise_active;
    if (s_cc_img) {
        if (cc) lv_obj_clear_flag(s_cc_img, LV_OBJ_FLAG_HIDDEN);
        else    lv_obj_add_flag(s_cc_img, LV_OBJ_FLAG_HIDDEN);
    }
}

/* Turn arrows as line segments rather than glyphs: no font here carries them,
 * and a canvas per frame would be wasteful when the shape only changes at a
 * turn. Coordinates sit in a 60x60 box with up meaning straight on.
 *
 * The head is traced — out to the tip, back, out again — because lv_line
 * draws a single polyline. Retraced segments land on themselves in the same
 * colour, so they cost nothing to look at. */
typedef struct { uint8_t n; lv_point_t p[7]; } arrow_t;

static const arrow_t k_arrows[NAV_TURN_COUNT] = {
    [NAV_TURN_STRAIGHT]    = { 5, { {30,58}, {30,8},  {18,22}, {30,8},  {42,22} } },
    [NAV_TURN_SLIGHT_LEFT] = { 6, { {38,58}, {38,32}, {14,10}, {14,26}, {14,10}, {30,10} } },
    [NAV_TURN_SLIGHT_RIGHT]= { 6, { {22,58}, {22,32}, {46,10}, {46,26}, {46,10}, {30,10} } },
    [NAV_TURN_LEFT]        = { 6, { {36,58}, {36,28}, {8,28},  {22,14}, {8,28},  {22,42} } },
    [NAV_TURN_RIGHT]       = { 6, { {24,58}, {24,28}, {52,28}, {38,14}, {52,28}, {38,42} } },
    [NAV_TURN_SHARP_LEFT]  = { 6, { {38,58}, {38,24}, {14,40}, {12,24}, {14,40}, {30,46} } },
    [NAV_TURN_SHARP_RIGHT] = { 6, { {22,58}, {22,24}, {46,40}, {48,24}, {46,40}, {30,46} } },
    [NAV_TURN_UTURN]       = { 7, { {42,58}, {42,26}, {30,14}, {18,26}, {18,46}, {8,34}, {28,34} } },
    /* Arrival: a flag, not an arrow. */
    [NAV_TURN_ARRIVE]      = { 5, { {20,58}, {20,10}, {46,18}, {20,26}, {20,10} } },
};

static void set_arrow(nav_turn_t t)
{
    static nav_turn_t shown = NAV_TURN_COUNT;
    if (t >= NAV_TURN_COUNT || t == shown) return;
    shown = t;
    lv_line_set_points(s_turn_arrow, k_arrows[t].p, k_arrows[t].n);
}

/* Picking a destination on the panel itself: tap the map, confirm, and the
 * phone is told where to route. The head unit knows exactly which patch of
 * ground each pixel is (it composed the view), so the tap needs nothing from
 * the phone to become a coordinate. */
static void pick_hide(void)
{
    if (s_pick_box) lv_obj_add_flag(s_pick_box, LV_OBJ_FLAG_HIDDEN);
    if (s_pick_timer) {
        lv_timer_del(s_pick_timer);
        s_pick_timer = NULL;
    }
}

/* A prompt nobody answers goes away on its own. A pocket or a bump can tap
 * the map, and the plate used to stay up over the map until someone pressed
 * one of its buttons. */
static void pick_timeout_cb(lv_timer_t *t)
{
    (void)t;
    s_pick_timer = NULL;   /* one-shot: LVGL deletes it after this */
    if (s_pick_box) lv_obj_add_flag(s_pick_box, LV_OBJ_FLAG_HIDDEN);
}

static void pick_go_cb(lv_event_t *e)
{
    (void)e;
    pick_hide();
    if (s_dest_cb) s_dest_cb(s_pick_lat_e7, s_pick_lon_e7);
}

static void pick_cancel_cb(lv_event_t *e)
{
    (void)e;
    pick_hide();
}

/* ---- typing an address on the panel ---- */

#define FIND_DEBOUNCE_MS 600
#define FIND_MIN_CHARS   3

static void find_close(void)
{
    if (s_query_timer) {
        lv_timer_del(s_query_timer);
        s_query_timer = NULL;
    }
    if (s_find_box) lv_obj_add_flag(s_find_box, LV_OBJ_FLAG_HIDDEN);
}

/* Ask the phone. It owns the offline search index that came with its map, so
 * this works with no signal — which is most of the point of typing here
 * rather than on the phone. */
static void send_query(lv_timer_t *t)
{
    (void)t;
    s_query_timer = NULL;    /* one-shot */
    if (!s_find_ta || !s_search_cb) return;
    const char *q = lv_textarea_get_text(s_find_ta);
    if (!q || strlen(q) < FIND_MIN_CHARS) return;
    if (s_find_hint) {
        lv_label_set_text(s_find_hint, "Looking...");
        lv_obj_clear_flag(s_find_hint, LV_OBJ_FLAG_HIDDEN);
    }
    s_search_cb(q);
}

static void query_changed_cb(lv_event_t *e)
{
    (void)e;
    if (s_query_timer) lv_timer_del(s_query_timer);
    s_query_timer = lv_timer_create(send_query, FIND_DEBOUNCE_MS, NULL);
    lv_timer_set_repeat_count(s_query_timer, 1);
}

/* Enter on the keyboard: do not wait out the debounce. */
static void query_ready_cb(lv_event_t *e)
{
    (void)e;
    if (s_query_timer) {
        lv_timer_del(s_query_timer);
        s_query_timer = NULL;
    }
    send_query(NULL);
}

static void find_open_cb(lv_event_t *e)
{
    (void)e;
    if (!s_find_box) return;
    pick_hide();
    lv_textarea_set_text(s_find_ta, "");
    lv_obj_clean(s_find_list);
    lv_label_set_text(s_find_hint, "Type a street or a place");
    lv_obj_clear_flag(s_find_hint, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_find_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_find_box);   /* over the zoom buttons */
    lv_keyboard_set_textarea(s_find_kb, s_find_ta);
}

static void find_close_cb(lv_event_t *e) { (void)e; find_close(); }

/* One of the results. Straight down the same path as a tap on the map: the
 * phone is told where to go and does the routing. */
static void hit_pressed_cb(lv_event_t *e)
{
    const int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= atomic_load(&s_nhits)) return;
    const int32_t lat_e7 = s_hits[idx].lat_e7, lon_e7 = s_hits[idx].lon_e7;
    find_close();
    if (s_dest_cb) s_dest_cb(lat_e7, lon_e7);
}

/* Rebuild the list from what the worker left us. LVGL task only. */
static void refresh_results(void)
{
    if (!s_find_list || !atomic_exchange(&s_hits_fresh, false)) return;
    lv_obj_clean(s_find_list);
    const int n = atomic_load(&s_nhits);
    if (n <= 0) {
        lv_label_set_text(s_find_hint, "Nothing found");
        lv_obj_clear_flag(s_find_hint, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_add_flag(s_find_hint, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < n; i++) {
        lv_obj_t *b = lv_btn_create(s_find_list);
        lv_obj_set_size(b, lv_pct(100), 46);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x1c2530), 0);
        lv_obj_set_style_radius(b, 8, 0);
        lv_obj_add_event_cb(b, hit_pressed_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
        lv_obj_set_width(l, lv_pct(100));
        lv_obj_set_style_text_font(l, &aabridge_font_24, 0);
        lv_label_set_text(l, s_hits[i].name);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
    }
}

/* Zoom, on the panel itself. The rider is the one looking at the map, so the
 * buttons change what is drawn immediately and the phone is told afterwards —
 * it is the only one that can fetch tiles for the new level. */
static void zoom_step(int delta)
{
    nav_map_view_t v;
    nav_map_get_view(&v);
    int z = (v.valid ? v.zoom : NAV_ZOOM_MAX - 1) + delta;
    if (z < NAV_ZOOM_MIN) z = NAV_ZOOM_MIN;
    if (z > NAV_ZOOM_MAX) z = NAV_ZOOM_MAX;
    if (s_zoom_cb) s_zoom_cb((uint8_t)z);   /* refresh_zoom() shows the level */
}

/* The level between the buttons, so it reads right from the first frame
 * rather than only after the rider has pressed something. */
static void refresh_zoom(void)
{
    static int shown = -1;
    if (!s_zoom_lbl) return;
    nav_map_view_t v;
    nav_map_get_view(&v);
    const int z = v.valid ? (int)v.zoom : -1;
    if (z == shown) return;
    shown = z;
    if (z < 0) {
        lv_label_set_text(s_zoom_lbl, "");
        lv_obj_add_flag(s_zoom_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    char buf[8];
    snprintf(buf, sizeof buf, "z%d", z);
    lv_label_set_text(s_zoom_lbl, buf);
    lv_obj_clear_flag(s_zoom_lbl, LV_OBJ_FLAG_HIDDEN);
}

static void zoom_in_cb(lv_event_t *e)  { (void)e; zoom_step(+1); }
static void zoom_out_cb(lv_event_t *e) { (void)e; zoom_step(-1); }

static void map_pressed_cb(lv_event_t *e)
{
    (void)e;
    if (!s_pick_box) return;
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    nav_map_unproject(p.x, p.y, NAV_SCREEN_W, NAV_SCREEN_H,
                      &s_pick_lat_e7, &s_pick_lon_e7);
    /* Printed from the integer, digit by digit: a "%.5f" would pull the
     * software double-precision formatter in for one label. */
    char buf[64];
    snprintf(buf, sizeof buf, "Go to %s%ld.%05ld, %s%ld.%05ld?",
             s_pick_lat_e7 < 0 ? "-" : "", labs(s_pick_lat_e7) / 10000000L,
             (labs(s_pick_lat_e7) % 10000000L) / 100L,
             s_pick_lon_e7 < 0 ? "-" : "", labs(s_pick_lon_e7) / 10000000L,
             (labs(s_pick_lon_e7) % 10000000L) / 100L);
    lv_label_set_text(s_pick_lbl, buf);
    lv_obj_clear_flag(s_pick_box, LV_OBJ_FLAG_HIDDEN);
    if (s_pick_timer) lv_timer_del(s_pick_timer);
    s_pick_timer = lv_timer_create(pick_timeout_cb, PICK_TIMEOUT_MS, NULL);
    lv_timer_set_repeat_count(s_pick_timer, 1);
}

/* The turn plate and the remaining-distance line, from whatever the phone
 * last said. Hidden when there is no route — the map alone is the whole
 * screen then. */
static void refresh_guide(void)
{
    if (!s_turn_box) return;
    nav_guide_t g;
    nav_route_get_guide(&g);
    if (!g.valid || nav_route_count() < 2) {
        lv_obj_add_flag(s_turn_box, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_remain_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(s_turn_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_remain_lbl, LV_OBJ_FLAG_HIDDEN);

    /* Off the route is the one state worth shouting about. */
    lv_obj_set_style_bg_color(s_turn_box,
                              lv_color_hex(g.off_route ? 0xB3261E : HUD_PLATE), 0);
    lv_obj_set_style_bg_opa(s_turn_box, g.off_route ? LV_OPA_90 : LV_OPA_70, 0);

    char buf[32];
    set_arrow(g.turn);
    const char *unit;
    if (g.dist_m < 1000) {
        snprintf(buf, sizeof buf, "%u", (unsigned)g.dist_m);
        unit = "M";
    } else {
        snprintf(buf, sizeof buf, "%.1f", g.dist_m / 1000.0);
        unit = "KM";
    }
    if (strcmp(buf, lv_label_get_text(s_turn_dist)) != 0) {
        lv_label_set_text(s_turn_dist, buf);
    }
    if (strcmp(unit, lv_label_get_text(s_turn_unit)) != 0) {
        lv_label_set_text(s_turn_unit, unit);
    }

    const unsigned mins = (g.remaining_s + 30) / 60;
    snprintf(buf, sizeof buf, "%.1f km  |  %u min",
             g.remaining_m / 1000.0, mins);
    if (strcmp(buf, lv_label_get_text(s_remain_lbl)) != 0) {
        lv_label_set_text(s_remain_lbl, buf);
    }
}

static void set_status(const char *text)
{
    if (text == s_status_text) return;
    s_status_text = text;
    if (!s_status_box) return;
    if (text == NULL) {
        lv_obj_add_flag(s_status_box, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_label_set_text(s_status_lbl, text);
    lv_obj_clear_flag(s_status_box, LV_OBJ_FLAG_HIDDEN);
}

static void show_committed_frame(void)
{
    s_front ^= 1;
    s_dsc.header.always_zero = 0;
    s_dsc.header.w  = NAV_SCREEN_W;
    s_dsc.header.h  = NAV_SCREEN_H;
    s_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_dsc.data      = (const uint8_t *)s_fb[s_front];
    s_dsc.data_size = s_fb_bytes;
    /* Same descriptor pointer every frame — drop LVGL's cached decode of it
     * or the previous picture stays on screen (music_info_view.c hit this). */
    lv_img_cache_invalidate_src(&s_dsc);
    lv_img_set_src(s_img, &s_dsc);
    lv_obj_invalidate(s_img);
    atomic_store(&s_frames, atomic_load(&s_frames) + 1);
    atomic_store(&s_last_frame_us, esp_timer_get_time());
}

static void tick_cb(lv_timer_t *t)
{
    (void)t;
    /* DOUBLE_DIRECT redraws only dirty regions, and an invalidate on a screen
     * that is not live bleeds onto the one that is. Do nothing at all unless
     * the rider is looking at us. */
    if (lv_scr_act() != s_screen) return;

    int cd = atomic_load(&s_cooldown);
    if (cd > 0) atomic_store(&s_cooldown, cd - 1);

    if (atomic_exchange(&s_pending, false)) {
        show_committed_frame();
        atomic_store(&s_cooldown, SWAP_COOLDOWN_TICKS);
    }

    /* A caption only when there is nothing to look at, or when the phone
     * said it stopped. Quiet is not a fault: the app skips frames whose
     * pixels did not change, so a parked bike sends nothing for minutes and
     * the last picture is still the right one. */
    refresh_hud();
    refresh_guide();
    refresh_zoom();
    refresh_results();

    const bool have_frame = atomic_load(&s_frames) > 0;
    if (!atomic_load(&s_phone)) {
        set_status("Phone not connected");
    } else if (!have_frame) {
        set_status("Waiting for the navigator app...");
    } else if (!atomic_load(&s_streaming)) {
        set_status("Navigator app stopped streaming");
    } else {
        set_status(NULL);
    }
}

esp_err_t nav_screen_init(void)
{
    if (s_screen) return ESP_OK;

    size_t line = 64;
    if (esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &line) != ESP_OK || line == 0) {
        line = 64;
    }
    s_fb_bytes = ((size_t)NAV_SCREEN_W * NAV_SCREEN_H * 2 + line - 1) & ~(line - 1);
    for (int i = 0; i < 2; i++) {
        /* The JPEG decoder and the PPA both write here over DMA. */
        s_fb[i] = heap_caps_aligned_calloc(line, 1, s_fb_bytes,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
        if (!s_fb[i]) {
            ESP_LOGE(TAG, "no PSRAM for a %u-byte framebuffer", (unsigned)s_fb_bytes);
            for (int j = 0; j < 2; j++) {
                if (s_fb[j]) { heap_caps_free(s_fb[j]); s_fb[j] = NULL; }
            }
            return ESP_ERR_NO_MEM;
        }
    }

    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl lock timeout");
        return ESP_FAIL;
    }

    s_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x101418), 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    s_img = lv_img_create(s_screen);
    lv_obj_set_pos(s_img, 0, 0);
    lv_obj_set_size(s_img, NAV_SCREEN_W, NAV_SCREEN_H);

    /* Placeholder card, centred, drawn over whatever the last picture was. */
    s_status_box = lv_obj_create(s_screen);
    lv_obj_set_size(s_status_box, 560, 96);
    lv_obj_align(s_status_box, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_status_box, lv_color_hex(0x1c2530), 0);
    lv_obj_set_style_bg_opa(s_status_box, LV_OPA_80, 0);
    lv_obj_set_style_border_width(s_status_box, 0, 0);
    lv_obj_set_style_radius(s_status_box, 12, 0);
    lv_obj_clear_flag(s_status_box, LV_OBJ_FLAG_SCROLLABLE);

    s_status_lbl = lv_label_create(s_status_box);
    lv_obj_center(s_status_lbl);
    lv_obj_set_style_text_color(s_status_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_status_lbl, &aabridge_font_32, 0);
    lv_label_set_text(s_status_lbl, "Phone not connected");
    s_status_text = "Phone not connected";

    lv_obj_t *left = hud_row_create(s_screen, LV_ALIGN_BOTTOM_LEFT, 10, -8);
    /* Cruise sits to the left of the digits, as it does in Android Auto, so
     * engaging it never shifts the speed. */
    s_cc_img = lv_img_create(left);
    lv_img_set_src(s_cc_img, &_cruise_control_alpha_38x38);
    lv_obj_set_style_img_recolor(s_cc_img, lv_color_hex(CC_COLOUR), 0);
    lv_obj_set_style_img_recolor_opa(s_cc_img, LV_OPA_COVER, 0);
    lv_obj_add_flag(s_cc_img, LV_OBJ_FLAG_HIDDEN);
    s_speed_lbl = hud_text(left, &lv_font_Antonio_Regular_64, 0xFFFFFF);
    s_speed_unit = hud_text(left, &lv_font_Antonio_Regular_22, 0xC8D0D8);

    lv_obj_t *right = hud_row_create(s_screen, LV_ALIGN_BOTTOM_RIGHT, -10, -8);
    s_batt_lbl = hud_text(right, &lv_font_Antonio_Regular_64, 0xFFFFFF);
    lv_label_set_text(hud_text(right, &lv_font_Antonio_Regular_22, 0xC8D0D8), "%");

    /* Tap anywhere on the map to offer it as a destination. */
    lv_obj_add_flag(s_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_img, map_pressed_cb, LV_EVENT_CLICKED, NULL);

    s_pick_box = lv_obj_create(s_screen);
    lv_obj_set_size(s_pick_box, 560, 74);
    /* Low and centred: at the top it sat across the turn plate, which is the
     * one thing on this screen that must never be covered. */
    lv_obj_align(s_pick_box, LV_ALIGN_BOTTOM_MID, 0, -96);
    lv_obj_set_style_bg_color(s_pick_box, lv_color_hex(0x1c2530), 0);
    lv_obj_set_style_bg_opa(s_pick_box, LV_OPA_90, 0);
    lv_obj_set_style_border_width(s_pick_box, 0, 0);
    lv_obj_set_style_radius(s_pick_box, 12, 0);
    lv_obj_set_style_pad_all(s_pick_box, 8, 0);
    lv_obj_clear_flag(s_pick_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_pick_box, LV_OBJ_FLAG_HIDDEN);

    s_pick_lbl = lv_label_create(s_pick_box);
    lv_obj_align(s_pick_lbl, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_color(s_pick_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_pick_lbl, &aabridge_font_24, 0);

    lv_obj_t *go = lv_btn_create(s_pick_box);
    lv_obj_set_size(go, 96, 56);
    lv_obj_align(go, LV_ALIGN_RIGHT_MID, -66, 0);
    lv_obj_set_style_bg_color(go, lv_color_hex(0x1E64DC), 0);
    lv_obj_add_event_cb(go, pick_go_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *go_lbl = lv_label_create(go);
    lv_label_set_text(go_lbl, "GO");
    lv_obj_center(go_lbl);

    lv_obj_t *no = lv_btn_create(s_pick_box);
    lv_obj_set_size(no, 56, 56);
    lv_obj_align(no, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(no, lv_color_hex(0x3A4450), 0);
    lv_obj_add_event_cb(no, pick_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *no_lbl = lv_label_create(no);
    lv_label_set_text(no_lbl, LV_SYMBOL_CLOSE);
    lv_obj_center(no_lbl);

    /* "FIND" opens the keyboard. Top right, above the zoom buttons — plain
     * ASCII on purpose, like the zoom glyphs. */
    lv_obj_t *find = lv_btn_create(s_screen);
    lv_obj_set_size(find, 96, 54);
    lv_obj_align(find, LV_ALIGN_TOP_RIGHT, -12, 10);
    lv_obj_set_style_bg_color(find, lv_color_hex(HUD_PLATE), 0);
    lv_obj_set_style_bg_opa(find, LV_OPA_70, 0);
    lv_obj_set_style_radius(find, 12, 0);
    lv_obj_add_event_cb(find, find_open_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *find_lbl = lv_label_create(find);
    lv_label_set_text(find_lbl, "FIND");
    lv_obj_set_style_text_font(find_lbl, &aabridge_font_24, 0);
    lv_obj_center(find_lbl);

    /* The overlay: a line to type in, what came back, and a keyboard. */
    s_find_box = lv_obj_create(s_screen);
    lv_obj_set_size(s_find_box, NAV_SCREEN_W, NAV_SCREEN_H);
    lv_obj_align(s_find_box, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(s_find_box, lv_color_hex(0x0c1116), 0);
    lv_obj_set_style_bg_opa(s_find_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_find_box, 0, 0);
    lv_obj_set_style_radius(s_find_box, 0, 0);
    lv_obj_set_style_pad_all(s_find_box, 0, 0);
    lv_obj_clear_flag(s_find_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_find_box, LV_OBJ_FLAG_HIDDEN);

    s_find_ta = lv_textarea_create(s_find_box);
    lv_obj_set_size(s_find_ta, 560, 56);
    lv_obj_align(s_find_ta, LV_ALIGN_TOP_LEFT, 12, 10);
    lv_textarea_set_one_line(s_find_ta, true);
    lv_textarea_set_placeholder_text(s_find_ta, "street, cafe, address");
    lv_obj_set_style_text_font(s_find_ta, &aabridge_font_24, 0);
    lv_obj_add_event_cb(s_find_ta, query_changed_cb, LV_EVENT_VALUE_CHANGED,
                        NULL);

    lv_obj_t *find_x = lv_btn_create(s_find_box);
    lv_obj_set_size(find_x, 90, 56);
    lv_obj_align(find_x, LV_ALIGN_TOP_RIGHT, -12, 10);
    lv_obj_set_style_bg_color(find_x, lv_color_hex(0x3A4450), 0);
    lv_obj_add_event_cb(find_x, find_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *find_x_lbl = lv_label_create(find_x);
    lv_label_set_text(find_x_lbl, "BACK");
    lv_obj_set_style_text_font(find_x_lbl, &aabridge_font_24, 0);
    lv_obj_center(find_x_lbl);

    s_find_list = lv_obj_create(s_find_box);
    lv_obj_set_size(s_find_list, NAV_SCREEN_W - 24, 150);
    lv_obj_align(s_find_list, LV_ALIGN_TOP_LEFT, 12, 74);
    lv_obj_set_style_bg_opa(s_find_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_find_list, 0, 0);
    lv_obj_set_style_pad_all(s_find_list, 0, 0);
    lv_obj_set_style_pad_row(s_find_list, 6, 0);
    lv_obj_set_flex_flow(s_find_list, LV_FLEX_FLOW_COLUMN);

    s_find_hint = lv_label_create(s_find_box);
    lv_obj_align(s_find_hint, LV_ALIGN_TOP_LEFT, 14, 80);
    lv_obj_set_style_text_color(s_find_hint, lv_color_hex(0x9AA7B4), 0);
    lv_obj_set_style_text_font(s_find_hint, &aabridge_font_24, 0);
    lv_label_set_text(s_find_hint, "");

    s_find_kb = lv_keyboard_create(s_find_box);
    lv_obj_set_size(s_find_kb, NAV_SCREEN_W, 240);
    lv_obj_align(s_find_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    /* Montserrat 24, not our subsetted font: the keyboard's backspace,
     * enter and hide keys are LV_SYMBOL glyphs from the FontAwesome range,
     * and a font without them draws four empty boxes along the bottom row
     * (the same trap as the zoom buttons, found the same way). */
    lv_obj_set_style_text_font(s_find_kb, &lv_font_montserrat_24, 0);
    lv_keyboard_set_textarea(s_find_kb, s_find_ta);
    lv_obj_add_event_cb(s_find_kb, query_ready_cb, LV_EVENT_READY, NULL);

    /* Zoom buttons down the right edge, above the charge readout, with the
     * level between them so a press shows what it did. */
    /* Plain ASCII, not LV_SYMBOL_PLUS: the symbols are FontAwesome glyphs and
     * our subsetted font has none of them — on the panel they came out as
     * empty boxes (the same trap as U+2026 in a Montserrat label). */
    struct { const char *text; lv_event_cb_t cb; int dy; } zbtns[] = {
        { "+", zoom_in_cb,  -104 },
        { "-", zoom_out_cb,   14 },
    };
    for (unsigned i = 0; i < sizeof zbtns / sizeof zbtns[0]; i++) {
        lv_obj_t *b = lv_btn_create(s_screen);
        lv_obj_set_size(b, 62, 62);
        lv_obj_align(b, LV_ALIGN_RIGHT_MID, -12, zbtns[i].dy);
        lv_obj_set_style_bg_color(b, lv_color_hex(HUD_PLATE), 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_70, 0);
        lv_obj_set_style_radius(b, 14, 0);
        lv_obj_add_event_cb(b, zbtns[i].cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, zbtns[i].text);
        lv_obj_set_style_text_font(l, &aabridge_font_32, 0);
        lv_obj_center(l);
    }
    s_zoom_lbl = lv_label_create(s_screen);
    lv_obj_align(s_zoom_lbl, LV_ALIGN_RIGHT_MID, -30, -42);
    lv_obj_set_style_text_color(s_zoom_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_zoom_lbl, &aabridge_font_24, 0);
    lv_obj_set_style_bg_color(s_zoom_lbl, lv_color_hex(HUD_PLATE), 0);
    lv_obj_set_style_bg_opa(s_zoom_lbl, LV_OPA_60, 0);
    lv_obj_set_style_pad_all(s_zoom_lbl, 4, 0);
    lv_obj_set_style_radius(s_zoom_lbl, 8, 0);
    lv_label_set_text(s_zoom_lbl, "");
    lv_obj_add_flag(s_zoom_lbl, LV_OBJ_FLAG_HIDDEN);

    /* The turn plate: where to go next, and how far. Top left, clear of the
     * destination prompt in the middle. */
    s_turn_box = hud_row_create(s_screen, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_pad_column(s_turn_box, 10, 0);
    lv_obj_add_flag(s_turn_box, LV_OBJ_FLAG_HIDDEN);

    s_turn_arrow = lv_line_create(s_turn_box);
    lv_obj_set_size(s_turn_arrow, 60, 66);
    lv_obj_set_style_line_color(s_turn_arrow, lv_color_white(), 0);
    lv_obj_set_style_line_width(s_turn_arrow, 8, 0);
    lv_obj_set_style_line_rounded(s_turn_arrow, true, 0);
    lv_line_set_points(s_turn_arrow, k_arrows[NAV_TURN_STRAIGHT].p,
                       k_arrows[NAV_TURN_STRAIGHT].n);

    s_turn_dist = hud_text(s_turn_box, &lv_font_Antonio_Regular_64, 0xFFFFFF);
    s_turn_unit = hud_text(s_turn_box, &lv_font_Antonio_Regular_22, 0xC8D0D8);

    /* What is left of the ride, under the turn plate. */
    s_remain_lbl = lv_label_create(s_screen);
    lv_obj_align(s_remain_lbl, LV_ALIGN_TOP_LEFT, 14, 92);
    lv_obj_set_style_text_color(s_remain_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_remain_lbl, &aabridge_font_24, 0);
    lv_obj_set_style_bg_color(s_remain_lbl, lv_color_hex(HUD_PLATE), 0);
    lv_obj_set_style_bg_opa(s_remain_lbl, LV_OPA_70, 0);
    lv_obj_set_style_pad_hor(s_remain_lbl, 8, 0);
    lv_obj_set_style_pad_ver(s_remain_lbl, 3, 0);
    lv_obj_set_style_radius(s_remain_lbl, 8, 0);
    lv_label_set_text(s_remain_lbl, "");
    lv_obj_add_flag(s_remain_lbl, LV_OBJ_FLAG_HIDDEN);

    s_tick = lv_timer_create(tick_cb, TICK_PERIOD_MS, NULL);

    bsp_display_unlock();
    ESP_LOGI(TAG, "navigator screen ready (2 x %u-byte framebuffers)",
             (unsigned)s_fb_bytes);
    return ESP_OK;
}

lv_obj_t *nav_screen_get(void) { return s_screen; }

uint16_t *nav_screen_back_buffer(void)
{
    if (!s_fb[0] || !s_fb[1]) return NULL;
    if (atomic_load(&s_pending)) return NULL;
    if (atomic_load(&s_cooldown) > 0) return NULL;
    return s_fb[s_front ^ 1];
}

size_t nav_screen_back_buffer_bytes(void) { return s_fb_bytes; }

void nav_screen_commit(void) { atomic_store(&s_pending, true); }

void nav_screen_set_active(bool active) { atomic_store(&s_active, active); }
bool nav_screen_active(void) { return atomic_load(&s_active); }

void nav_screen_set_dest_cb(nav_screen_dest_cb_t cb) { s_dest_cb = cb; }
void nav_screen_set_zoom_cb(nav_screen_zoom_cb_t cb) { s_zoom_cb = cb; }
void nav_screen_set_search_cb(nav_screen_search_cb_t cb) { s_search_cb = cb; }

void nav_screen_set_results(const nav_search_hit_t *hits, size_t n)
{
    if (n > NAV_SEARCH_MAX) n = NAV_SEARCH_MAX;
    if (hits && n) memcpy(s_hits, hits, n * sizeof(s_hits[0]));
    atomic_store(&s_nhits, (int)n);
    atomic_store(&s_hits_fresh, true);
}

void nav_screen_set_phone(bool connected)
{
    atomic_store(&s_phone, connected);
    if (!connected) atomic_store(&s_streaming, false);
}

void nav_screen_set_streaming(bool streaming)
{
    atomic_store(&s_streaming, streaming);
}

uint32_t nav_screen_frames_shown(void) { return atomic_load(&s_frames); }
int64_t  nav_screen_last_frame_us(void) { return atomic_load(&s_last_frame_us); }
