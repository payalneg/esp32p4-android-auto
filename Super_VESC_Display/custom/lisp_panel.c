/*
 * LISP quick-action panel (show_lisp_panel / lisp_panel_open_async).
 *
 * A left-docked drawer that floats on lv_layer_top() over the VESC dashboard,
 * opened by a left-edge swipe (main/touch_input.c → main.c registers
 * lisp_panel_open_async as the edge-swipe callback). Its controls are NOT
 * hard-coded: the master LISP script running on the VESC describes them at
 * runtime (toggles / buttons / numbers / read-only labels) over
 * COMM_CUSTOM_APP_DATA, and the P4 renders them and sends back interactions.
 * See components/vesc_can/vesc_lisp_panel.h for the wire protocol.
 *
 * Threading: the transport's model is filled on the CAN task; this screen only
 * snapshots it (vesc_lisp_panel_get_model) from an lv_timer on the LVGL task,
 * comparing ui_epoch/state_epoch to decide rebuild vs value-refresh. Opening is
 * marshalled onto the LVGL task via lv_async_call.
 *
 * Device-only (needs vesc_can); the desktop simulator gets a static-demo
 * placeholder so layout can still be eyeballed there.
 */
#include "lvgl.h"
#include "custom.h"

/* ---- palette (matches the rest of the UI) ---- */
#define COL_BG        0x07090A
#define COL_PANEL     0x12181C
#define COL_BTN       0x2a3440
#define COL_ACCENT    0xB6FF2E
#define COL_CYAN      0x00a9ff
#define COL_TEXT      0xFFFFFF
#define COL_DIM       0x8A9499

#define DRAWER_W      300
#define SCREEN_W      800
#define SCREEN_H      480
#define SLIDE_MS      220
#define DRAWER_ON_X   0
#define DRAWER_OFF_X  (-DRAWER_W)

#ifdef LV_REALDEVICE

#include "vesc_can/vesc_lisp_panel.h"
#include "vesc_ui.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t   id;
    uint8_t   type;
    lv_obj_t *editor;     /* switch / button */
    lv_obj_t *value_lbl;  /* number / label readout */
    float     cur, vmin, vmax, vstep;
    char      suffix[VLP_SUFFIX_MAX];
} row_t;

static lv_obj_t  *s_scrim;        /* full-screen tap-catcher behind the drawer */
static lv_obj_t  *s_drawer;       /* the sliding left dock */
static lv_obj_t  *s_list;         /* scrollable controls container */
static lv_obj_t  *s_status;       /* "Loading..." / "No actions" hint */
static lv_timer_t *s_timer;
static bool       s_open;
static row_t      s_rows[VLP_MAX_CTRLS];
static uint8_t    s_row_count;
static uint32_t   s_seen_ui_epoch;
static uint32_t   s_seen_state_epoch;

/* ---- value formatting ---- */
static void fmt_num(char *buf, size_t n, float v, float step, const char *suffix)
{
    int dec = (step != 0.0f && step < 1.0f) ? 1 : 0;
    if (suffix && suffix[0]) snprintf(buf, n, "%.*f %s", dec, (double)v, suffix);
    else                     snprintf(buf, n, "%.*f", dec, (double)v);
}

/* ---- control callbacks (LVGL task) ---- */
static void toggle_cb(lv_event_t *e)
{
    row_t *r = (row_t *)lv_event_get_user_data(e);
    bool on = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    vesc_lisp_panel_send_action(r->id, on ? 1.0f : 0.0f);
}

static void button_cb(lv_event_t *e)
{
    row_t *r = (row_t *)lv_event_get_user_data(e);
    vesc_lisp_panel_send_action(r->id, 1.0f);
}

static void num_step(row_t *r, int dir)
{
    float v = r->cur + (float)dir * r->vstep;
    if (v < r->vmin) v = r->vmin;
    if (v > r->vmax) v = r->vmax;
    r->cur = v;
    char b[32];
    fmt_num(b, sizeof b, v, r->vstep, r->suffix);
    lv_label_set_text(r->value_lbl, b);
    vesc_lisp_panel_send_action(r->id, v);
}
static void num_dec_cb(lv_event_t *e) { num_step((row_t *)lv_event_get_user_data(e), -1); }
static void num_inc_cb(lv_event_t *e) { num_step((row_t *)lv_event_get_user_data(e), +1); }

/* ---- row builders ---- */
static lv_obj_t *new_row(void)
{
    lv_obj_t *row = lv_obj_create(s_list);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_set_style_pad_row(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    return row;
}

static lv_obj_t *row_label(lv_obj_t *row, const char *text, uint32_t color)
{
    lv_obj_t *l = lv_label_create(row);
    lv_label_set_text(l, text);
    lv_obj_set_width(l, lv_pct(100));
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserratMedium_16, 0);
    return l;
}

