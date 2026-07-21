/*
 * Pedal-Assist (PAS) settings screen (show_pas_settings).
 *
 * A self-contained LVGL screen — created on demand, destroyed on unload, like
 * the realtime viewer / VESC Tool menu — that hosts the WHOLE PAS feature on
 * the head unit (no phone involvement):
 *   - cadence-sensor pairing: a "Search" button kicks the BLE central scan
 *     (ble_cadence_scan_start), found sensors stream in via the scan callback
 *     and become tappable rows; tapping binds that specific sensor
 *     (pas_sensor_select). "Forget" unbinds.
 *   - live signed cadence + assist current + sensor status/battery, refreshed
 *     by an lv_timer reading the pas_get_telem() snapshot.
 *   - assist tuning: enable, reverse, level, max current, mode, and the
 *     start/stop/ramp/cadence-threshold params — each control writes straight
 *     to the PAS backend (pas_set_settings), which persists to NVS.
 *
 * The scan callback fires on the NimBLE host task, so it only stashes the hit
 * and marshals the list rebuild onto the LVGL task with lv_async_call.
 *
 * Device-only (needs the PAS backend in main/); the simulator gets a stub.
 */
#include "lvgl.h"
#include "custom.h"

extern lv_ui guider_ui;

#ifdef LV_REALDEVICE

#include "freertos/FreeRTOS.h"
#include "pas.h"
#include "ble_cadence_client.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---- palette (matches realtime_viewer.c / the rest of the UI) ---- */
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

typedef struct {
    uint8_t addr[6];
    uint8_t type;
    char    name[28];
    int8_t  rssi;
} scanhit_t;

/* A numeric stepper bound to one config field (display units). */
typedef struct {
    int        val, vmin, vmax, vstep;
    bool       decimal;   /* show val/10 with one decimal */
    const char *unit;
    lv_obj_t  *lbl;
} spin_t;

static lv_obj_t  *s_screen;
static lv_timer_t *s_timer;
static bool       s_alive;

/* live labels */
static lv_obj_t *s_rpm_lbl, *s_dir_lbl, *s_assist_lbl, *s_status_lbl, *s_batt_lbl;

/* editors */
static lv_obj_t *s_sw_enable, *s_sw_reverse, *s_dd_mode;
static spin_t   *s_sp_level, *s_sp_maxA, *s_sp_start_delay, *s_sp_start_pct,
                *s_sp_ramp, *s_sp_stop_delay, *s_sp_min_cad, *s_sp_full_cad;
static spin_t    s_spins[8];
static int       s_spin_n;
static uint8_t   s_level_count = 5;

/* scan results (written on the NimBLE host task, read on the LVGL task) */
static portMUX_TYPE s_hit_mux = portMUX_INITIALIZER_UNLOCKED;
static scanhit_t    s_hits[MAX_SCAN];
static volatile int s_hit_count;
static int          s_built_count;
static lv_obj_t    *s_scan_list;

/* ---- config apply ---- */

static void apply_cfg(void)
{
    pas_settings_t c;
    pas_get_settings(&c);                 /* keep fields we don't edit here */
    c.enabled           = lv_obj_has_state(s_sw_enable, LV_STATE_CHECKED);
    c.reverse           = lv_obj_has_state(s_sw_reverse, LV_STATE_CHECKED);
    c.level             = (uint8_t)s_sp_level->val;
    c.level_count       = s_level_count;
    c.max_current_a     = s_sp_maxA->val / 10.0f;
    c.mode              = (uint8_t)lv_dropdown_get_selected(s_dd_mode);
    c.start_delay_ms    = (uint16_t)s_sp_start_delay->val;
    c.start_current_pct = (uint8_t)s_sp_start_pct->val;
    c.ramp_up_aps       = s_sp_ramp->val / 10.0f;
    c.stop_delay_ms     = (uint16_t)s_sp_stop_delay->val;
    c.min_cadence_rpm   = (uint16_t)s_sp_min_cad->val;
    c.full_cadence_rpm  = (uint16_t)s_sp_full_cad->val;
    pas_set_settings(&c);
}

/* ---- widget builders ---- */

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

