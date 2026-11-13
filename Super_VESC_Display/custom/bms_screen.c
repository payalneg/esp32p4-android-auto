/*
    Battery state screen + the dashboard tap target behind the "%" readout.

    Opened from two places:
      - dashboard: tap the battery percentage / "%" glyph;
      - Settings -> Bike -> "Battery (JK-BMS)" -> Open.

    Talks to main/bms_battery.h (JK-BMS over BLE) directly on the device; the
    desktop simulator gets a small fake reading so the layout can be checked.
    Currents and power shown here are what the BMS reports; the ride itself
    still uses the VESC for current, exactly as before.
*/

#include "lvgl.h"
#include "ui_geom.h"
#include "custom.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define BMS_BATTERY_MAX_CELLS 32

#if defined(LV_REALDEVICE) || defined(LV_SIMSHIM)
#include "freertos/FreeRTOS.h"
#include "bms_battery.h"
#else
/* Plain desktop simulator: no BLE backend. */
typedef struct {
    bool bound, connected, scanning, fresh, has_data;
    float soc, remaining_ah, full_ah, voltage, current, power;
    float temp1, temp2, temp_mos;
    float cell_min, cell_max, cell_delta;
    int cell_min_index, cell_max_index, cell_count;
    float cells[BMS_BATTERY_MAX_CELLS];
    uint32_t cycles;
    float soh;
    bool charging_mos, discharging_mos, balancing, heating;
    char model[20];
    uint32_t age_ms;
} bms_telem_t;

static void bms_battery_get(bms_telem_t *t)
{
    memset(t, 0, sizeof(*t));
    t->bound = true;
    t->connected = true;
    t->fresh = true;
    t->soc = 84;
    t->remaining_ah = 68.49f;
    t->full_ah = 81.0f;
    t->voltage = 53.25f;
    t->current = -4.12f;
    t->power = -219.4f;
    t->temp1 = 19.0f;
    t->temp2 = 19.1f;
    t->temp_mos = 21.0f;
    t->cell_min = 3.327f;
    t->cell_max = 3.329f;
    t->cell_delta = 0.002f;
    t->cell_count = 16;
    t->cycles = 3;
    t->soh = 100;
    t->charging_mos = true;
    t->discharging_mos = true;
    strcpy(t->model, "JK-B2A24S15P");
}
#endif

/* ---- palette (matches speed_screen.c / the rest of the UI) ---- */
#define COL_BG      0x07090A
#define COL_PANEL   0x12181C
#define COL_BTN     0x2a3440
#define COL_ACCENT  0x00a9ff
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

static lv_obj_t   *s_screen;
static lv_obj_t   *s_prev_screen;
static lv_timer_t *s_timer;
static bool        s_alive;

/* live labels */
static lv_obj_t *s_status, *s_soc, *s_rem, *s_full, *s_volt, *s_curr, *s_power,
                *s_temps, *s_cells, *s_cycles, *s_mos, *s_model;

/* scan results (written on the NimBLE host task) */
#if defined(LV_REALDEVICE) || defined(LV_SIMSHIM)
static portMUX_TYPE s_hit_mux = portMUX_INITIALIZER_UNLOCKED;
#endif
static scanhit_t    s_hits[MAX_SCAN];
static volatile int s_hit_count;
static int          s_built_count;
static lv_obj_t    *s_scan_list;

/* ---- small helpers ---- */

static void set_text(lv_obj_t *label, const char *txt)
{
    if (!label) return;
    const char *cur = lv_label_get_text(label);
    if (cur && strcmp(cur, txt) == 0) return;   /* no needless repaint */
    lv_label_set_text(label, txt);
}

static void fmt_v(char *b, size_t n, float v, const char *unit)
{
    if (!isfinite(v)) snprintf(b, n, "--");
    else              snprintf(b, n, "%.2f %s", (double)v, unit);
}

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
        lv_obj_set_width(n, lv_pct(46));
        lv_label_set_long_mode(n, LV_LABEL_LONG_DOT);
        lv_obj_align(n, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_set_style_text_color(n, lv_color_hex(COL_DIM), 0);
    }
    return row;
}

static lv_obj_t *live_cell(lv_obj_t *row, uint32_t col, const lv_font_t *font)
{
    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, "--");
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(col), 0);
    if (font) lv_obj_set_style_text_font(v, font, 0);
    return v;
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

/* ---- sensor pairing ---- */

#if defined(LV_REALDEVICE) || defined(LV_SIMSHIM)
static void hit_select_cb(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (i < 0 || i >= s_built_count) return;
    bms_battery_select(s_hits[i].addr, s_hits[i].type);
}

