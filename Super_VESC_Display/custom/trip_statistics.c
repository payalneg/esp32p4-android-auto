/*
 * Trip statistics window (show_trips_statistics).
 *
 * A self-contained LVGL screen — created on demand, destroyed on unload, like
 * the VESC Tool menu and the logs/QR screens — that reads the raw trip log
 * (components/trip_log) and shows:
 *   - a scrollable table of trips (newest first: #, distance, time, avg, Wh),
 *   - a per-trip detail panel (summary + one chart with a metric selector:
 *     speed / power / voltage / temperatures / remaining capacity).
 *
 * The flash scan is heavy, so on the device it runs on a short-lived worker
 * task and the result is marshalled back to the LVGL thread via lv_async_call;
 * an s_alive guard prevents a late result from touching a destroyed screen.
 *
 * The desktop simulator has no flash (trip_log is not compiled there), so it
 * fills the same UI from a synthetic dataset — the window stays demoable.
 */
#include "lvgl.h"
#include "custom.h"
#include "settings_wrapper.h"   /* km/miles toggle + unit conversion helpers */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

extern lv_ui guider_ui;

/* ---- palette (matches the rest of the UI) ---- */
#define COL_BG     0x07090A
#define COL_PANEL  0x12181C
#define COL_BTN    0x2a3440
#define COL_ACCENT 0xB6FF2E   /* lime  */
#define COL_CYAN   0x00a9ff
#define COL_ORANGE 0xffa500
#define COL_RED    0xFF3B30
#define COL_TEXT   0xFFFFFF
#define COL_DIM    0x8A9499

#define MAX_TRIPS_UI 50
#define MAX_SERIES   150

#ifdef LV_REALDEVICE
#  include "trip_log.h"                 /* trip_summary_t / trip_sample_t + reader */
#  include "vesc_trip_persist.h"        /* live current-trip totals */
#  include "freertos/FreeRTOS.h"
#  include "freertos/task.h"
#else
/* Mirror the reader structs so the shared UI code compiles in the simulator. */
typedef struct {
    uint32_t trip_id, duration_s, distance_m, sample_count;
    float    ah, wh;
    uint16_t avg_speed_dkmh, max_speed_dkmh, min_voltage_dv;
    bool     is_current;
} trip_summary_t;

typedef struct {
    uint32_t t_s;
    int16_t  speed_dkmh, power_w, temp_motor_dc, temp_fet_dc;
    uint16_t voltage_dv;
    uint8_t  batt_pct;
} trip_sample_t;
#endif

/* ---- screen state ---- */
static lv_obj_t *s_screen;
static lv_obj_t *s_title;
static lv_obj_t *s_back_btn;
static lv_obj_t *s_list_view, *s_detail_view;
static lv_obj_t *s_table;
static lv_obj_t *s_totals;
static lv_obj_t *s_empty_lbl;
static lv_obj_t *s_summary;
static lv_obj_t *s_metric_btns[5];   /* metric selector buttons */
static lv_obj_t *s_chart, *s_chart_title;
static lv_obj_t *s_ylbl[5];          /* custom Y-axis value labels */
static lv_obj_t *s_xlbl[5];          /* custom X-axis time labels (M:SS) */
static lv_obj_t *s_btn_xp, *s_btn_xm; /* X (time) zoom +/- */
static lv_chart_series_t *s_ser_a, *s_ser_b;
static lv_obj_t *s_spinner_modal;
static lv_obj_t *s_delete_btn;       /* detail view: hide this trip */
static lv_obj_t *s_confirm_modal;    /* delete confirmation overlay */
static lv_timer_t *s_live_timer;
static bool      s_alive;
static bool      s_busy;             /* a scan is in flight */
static int       s_metric;           /* 0..4 */
static float     s_xzoom = 1.0f;          /* X (time) magnification, 1 = whole trip */
static float     s_xoff;                  /* X window start, fraction 0..1 */

/* data buffers (filled by the worker / synthetic generator) */
static trip_summary_t s_trips[MAX_TRIPS_UI];
static int            s_trip_count;
static trip_sample_t  s_series[MAX_SERIES];
static int            s_series_count;
static int            s_detail_idx;       /* row index into s_trips for the open detail */
static uint32_t       s_detail_trip_id;

static void request_trips(void);
static void request_series(uint32_t trip_id);
static void populate_list(void);
static void refresh_chart(int metric);
static void style_metric_btn(lv_obj_t *b, bool sel);

/* ===================== helpers ===================== */

static void fmt_dur(uint32_t secs, char *buf, size_t n)
{
    uint32_t h = secs / 3600u, m = (secs / 60u) % 60u, s = secs % 60u;
    if (h > 0) snprintf(buf, n, "%uh%02u", (unsigned)h, (unsigned)m);
    else       snprintf(buf, n, "%u:%02u", (unsigned)m, (unsigned)s);
}

