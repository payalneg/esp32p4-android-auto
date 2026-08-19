/*
 * Wheel-speed sensor settings screen (show_speed_settings).
 *
 * A self-contained LVGL screen — created on demand, destroyed on unload,
 * mirroring pas_screen.c — that hosts the whole BLE wheel-speed feature:
 *   - the speed-source selector (VESC vs BLE sensor) — an explicit user
 *     choice persisted by the speed_sensor backend on its own writer task;
 *   - sensor pairing: "Search" kicks the BLE central scan
 *     (ble_speed_scan_start), found CSC advertisers stream in via the scan
 *     callback and become tappable rows; tapping binds that sensor
 *     (speed_sensor_select). "Forget" unbinds;
 *   - live speed / trip / odometer / sensor status + battery, refreshed by
 *     an lv_timer reading the speed_sensor_get_telem() snapshot;
 *   - the wheel-diameter setting (shared vesc_cfg "wheel_mm") via a
 *     volatile setter + a local 800 ms debounce before the NVS persist, so
 *     rapid +/- taps never stall the LVGL task in a flash commit (unlike
 *     pas_screen's per-tap commits — a known freeze source, don't copy it).
 *
 * The scan callback fires on the NimBLE host task, so it only stashes the
 * hit and marshals the list rebuild onto the LVGL task with lv_async_call.
 *
 * Device-only (needs the speed backend in main/); the simulator gets a stub.
 */
#include "lvgl.h"
#include "custom.h"

extern lv_ui guider_ui;

#ifdef LV_REALDEVICE

#include "freertos/FreeRTOS.h"
#include "speed_sensor.h"
#include "ble_speed_client.h"
#include "settings_wrapper.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---- palette (matches pas_screen.c / the rest of the UI) ---- */
#define COL_BG      0x07090A
#define COL_PANEL   0x12181C
#define COL_BTN     0x2a3440
#define COL_ACCENT  0xB6FF2E
#define COL_CYAN    0x00a9ff
#define COL_RED     0xFF3B30
#define COL_AMBER   0xFFA500
#define COL_TEXT    0xFFFFFF
#define COL_DIM     0x8A9499

#define MAX_SCAN 12

#define WHEEL_MM_MIN   100
#define WHEEL_MM_MAX   3000
#define WHEEL_MM_STEP  5
#define WHEEL_PERSIST_DEBOUNCE_MS 800

typedef struct {
    uint8_t addr[6];
    uint8_t type;
    char    name[28];
    int8_t  rssi;
} scanhit_t;

static lv_obj_t   *s_screen;
static lv_timer_t *s_timer;
static bool        s_alive;

/* live labels */
static lv_obj_t *s_status_lbl, *s_batt_lbl, *s_speed_lbl, *s_trip_lbl,
                *s_odo_lbl;

/* wheel-diameter stepper + its debounced NVS persist */
static lv_obj_t   *s_wheel_lbl;
static int         s_wheel_mm;
static lv_timer_t *s_wheel_persist_timer;   /* one-shot; NULL = nothing pending */

/* scan results (written on the NimBLE host task, read on the LVGL task) */
static portMUX_TYPE s_hit_mux = portMUX_INITIALIZER_UNLOCKED;
static scanhit_t    s_hits[MAX_SCAN];
static volatile int s_hit_count;
static int          s_built_count;
static lv_obj_t    *s_scan_list;

/* ---- widget builders (same shapes as pas_screen.c) ---- */

static lv_obj_t *row_base(lv_obj_t *list, const char *name, int h)
{
    lv_obj_t *row = lv_obj_create(list);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, h);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 2, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    if (name) {
        lv_obj_t *n = lv_label_create(row);
        lv_label_set_text(n, name);
        lv_obj_align(n, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_set_style_text_color(n, lv_color_hex(COL_TEXT), 0);
    }
    return row;
}