static lv_obj_t *step_btn(lv_obj_t *parent, const char *glyph, lv_event_cb_t cb, void *ud)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 54, 44);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN), 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, glyph);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, ud);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_LONG_PRESSED_REPEAT, ud);
    return btn;
}

static void build_toggle(row_t *r, const vlp_ctrl_t *c)
{
    lv_obj_t *row = new_row();
    lv_obj_t *line = lv_obj_create(row);     /* label left, switch right */
    lv_obj_set_width(line, lv_pct(100));
    lv_obj_set_height(line, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(line, 0, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(line, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(line, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *l = row_label(line, c->label, COL_TEXT);
    lv_obj_set_flex_grow(l, 1);

    lv_obj_t *sw = lv_switch_create(line);
    lv_obj_set_size(sw, 60, 30);
    lv_obj_set_style_bg_color(sw, lv_color_hex(COL_BTN), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(COL_CYAN),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (c->value != 0.0f) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, toggle_cb, LV_EVENT_VALUE_CHANGED, r);
    r->editor = sw;
}

static void build_button(row_t *r, const vlp_ctrl_t *c)
{
    lv_obj_t *row = new_row();
    lv_obj_t *btn = lv_btn_create(row);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 46);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN), 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, c->label);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TEXT), 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(btn, button_cb, LV_EVENT_CLICKED, r);
    r->editor = btn;
}