static void show_spinner(const char *text)
{
    if (s_spinner_modal) return;
    s_spinner_modal = lv_obj_create(s_screen);
    lv_obj_set_size(s_spinner_modal, 800, 480);
    lv_obj_set_pos(s_spinner_modal, 0, 0);
    lv_obj_set_style_bg_color(s_spinner_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_spinner_modal, LV_OPA_60, 0);
    lv_obj_set_style_border_width(s_spinner_modal, 0, 0);
    lv_obj_clear_flag(s_spinner_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_spinner_modal, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *sp = lv_spinner_create(s_spinner_modal, 1000, 60);
    lv_obj_set_size(sp, 80, 80);
    lv_obj_align(sp, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_arc_color(sp, lv_color_hex(COL_CYAN), LV_PART_INDICATOR);

    lv_obj_t *l = lv_label_create(s_spinner_modal);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, 50);
}

static void hide_spinner(void)
{
    if (s_spinner_modal) { lv_obj_del(s_spinner_modal); s_spinner_modal = NULL; }
}

/* ===================== list ===================== */

static void populate_list(void)
{
    if (!s_table) return;

    if (s_trip_count <= 0) {
        lv_obj_add_flag(s_table, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_totals, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_empty_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(s_table, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_totals, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_empty_lbl, LV_OBJ_FLAG_HIDDEN);

    lv_table_set_row_cnt(s_table, s_trip_count + 1);   /* +1 header row */
    lv_table_set_cell_value(s_table, 0, 0, "#");
    lv_table_set_cell_value(s_table, 0, 1, "Dist");
    lv_table_set_cell_value(s_table, 0, 2, "Time");
    lv_table_set_cell_value(s_table, 0, 3, "Avg");
    lv_table_set_cell_value(s_table, 0, 4, "Wh");

    double sum_km = 0, sum_wh = 0;
    uint32_t sum_s = 0;
    char b[24];
    for (int i = 0; i < s_trip_count; i++) {
        trip_summary_t *t = &s_trips[i];
        int row = i + 1;
        if (t->is_current) snprintf(b, sizeof b, "%u*", (unsigned)t->trip_id);
        else               snprintf(b, sizeof b, "%u",  (unsigned)t->trip_id);
        lv_table_set_cell_value(s_table, row, 0, b);
        snprintf(b, sizeof b, "%.1f", settings_wrapper_dist_to_display(t->distance_m / 1000.0)); lv_table_set_cell_value(s_table, row, 1, b);
        fmt_dur(t->duration_s, b, sizeof b);                   lv_table_set_cell_value(s_table, row, 2, b);
        snprintf(b, sizeof b, "%.0f", settings_wrapper_speed_to_display(t->avg_speed_dkmh / 10.0)); lv_table_set_cell_value(s_table, row, 3, b);
        snprintf(b, sizeof b, "%.0f", t->wh);                  lv_table_set_cell_value(s_table, row, 4, b);
        sum_km += t->distance_m / 1000.0;
        sum_wh += t->wh;
        sum_s  += t->duration_s;
    }
    char tot[64];
    char dur[16]; fmt_dur(sum_s, dur, sizeof dur);
    const char *du = settings_wrapper_get_use_imperial() ? "mi" : "km";
    snprintf(tot, sizeof tot, "Total: %.1f %s  |  %s  |  %.2f kWh",
             settings_wrapper_dist_to_display(sum_km), du, dur, sum_wh / 1000.0);
    lv_label_set_text(s_totals, tot);
}

/* ===================== detail ===================== */

static void open_detail(int idx)
{
    if (idx < 0 || idx >= s_trip_count) return;
    s_detail_idx = idx;
    trip_summary_t *t = &s_trips[idx];

    lv_label_set_text_fmt(s_title, "Trip #%u", (unsigned)t->trip_id);

    char dur[16]; fmt_dur(t->duration_s, dur, sizeof dur);
    double disp_dist = settings_wrapper_dist_to_display(t->distance_m / 1000.0);
    /* Wh per display-distance (Wh/km or Wh/mi). */
    double wh_per_dist = (disp_dist > 0.0) ? (t->wh / disp_dist) : 0.0;
    const char *du = settings_wrapper_get_use_imperial() ? "mi"  : "km";
    const char *su = settings_wrapper_get_use_imperial() ? "mph" : "km/h";
    lv_label_set_text_fmt(s_summary,
        "%.1f %s  |  %s\n"
        "avg %.0f %s    max %.0f %s\n"
        "%.0f Wh  |  %.2f Ah\n"
        "%.1f Wh/%s    min %.1f V",
        disp_dist, du, dur,
        settings_wrapper_speed_to_display(t->avg_speed_dkmh / 10.0), su,
        settings_wrapper_speed_to_display(t->max_speed_dkmh / 10.0), su,
        t->wh, t->ah, wh_per_dist, du, t->min_voltage_dv / 10.0);

    lv_obj_add_flag(s_list_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_detail_view, LV_OBJ_FLAG_HIDDEN);

    /* The live trip is still being written — don't offer to hide it. */
    if (s_delete_btn) {
        if (t->is_current) lv_obj_add_flag(s_delete_btn, LV_OBJ_FLAG_HIDDEN);
        else               lv_obj_clear_flag(s_delete_btn, LV_OBJ_FLAG_HIDDEN);
    }

    s_metric = 0;
    for (int i = 0; i < 5; i++) style_metric_btn(s_metric_btns[i], i == 0);

    request_series(t->trip_id);   /* fills s_series → on_series_ready → refresh_chart */
}

static void show_list_view(void)
{
    lv_label_set_text(s_title, "Trip Statistics");
    lv_obj_add_flag(s_detail_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_list_view, LV_OBJ_FLAG_HIDDEN);
}

/* Map sample i to the display value(s) of the current metric (temp → two). */
static void sample_metric(int i, int32_t *va, int32_t *vb)
{
    *vb = 0;
    switch (s_metric) {
    case 0: *va = (int32_t)settings_wrapper_speed_to_display(s_series[i].speed_dkmh / 10.0f); break;
    case 1: *va = s_series[i].power_w;         break;
    case 2: *va = s_series[i].voltage_dv / 10; break;
    case 3: *va = (int32_t)settings_wrapper_temp_to_display(s_series[i].temp_motor_dc / 10.0f);
            *vb = (int32_t)settings_wrapper_temp_to_display(s_series[i].temp_fet_dc / 10.0f); break;
    default: *va = s_series[i].batt_pct;       break;
    }
}

/* Y axis auto-fits the WHOLE trip and stays fixed while zooming X. Sets the
 * chart range and the gutter value labels (top = hi, bottom = lo). */
static void set_y_axis(void)
{
    int32_t mn = INT32_MAX, mx = INT32_MIN;
    for (int i = 0; i < s_series_count; i++) {
        int32_t va, vb;
        sample_metric(i, &va, &vb);
        if (va < mn) mn = va;
        if (va > mx) mx = va;
        if (s_metric == 3) {
            if (vb < mn) mn = vb;
            if (vb > mx) mx = vb;
        }
    }
    if (s_series_count == 0) { mn = 0; mx = 1; }
    if (mn == mx) { mn -= 1; mx += 1; }
    int32_t pad = (mx - mn) / 10; if (pad < 1) pad = 1;
    int32_t lo = mn - pad, hi = mx + pad;
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, lo, hi);
    for (int i = 0; i < 5; i++) {
        int32_t v = hi - (int32_t)(((int64_t)(hi - lo) * i) / 4);
        lv_label_set_text_fmt(s_ylbl[i], "%d", (int)v);
    }
}

/* Plot the current X (time) window — a sub-range of the samples chosen by the
 * X zoom (s_xzoom) and pan offset (s_xoff). Zoom 1 shows the whole trip. */
static void plot_chart(void)
{
    if (!s_chart) return;
    bool two = (s_metric == 3);
    lv_chart_hide_series(s_chart, s_ser_b, !two);

    int total = s_series_count;
    if (total < 1) { lv_chart_set_point_count(s_chart, 1); lv_chart_refresh(s_chart); return; }

    if (s_xzoom < 1.0f) s_xzoom = 1.0f;
    int win = (int)((float)total / s_xzoom + 0.5f);
    if (win < 2) win = 2;
    if (win > total) win = total;
    int maxstart = total - win;
    if (s_xoff < 0.0f) s_xoff = 0.0f;
    if (s_xoff > 1.0f) s_xoff = 1.0f;
    int start = (int)(s_xoff * maxstart + 0.5f);
    if (start < 0) start = 0;
    if (start > maxstart) start = maxstart;

    lv_chart_set_point_count(s_chart, win);
    for (int j = 0; j < win; j++) {
        int32_t va, vb;
        sample_metric(start + j, &va, &vb);
        lv_chart_set_value_by_id(s_chart, s_ser_a, j, va);
        if (two) lv_chart_set_value_by_id(s_chart, s_ser_b, j, vb);
    }

    /* X-axis time labels for the visible window (trip time at each gridline). */
    uint32_t t0 = s_series[start].t_s;
    uint32_t t1 = s_series[start + win - 1].t_s;
    for (int i = 0; i < 5; i++) {
        uint32_t tv = t0 + (uint32_t)(((uint64_t)(t1 - t0) * i) / 4);
        char b[16];
        fmt_dur(tv, b, sizeof b);
        lv_label_set_text(s_xlbl[i], b);
    }

    lv_chart_refresh(s_chart);
}

static void refresh_chart(int metric)
{
    if (!s_chart) return;
    s_metric = metric;
    s_xzoom = 1.0f;     /* reset the X view for the new metric */
    s_xoff = 0.0f;

    const char *t;
    switch (metric) {
    case 0: t = settings_wrapper_get_use_imperial() ? "Speed (mph)" : "Speed (km/h)"; break;
    case 1: t = "Power (W)";               break;
    case 2: t = "Voltage (V)";             break;
    case 3: t = settings_wrapper_get_use_fahrenheit() ? "Temp (F)  motor / FET"
                                                      : "Temp (C)  motor / FET"; break;
    default: t = "Battery (%)";            break;
    }
    lv_label_set_text(s_chart_title, t);

    set_y_axis();
    plot_chart();
}

/* ===================== async hand-off ===================== */

static void on_trips_ready(void *p)
{
    (void)p;
    if (!s_alive) return;
    s_busy = false;
    hide_spinner();
    populate_list();
}

static void on_series_ready(void *p)
{
    (void)p;
    if (!s_alive) return;
    s_busy = false;
    hide_spinner();
    refresh_chart(0);
}

/* ===================== events ===================== */

static void table_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    if (s_busy) return;
    uint16_t row, col;
    lv_table_get_selected_cell(s_table, &row, &col);
    if (row == LV_TABLE_CELL_NONE || row == 0) return;   /* header / none */
    open_detail((int)row - 1);
}

static void style_metric_btn(lv_obj_t *b, bool sel)
{
    lv_obj_set_style_bg_color(b, lv_color_hex(sel ? COL_ACCENT : COL_BTN), 0);
    lv_obj_set_style_text_color(b, lv_color_hex(sel ? COL_BG : COL_TEXT), 0);
}

static void set_metric(int m)
{
    for (int i = 0; i < 5; i++) style_metric_btn(s_metric_btns[i], i == m);
    refresh_chart(m);
}

static void metric_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    set_metric((int)(intptr_t)lv_event_get_user_data(e));
}