static void add_section(lv_obj_t *list, const char *txt)
{
    lv_obj_t *s = lv_label_create(list);
    lv_label_set_text(s, txt);
    lv_obj_set_style_text_color(s, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_text_font(s, &lv_font_montserratMedium_16, 0);
    lv_obj_set_style_pad_top(s, 8, 0);
}

static void add_hint(lv_obj_t *list, const char *txt)
{
    lv_obj_t *s = lv_label_create(list);
    lv_label_set_text(s, txt);
    lv_obj_set_style_text_color(s, lv_color_hex(COL_DIM), 0);
    lv_obj_set_width(s, lv_pct(100));
    lv_label_set_long_mode(s, LV_LABEL_LONG_WRAP);
}

static lv_obj_t *step_btn(lv_obj_t *parent, const char *glyph, uint32_t col)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, 54, 42);
    lv_obj_set_style_bg_color(b, lv_color_hex(col), 0);
    lv_obj_set_style_radius(b, 6, 0);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, glyph);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_center(l);
    return b;
}

/* A live read-only value cell on the right of a row. */
static lv_obj_t *live_cell(lv_obj_t *row, uint32_t col, const lv_font_t *font)
{
    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, "--");
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(col), 0);
    if (font) lv_obj_set_style_text_font(v, font, 0);
    return v;
}

/* ---- speed source ---- */

static void source_dd_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    /* Persisted off the LVGL thread by speed_sensor's writer task. */
    speed_sensor_set_source(lv_dropdown_get_selected(dd) == 1);
}

/* ---- wheel diameter (volatile now, NVS commit on a debounce) ---- */

static void wheel_refresh(void)
{
    if (!s_wheel_lbl) return;
    char b[24];
    snprintf(b, sizeof b, "%d mm", s_wheel_mm);
    lv_label_set_text(s_wheel_lbl, b);
}

static void wheel_persist_timer_cb(lv_timer_t *t)
{
    (void)t;
    s_wheel_persist_timer = NULL;   /* one-shot */
    settings_wrapper_persist_wheel_diameter_mm();
}

static void wheel_changed(void)
{
    settings_wrapper_set_wheel_diameter_mm_volatile((uint16_t)s_wheel_mm);
    wheel_refresh();
    if (s_wheel_persist_timer) {
        lv_timer_reset(s_wheel_persist_timer);
    } else {
        s_wheel_persist_timer =
            lv_timer_create(wheel_persist_timer_cb,
                            WHEEL_PERSIST_DEBOUNCE_MS, NULL);
        lv_timer_set_repeat_count(s_wheel_persist_timer, 1);
    }
}

static void wheel_minus_cb(lv_event_t *e)
{
    (void)e;
    s_wheel_mm -= WHEEL_MM_STEP;
    if (s_wheel_mm < WHEEL_MM_MIN) s_wheel_mm = WHEEL_MM_MIN;
    wheel_changed();
}

static void wheel_plus_cb(lv_event_t *e)
{
    (void)e;
    s_wheel_mm += WHEEL_MM_STEP;
    if (s_wheel_mm > WHEEL_MM_MAX) s_wheel_mm = WHEEL_MM_MAX;
    wheel_changed();
}

/* ---- sensor pairing ---- */

static void hit_select_cb(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (i < 0 || i >= s_built_count) return;
    speed_sensor_select(s_hits[i].addr, s_hits[i].type);
}

static void rebuild_scan_async(void *p)
{
    (void)p;
    if (!s_alive || !s_scan_list) return;
    portENTER_CRITICAL(&s_hit_mux);
    int n = s_hit_count;
    portEXIT_CRITICAL(&s_hit_mux);
    if (n == s_built_count) return;        /* nothing new */

    lv_obj_clean(s_scan_list);
    for (int i = 0; i < n; i++) {
        lv_obj_t *b = lv_btn_create(s_scan_list);
        lv_obj_set_width(b, lv_pct(100));
        lv_obj_set_height(b, 44);
        lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN), 0);
        lv_obj_set_style_radius(b, 6, 0);
        lv_obj_t *l = lv_label_create(b);
        char buf[48];
        snprintf(buf, sizeof buf, "%s  (%d dBm)",
                 s_hits[i].name[0] ? s_hits[i].name : "?", s_hits[i].rssi);
        lv_label_set_text(l, buf);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_add_event_cb(b, hit_select_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
    }
    s_built_count = n;
}