static void rebuild_scan_async(void *p)
{
    (void)p;
    if (!s_alive || !s_scan_list) return;
    portENTER_CRITICAL(&s_hit_mux);
    int n = s_hit_count;
    portEXIT_CRITICAL(&s_hit_mux);
    if (n == s_built_count) return;

    lv_obj_clean(s_scan_list);
    for (int i = 0; i < n; i++) {
        lv_obj_t *b = lv_btn_create(s_scan_list);
        lv_obj_set_width(b, lv_pct(100));
        lv_obj_set_height(b, 44);
        lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN), 0);
        lv_obj_set_style_radius(b, 6, 0);
        lv_obj_t *l = lv_label_create(b);
        char buf[56];
        snprintf(buf, sizeof buf, "%s  (%d dBm)",
                 s_hits[i].name[0] ? s_hits[i].name : "?", s_hits[i].rssi);
        lv_label_set_text(l, buf);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_add_event_cb(b, hit_select_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
    }
    s_built_count = n;
}

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
#endif /* LV_REALDEVICE || LV_SIMSHIM */

static void search_cb(lv_event_t *e)
{
    (void)e;
#if defined(LV_REALDEVICE) || defined(LV_SIMSHIM)
    portENTER_CRITICAL(&s_hit_mux);
    s_hit_count = 0;
    portEXIT_CRITICAL(&s_hit_mux);
#endif
    s_built_count = 0;
    if (s_scan_list) lv_obj_clean(s_scan_list);
#if defined(LV_REALDEVICE) || defined(LV_SIMSHIM)
    bms_battery_scan_start();
#endif
}

static void forget_cb(lv_event_t *e)
{
    (void)e;
#if defined(LV_REALDEVICE) || defined(LV_SIMSHIM)
    bms_battery_forget();
#endif
}

/* ---- live updater ---- */

static void update_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_alive) return;

    bms_telem_t tm;
    bms_battery_get(&tm);

    const char *st;
    uint32_t scol;
    if (tm.connected)      { st = "Connected";       scol = COL_ACCENT; }
    else if (tm.scanning)  { st = "Searching...";    scol = COL_AMBER; }
    else if (tm.bound)     { st = "Reconnecting..."; scol = COL_AMBER; }
    else                   { st = "Not paired";      scol = COL_DIM; }
    set_text(s_status, st);
    lv_obj_set_style_text_color(s_status, lv_color_hex(scol), 0);

    char b[64];
    if (isfinite(tm.soc) && tm.connected) snprintf(b, sizeof b, "%.0f %%", (double)tm.soc);
    else                                  snprintf(b, sizeof b, "--");
    set_text(s_soc, b);
    if (isfinite(tm.soc)) {
        uint32_t col = (tm.soc > 50) ? COL_ACCENT
                      : (tm.soc > 20) ? COL_AMBER : COL_RED;
        lv_obj_set_style_text_color(s_soc, lv_color_hex(col), 0);
    }

    if (tm.connected && isfinite(tm.remaining_ah)) {
        snprintf(b, sizeof b, "%.2f Ah", (double)tm.remaining_ah);
    } else snprintf(b, sizeof b, "--");
    set_text(s_rem, b);

    if (tm.connected && isfinite(tm.full_ah)) {
        snprintf(b, sizeof b, "%.2f Ah", (double)tm.full_ah);
    } else snprintf(b, sizeof b, "--");
    set_text(s_full, b);

    if (tm.connected) fmt_v(b, sizeof b, tm.voltage, "V"); else snprintf(b, sizeof b, "--");
    set_text(s_volt, b);

    if (tm.connected && isfinite(tm.current)) {
        snprintf(b, sizeof b, "%+.2f A", (double)tm.current);
    } else snprintf(b, sizeof b, "--");
    set_text(s_curr, b);

    if (tm.connected && isfinite(tm.power)) {
        snprintf(b, sizeof b, "%+.0f W", (double)tm.power);
    } else snprintf(b, sizeof b, "--");
    set_text(s_power, b);

    if (tm.connected) {
        char t1[16] = "--", t2[16] = "--", tmos[16] = "--";
        if (isfinite(tm.temp1))   snprintf(t1, sizeof t1, "%.1f", (double)tm.temp1);
        if (isfinite(tm.temp2))   snprintf(t2, sizeof t2, "%.1f", (double)tm.temp2);
        if (isfinite(tm.temp_mos)) snprintf(tmos, sizeof tmos, "%.1f", (double)tm.temp_mos);
        snprintf(b, sizeof b, "MOS %s, T1 %s, T2 %s C", tmos, t1, t2);
    } else snprintf(b, sizeof b, "--");
    set_text(s_temps, b);

    if (tm.connected && isfinite(tm.cell_min) && isfinite(tm.cell_max)) {
        snprintf(b, sizeof b, "%.3f .. %.3f V (%.0f mV)",
                 (double)tm.cell_min, (double)tm.cell_max,
                 (double)(tm.cell_delta * 1000.0f));
    } else snprintf(b, sizeof b, "--");
    set_text(s_cells, b);

    if (tm.connected) {
        char soh[16] = "--";
        if (isfinite(tm.soh)) snprintf(soh, sizeof soh, "%.0f%%", (double)tm.soh);
        snprintf(b, sizeof b, "%u cycles, SOH %s",
                 (unsigned)tm.cycles, soh);
    } else snprintf(b, sizeof b, "--");
    set_text(s_cycles, b);

    if (tm.connected) {
        snprintf(b, sizeof b, "chg %s, dis %s%s",
                 tm.charging_mos ? "on" : "off",
                 tm.discharging_mos ? "on" : "off",
                 tm.balancing ? ", balancing" : "");
    } else snprintf(b, sizeof b, "--");
    set_text(s_mos, b);

    if (tm.model[0]) set_text(s_model, tm.model);
    else             set_text(s_model, "--");
}