static void xzoom_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if ((intptr_t)lv_event_get_user_data(e) > 0) s_xzoom *= 1.5f;
    else                                         s_xzoom /= 1.5f;
    if (s_xzoom < 1.0f) s_xzoom = 1.0f;
    float maxz = (s_series_count >= 4) ? (float)s_series_count / 2.0f : 1.0f;
    if (s_xzoom > maxz) s_xzoom = maxz;
    plot_chart();
}

/* Drag left/right on the chart to pan the time window (only when zoomed in). */
static void chart_press_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_PRESSING) return;
    if (s_xzoom <= 1.0f) return;
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_point_t v;
    lv_indev_get_vect(indev, &v);
    if (v.x == 0) return;
    int total = s_series_count;
    int win = (int)((float)total / s_xzoom + 0.5f);
    if (win < 2) win = 2;
    if (win > total) win = total;
    int maxstart = total - win;
    if (maxstart < 1) return;
    /* the ~712 px plot shows `win` samples; drag right → earlier time. */
    float dstart = (float)(-v.x) * (float)win / 712.0f;
    s_xoff += dstart / (float)maxstart;
    if (s_xoff < 0.0f) s_xoff = 0.0f;
    if (s_xoff > 1.0f) s_xoff = 1.0f;
    plot_chart();
}