/* NimBLE host task — only stash + marshal to the LVGL task. */
static void scan_cb(const uint8_t addr[6], uint8_t addr_type,
                    const char *name, int8_t rssi)
{
    portENTER_CRITICAL(&s_hit_mux);
    int found = -1;
    for (int i = 0; i < s_hit_count; i++) {
        if (s_hits[i].type == addr_type &&
            memcmp(s_hits[i].addr, addr, 6) == 0) { found = i; break; }
    }
    if (found < 0 && s_hit_count < MAX_SCAN) {
        found = s_hit_count++;
        memcpy(s_hits[found].addr, addr, 6);
        s_hits[found].type = addr_type;
    }
    if (found >= 0) {
        s_hits[found].rssi = rssi;
        size_t nl = name ? strlen(name) : 0;
        if (nl >= sizeof(s_hits[0].name)) nl = sizeof(s_hits[0].name) - 1;
        if (nl) memcpy(s_hits[found].name, name, nl);
        s_hits[found].name[nl] = '\0';
    }
    portEXIT_CRITICAL(&s_hit_mux);
    lv_async_call(rebuild_scan_async, NULL);
}

static void search_cb(lv_event_t *e)
{
    (void)e;
    portENTER_CRITICAL(&s_hit_mux);
    s_hit_count = 0;
    portEXIT_CRITICAL(&s_hit_mux);
    s_built_count = 0;
    if (s_scan_list) lv_obj_clean(s_scan_list);
    ble_speed_scan_start();
}

static void forget_cb(lv_event_t *e) { (void)e; speed_sensor_forget(); }

/* ---- live updater ---- */

static void update_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_alive) return;
    speed_telem_t tm;
    speed_sensor_get_telem(&tm);

    bool imp = settings_wrapper_get_use_imperial();
    float unit_scale = imp ? 0.621371f : 1.0f;

    char b[32];
    snprintf(b, sizeof b, "%.1f %s", (double)(tm.kmh * unit_scale),
             imp ? "mph" : "km/h");
    lv_label_set_text(s_speed_lbl, b);

    snprintf(b, sizeof b, "%.2f %s", (double)(tm.trip_km * unit_scale),
             imp ? "mi" : "km");
    lv_label_set_text(s_trip_lbl, b);

    snprintf(b, sizeof b, "%.1f %s", (double)(tm.odo_km * unit_scale),
             imp ? "mi" : "km");
    lv_label_set_text(s_odo_lbl, b);

    const char *st; uint32_t scol;
    if (tm.sensor_connected)   { st = "Connected";       scol = COL_ACCENT; }
    else if (tm.scanning)      { st = "Searching...";    scol = COL_AMBER; }
    else if (tm.sensor_bound)  { st = "Reconnecting..."; scol = COL_AMBER; }
    else                       { st = "Not paired";      scol = COL_DIM; }
    lv_label_set_text(s_status_lbl, st);
    lv_obj_set_style_text_color(s_status_lbl, lv_color_hex(scol), 0);

    if (tm.battery == 0xFF) lv_label_set_text(s_batt_lbl, "--");
    else { snprintf(b, sizeof b, "%u%%", tm.battery); lv_label_set_text(s_batt_lbl, b); }
}

/* ---- lifecycle ---- */

