/*
    Copyright 2026 Super VESC Display

    Generic dashboard renderer — see theme_generic.h for the widget naming
    convention and the build-time discovery mechanism.

    The ops here are screen-agnostic: they read from whatever dashboard_widgets_t
    the active generated theme installed via dashboard_generic_set_active(), and
    skip any NULL widget. Rendering mirrors the cockpit/amber themes (formats,
    unit conversion, bar fill direction) so a screen cloned from one of those in
    GUI Guider behaves the same with zero hand-written C. Colours fall back to a
    neutral accent — a screen that wants a bespoke palette/segment-gradient
    treatment writes its own theme module instead.

    No dependency on guider_ui or any specific screen, so it compiles unchanged
    in the desktop simulator alongside the firmware.
*/

#include "theme_generic.h"
#include "settings_wrapper.h"

#include <stdio.h>
#include <math.h>

/* Neutral palette (amber-leaning default). */
#define GEN_SEG_OFF   lv_color_hex(0x202020)
#define GEN_ACCENT    lv_color_hex(0xFF7A1A)
#define GEN_REGEN     lv_color_hex(0x2EB6FF)
#define GEN_BATT_OK   lv_color_hex(0xFFB35C)
#define GEN_BATT_WARN lv_color_hex(0xFFB02E)
#define GEN_BATT_CRIT lv_color_hex(0xFF3B2F)
#define GEN_BT_ON     lv_color_hex(0xFFB35C)
#define GEN_BT_OFF    lv_color_hex(0x6B4A2C)
#define GEN_WARN      lv_color_hex(0xE70023)
#define GEN_WARN_BG   lv_color_hex(0x2A0007)

/* Authored (single-head) geometry of one temperature value label plus its unit
 * label. Captured lazily on the first render, before anything is moved: a
 * dual-head reading ("34/37") is wider than the box GUI Guider sized for a
 * single number, so the box has to grow — and shrink back if the second head
 * goes away. Reading it off the live widgets keeps the design in charge of the
 * layout; nothing is hard-coded per screen. */
typedef struct {
    bool       captured;
    lv_coord_t x, w;        /* value label */
    lv_coord_t unit_x;      /* its "°C" label, ignored when the screen has none */
} temp_geom_t;

static const dashboard_widgets_t *s_w;
static float s_last_current_a;
static float s_last_voltage_v;
static temp_geom_t s_geom_fet;
static temp_geom_t s_geom_mot;
/* Dead-link banner state (see g_esc_connection_status). */
static lv_obj_t *s_warn_auto;
static bool      s_warn_connected;
static bool      s_warn_shown;
static uint32_t  s_warn_blink_ms;