static void back_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (s_busy) return;
    if (!lv_obj_has_flag(s_detail_view, LV_OBJ_FLAG_HIDDEN)) {
        show_list_view();   /* detail → list */
        return;
    }
    lv_scr_load_anim(guider_ui.dashboard, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

static void screen_unloaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    s_alive = false;
    if (s_live_timer) { lv_timer_del(s_live_timer); s_live_timer = NULL; }
    if (s_screen) { lv_obj_del_async(s_screen); s_screen = NULL; }
    s_spinner_modal = NULL;
    s_delete_btn = NULL;
    s_confirm_modal = NULL;
    s_table = s_totals = s_empty_lbl = NULL;
    s_summary = s_chart = s_chart_title = s_btn_xp = s_btn_xm = NULL;
    for (int i = 0; i < 5; i++) { s_metric_btns[i] = NULL; s_ylbl[i] = NULL; s_xlbl[i] = NULL; }
    s_list_view = s_detail_view = s_title = s_back_btn = NULL;
    s_busy = false;
}

/* ===================== delete trip ===================== */

static void confirm_no_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (s_confirm_modal) { lv_obj_del(s_confirm_modal); s_confirm_modal = NULL; }
}

static void confirm_yes_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    uint32_t id = s_detail_trip_id;
    if (s_confirm_modal) { lv_obj_del(s_confirm_modal); s_confirm_modal = NULL; }

#ifdef LV_REALDEVICE
    /* Persist the hide so it survives reboot and stops counting toward the
     * MAX_TRIPS window. Simulator has no flash log — just drop it in memory. */
    trip_log_delete_trip(id);
#endif
    /* Remove from the in-memory list for instant feedback. A re-open of the
     * window re-scans flash (device), which now also surfaces an older trip. */
    int w = 0;
    for (int i = 0; i < s_trip_count; i++)
        if (s_trips[i].trip_id != id) s_trips[w++] = s_trips[i];
    s_trip_count = w;

    show_list_view();
    populate_list();
}