static void spin_refresh(spin_t *s)
{
    if (!s->lbl) return;
    char b[24];
    if (s->decimal) {
        snprintf(b, sizeof b, "%d.%d%s", s->val / 10, s->val % 10,
                 s->unit ? s->unit : "");
    } else {
        snprintf(b, sizeof b, "%d%s", s->val, s->unit ? s->unit : "");
    }
    lv_label_set_text(s->lbl, b);
}

static void spin_minus_cb(lv_event_t *e)
{
    spin_t *s = (spin_t *)lv_event_get_user_data(e);
    s->val -= s->vstep;
    if (s->val < s->vmin) s->val = s->vmin;
    spin_refresh(s);
    apply_cfg();
}

static void spin_plus_cb(lv_event_t *e)
{
    spin_t *s = (spin_t *)lv_event_get_user_data(e);
    s->val += s->vstep;
    if (s->val > s->vmax) s->val = s->vmax;
    spin_refresh(s);
    apply_cfg();
}

static spin_t *add_spinner(lv_obj_t *list, const char *name, int val, int vmin,
                           int vmax, int vstep, bool decimal, const char *unit)
{
    spin_t *s = &s_spins[s_spin_n++];
    s->val = val; s->vmin = vmin; s->vmax = vmax; s->vstep = vstep;
    s->decimal = decimal; s->unit = unit;

    lv_obj_t *row = row_base(list, name, 50);

    lv_obj_t *minus = step_btn(row, "-", COL_RED);
    lv_obj_align(minus, LV_ALIGN_RIGHT_MID, -190, 0);
    lv_obj_add_event_cb(minus, spin_minus_cb, LV_EVENT_CLICKED, s);

    s->lbl = lv_label_create(row);
    lv_obj_set_width(s->lbl, 120);
    lv_obj_align(s->lbl, LV_ALIGN_RIGHT_MID, -66, 0);
    lv_obj_set_style_text_align(s->lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s->lbl, lv_color_hex(COL_CYAN), 0);
    lv_obj_set_style_text_font(s->lbl, &lv_font_montserrat_24, 0);

    lv_obj_t *plus = step_btn(row, "+", COL_CYAN);
    lv_obj_align(plus, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_add_event_cb(plus, spin_plus_cb, LV_EVENT_CLICKED, s);

    spin_refresh(s);
    return s;
}

static void switch_cb(lv_event_t *e) { (void)e; apply_cfg(); }
static void dropdown_cb(lv_event_t *e) { (void)e; apply_cfg(); }

static lv_obj_t *add_switch(lv_obj_t *list, const char *name, bool on)
{
    lv_obj_t *row = row_base(list, name, 50);
    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_set_size(sw, 60, 30);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -8, 0);
    if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, lv_color_hex(COL_BTN), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(COL_CYAN),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    return sw;
}

/* ---- sensor pairing ---- */

static void hit_select_cb(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (i < 0 || i >= s_built_count) return;
    pas_sensor_select(s_hits[i].addr, s_hits[i].type);
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
    ble_cadence_scan_start();
}

static void forget_cb(lv_event_t *e) { (void)e; pas_sensor_forget(); }

/* ---- live updater ---- */

static void update_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_alive) return;
    pas_telem_t tm;
    pas_get_telem(&tm);

    char b[24];
    snprintf(b, sizeof b, "%d", tm.centi_rpm / 100);
    lv_label_set_text(s_rpm_lbl, b);

    const char *dir; uint32_t dcol;
    if (!tm.sensor_connected)      { dir = "--";      dcol = COL_DIM; }
    else if (tm.centi_rpm > 50)    { dir = "forward"; dcol = COL_ACCENT; }
    else if (tm.centi_rpm < -50)   { dir = "reverse"; dcol = COL_RED; }
    else                           { dir = "idle";    dcol = COL_DIM; }
    lv_label_set_text(s_dir_lbl, dir);
    lv_obj_set_style_text_color(s_dir_lbl, lv_color_hex(dcol), 0);

    snprintf(b, sizeof b, "%.1f A", (double)tm.assist_a);
    lv_label_set_text(s_assist_lbl, b);

    const char *st; uint32_t scol;
    if (tm.sensor_connected)   { st = "Connected";       scol = COL_ACCENT; }
    else if (tm.scanning)      { st = "Searching...";    scol = COL_AMBER; }
    else if (tm.sensor_bound)  { st = "Reconnecting...";  scol = COL_AMBER; }
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
    ble_cadence_set_scan_cb(NULL);
    if (s_timer) { lv_timer_del(s_timer); s_timer = NULL; }
    if (s_screen) { lv_obj_del_async(s_screen); s_screen = NULL; }
    s_scan_list = NULL;
    s_spin_n = 0;
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