/* ---- lifecycle ---- */

static void back_cb(lv_event_t *e)
{
    (void)e;
    lv_obj_t *back = s_prev_screen ? s_prev_screen : guider_ui.settings;
    lv_scr_load_anim(back, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

static void screen_unloaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    s_alive = false;
#if defined(LV_REALDEVICE) || defined(LV_SIMSHIM)
    bms_battery_set_scan_cb(NULL);
    bms_battery_scan_stop();
#endif
    if (s_timer) { lv_timer_del(s_timer); s_timer = NULL; }
    if (s_screen) { lv_obj_del_async(s_screen); s_screen = NULL; }
    s_scan_list = NULL;
}

void show_bms_settings(void)
{
    if (s_screen) return;   /* re-entrancy guard */

    s_prev_screen = lv_scr_act();
    s_built_count = 0;
    s_hit_count = 0;

    s_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_screen, UI_W, UI_H);
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
    lv_label_set_text(title, "Battery (JK-BMS)");
    lv_obj_set_pos(title, 110, 16);
    lv_obj_set_style_text_color(title, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(title, UI_FONT_MENU, 0);

    /* scrollable content */
    lv_obj_t *list = lv_obj_create(s_screen);
    lv_obj_set_pos(list, 8, 56);
    lv_obj_set_size(list, UI_W - 16, UI_H - 64);
    lv_obj_set_style_bg_color(list, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_radius(list, 6, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

    /* ---- live state ---- */
    add_section(list, "Pack");

    lv_obj_t *strow = row_base(list, "Status", 40);
    s_status = live_cell(strow, COL_DIM, NULL);

    lv_obj_t *socrow = row_base(list, "State of charge", 64);
    s_soc = live_cell(socrow, COL_ACCENT, &lv_font_montserrat_48);

    lv_obj_t *rrow = row_base(list, "Remaining", 40);
    s_rem = live_cell(rrow, COL_TEXT, NULL);
    lv_obj_t *frow = row_base(list, "Full capacity", 40);
    s_full = live_cell(frow, COL_TEXT, NULL);

    lv_obj_t *vrow = row_base(list, "Voltage", 40);
    s_volt = live_cell(vrow, COL_TEXT, NULL);
    lv_obj_t *crow = row_base(list, "Current", 40);
    s_curr = live_cell(crow, COL_CYAN, NULL);
    lv_obj_t *prow = row_base(list, "Power", 40);
    s_power = live_cell(prow, COL_TEXT, NULL);

    lv_obj_t *trow = row_base(list, "Temperatures", 40);
    s_temps = live_cell(trow, COL_TEXT, NULL);
    lv_obj_t *celrow = row_base(list, "Cell range", 40);
    s_cells = live_cell(celrow, COL_TEXT, NULL);
    lv_obj_t *cycrow = row_base(list, "Cycles", 40);
    s_cycles = live_cell(cycrow, COL_TEXT, NULL);
    lv_obj_t *mrow = row_base(list, "MOSFETs", 40);
    s_mos = live_cell(mrow, COL_TEXT, NULL);
    lv_obj_t *modrow = row_base(list, "Model", 40);
    s_model = live_cell(modrow, COL_DIM, NULL);

    add_hint(list, "SOC, remaining Ah and range come straight from the pack. "
                   "Motor current and power still come from the VESC.");

    /* ---- pairing ---- */
    add_section(list, "Bluetooth pairing");
    add_hint(list, "The BMS accepts one BLE connection: close the JK-BMS "
                   "phone app before pairing here.");

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

    s_scan_list = lv_obj_create(list);
    lv_obj_set_width(s_scan_list, lv_pct(100));
    lv_obj_set_height(s_scan_list, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(s_scan_list, 0, 0);
    lv_obj_set_style_border_width(s_scan_list, 0, 0);
    lv_obj_set_style_pad_all(s_scan_list, 2, 0);
    lv_obj_set_flex_flow(s_scan_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(s_scan_list, LV_DIR_VER);

    lv_obj_add_event_cb(s_screen, screen_unloaded_cb,
                        LV_EVENT_SCREEN_UNLOADED, NULL);
    s_alive = true;

#if defined(LV_REALDEVICE) || defined(LV_SIMSHIM)
    bms_battery_set_scan_cb(scan_cb);
#endif
    s_timer = lv_timer_create(update_cb, 250, NULL);
    update_cb(s_timer);

    lv_scr_load_anim(s_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

/* Make a dashboard label (the "84" or the "%" next to it) a way into the
 * battery screen. Called by the cockpit and generic themes on every build. */
static void pct_clicked_cb(lv_event_t *e)
{
    (void)e;
    show_bms_settings();
}

void bms_screen_attach_percent(lv_obj_t *obj)
{
    if (!obj) return;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(obj, 8);
    lv_obj_add_event_cb(obj, pct_clicked_cb, LV_EVENT_CLICKED, NULL);
}