static void back_cb(lv_event_t *e)
{
    (void)e;
    lv_scr_load_anim(guider_ui.settings, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

static void screen_unloaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    s_alive = false;
    ble_speed_set_scan_cb(NULL);
    if (s_wheel_persist_timer) {
        /* Flush the pending wheel-diameter commit before the timer dies —
         * one commit on leaving the screen, same cost as any settings row. */
        lv_timer_del(s_wheel_persist_timer);
        s_wheel_persist_timer = NULL;
        settings_wrapper_persist_wheel_diameter_mm();
    }
    if (s_timer) { lv_timer_del(s_timer); s_timer = NULL; }
    if (s_screen) { lv_obj_del_async(s_screen); s_screen = NULL; }
    s_scan_list = NULL;
    s_wheel_lbl = NULL;
}

void show_speed_settings(void)
{
    if (s_screen) return;  /* re-entrancy guard */

    s_built_count = 0;
    s_hit_count = 0;
    s_wheel_mm = settings_wrapper_get_wheel_diameter_mm();

    s_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_screen, 800, 480);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_screen, 255, 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* header */
    lv_obj_t *back = lv_btn_create(s_screen);
    lv_obj_set_pos(back, 8, 8);
    lv_obj_set_size(back, 90, 40);
    lv_obj_set_style_bg_color(back, lv_color_hex(COL_BTN), 0);
    lv_obj_set_style_radius(back, 6, 0);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "Back");
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, "Speed Sensor");
    lv_obj_set_pos(title, 110, 16);
    lv_obj_set_style_text_color(title, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);

    /* scrollable content */
    lv_obj_t *list = lv_obj_create(s_screen);
    lv_obj_set_pos(list, 8, 56);
    lv_obj_set_size(list, 784, 416);
    lv_obj_set_style_bg_color(list, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_radius(list, 6, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

    /* ---- Speed source ---- */
    add_section(list, "Speed source");
    lv_obj_t *srcrow = row_base(list, "Dashboard speed from", 50);
    lv_obj_t *dd = lv_dropdown_create(srcrow);
    lv_dropdown_set_options(dd, "VESC (motor)\nBLE wheel sensor");
    lv_obj_set_width(dd, 320);
    lv_obj_align(dd, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_dropdown_set_selected(dd, speed_source_is_ble() ? 1 : 0);
    lv_obj_add_event_cb(dd, source_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);
    add_hint(list, "With the BLE sensor selected, speed, trip and odometer "
                   "come from the wheel sensor (odometer is stored on this "
                   "display).");

    /* ---- Sensor section ---- */
    add_section(list, "Wheel sensor");

    lv_obj_t *strow = row_base(list, "Status", 40);
    s_status_lbl = live_cell(strow, COL_DIM, NULL);
    lv_obj_t *btrow = row_base(list, "Sensor battery", 40);
    s_batt_lbl = live_cell(btrow, COL_TEXT, NULL);

    /* Live speed — big, right under the status, so pairing is instantly
     * verifiable by spinning the wheel. */
    lv_obj_t *sprow = row_base(list, "Speed", 64);
    s_speed_lbl = live_cell(sprow, COL_CYAN, &lv_font_montserrat_48);
    lv_obj_t *trrow = row_base(list, "Trip", 40);
    s_trip_lbl = live_cell(trrow, COL_TEXT, NULL);
    lv_obj_t *odrow = row_base(list, "Odometer", 40);
    s_odo_lbl = live_cell(odrow, COL_TEXT, NULL);

    lv_obj_t *brow = row_base(list, NULL, 52);
    lv_obj_t *search = lv_btn_create(brow);
    lv_obj_set_size(search, 150, 44);
    lv_obj_align(search, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_bg_color(search, lv_color_hex(COL_CYAN), 0);
    lv_obj_set_style_radius(search, 8, 0);
    lv_obj_t *sl = lv_label_create(search);
    lv_label_set_text(sl, "Search");
    lv_obj_center(sl);
    lv_obj_add_event_cb(search, search_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *forget = lv_btn_create(brow);
    lv_obj_set_size(forget, 150, 44);
    lv_obj_align(forget, LV_ALIGN_LEFT_MID, 168, 0);
    lv_obj_set_style_bg_color(forget, lv_color_hex(COL_BTN), 0);
    lv_obj_set_style_radius(forget, 8, 0);
    lv_obj_t *fl = lv_label_create(forget);
    lv_label_set_text(fl, "Forget");
    lv_obj_center(fl);
    lv_obj_add_event_cb(forget, forget_cb, LV_EVENT_CLICKED, NULL);

    /* dynamic scan-result list */
    s_scan_list = lv_obj_create(list);
    lv_obj_set_width(s_scan_list, lv_pct(100));
    lv_obj_set_height(s_scan_list, LV_SIZE_CONTENT);  /* 0 when empty → no gap */
    lv_obj_set_style_bg_opa(s_scan_list, 0, 0);
    lv_obj_set_style_border_width(s_scan_list, 0, 0);
    lv_obj_set_style_pad_all(s_scan_list, 2, 0);
    lv_obj_set_flex_flow(s_scan_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(s_scan_list, LV_DIR_VER);

    /* ---- Wheel ---- */
    add_section(list, "Wheel");
    lv_obj_t *whrow = row_base(list, "Wheel diameter", 50);

    lv_obj_t *minus = step_btn(whrow, "-", COL_RED);
    lv_obj_align(minus, LV_ALIGN_RIGHT_MID, -190, 0);
    lv_obj_add_event_cb(minus, wheel_minus_cb, LV_EVENT_CLICKED, NULL);

    s_wheel_lbl = lv_label_create(whrow);
    lv_obj_set_width(s_wheel_lbl, 120);
    lv_obj_align(s_wheel_lbl, LV_ALIGN_RIGHT_MID, -66, 0);
    lv_obj_set_style_text_align(s_wheel_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_wheel_lbl, lv_color_hex(COL_CYAN), 0);
    lv_obj_set_style_text_font(s_wheel_lbl, &lv_font_montserrat_24, 0);

    lv_obj_t *plus = step_btn(whrow, "+", COL_CYAN);
    lv_obj_align(plus, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_add_event_cb(plus, wheel_plus_cb, LV_EVENT_CLICKED, NULL);
    wheel_refresh();

    add_hint(list, "Outer tire diameter. Also used by the VESC speed "
                   "calculation helper.");

    lv_obj_add_event_cb(s_screen, screen_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    s_alive = true;

    /* Receive scan hits while this screen is open, and refresh live values. */
    ble_speed_set_scan_cb(scan_cb);
    s_timer = lv_timer_create(update_cb, 150, NULL);
    update_cb(s_timer);

    lv_scr_load_anim(s_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

#else  /* !LV_REALDEVICE — desktop simulator stub */

static lv_obj_t *s_sim_screen;

static void sim_unloaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    if (s_sim_screen) { lv_obj_del_async(s_sim_screen); s_sim_screen = NULL; }
}

static void sim_back_cb(lv_event_t *e)
{
    (void)e;
    lv_scr_load_anim(guider_ui.settings, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

void show_speed_settings(void)
{
    if (s_sim_screen) return;
    s_sim_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_sim_screen, 800, 480);
    lv_obj_set_style_bg_color(s_sim_screen, lv_color_hex(0x07090A), 0);

    lv_obj_t *btn = lv_btn_create(s_sim_screen);
    lv_obj_set_pos(btn, 17, 14);
    lv_obj_set_size(btn, 120, 40);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "Back");
    lv_obj_center(bl);
    lv_obj_add_event_cb(btn, sim_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(s_sim_screen);
    lv_label_set_text(lbl, "Speed-sensor settings are available on the device build.");
    lv_obj_center(lbl);

    lv_obj_add_event_cb(s_sim_screen, sim_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_scr_load_anim(s_sim_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

#endif /* LV_REALDEVICE */