void dashboard_generic_set_active(const dashboard_widgets_t *w)
{
    s_w = w;
    s_last_current_a = 0.0f;
    s_last_voltage_v = 0.0f;
    /* Every screen brings its own widgets: drop the captured geometry and the
     * auto-created banner (a child of the outgoing screen, freed with it by the
     * switcher right after this runs). */
    s_geom_fet = (temp_geom_t){0};
    s_geom_mot = (temp_geom_t){0};
    s_warn_auto      = NULL;
    s_warn_connected = true;
    s_warn_shown     = false;
    s_warn_blink_ms  = 0;

    /* Phantom-touch hardening for the invisible brightness drag slider — the
     * same fix the cockpit theme carries in custom.c. GUI Guider styles the
     * slider transparent only in LV_STATE_DEFAULT, so a vibration-induced
     * phantom press paints LVGL's default-theme blue PRESSED/FOCUSED fill
     * across the whole slider area. Zero the overlay in every state; the
     * slider stays touch-active. Also seed it from the saved brightness so
     * the first (real or phantom) touch doesn't jump the backlight to the
     * GUI Guider default of 50. */
    if (w && w->brightness_slider) {
        lv_obj_t *sl = w->brightness_slider;
        lv_slider_set_value(sl, settings_wrapper_get_brightness(), LV_ANIM_OFF);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_MAIN      | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_INDICATOR | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_KNOB      | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_MAIN      | LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_INDICATOR | LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(sl, 0, LV_PART_KNOB      | LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(sl, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_outline_width(sl, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_set_style_shadow_width(sl, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(sl, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    }
}

/* ---- helpers ---- */

static lv_color_t batt_color(int pct)
{
    if (pct > 50) return GEN_BATT_OK;
    if (pct > 20) return GEN_BATT_WARN;
    return GEN_BATT_CRIT;
}

/* Vertical bar: segs[0] is the top cell, fill bottom→top (battery / power). */
static void paint_v_bar(lv_obj_t *const *segs, int count, int filled, lv_color_t on)
{
    if (filled < 0) filled = 0;
    if (filled > count) filled = count;
    for (int i = 0; i < count; i++) {
        if (!segs[i]) continue;
        bool lit = (count - 1 - i) < filled;
        dash_set_bg_color(segs[i], lit ? on : GEN_SEG_OFF, LV_PART_MAIN);
    }
}

/* lv_bar, clamped to the range the screen was authored with. Skips the write
 * when nothing changed — lv_bar_set_value invalidates unconditionally. */
static void bar_set(lv_obj_t *bar, int32_t v)
{
    if (!bar) return;
    int32_t lo = lv_bar_get_min_value(bar);
    int32_t hi = lv_bar_get_max_value(bar);
    if (v < lo) v = lo; else if (v > hi) v = hi;
    if (lv_bar_get_value(bar) == v) return;
    lv_bar_set_value(bar, v, LV_ANIM_OFF);
}

/* Horizontal bar: seg_00 lights first (speed). */
static void paint_h_bar(lv_obj_t *const *segs, int count, int filled, lv_color_t on)
{
    if (filled < 0) filled = 0;
    if (filled > count) filled = count;
    for (int i = 0; i < count; i++) {
        if (!segs[i]) continue;
        dash_set_bg_color(segs[i], (i < filled) ? on : GEN_SEG_OFF, LV_PART_MAIN);
    }
}

static void render_power(void)
{
    if (!s_w) return;
    /* The configured power ceiling (a setting, not telemetry). Refreshed from
     * here rather than on a dedicated hook: dash_label_set short-circuits when
     * the text is unchanged, so this costs nothing on the hot path, and a
     * Classic-derived screen would otherwise show its "-.- KW" placeholder
     * forever. */
    if (s_w->power_max_val) {
        char text[16];
        snprintf(text, sizeof(text), "%.1f KW", settings_wrapper_get_power_max_kw());
        dash_label_set(s_w->power_max_val, text);
    }
    float power_kw = s_last_current_a * s_last_voltage_v / 1000.0f;
    lv_color_t color = (power_kw < 0.0f) ? GEN_REGEN : GEN_ACCENT;
    if (s_w->power_value) {
        char text[16];
        snprintf(text, sizeof(text), "%.1f", power_kw);
        dash_label_set(s_w->power_value, text);
        dash_set_text_color(s_w->power_value, color, LV_PART_MAIN);
    }
    float pmax = settings_wrapper_get_power_max_kw();
    if (pmax <= 0.0f) pmax = 4.5f;
    float ratio = fabsf(power_kw) / pmax;
    if (ratio > 1.0f) ratio = 1.0f;
    if (s_w->power_seg_n > 0) {
        paint_v_bar(s_w->power_seg, s_w->power_seg_n,
                    (int)(ratio * s_w->power_seg_n + 0.5f), color);
    }
    if (s_w->power_bar) {
        /* Signed fraction of the bar's own span. On a symmetrical bar authored
         * with a negative minimum this reads as regen (left) / drive (right);
         * on a plain 0..N bar the clamp in bar_set() pins regen at zero. */
        int32_t hi = lv_bar_get_max_value(s_w->power_bar);
        float signed_ratio = (power_kw < 0.0f) ? -ratio : ratio;
        bar_set(s_w->power_bar, (int32_t)(signed_ratio * hi + (signed_ratio < 0 ? -0.5f : 0.5f)));
        dash_set_bg_color(s_w->power_bar, color, LV_PART_INDICATOR);
    }
}

/* ---- ops ---- */

static void g_speed(float speed)
{
    if (!s_w) return;
    int disp = (int)settings_wrapper_speed_to_display(speed);
    if (disp < 0) disp = 0; else if (disp > 999) disp = 999;
    if (s_w->speed_text) {
        char text[10];
        snprintf(text, sizeof(text), "%02d", disp);
        dash_label_set(s_w->speed_text, text);
    }
    if (s_w->speed_seg_n > 0) {
        int smax = 60;
        int filled = ((int)speed * s_w->speed_seg_n + smax / 2) / smax;
        paint_h_bar(s_w->speed_seg, s_w->speed_seg_n, filled, GEN_ACCENT);
    }
    /* Sweep the gauge with the DISPLAYED speed, so the arc and the digits agree
     * after a km/h <-> mph switch. The scale's authored range is the full-scale
     * value; lv_meter clamps nothing, hence the explicit clamp. */
    if (s_w->speed_arc && s_w->speed_meter && s_w->speed_scale) {
        int32_t lo = s_w->speed_scale->min;
        int32_t hi = s_w->speed_scale->max;
        int32_t v = disp;
        if (v < lo) v = lo; else if (v > hi) v = hi;
        lv_meter_set_indicator_end_value(s_w->speed_meter, s_w->speed_arc, v);
    }
}

static void g_current(float current)
{
    if (!s_w) return;
    s_last_current_a = current;
    if (s_w->current_text) {
        char text[12];
        snprintf(text, sizeof(text), "%.1f A", current);
        dash_label_set(s_w->current_text, text);
    }
    render_power();
}

static void g_battery_voltage(float v)
{
    if (!s_w) return;
    s_last_voltage_v = v;
    if (s_w->voltage_text) {
        char text[10];
        snprintf(text, sizeof(text), "%.1f", v);
        dash_label_set(s_w->voltage_text, text);
    }
    render_power();
}

static void g_battery_proc(float pct)
{
    if (!s_w) return;
    int v = (int)pct; if (v > 99) v = 99; else if (v < 0) v = 0;
    if (s_w->batt_pct_text) {
        char text[8];
        snprintf(text, sizeof(text), "%d", v);
        dash_label_set(s_w->batt_pct_text, text);
        dash_set_text_color(s_w->batt_pct_text, batt_color(v), LV_PART_MAIN);
    }
    if (s_w->batt_seg_n > 0) {
        int filled = (v * s_w->batt_seg_n + 50) / 100;
        paint_v_bar(s_w->batt_seg, s_w->batt_seg_n, filled, batt_color(v));
    }
    if (s_w->batt_bar) {
        bar_set(s_w->batt_bar, v);
        dash_set_bg_color(s_w->batt_bar, batt_color(v), LV_PART_INDICATOR);
    }
}

/* Width [text] needs inside [label], with the label's own font, letter spacing
 * and horizontal padding. */
static lv_coord_t label_fit_width(lv_obj_t *label, const char *text)
{
    lv_point_t sz;
    lv_txt_get_size(&sz, text,
                    lv_obj_get_style_text_font(label, LV_PART_MAIN),
                    lv_obj_get_style_text_letter_space(label, LV_PART_MAIN),
                    0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return sz.x + lv_obj_get_style_pad_left(label, LV_PART_MAIN)
                + lv_obj_get_style_pad_right(label, LV_PART_MAIN);
}

/* Make [label] wide enough for [text] without breaking the design: the edge the
 * digits are aligned to stays where GUI Guider put it, so a right-aligned value
 * grows leftward (its unit label doesn't move) and a left- or centre-aligned one
 * grows rightward, pushing the unit label along. Single-head strings fit the
 * authored box, so the geometry is restored exactly. */
static void temp_fit_layout(lv_obj_t *label, lv_obj_t *unit, temp_geom_t *g,
                            const char *text)
{
    /* A value the design left auto-sizing already grows by itself; pinning a
     * pixel width here would freeze it at whatever it measured first. */
    if (lv_obj_get_style_width(label, LV_PART_MAIN) == LV_SIZE_CONTENT) return;

    if (!g->captured) {
        lv_obj_update_layout(label);   /* authored coords, nothing moved yet */
        g->x        = lv_obj_get_x(label);
        g->w        = lv_obj_get_width(label);
        g->unit_x   = unit ? lv_obj_get_x(unit) : 0;
        g->captured = true;
    }

    lv_coord_t need = label_fit_width(label, text);
    lv_coord_t w    = (need > g->w) ? need : g->w;
    lv_coord_t grow = w - g->w;
    lv_coord_t x = g->x, unit_x = g->unit_x;

    switch (lv_obj_get_style_text_align(label, LV_PART_MAIN)) {
    case LV_TEXT_ALIGN_RIGHT:
        x -= grow;
        break;
    case LV_TEXT_ALIGN_CENTER:
        x      -= grow / 2;
        unit_x += grow - grow / 2;
        break;
    default:                       /* LEFT / AUTO */
        unit_x += grow;
        break;
    }

    /* Compare before writing: lv_obj_set_pos/set_width invalidate unconditionally
     * and this runs at the 10 Hz pump rate. */
    if (lv_obj_get_width(label) != w) lv_obj_set_width(label, w);
    if (lv_obj_get_x(label) != x)     lv_obj_set_x(label, x);
    if (unit && lv_obj_get_x(unit) != unit_x) lv_obj_set_x(unit, unit_x);
}

/* Both temperatures render the same way: digits plus an optional bar whose
 * range the screen author picked (e.g. 0..100 degrees). On a dual-motor board
 * the value reads "h1/h2" (e.g. "34/37"), same as the Classic theme — the
 * second head's temps are not aggregated by the VESC firmware, so a theme that
 * printed only head 1 silently hid half the drivetrain. */
static void render_temp(lv_obj_t *label, lv_obj_t *unit, lv_obj_t *bar,
                        temp_geom_t *g, float celsius, bool dual, float celsius2)
{
    int v1 = (int)settings_wrapper_temp_to_display(celsius);
    int v2 = (int)settings_wrapper_temp_to_display(celsius2);

    char text[16];
    if (dual) snprintf(text, sizeof(text), "%d/%d", v1, v2);
    else      snprintf(text, sizeof(text), "%d", v1);

    if (label) {
        temp_fit_layout(label, unit, g, text);
        dash_label_set(label, text);
    }
    /* One bar, two circuits: track the hotter head so the gauge keeps meaning
     * "how close is this to a thermal cutback". */
    bar_set(bar, (dual && v2 > v1) ? v2 : v1);
}

static void g_temp_fet(float c)
{
    if (!s_w) return;
    float fet2 = 0.0f, mot2 = 0.0f;
    bool dual = settings_wrapper_head2_temps(&fet2, &mot2);
    render_temp(s_w->temp_fet_text, s_w->temp_fet_unit, s_w->temp_esc_bar,
                &s_geom_fet, c, dual, fet2);
}

static void g_temp_motor(float c)
{
    if (!s_w) return;
    float fet2 = 0.0f, mot2 = 0.0f;
    bool dual = settings_wrapper_head2_temps(&fet2, &mot2);
    render_temp(s_w->temp_motor_text, s_w->temp_motor_unit, s_w->temp_mot_bar,
                &s_geom_mot, c, dual, mot2);
}

static void g_trip(float km)
{
    if (!s_w || !s_w->trip_text) return;
    char text[10];
    snprintf(text, sizeof(text), "%.1f", settings_wrapper_dist_to_display(km));
    dash_label_set(s_w->trip_text, text);
}

static void g_range(float km)
{
    if (!s_w || !s_w->range_text) return;
    char text[10];
    snprintf(text, sizeof(text), "%.1f", settings_wrapper_dist_to_display(km));
    dash_label_set(s_w->range_text, text);
}

static void g_odometer(float km)
{
    if (!s_w || !s_w->odo_text) return;
    char text[10];
    snprintf(text, sizeof(text), "%05d", (int)settings_wrapper_dist_to_display(km));
    dash_label_set(s_w->odo_text, text);
}

static void g_amp_hours(float ah)
{
    if (!s_w || !s_w->ah_text) return;
    char text[16];
    snprintf(text, sizeof(text), "%.1f Ah", ah);
    dash_label_set(s_w->ah_text, text);
}

static void g_uptime(uint32_t ms)
{
    if (!s_w || !s_w->uptime_text) return;
    int v = ms / 1000;
    char text[20];
    snprintf(text, sizeof(text), "%02d:%02d:%02d", v / 3600, (v % 3600) / 60, v % 60);
    dash_label_set(s_w->uptime_text, text);
}

static void g_hide_mode_text(void)
{
    if (!s_w || !s_w->mode_text) return;
    if (!lv_obj_has_flag(s_w->mode_text, LV_OBJ_FLAG_HIDDEN))
        lv_obj_add_flag(s_w->mode_text, LV_OBJ_FLAG_HIDDEN);
}

static void g_mode_text(uint8_t mode)
{
    if (!s_w || !s_w->mode_text) return;
    if (lv_obj_has_flag(s_w->mode_text, LV_OBJ_FLAG_HIDDEN))
        lv_obj_clear_flag(s_w->mode_text, LV_OBJ_FLAG_HIDDEN);   /* re-show after no-Lisp */
    char text[16];
    snprintf(text, sizeof(text), "MODE %d", mode + 1);
    dash_label_set(s_w->mode_text, text);
}

static void g_cur_time(int hour, int minute, int second)
{
    if (!s_w || !s_w->time_label) return;
    char text[12];
    snprintf(text, sizeof(text), "%02d:%02d:%02d", hour, minute, second);
    dash_label_set(s_w->time_label, text);
}

static void g_cur_time_hm(int hour, int minute)
{
    if (!s_w || !s_w->time_label) return;
    char text[8];
    snprintf(text, sizeof(text), "%02d:%02d", hour, minute);
    dash_label_set(s_w->time_label, text);
    if (lv_obj_has_flag(s_w->time_label, LV_OBJ_FLAG_HIDDEN))
        lv_obj_clear_flag(s_w->time_label, LV_OBJ_FLAG_HIDDEN);
}

static void g_hide_cur_time(void)
{
    if (!s_w || !s_w->time_label) return;
    if (!lv_obj_has_flag(s_w->time_label, LV_OBJ_FLAG_HIDDEN))
        lv_obj_add_flag(s_w->time_label, LV_OBJ_FLAG_HIDDEN);
}

/* Session peaks, tracked centrally in dashboard_theme.c so they are the same
 * numbers on every theme. The unit suffix is baked in: these labels sit in a
 * cramped strip and a separate unit widget per value would not fit. */
static void g_max_values(float max_speed_kmh, float max_power_kw)
{
    if (!s_w) return;
    if (s_w->max_power_text) {
        char text[16];
        snprintf(text, sizeof(text), "%.1f KW", max_power_kw);
        dash_label_set(s_w->max_power_text, text);
    }
    if (s_w->max_speed_text) {
        int disp = (int)settings_wrapper_speed_to_display(max_speed_kmh);
        if (disp < 0) disp = 0; else if (disp > 999) disp = 999;
        char text[16];
        snprintf(text, sizeof(text), "%d %s", disp,
                 settings_wrapper_speed_unit());
        dash_label_set(s_w->max_speed_text, text);
    }
}

static void g_ble_status(bool connected)
{
    if (!s_w || !s_w->status_bt) return;
    dash_set_text_color(s_w->status_bt, connected ? GEN_BT_ON : GEN_BT_OFF, LV_PART_MAIN);
    lv_obj_set_style_text_opa(s_w->status_bt, connected ? LV_OPA_COVER : LV_OPA_50, LV_PART_MAIN);
}

/* ---- dead VESC link ------------------------------------------------------ *
 * A theme cloned from Classic carries the authored, initially-hidden
 * <scr>_esc_not_connected_text label (and the STATISTICS label that shares its
 * slot). A theme that has none gets a banner built on the fly the first time the
 * link drops — otherwise the screen just froze on its last numbers with nothing
 * telling the rider the controller went silent. */
static lv_obj_t *warn_label(void)
{
    if (s_w->esc_warn_text) return s_w->esc_warn_text;
    if (s_warn_auto)        return s_warn_auto;
    if (!s_w->screen)       return NULL;

    lv_obj_t *l = lv_label_create(s_w->screen);
    lv_label_set_text(l, "ESC NOT CONNECTED");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, LV_PART_MAIN);
    dash_set_text_color(l, GEN_WARN, LV_PART_MAIN);
    /* Opaque chip: the banner has to stay readable over whatever the theme drew
     * underneath, and its placement can't be tuned per screen from here. */
    dash_set_bg_color(l, GEN_WARN_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(l, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(l, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(l, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(l, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(l, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(l, GEN_WARN, LV_PART_MAIN);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_add_flag(l, LV_OBJ_FLAG_HIDDEN);
    s_warn_auto = l;
    return l;
}

static void stats_btn_show(bool show);

static void warn_show(bool show)
{
    lv_obj_t *l = warn_label();
    if (!l) return;
    s_warn_shown = show;
    if (show) lv_obj_clear_flag(l, LV_OBJ_FLAG_HIDDEN);
    else      lv_obj_add_flag(l, LV_OBJ_FLAG_HIDDEN);

    /* The authored banner overlaps the STATISTICS entry point (Classic-derived
     * layouts put both at the top centre), so the two alternate. The runtime
     * banner sits on its own, and leaves STATISTICS alone. */
    if (s_w->esc_warn_text && s_w->statistics_button) {
        stats_btn_show(!show);
    }
}

/* STATISTICS entry point: only ever visible while Settings → Trip statistics
 * is on (there is no log behind the screen otherwise). Shared by the banner
 * alternation above and the Settings switch below. */
static void stats_btn_show(bool show)
{
    if (!s_w || !s_w->statistics_button) return;
    if (show && settings_wrapper_get_trip_stats_enabled()) {
        lv_obj_clear_flag(s_w->statistics_button, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_w->statistics_button, LV_OBJ_FLAG_HIDDEN);
    }
}

static void g_trip_stats(bool enabled)
{
    /* On: show unless the authored banner currently owns the slot (the blink
     * loop hands it back on reconnect). Off: hide outright. */
    bool banner_up = s_w && s_w->esc_warn_text && s_warn_shown;
    stats_btn_show(enabled && !banner_up);
}

/* Called from the 100 ms data pump (and only while the dashboard is on screen),
 * so the blink is driven straight off the call rather than an extra timer. */
static void g_esc_connection_status(bool connected)
{
    if (!s_w) return;
    uint32_t now = lv_tick_get();

    if (connected != s_warn_connected) {
        s_warn_connected = connected;
        s_warn_blink_ms  = now;
        warn_show(!connected);
        return;
    }
    if (connected) return;

    if (now - s_warn_blink_ms >= 500) {
        s_warn_blink_ms = now;
        warn_show(!s_warn_shown);
    }
}

/* HIDDEN also takes the slider out of LVGL's hit-testing, so hiding it is what
 * actually disables the gesture — not just what makes it invisible (it already
 * is: the screens author it with bg_opa 0). */
static void g_brightness_gesture(bool enabled)
{
    if (!s_w || !s_w->brightness_slider) return;
    if (enabled) lv_obj_clear_flag(s_w->brightness_slider, LV_OBJ_FLAG_HIDDEN);
    else         lv_obj_add_flag(s_w->brightness_slider, LV_OBJ_FLAG_HIDDEN);
}

const dashboard_theme_ops_t dashboard_generic_ops = {
    .speed           = g_speed,
    .current         = g_current,
    .battery_proc    = g_battery_proc,
    .battery_voltage = g_battery_voltage,
    .temp_fet        = g_temp_fet,
    .temp_motor      = g_temp_motor,
    .trip            = g_trip,
    .range           = g_range,
    .odometer        = g_odometer,
    .amp_hours       = g_amp_hours,
    .uptime          = g_uptime,
    .mode_text       = g_mode_text,
    .hide_mode_text  = g_hide_mode_text,
    .cur_time        = g_cur_time,
    .cur_time_hm     = g_cur_time_hm,
    .hide_cur_time   = g_hide_cur_time,
    .ble_status      = g_ble_status,
    .esc_connection_status = g_esc_connection_status,
    .max_values      = g_max_values,
    .brightness_gesture = g_brightness_gesture,
    .trip_stats         = g_trip_stats,
    /* battery_temp / fps / units_changed / cruise_* / navigation_* / music_*:
     * theme-specific or need extra widgets — left NULL (skipped). */
};
