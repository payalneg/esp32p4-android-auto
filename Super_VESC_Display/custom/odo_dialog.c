/*
 * Odometer dialog — Settings → Bike → "Set".
 *
 * The lifetime distance lives on the display now (components/dev_settings/
 * vesc_odo_persist.c), because the controller only flushes its own counter to
 * flash every so often and a key-off straight after a ride loses the tail. A
 * replacement controller, a frame swap or a freshly flashed VESC still needs a
 * way to say "this bike has 4128 km on it" — that is this dialog.
 *
 * Edits whichever odometer the dashboard is showing: the VESC mirror, or the
 * BLE wheel sensor's when that is the selected speed source. The number is
 * entered in the user's units (km or mi) and converted before it is stored.
 *
 * Modal on lv_layer_top() plus the on-screen keyboard, so it works over the
 * Settings screen without rebuilding it. Device-only (this file is compiled
 * into the plain simulator too, where it is a stub — the real screens come
 * from SIM_SHIM builds, see speed_screen.c).
 */
#include <stdio.h>
#include <stdlib.h>

#include "lvgl.h"
#include "ui_geom.h"
#include "custom.h"

#if defined(LV_REALDEVICE) || defined(LV_SIMSHIM)

#include "settings_wrapper.h"
#include "speed_sensor.h"
#include "vesc_odo_persist.h"

#define COL_PANEL  0x12181C
#define COL_BTN    0x2a3440
#define COL_ACCENT 0xB6FF2E
#define COL_CYAN   0x00a9ff
#define COL_RED    0xFF3B30
#define COL_TEXT   0xFFFFFF
#define COL_DIM    0x8A9499

/* The dashboard's ODO field is five digits wide (%05d in custom.c). */
#define ODO_MAX   999999.0

static lv_obj_t *s_modal;
static lv_obj_t *s_kb;
static lv_obj_t *s_ta;
static lv_obj_t *s_msg;

/* ---- helpers ---- */

static void close_dialog(void)
{
    /* Keyboard first: it points at the textarea, which the modal owns.
     * Deferred deletion: both are torn down from their own event callbacks
     * (keyboard READY/CANCEL, Save/Cancel), and deleting the live event target
     * (or its ancestor) right there is a use-after-free in LVGL 8. */
    if (s_kb)    { lv_obj_del_async(s_kb);    s_kb = NULL; }
    if (s_modal) { lv_obj_del_async(s_modal); s_modal = NULL; }
    s_ta  = NULL;
    s_msg = NULL;
}

/* Odometer of whichever source the dashboard is showing, in display units. */
static float current_display_value(void)
{
    float km = speed_source_is_ble() ? speed_sensor_get_odometer_km()
                                     : odo_persist_get_km();
    return settings_wrapper_dist_to_display(km);
}

static void set_msg(const char *txt)
{
    if (s_msg) lv_label_set_text(s_msg, txt);
}

static void apply_value(double display_value)
{
    /* display = km * factor, factor = 1.0 (km) or 0.621371 (mi). */
    float factor = settings_wrapper_dist_to_display(1.0f);
    if (!(factor > 0.05f)) factor = 1.0f;      /* paranoia; never 0 in practice */
    float km = (float)(display_value / (double)factor);

    if (speed_source_is_ble()) speed_sensor_set_odometer_km(km);
    else                       odo_persist_set_km(km);

    close_dialog();
    settings_odometer_refresh();                /* keep the settings caption honest */
}

/* ---- event handlers ---- */

static void save_cb(lv_event_t *e)
{
    (void)e;
    const char *txt = s_ta ? lv_textarea_get_text(s_ta) : NULL;
    if (!txt || !txt[0]) { set_msg("Enter a number"); return; }

    char *end = NULL;
    double v = strtod(txt, &end);
    if (end == txt) { set_msg("Not a number"); return; }
    while (end && *end == ' ') end++;
    if (end && *end) { set_msg("Digits only"); return; }
    if (!(v >= 0.0) || v > ODO_MAX) { set_msg("0 to 999999 only"); return; }

    apply_value(v);
}

static void cancel_cb(lv_event_t *e) { (void)e; close_dialog(); }

static void kb_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY)  save_cb(e);
    if (code == LV_EVENT_CANCEL) close_dialog();
}

/* ---- widget builders ---- */