static void build_number(row_t *r, const vlp_ctrl_t *c)
{
    lv_obj_t *row = new_row();
    row_label(row, c->label, COL_TEXT);

    lv_obj_t *line = lv_obj_create(row);     /* [-]  value  [+] */
    lv_obj_set_width(line, lv_pct(100));
    lv_obj_set_height(line, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(line, 0, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(line, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(line, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    step_btn(line, "-", num_dec_cb, r);

    lv_obj_t *v = lv_label_create(line);
    lv_obj_set_flex_grow(v, 1);
    lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_text_font(v, &lv_font_montserrat_24, 0);
    char b[32];
    fmt_num(b, sizeof b, c->value, c->vstep, c->suffix);
    lv_label_set_text(v, b);
    r->value_lbl = v;

    step_btn(line, "+", num_inc_cb, r);
}

static void build_readout(row_t *r, const vlp_ctrl_t *c)
{
    lv_obj_t *row = new_row();
    lv_obj_t *line = lv_obj_create(row);
    lv_obj_set_width(line, lv_pct(100));
    lv_obj_set_height(line, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(line, 0, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(line, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(line, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *l = row_label(line, c->label, COL_DIM);
    lv_obj_set_flex_grow(l, 1);

    lv_obj_t *v = lv_label_create(line);
    lv_obj_set_style_text_color(v, lv_color_hex(COL_TEXT), 0);
    char b[32];
    fmt_num(b, sizeof b, c->value, 0.0f, c->suffix);
    lv_label_set_text(v, b);
    r->value_lbl = v;
}

static void rebuild_controls(const vlp_model_t *m)
{
    lv_obj_clean(s_list);
    memset(s_rows, 0, sizeof s_rows);
    s_row_count = 0;

    if (m->count == 0) {
        lv_label_set_text(s_status, "No actions");
        lv_obj_clear_flag(s_status, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_add_flag(s_status, LV_OBJ_FLAG_HIDDEN);

    for (uint8_t i = 0; i < m->count && s_row_count < VLP_MAX_CTRLS; i++) {
        const vlp_ctrl_t *c = &m->ctrl[i];
        row_t *r = &s_rows[s_row_count];
        r->id    = c->id;
        r->type  = c->type;
        r->cur   = c->value;
        r->vmin  = c->vmin;
        r->vmax  = c->vmax;
        r->vstep = c->vstep;
        strncpy(r->suffix, c->suffix, sizeof r->suffix - 1);

        switch (c->type) {
        case VLP_CTRL_TOGGLE: build_toggle(r, c);  break;
        case VLP_CTRL_BUTTON: build_button(r, c);  break;
        case VLP_CTRL_NUMBER: build_number(r, c);  break;
        case VLP_CTRL_LABEL:  build_readout(r, c); break;
        default: continue;   /* skip — don't advance s_row_count */
        }
        s_row_count++;
    }
}

static void refresh_values(const vlp_model_t *m)
{
    for (uint8_t i = 0; i < m->count; i++) {
        const vlp_ctrl_t *c = &m->ctrl[i];
        for (uint8_t k = 0; k < s_row_count; k++) {
            row_t *r = &s_rows[k];
            if (r->id != c->id) continue;
            if (r->type == VLP_CTRL_TOGGLE && r->editor) {
                /* add/clear_state does NOT fire VALUE_CHANGED, so this won't
                 * loop back into toggle_cb. */
                if (c->value != 0.0f) lv_obj_add_state(r->editor, LV_STATE_CHECKED);
                else                  lv_obj_clear_state(r->editor, LV_STATE_CHECKED);
            } else if (r->type == VLP_CTRL_NUMBER && r->value_lbl) {
                r->cur = c->value;
                char b[32];
                fmt_num(b, sizeof b, c->value, r->vstep, r->suffix);
                lv_label_set_text(r->value_lbl, b);
            } else if (r->type == VLP_CTRL_LABEL && r->value_lbl) {
                char b[32];
                fmt_num(b, sizeof b, c->value, 0.0f, r->suffix);
                lv_label_set_text(r->value_lbl, b);
            }
            break;
        }
    }
}

/* ---- open / close ---- */
static void slide_drawer(int32_t from, int32_t to, lv_anim_path_cb_t path,
                         lv_anim_ready_cb_t ready)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_drawer);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_time(&a, SLIDE_MS);
    lv_anim_set_path_cb(&a, path);
    lv_anim_set_values(&a, from, to);
    if (ready) lv_anim_set_ready_cb(&a, ready);
    lv_anim_start(&a);
}

static void teardown_ready_cb(lv_anim_t *a)
{
    (void)a;
    if (s_drawer) { lv_obj_del_async(s_drawer); s_drawer = NULL; }
    if (s_scrim)  { lv_obj_del_async(s_scrim);  s_scrim  = NULL; }
    s_list = NULL;
    s_status = NULL;
    s_row_count = 0;
}

void lisp_panel_close(void)
{
    if (!s_open) return;
    s_open = false;
    vesc_lisp_panel_set_enabled(false);   /* stop the CAN poll task polling */
    if (s_timer) { lv_timer_del(s_timer); s_timer = NULL; }
    if (s_drawer) {
        slide_drawer(DRAWER_ON_X, DRAWER_OFF_X, lv_anim_path_ease_in,
                     teardown_ready_cb);
    } else {
        teardown_ready_cb(NULL);
    }
}

static void scrim_cb(lv_event_t *e)
{
    (void)e;
    lisp_panel_close();   /* tap outside the drawer */
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_open) return;

    /* Auto-close if the dashboard is no longer the active screen (user opened
     * Settings, the config menu, switched to AA, …). The drawer lives on
     * lv_layer_top() so it would otherwise float over the new screen. */
    if (lv_scr_act() != vesc_ui_get_screen()) { lisp_panel_close(); return; }

    /* This timer only renders from the model snapshot now; the CAN poll task
     * (vesc_lisp_panel_poll_loop) issues the UI_DESC/STATE requests that keep
     * the model fresh — so the LVGL task never blocks on CAN. */
    vlp_model_t m;
    if (!vesc_lisp_panel_get_model(&m)) {
        return;   /* no UI descriptor yet — the poll task is still asking */
    }
    if (m.ui_epoch != s_seen_ui_epoch) {
        rebuild_controls(&m);
        s_seen_ui_epoch    = m.ui_epoch;
        s_seen_state_epoch = m.state_epoch;
    } else if (m.state_epoch != s_seen_state_epoch) {
        refresh_values(&m);
        s_seen_state_epoch = m.state_epoch;
    }
}

static void do_open(void)
{
    if (s_open) return;
    /* Dashboard-only: never open over Settings / config menu / editor / AA. */
    if (lv_scr_act() != vesc_ui_get_screen()) return;

    /* Scrim (behind) — semi-transparent, catches taps outside the drawer. */
    s_scrim = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_scrim);
    lv_obj_set_size(s_scrim, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(s_scrim, 0, 0);
    lv_obj_set_style_bg_color(s_scrim, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_scrim, LV_OPA_40, 0);
    lv_obj_add_flag(s_scrim, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_scrim, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_scrim, scrim_cb, LV_EVENT_CLICKED, NULL);

    /* Drawer (in front) — a sibling of the scrim, so taps on it don't bubble
     * up to the scrim's close handler. */
    s_drawer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_drawer, DRAWER_W, SCREEN_H);
    lv_obj_set_pos(s_drawer, DRAWER_OFF_X, 0);
    lv_obj_set_style_bg_color(s_drawer, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_drawer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_drawer, 0, 0);
    lv_obj_set_style_radius(s_drawer, 0, 0);
    lv_obj_set_style_pad_all(s_drawer, 10, 0);
    lv_obj_clear_flag(s_drawer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_drawer);
    lv_label_set_text(title, "Quick actions");
    lv_obj_set_pos(title, 4, 4);
    lv_obj_set_style_text_color(title, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);

    s_status = lv_label_create(s_drawer);
    lv_label_set_text(s_status, "Loading...");
    lv_obj_set_pos(s_status, 4, 44);
    lv_obj_set_style_text_color(s_status, lv_color_hex(COL_DIM), 0);

    s_list = lv_obj_create(s_drawer);
    lv_obj_set_pos(s_list, 0, 44);
    lv_obj_set_size(s_list, DRAWER_W - 20, SCREEN_H - 44 - 20);
    lv_obj_set_style_bg_opa(s_list, 0, 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_style_pad_all(s_list, 0, 0);
    lv_obj_set_style_pad_row(s_list, 10, 0);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(s_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_list, LV_SCROLLBAR_MODE_AUTO);

    s_open = true;
    s_seen_ui_epoch    = 0;   /* force a rebuild from whatever the model holds */
    s_seen_state_epoch = 0;

    vesc_lisp_panel_set_enabled(true);   /* CAN poll task starts requesting UI/state */
    slide_drawer(DRAWER_OFF_X, DRAWER_ON_X, lv_anim_path_ease_out, NULL);
    s_timer = lv_timer_create(poll_cb, 200, NULL);
    poll_cb(s_timer);  /* paint last-known model instantly if we have one */
}

void show_lisp_panel(void) { do_open(); }

static void open_async_cb(void *p) { (void)p; do_open(); }

void lisp_panel_open_async(void)
{
    /* Called from the touch poll task — marshal onto the LVGL task. */
    lv_async_call(open_async_cb, NULL);
}

#else  /* !LV_REALDEVICE — desktop simulator static-demo placeholder */

static lv_obj_t *s_scrim;
static lv_obj_t *s_drawer;
static bool      s_open;

static void teardown_ready_cb(lv_anim_t *a)
{
    (void)a;
    if (s_drawer) { lv_obj_del_async(s_drawer); s_drawer = NULL; }
    if (s_scrim)  { lv_obj_del_async(s_scrim);  s_scrim  = NULL; }
}

void lisp_panel_close(void)
{
    if (!s_open) return;
    s_open = false;
    if (!s_drawer) { teardown_ready_cb(NULL); return; }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_drawer);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_time(&a, SLIDE_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_values(&a, DRAWER_ON_X, DRAWER_OFF_X);
    lv_anim_set_ready_cb(&a, teardown_ready_cb);
    lv_anim_start(&a);
}

static void scrim_cb(lv_event_t *e) { (void)e; lisp_panel_close(); }

static void demo_toggle(lv_obj_t *list, const char *name, bool on)
{
    lv_obj_t *line = lv_obj_create(list);
    lv_obj_set_width(line, lv_pct(100));
    lv_obj_set_height(line, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(line, 0, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_flex_flow(line, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(line, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *l = lv_label_create(line);
    lv_label_set_text(l, name);
    lv_obj_set_flex_grow(l, 1);
    lv_obj_t *sw = lv_switch_create(line);
    if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
}

void show_lisp_panel(void)
{
    if (s_open) return;
    s_scrim = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_scrim);
    lv_obj_set_size(s_scrim, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(s_scrim, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_scrim, LV_OPA_40, 0);
    lv_obj_add_flag(s_scrim, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_scrim, scrim_cb, LV_EVENT_CLICKED, NULL);

    s_drawer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_drawer, DRAWER_W, SCREEN_H);
    lv_obj_set_pos(s_drawer, DRAWER_OFF_X, 0);
    lv_obj_set_style_bg_color(s_drawer, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_pad_all(s_drawer, 10, 0);
    lv_obj_set_flex_flow(s_drawer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_drawer, 12, 0);

    lv_obj_t *title = lv_label_create(s_drawer);
    lv_label_set_text(title, "Quick actions (demo)");
    lv_obj_set_style_text_color(title, lv_color_hex(COL_TEXT), 0);

    demo_toggle(s_drawer, "Throttle", true);
    demo_toggle(s_drawer, "Traction Control", false);

    s_open = true;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_drawer);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_time(&a, SLIDE_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_values(&a, DRAWER_OFF_X, DRAWER_ON_X);
    lv_anim_start(&a);
}

void lisp_panel_open_async(void) { show_lisp_panel(); }

#endif /* LV_REALDEVICE */