static void show_confirm_delete(void)
{
    if (s_confirm_modal) return;
    s_confirm_modal = lv_obj_create(s_screen);
    lv_obj_set_size(s_confirm_modal, 800, 480);
    lv_obj_set_pos(s_confirm_modal, 0, 0);
    lv_obj_set_style_bg_color(s_confirm_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_confirm_modal, LV_OPA_70, 0);
    lv_obj_set_style_border_width(s_confirm_modal, 0, 0);
    lv_obj_clear_flag(s_confirm_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_confirm_modal, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *box = lv_obj_create(s_confirm_modal);
    lv_obj_set_size(box, 460, 200);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_radius(box, 10, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *l = lv_label_create(box);
    lv_label_set_text_fmt(l, "Delete trip #%u?", (unsigned)s_detail_trip_id);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t *yes = lv_btn_create(box);
    lv_obj_set_size(yes, 180, 60);
    lv_obj_align(yes, LV_ALIGN_BOTTOM_LEFT, 6, -6);
    lv_obj_set_style_bg_color(yes, lv_color_hex(COL_RED), 0);
    lv_obj_set_style_radius(yes, 8, 0);
    lv_obj_set_style_border_width(yes, 0, 0);
    { lv_obj_t *bl = lv_label_create(yes); lv_label_set_text(bl, "Delete");
      lv_obj_set_style_text_font(bl, &lv_font_montserrat_24, 0); lv_obj_center(bl); }
    lv_obj_add_event_cb(yes, confirm_yes_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *no = lv_btn_create(box);
    lv_obj_set_size(no, 180, 60);
    lv_obj_align(no, LV_ALIGN_BOTTOM_RIGHT, -6, -6);
    lv_obj_set_style_bg_color(no, lv_color_hex(COL_BTN), 0);
    lv_obj_set_style_radius(no, 8, 0);
    lv_obj_set_style_border_width(no, 0, 0);
    { lv_obj_t *bl = lv_label_create(no); lv_label_set_text(bl, "Cancel");
      lv_obj_set_style_text_font(bl, &lv_font_montserrat_24, 0); lv_obj_center(bl); }
    lv_obj_add_event_cb(no, confirm_no_cb, LV_EVENT_CLICKED, NULL);
}

static void delete_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (s_busy) return;
    show_confirm_delete();
}

/* ===================== build ===================== */

static const char *s_metric_names[5] = { "Speed", "Power", "Voltage", "Temp", "Batt" };

static void build_screen(void)
{
    s_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_screen, 800, 480);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_screen, 255, 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* header */
    s_title = lv_label_create(s_screen);
    lv_label_set_text(s_title, "Trip Statistics");
    lv_obj_set_pos(s_title, 16, 14);
    lv_obj_set_style_text_color(s_title, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_24, 0);

    s_back_btn = lv_btn_create(s_screen);
    lv_obj_set_pos(s_back_btn, 696, 8);
    lv_obj_set_size(s_back_btn, 92, 40);
    lv_obj_set_style_bg_color(s_back_btn, lv_color_hex(COL_BTN), 0);
    lv_obj_set_style_radius(s_back_btn, 6, 0);
    lv_obj_set_style_border_width(s_back_btn, 0, 0);
    lv_obj_t *bl = lv_label_create(s_back_btn);
    lv_label_set_text(bl, "Back");
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_24, 0);
    lv_obj_center(bl);
    lv_obj_add_event_cb(s_back_btn, back_cb, LV_EVENT_CLICKED, NULL);

    /* ---- list view ---- */
    s_list_view = lv_obj_create(s_screen);
    lv_obj_set_pos(s_list_view, 0, 58);
    lv_obj_set_size(s_list_view, 800, 422);
    lv_obj_set_style_bg_opa(s_list_view, 0, 0);
    lv_obj_set_style_border_width(s_list_view, 0, 0);
    lv_obj_set_style_pad_all(s_list_view, 0, 0);
    lv_obj_clear_flag(s_list_view, LV_OBJ_FLAG_SCROLLABLE);

    s_table = lv_table_create(s_list_view);
    lv_obj_set_pos(s_table, 8, 0);
    lv_obj_set_size(s_table, 784, 384);
    lv_table_set_col_cnt(s_table, 5);
    lv_table_set_col_width(s_table, 0, 90);
    lv_table_set_col_width(s_table, 1, 180);
    lv_table_set_col_width(s_table, 2, 160);
    lv_table_set_col_width(s_table, 3, 150);
    lv_table_set_col_width(s_table, 4, 170);
    lv_obj_set_style_bg_color(s_table, lv_color_hex(COL_PANEL), LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_table, lv_color_hex(COL_TEXT), LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_table, &lv_font_montserratMedium_16, LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_table, lv_color_hex(COL_BTN), LV_PART_ITEMS);
    lv_obj_set_style_pad_top(s_table, 10, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(s_table, 10, LV_PART_ITEMS);
    lv_obj_set_style_pad_left(s_table, 8, LV_PART_ITEMS);
    /* table MAIN bg defaults to white in the theme — match the screen so the
     * area below the last row (and the slack past the last column) isn't white. */
    lv_obj_set_style_bg_color(s_table, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_table, 255, 0);
    lv_obj_set_style_border_width(s_table, 0, 0);
    lv_obj_set_style_pad_all(s_table, 0, 0);
    lv_obj_add_event_cb(s_table, table_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_totals = lv_label_create(s_list_view);
    lv_obj_set_pos(s_totals, 12, 392);
    lv_obj_set_style_text_color(s_totals, lv_color_hex(COL_DIM), 0);
    lv_obj_set_style_text_font(s_totals, &lv_font_montserratMedium_16, 0);
    lv_label_set_text(s_totals, "");

    s_empty_lbl = lv_label_create(s_list_view);
    lv_label_set_text(s_empty_lbl, "No trips recorded yet.");
    lv_obj_set_style_text_color(s_empty_lbl, lv_color_hex(COL_DIM), 0);
    lv_obj_set_style_text_font(s_empty_lbl, &lv_font_montserrat_24, 0);
    lv_obj_align(s_empty_lbl, LV_ALIGN_CENTER, 0, -40);
    lv_obj_add_flag(s_empty_lbl, LV_OBJ_FLAG_HIDDEN);

    /* ---- detail view ---- */
    s_detail_view = lv_obj_create(s_screen);
    lv_obj_set_pos(s_detail_view, 0, 58);
    lv_obj_set_size(s_detail_view, 800, 422);
    lv_obj_set_style_bg_opa(s_detail_view, 0, 0);
    lv_obj_set_style_border_width(s_detail_view, 0, 0);
    lv_obj_set_style_pad_all(s_detail_view, 0, 0);
    lv_obj_clear_flag(s_detail_view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_detail_view, LV_OBJ_FLAG_HIDDEN);

    s_summary = lv_label_create(s_detail_view);
    lv_obj_set_pos(s_summary, 16, 4);
    lv_obj_set_style_text_color(s_summary, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(s_summary, &lv_font_montserratMedium_16, 0);
    lv_label_set_text(s_summary, "");

    /* Delete-trip button — top-right of the detail view, clear of the summary
     * text on the left. Hidden for the live trip (see open_detail). */
    s_delete_btn = lv_btn_create(s_detail_view);
    lv_obj_set_pos(s_delete_btn, 624, 4);
    lv_obj_set_size(s_delete_btn, 160, 40);
    lv_obj_set_style_bg_color(s_delete_btn, lv_color_hex(COL_RED), 0);
    lv_obj_set_style_radius(s_delete_btn, 6, 0);
    lv_obj_set_style_border_width(s_delete_btn, 0, 0);
    { lv_obj_t *l = lv_label_create(s_delete_btn); lv_label_set_text(l, "Delete");
      lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0); lv_obj_center(l); }
    lv_obj_add_event_cb(s_delete_btn, delete_btn_cb, LV_EVENT_CLICKED, NULL);

    /* metric selector — five explicit buttons (a btnmatrix renders as thin
     * strips under this theme), selected one filled lime with dark text. */
    for (int i = 0; i < 5; i++) {
        lv_obj_t *b = lv_btn_create(s_detail_view);
        lv_obj_set_pos(b, 4 + i * 156, 92);
        lv_obj_set_size(b, 148, 40);
        lv_obj_set_style_radius(b, 6, 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_set_style_text_font(b, &lv_font_montserratMedium_16, 0);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, s_metric_names[i]);
        lv_obj_center(l);
        lv_obj_add_event_cb(b, metric_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s_metric_btns[i] = b;
        style_metric_btn(b, i == 0);
    }

    s_chart_title = lv_label_create(s_detail_view);
    lv_obj_set_pos(s_chart_title, 16, 146);
    lv_obj_set_style_text_color(s_chart_title, lv_color_hex(COL_DIM), 0);
    lv_obj_set_style_text_font(s_chart_title, &lv_font_montserratMedium_16, 0);
    lv_label_set_text(s_chart_title, "");

    s_chart = lv_chart_create(s_detail_view);
    lv_obj_set_pos(s_chart, 48, 176);    /* shifted right → room for Y labels on the left */
    lv_obj_set_size(s_chart, 736, 224);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(s_chart, 3, 6);
    /* The built-in chart axis labels overflow the panel in this LVGL build, so
     * draw our own Y value labels in a reserved left gutter (pad_left) — fully
     * contained, updated in refresh_chart. */
    lv_obj_set_style_pad_top(s_chart, 10, 0);
    lv_obj_set_style_pad_bottom(s_chart, 10, 0);
    lv_obj_set_style_pad_right(s_chart, 10, 0);
    lv_obj_set_style_pad_left(s_chart, 8, 0);
    lv_obj_set_style_radius(s_chart, 6, 0);
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_width(s_chart, 0, 0);
    lv_obj_set_style_line_color(s_chart, lv_color_hex(COL_BTN), LV_PART_MAIN);
    lv_obj_set_style_width(s_chart, 0, LV_PART_INDICATOR);    /* no point markers */
    lv_obj_set_style_height(s_chart, 0, LV_PART_INDICATOR);
    s_ser_a = lv_chart_add_series(s_chart, lv_color_hex(COL_ACCENT), LV_CHART_AXIS_PRIMARY_Y);
    s_ser_b = lv_chart_add_series(s_chart, lv_color_hex(COL_ORANGE), LV_CHART_AXIS_PRIMARY_Y);

    /* Y-axis value labels to the LEFT of the chart panel (children of the detail
     * view, not the chart). The plot spans detail-view y 186..390 (chart y 176 +
     * pad_top 10, height 204); centre each label on its gridline. */
    for (int i = 0; i < 5; i++) {
        lv_obj_t *yl = lv_label_create(s_detail_view);
        lv_obj_set_width(yl, 42);
        lv_obj_set_pos(yl, 2, 186 + (204 * i) / 4 - 8);
        lv_obj_set_style_text_align(yl, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_color(yl, lv_color_hex(COL_DIM), 0);
        lv_obj_set_style_text_font(yl, &lv_font_montserratMedium_16, 0);
        lv_label_set_text(yl, "");
        s_ylbl[i] = yl;
    }

    /* X-axis time labels below the chart (trip time, M:SS), centred under the
     * plot gridlines. Updated in plot_chart for the visible time window. */
    for (int i = 0; i < 5; i++) {
        lv_obj_t *xl = lv_label_create(s_detail_view);
        lv_obj_set_width(xl, 52);
        lv_obj_set_pos(xl, 56 + (718 * i) / 4 - 26, 403);
        lv_obj_set_style_text_align(xl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(xl, lv_color_hex(COL_DIM), 0);
        lv_obj_set_style_text_font(xl, &lv_font_montserratMedium_16, 0);
        lv_label_set_text(xl, "");
        s_xlbl[i] = xl;
    }

    /* Drag the chart to pan the Y window when zoomed (not its own scroll). */
    lv_obj_add_flag(s_chart, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_chart, chart_press_cb, LV_EVENT_PRESSING, NULL);

    /* X (time) zoom buttons (top-right, on the chart-title row). */
    s_btn_xm = lv_btn_create(s_detail_view);
    lv_obj_set_pos(s_btn_xm, 686, 140);
    lv_obj_set_size(s_btn_xm, 48, 30);
    lv_obj_set_style_radius(s_btn_xm, 6, 0);
    lv_obj_set_style_border_width(s_btn_xm, 0, 0);
    lv_obj_set_style_bg_color(s_btn_xm, lv_color_hex(COL_BTN), 0);
    { lv_obj_t *l = lv_label_create(s_btn_xm); lv_label_set_text(l, "X-"); lv_obj_center(l); }
    lv_obj_add_event_cb(s_btn_xm, xzoom_cb, LV_EVENT_CLICKED, (void *)(intptr_t)-1);

    s_btn_xp = lv_btn_create(s_detail_view);
    lv_obj_set_pos(s_btn_xp, 738, 140);
    lv_obj_set_size(s_btn_xp, 48, 30);
    lv_obj_set_style_radius(s_btn_xp, 6, 0);
    lv_obj_set_style_border_width(s_btn_xp, 0, 0);
    lv_obj_set_style_bg_color(s_btn_xp, lv_color_hex(COL_BTN), 0);
    { lv_obj_t *l = lv_label_create(s_btn_xp); lv_label_set_text(l, "X+"); lv_obj_center(l); }
    lv_obj_add_event_cb(s_btn_xp, xzoom_cb, LV_EVENT_CLICKED, (void *)(intptr_t)1);

    lv_obj_add_event_cb(s_screen, screen_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
}

/* ===================== data source (device vs simulator) ===================== */

#ifdef LV_REALDEVICE

static void live_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_alive || !s_table) return;
    if (s_trip_count < 1 || !s_trips[0].is_current) return;
    /* The current trip is always row 1 (newest first). Refresh its live fields
     * from the running totals (the snapshot is up to 10 s stale). */
    float km = trip_persist_get_trip_km();
    uint32_t dur = trip_persist_get_uptime_ms() / 1000u;
    char b[24];
    snprintf(b, sizeof b, "%.1f", settings_wrapper_dist_to_display(km)); lv_table_set_cell_value(s_table, 1, 1, b);
    fmt_dur(dur, b, sizeof b);                          lv_table_set_cell_value(s_table, 1, 2, b);
    snprintf(b, sizeof b, "%.0f", settings_wrapper_speed_to_display(dur > 0 ? km / (dur / 3600.0f) : 0.0f));
    lv_table_set_cell_value(s_table, 1, 3, b);
}

static void trips_task(void *arg)
{
    (void)arg;
    s_trip_count = trip_log_list_trips(s_trips, MAX_TRIPS_UI);
    lv_async_call(on_trips_ready, NULL);
    vTaskDelete(NULL);
}

static void series_task(void *arg)
{
    (void)arg;
    s_series_count = trip_log_read_series(s_detail_trip_id, s_series, MAX_SERIES);
    lv_async_call(on_series_ready, NULL);
    vTaskDelete(NULL);
}

static void request_trips(void)
{
    s_busy = true;
    show_spinner("Reading trips...");
    if (xTaskCreate(trips_task, "trip_stat", 4096, NULL, 3, NULL) != pdPASS) {
        s_trip_count = 0; on_trips_ready(NULL);
    }
}

static void request_series(uint32_t trip_id)
{
    s_detail_trip_id = trip_id;
    s_busy = true;
    show_spinner("Reading trip...");
    if (xTaskCreate(series_task, "trip_ser", 4096, NULL, 3, NULL) != pdPASS) {
        s_series_count = 0; on_series_ready(NULL);
    }
}

static void start_live(void)
{
    if (!s_live_timer) s_live_timer = lv_timer_create(live_cb, 1000, NULL);
}

#else  /* !LV_REALDEVICE — desktop simulator: synthetic dataset */

static int synth_fill_trips(trip_summary_t *o, int max)
{
    static const struct { uint32_t id, dur, dist; float ah, wh; int avg, mx, minv; } demo[] = {
        { 12, 2290, 14200, 8.6f, 410.0f, 223, 412, 462 },
        { 11, 1263,  8000, 4.8f, 230.0f, 228, 410, 471 },
        { 10,  760,  5100, 4.2f, 150.0f, 241, 392, 458 },
        {  9,  550,  3200, 2.7f,  95.0f, 209, 381, 469 },
    };
    int n = (int)(sizeof demo / sizeof demo[0]);
    if (n > max) n = max;
    for (int i = 0; i < n; i++) {
        o[i].trip_id        = demo[i].id;
        o[i].duration_s     = demo[i].dur;
        o[i].distance_m     = demo[i].dist;
        o[i].sample_count   = demo[i].dur / 10;
        o[i].ah             = demo[i].ah;
        o[i].wh             = demo[i].wh;
        o[i].avg_speed_dkmh = (uint16_t)demo[i].avg;
        o[i].max_speed_dkmh = (uint16_t)demo[i].mx;
        o[i].min_voltage_dv = (uint16_t)demo[i].minv;
        o[i].is_current     = (i == 0);
    }
    return n;
}

static int synth_fill_series(uint32_t id, trip_sample_t *o, int max)
{
    int n = 90; if (n > max) n = max;
    float ph = (float)(id % 7);
    for (int i = 0; i < n; i++) {
        float x = (float)i;
        float spd = 22.0f + 12.0f * sinf(x / 6.0f + ph) + 4.0f * sinf(x / 2.0f);
        if (spd < 0) spd = 0;
        float cur = 6.0f + 9.0f * fabsf(sinf(x / 5.0f + ph));
        float volt = 50.0f - 0.05f * x - 1.5f * sinf(x / 5.0f + ph);
        o[i].t_s           = (uint32_t)(i * 15);
        o[i].speed_dkmh    = (int16_t)(spd * 10.0f);
        o[i].power_w       = (int16_t)(volt * cur);
        o[i].voltage_dv    = (uint16_t)(volt * 10.0f);
        o[i].temp_motor_dc = (int16_t)((35.0f + 0.15f * x) * 10.0f);
        o[i].temp_fet_dc   = (int16_t)((30.0f + 0.10f * x) * 10.0f);
        o[i].batt_pct      = (uint8_t)(95 - (i * 60) / n);
    }
    return n;
}

static void request_trips(void)
{
    s_trip_count = synth_fill_trips(s_trips, MAX_TRIPS_UI);
    on_trips_ready(NULL);
}

static void request_series(uint32_t trip_id)
{
    s_detail_trip_id = trip_id;
    s_series_count = synth_fill_series(trip_id, s_series, MAX_SERIES);
    on_series_ready(NULL);
}

static void start_live(void) { }   /* no live totals in the simulator */

#endif /* LV_REALDEVICE */

/* ===================== entry ===================== */

void show_trips_statistics(void)
{
    if (s_screen) return;   /* re-entrancy guard */

    s_alive = false;
    s_busy = false;
    s_metric = 0;
    s_trip_count = 0;
    s_series_count = 0;
    s_detail_idx = 0;
    s_spinner_modal = NULL;
    s_live_timer = NULL;

    build_screen();
    s_alive = true;

    request_trips();   /* device: spinner + worker task; simulator: synthetic now */
    start_live();

    lv_scr_load_anim(s_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}