void show_pas_settings(void)
{
    if (s_screen) return;  /* re-entrancy guard */

    s_spin_n = 0;
    s_built_count = 0;
    s_hit_count = 0;

    pas_settings_t cfg;
    pas_get_settings(&cfg);
    s_level_count = cfg.level_count ? cfg.level_count : 5;

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
    lv_label_set_text(title, "Pedal Assist");
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

    /* ---- Sensor section ---- */
    add_section(list, "Cadence sensor");

    lv_obj_t *strow = row_base(list, "Status", 40);
    s_status_lbl = live_cell(strow, COL_DIM, NULL);
    lv_obj_t *btrow = row_base(list, "Sensor battery", 40);
    s_batt_lbl = live_cell(btrow, COL_TEXT, NULL);

    /* Live cadence — right next to the sensor status/battery so it's the first
     * thing the rider sees (signed RPM = direction). */
    lv_obj_t *rpmrow = row_base(list, "RPM (cadence)", 64);
    s_rpm_lbl = live_cell(rpmrow, COL_CYAN, &lv_font_montserrat_48);
    lv_obj_t *dirrow = row_base(list, "Direction", 40);
    s_dir_lbl = live_cell(dirrow, COL_DIM, NULL);
    lv_obj_t *asrow = row_base(list, "Assist current", 40);
    s_assist_lbl = live_cell(asrow, COL_ACCENT, NULL);

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

    /* ---- Assist tuning ---- */
    add_section(list, "Assist");
    s_sw_enable  = add_switch(list, "Throttle / assist enabled", cfg.enabled);
    s_sw_reverse = add_switch(list, "Reverse rotation", cfg.reverse);
    s_sp_level   = add_spinner(list, "Assist level", cfg.level, 0,
                               s_level_count, 1, false, NULL);
    s_sp_maxA    = add_spinner(list, "Max current", (int)(cfg.max_current_a * 10),
                               0, 1000, 5, true, " A");

    lv_obj_t *moderow = row_base(list, "Mode", 50);
    s_dd_mode = lv_dropdown_create(moderow);
    lv_dropdown_set_options(s_dd_mode, "On/off (level)\nProportional to cadence");
    lv_obj_set_width(s_dd_mode, 320);
    lv_obj_align(s_dd_mode, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_dropdown_set_selected(s_dd_mode, cfg.mode);
    lv_obj_add_event_cb(s_dd_mode, dropdown_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ---- Advanced ---- */
    add_section(list, "Advanced");
    s_sp_start_delay = add_spinner(list, "Start delay", cfg.start_delay_ms,
                                   0, 3000, 50, false, " ms");
    s_sp_start_pct   = add_spinner(list, "Start current", cfg.start_current_pct,
                                   0, 100, 5, false, " %");
    s_sp_ramp        = add_spinner(list, "Ramp", (int)(cfg.ramp_up_aps * 10),
                                   1, 1000, 5, true, " A/s");
    s_sp_stop_delay  = add_spinner(list, "Stop delay", cfg.stop_delay_ms,
                                   0, 3000, 50, false, " ms");
    s_sp_min_cad     = add_spinner(list, "Min cadence", cfg.min_cadence_rpm,
                                   1, 120, 1, false, " RPM");
    s_sp_full_cad    = add_spinner(list, "Full-assist cadence", cfg.full_cadence_rpm,
                                   10, 200, 5, false, " RPM");

    lv_obj_add_event_cb(s_screen, screen_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    s_alive = true;

    /* Receive scan hits while this screen is open, and refresh live values. */
    ble_cadence_set_scan_cb(scan_cb);
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

void show_pas_settings(void)
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
    lv_label_set_text(lbl, "Pedal-assist settings are available on the device build.");
    lv_obj_center(lbl);

    lv_obj_add_event_cb(s_sim_screen, sim_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_scr_load_anim(s_sim_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

#endif /* LV_REALDEVICE */