static lv_obj_t *make_button(lv_obj_t *parent, const char *text, uint32_t bg,
                             lv_coord_t w, lv_coord_t h, lv_event_cb_t cb)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
    lv_obj_set_style_border_width(b, 0, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_radius(b, 8, 0);

    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(l, UI_FONT_MENU, 0);
    lv_obj_center(l);
    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

/* ---- public ---- */

void odo_settings_summary(char *buf, int n)
{
    if (!buf || n <= 0) return;
    snprintf(buf, (size_t)n, "Odometer: %.0f %s",
             (double)current_display_value(),
             settings_wrapper_get_use_imperial() ? "mi" : "km");
}

void show_odo_set_dialog(void)
{
    if (s_modal) return;                        /* already open */

    const lv_coord_t w = UI_W - 40;
    const lv_coord_t h = 208;

    s_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_modal, w, h);
    /* Top-aligned: the keyboard takes the bottom 200 px. */
    lv_obj_align(s_modal, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(s_modal, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_bg_opa(s_modal, 255, 0);
    lv_obj_set_style_border_width(s_modal, 1, 0);
    lv_obj_set_style_border_color(s_modal, lv_color_hex(COL_BTN), 0);
    lv_obj_set_style_radius(s_modal, 10, 0);
    lv_obj_set_style_pad_all(s_modal, 0, 0);
    lv_obj_clear_flag(s_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_modal);
    lv_label_set_text(title, "Set odometer");
    lv_obj_set_pos(title, 20, 12);
    lv_obj_set_style_text_color(title, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);

    lv_obj_t *unit = lv_label_create(s_modal);
    lv_label_set_text(unit, settings_wrapper_get_use_imperial()
                                ? "Total distance in miles"
                                : "Total distance in kilometres");
    lv_obj_set_pos(unit, 20, 44);
    lv_obj_set_style_text_color(unit, lv_color_hex(COL_DIM), 0);
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_14, 0);

    /* Current value, in the same units, ready to be replaced. */
    char cur[16];
    snprintf(cur, sizeof cur, "%.0f", (double)current_display_value());
    s_ta = lv_textarea_create(s_modal);
    lv_obj_set_pos(s_ta, 20, 66);
    lv_obj_set_size(s_ta, w - 40, 50);
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_accepted_chars(s_ta, "0123456789");
    lv_textarea_set_max_length(s_ta, 6);
    lv_textarea_set_text(s_ta, cur);
    lv_obj_set_style_text_font(s_ta, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_ta, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_bg_color(s_ta, lv_color_hex(0x07090A), 0);
    lv_obj_set_style_border_width(s_ta, 1, 0);
    lv_obj_set_style_border_color(s_ta, lv_color_hex(COL_CYAN), 0);

    s_msg = lv_label_create(s_modal);
    lv_label_set_text(s_msg, "");
    lv_obj_set_pos(s_msg, 20, 124);
    lv_obj_set_style_text_color(s_msg, lv_color_hex(COL_RED), 0);
    lv_obj_set_style_text_font(s_msg, &lv_font_montserrat_16, 0);

    /* Buttons on the bottom-right of the modal. */
    lv_obj_t *cancel = make_button(s_modal, "Cancel", COL_BTN, 140, 50, cancel_cb);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_RIGHT, -12, -12);
    lv_obj_t *save = make_button(s_modal, "Save", 0x2e7d32, 140, 50, save_cb);
    lv_obj_align(save, LV_ALIGN_BOTTOM_RIGHT, -160, -12);

    /* Number keyboard, owned by the top layer so the modal frame can stay
     * compact. No letters to hit by accident. */
    s_kb = lv_keyboard_create(lv_layer_top());
    lv_obj_set_size(s_kb, UI_W, 200);
    lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_mode(s_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(s_kb, s_ta);
    /* "Enter" commits, the keyboard's close key dismisses. */
    lv_obj_add_event_cb(s_kb, kb_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(s_kb, kb_cb, LV_EVENT_CANCEL, NULL);
}

#else  /* plain desktop simulator: the odometer backend is not linked */

void odo_settings_summary(char *buf, int n)
{
    if (!buf || n <= 0) return;
    snprintf(buf, (size_t)n, "Odometer");
}

void show_odo_set_dialog(void) { /* available on the device build */ }

#endif
