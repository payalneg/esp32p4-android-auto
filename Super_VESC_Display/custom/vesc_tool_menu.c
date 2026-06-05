/*
 * VESC Tool controller-config menu (run_vesc_tool_menu).
 *
 * A self-contained LVGL screen — created on demand, destroyed on unload, like
 * the logs/QR screens in custom.c — that mirrors the mobile VESC Tool config:
 * a Motor/App selector, group + subgroup dropdowns, and a scrollable list of
 * type-specific editors for every parameter in the selected subgroup. Reads
 * and writes go through the transport-agnostic vesc_config API (real VESC over
 * CAN, or the in-RAM emulator when vesc_sim is active).
 *
 * Only the visible subgroup's rows exist at any time (rebuilt on navigation)
 * to bound LVGL object count. CAN replies arrive on the CAN task and are
 * marshalled to the LVGL thread via lv_async_call; an s_alive guard prevents a
 * late reply from touching a destroyed screen.
 *
 * The full implementation needs the vesc_config component (device only); the
 * desktop simulator gets a small placeholder so the build still links.
 */
#include "lvgl.h"
#include "custom.h"

extern lv_ui guider_ui;

#ifdef LV_REALDEVICE

#include "vesc_config/vesc_config.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---- palette (matches the rest of the UI) ---- */
#define COL_BG        0x07090A
#define COL_PANEL     0x12181C
#define COL_BTN       0x2a3440
#define COL_ACCENT    0xB6FF2E   /* lime — selected / OK / dirty */
#define COL_CYAN      0x00a9ff   /* in-flight / plus */
#define COL_ORANGE    0xffa500
#define COL_RED       0xFF3B30
#define COL_TEXT      0xFFFFFF
#define COL_DIM       0x8A9499

#define VT_MAX_ROWS   96
#define VT_MAX_BITS   16

#define ROW_H         54
#define NAME_W        470

typedef struct {
    int       param_idx;
    uint8_t   type;
    lv_obj_t *name_lbl;
    lv_obj_t *value_lbl;       /* numeric / qstring */
    lv_obj_t *editor;          /* dropdown / switch */
    lv_obj_t *bits[VT_MAX_BITS];
    uint8_t   bit_count;
    /* precomputed numeric display params */
    double    disp_min, disp_max, disp_step;
    double    editor_scale, pct_max;
    uint8_t   decimals;
    bool      as_pct;
    char      suffix[12];
} vt_row_t;

/* ---- screen state ---- */
static lv_obj_t *s_screen;
static lv_obj_t *s_title;
static lv_obj_t *s_emu_banner;
static lv_obj_t *s_status;
static lv_obj_t *s_btn_motor, *s_btn_app;
static lv_obj_t *s_dd_group, *s_dd_sub;
static lv_obj_t *s_list;
static lv_obj_t *s_spinner_modal;
static lv_obj_t *s_write_btn, *s_default_btn, *s_detect_btn, *s_restore_btn;
static lv_timer_t *s_ready_poll;   /* polls for late VESC detection */
static lv_timer_t *s_fs_timer;     /* polls backup-FS mount/format state */
static lv_obj_t *s_fmt_modal;      /* "Formatting backup storage" overlay */
static bool      s_alive;

/* restore-picker dialog */
static lv_obj_t *s_restore_dlg;

/* FOC detection dialog state */
static lv_obj_t *s_detect_dlg;
static lv_obj_t *s_detect_pl_lbl;
static int       s_detect_power_loss = 60;   /* W */
static bool      s_detect_can_other  = false;
static volatile int s_detect_result;

static vc_kind_t s_kind;
static int       s_group_idx;
static int       s_sub_idx;
static bool      s_loaded[2];
static bool      s_req_in_flight;
static bool      s_last_defaults;   /* whether the in-flight read was a defaults read */
static int       s_retries;         /* consecutive timeout retries for the current op */
#define VT_MAX_RETRIES 3

static vt_row_t  s_rows[VT_MAX_ROWS];
static int       s_row_count;

/* async hand-off from the CAN task */
static volatile vc_kind_t   s_async_kind;
static volatile vc_result_t s_async_res;
static volatile bool        s_async_is_write;

static void vt_build_list(void);
static void vt_populate_selectors(void);
static void vt_update_kind_buttons(void);
static void vt_set_status(const char *txt, uint32_t color);
static lv_obj_t *make_footer_btn(const char *txt, int x, int w, uint32_t bg, lv_event_cb_t cb);
static void start_ready_poll(void);
static void vt_update_write_enabled(void);
static void vt_update_restore_enabled(void);
static void vt_request_read(vc_kind_t kind, bool defaults);
static void vt_request_write(void);
static void detect_cb(lv_event_t *e);
static void restore_cb(lv_event_t *e);

/* ===================== helpers ===================== */

static void style_btn(lv_obj_t *btn, uint32_t bg)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg), 0);
    lv_obj_set_style_text_color(btn, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(btn, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
}

static void style_dropdown(lv_obj_t *dd)
{
    lv_obj_set_style_bg_color(dd, lv_color_hex(COL_BTN), 0);
    lv_obj_set_style_text_color(dd, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserratMedium_16, 0);
    lv_obj_set_style_radius(dd, 6, 0);
    lv_obj_set_style_border_width(dd, 0, 0);
    lv_obj_t *list = lv_dropdown_get_list(dd);
    if (list) {
        lv_obj_set_style_bg_color(list, lv_color_hex(COL_PANEL), 0);
        lv_obj_set_style_text_color(list, lv_color_hex(COL_TEXT), 0);
        lv_obj_set_style_text_font(list, &lv_font_montserratMedium_16, 0);
        lv_obj_set_style_max_height(list, 360, 0);
    }
}

/* display value <-> stored value, mirroring ParamEditDouble.qml */
static double disp_from_stored(const vt_row_t *r, double stored)
{
    if (r->as_pct) {
        return r->pct_max != 0.0 ? stored * 100.0 / r->pct_max : 0.0;
    }
    return stored * r->editor_scale;
}

static double stored_from_disp(const vt_row_t *r, double disp)
{
    if (r->as_pct) {
        return disp / 100.0 * r->pct_max;
    }
    return r->editor_scale != 0.0 ? disp / r->editor_scale : disp;
}

static void fmt_disp(const vt_row_t *r, double disp, char *buf, size_t n)
{
    int dec = r->as_pct ? 0 : r->decimals;
    snprintf(buf, n, "%.*f%s", dec, disp, r->suffix);
}

/* ===================== spinner / status ===================== */

static void vt_show_spinner(const char *text)
{
    if (s_spinner_modal) return;
    s_spinner_modal = lv_obj_create(s_screen);
    lv_obj_set_size(s_spinner_modal, 800, 480);
    lv_obj_set_pos(s_spinner_modal, 0, 0);
    lv_obj_set_style_bg_color(s_spinner_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_spinner_modal, LV_OPA_60, 0);
    lv_obj_set_style_border_width(s_spinner_modal, 0, 0);
    lv_obj_clear_flag(s_spinner_modal, LV_OBJ_FLAG_SCROLLABLE);
    /* Absorb touches so the user can't edit/navigate (racing the deserialize
     * that's writing values on the CAN task) while a read/write is in flight. */
    lv_obj_add_flag(s_spinner_modal, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *sp = lv_spinner_create(s_spinner_modal, 1000, 60);
    lv_obj_set_size(sp, 80, 80);
    lv_obj_align(sp, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_arc_color(sp, lv_color_hex(COL_CYAN), LV_PART_INDICATOR);

    lv_obj_t *lbl = lv_label_create(s_spinner_modal);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 50);
}

static void vt_hide_spinner(void)
{
    if (s_spinner_modal) {
        lv_obj_del(s_spinner_modal);
        s_spinner_modal = NULL;
    }
}

static void vt_set_status(const char *txt, uint32_t color)
{
    if (!s_status) return;
    lv_label_set_text(s_status, txt);
    lv_obj_set_style_text_color(s_status, lv_color_hex(color), 0);
}

/* ===================== async read/write ===================== */

static void vt_apply_async(void *p)
{
    (void)p;
    if (!s_alive) return;
    vt_hide_spinner();
    s_req_in_flight = false;
    vc_result_t res = s_async_res;
    vc_kind_t   k   = s_async_kind;

    /* A dropped CAN fragment shows up as a timeout — the config blob is ~70
     * frames, so an occasional miss is expected. Retry a few times before
     * surfacing the error. */
    if (res == VC_ERR_TIMEOUT && s_retries < VT_MAX_RETRIES) {
        s_retries++;
        char b[24];
        snprintf(b, sizeof b, "Retry %d/%d...", s_retries, VT_MAX_RETRIES);
        vt_set_status(b, COL_CYAN);
        if (s_async_is_write) {
            vt_request_write();
        } else {
            vt_request_read(k, s_last_defaults);
        }
        return;
    }
    s_retries = 0;

    if (res == VC_OK) {
        s_loaded[k] = true;
        if (s_async_is_write) {
            vt_set_status("Write OK", COL_ACCENT);
        } else {
            vt_set_status("Read OK", COL_ACCENT);
        }
        if (k == s_kind) {
            vt_build_list();   /* refresh editors with fresh values */
        }
        vt_update_write_enabled();
        vt_update_restore_enabled();   /* a fresh read auto-creates a backup */
    } else {
        const char *msg = "Error";
        switch (res) {
        case VC_ERR_TIMEOUT:   msg = "No response from VESC"; break;
        case VC_ERR_SIGNATURE: msg = "Signature mismatch";    break;
        case VC_ERR_READONLY:  msg = "Config is read-only";   break;
        case VC_ERR_NO_TABLE:  msg = "VESC not detected";     break;
        case VC_ERR_NO_READ:   msg = "Read config first";     break;
        default: break;
        }
        vt_set_status(msg, COL_RED);
    }
}

/* fires on the CAN/timer task (real) or synchronously (emulator) */
static void vt_cfg_done(vc_kind_t kind, vc_result_t res, void *user)
{
    s_async_is_write = (bool)(uintptr_t)user;
    s_async_kind = kind;
    s_async_res  = res;
    lv_async_call(vt_apply_async, NULL);
}

static void vt_request_read(vc_kind_t kind, bool defaults)
{
    if (s_req_in_flight) return;
    if (!vesc_config_ready()) {
        vesc_config_probe_fw();
        start_ready_poll();
        vt_set_status("Detecting VESC...", COL_CYAN);
        return;
    }
    s_req_in_flight = true;
    s_last_defaults = defaults;
    vt_set_status(defaults ? "Loading defaults..." : "Reading...", COL_CYAN);
    if (!vesc_config_is_emulator()) {
        vt_show_spinner(defaults ? "Loading defaults..." : "Reading config...");
    }
    vc_result_t r = vesc_config_read(kind, defaults, vt_cfg_done, (void *)0);
    if (r != VC_OK) {
        s_req_in_flight = false;
        vt_hide_spinner();
        vt_set_status(r == VC_ERR_BUSY ? "Busy" : "Read failed", COL_RED);
    }
}

static void vt_request_write(void)
{
    if (s_req_in_flight) return;
    if (vesc_config_is_readonly()) {
        vt_set_status("Read-only (FW not supported)", COL_ORANGE);
        return;
    }
    if (!vesc_config_read_ok(s_kind)) {
        /* Guard: writing now would push locally-seeded defaults and wipe the VESC. */
        vt_set_status("Read config first (avoids wiping VESC)", COL_ORANGE);
        return;
    }
    if (!vesc_config_dirty(s_kind)) {
        vt_set_status("No changes", COL_DIM);
        return;
    }
    s_req_in_flight = true;
    vt_set_status("Writing...", COL_CYAN);
    if (!vesc_config_is_emulator()) {
        vt_show_spinner("Writing config...");
    }
    vc_result_t r = vesc_config_write(s_kind, vt_cfg_done, (void *)1);
    if (r != VC_OK) {
        s_req_in_flight = false;
        vt_hide_spinner();
        vt_set_status(r == VC_ERR_NO_READ ? "Read config first (avoids wiping VESC)"
                                          : "Write failed", COL_RED);
    }
}

/* ===================== editors ===================== */

static void mark_row_dirty_color(vt_row_t *r)
{
    bool dirty = vesc_config_param_dirty(s_kind, r->param_idx);
    lv_obj_set_style_text_color(r->name_lbl,
                                lv_color_hex(dirty ? COL_ACCENT : COL_TEXT), 0);
}

static void numeric_refresh(vt_row_t *r)
{
    double stored = vesc_config_get_double(s_kind, r->param_idx);
    if (r->type == VC_T_INT) {
        stored = (double)vesc_config_get_int(s_kind, r->param_idx);
    }
    char buf[24];
    fmt_disp(r, disp_from_stored(r, stored), buf, sizeof buf);
    lv_label_set_text(r->value_lbl, buf);
    mark_row_dirty_color(r);
}

static void numeric_step(vt_row_t *r, int dir)
{
    double stored = (r->type == VC_T_INT)
                        ? (double)vesc_config_get_int(s_kind, r->param_idx)
                        : vesc_config_get_double(s_kind, r->param_idx);
    double disp = disp_from_stored(r, stored) + dir * r->disp_step;
    if (disp < r->disp_min) disp = r->disp_min;
    if (disp > r->disp_max) disp = r->disp_max;
    double nv = stored_from_disp(r, disp);
    if (r->type == VC_T_INT) {
        vesc_config_set_int(s_kind, r->param_idx, (int32_t)lround(nv));
    } else {
        vesc_config_set_double(s_kind, r->param_idx, nv);
    }
    numeric_refresh(r);
}

static void num_inc_cb(lv_event_t *e) { numeric_step((vt_row_t *)lv_event_get_user_data(e), +1); }
static void num_dec_cb(lv_event_t *e) { numeric_step((vt_row_t *)lv_event_get_user_data(e), -1); }

static void enum_cb(lv_event_t *e)
{
    vt_row_t *r = (vt_row_t *)lv_event_get_user_data(e);
    int sel = (int)lv_dropdown_get_selected(r->editor);
    vesc_config_set_int(s_kind, r->param_idx, sel);
    mark_row_dirty_color(r);
}

static void bool_cb(lv_event_t *e)
{
    vt_row_t *r = (vt_row_t *)lv_event_get_user_data(e);
    bool on = lv_obj_has_state(r->editor, LV_STATE_CHECKED);
    vesc_config_set_int(s_kind, r->param_idx, on ? 1 : 0);
    mark_row_dirty_color(r);
}

static void bitfield_cb(lv_event_t *e)
{
    vt_row_t *r = (vt_row_t *)lv_event_get_user_data(e);
    int32_t v = 0;
    for (int b = 0; b < r->bit_count; b++) {
        if (r->bits[b] && lv_obj_has_state(r->bits[b], LV_STATE_CHECKED)) {
            v |= (1 << b);
        }
    }
    vesc_config_set_int(s_kind, r->param_idx, v);
    mark_row_dirty_color(r);
}

static lv_obj_t *row_container(void)
{
    lv_obj_t *row = lv_obj_create(s_list);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_set_style_pad_row(row, 2, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

static lv_obj_t *row_name_label(vt_row_t *r, lv_obj_t *row, const char *txt)
{
    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, NAME_W);
    lv_obj_set_pos(lbl, 0, 4);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserratMedium_16, 0);
    r->name_lbl = lbl;
    return lbl;
}

static lv_obj_t *small_step_btn(lv_obj_t *parent, int x, const char *glyph,
                                uint32_t bg, lv_event_cb_t cb, void *ud)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 54, 44);
    lv_obj_set_pos(btn, x, 0);
    style_btn(btn, bg);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, glyph);
    lv_obj_center(l);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, ud);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_LONG_PRESSED_REPEAT, ud);
    return btn;
}

static void build_numeric_row(vt_row_t *r, const vc_param_t *p)
{
    /* precompute display range/step (mirrors ParamEditDouble.qml) */
    r->editor_scale = (p->editor_scale != 0.0f) ? p->editor_scale : 1.0;
    r->as_pct = (p->flags & VC_FLAG_EDIT_PCT) != 0;
    r->pct_max = p->max;
    r->decimals = p->decimals;
    const char *suf = vesc_config_pool(s_kind, p->suffix_off);
    if (r->as_pct) {
        r->disp_min = 0.0; r->disp_max = 100.0; r->disp_step = 1.0;
        snprintf(r->suffix, sizeof r->suffix, " %%");
    } else {
        r->disp_min = (double)p->min * r->editor_scale;
        r->disp_max = (double)p->max * r->editor_scale;
        r->disp_step = (double)p->step;
        snprintf(r->suffix, sizeof r->suffix, "%s", suf ? suf : "");
    }
    if (r->disp_step <= 0.0) {
        r->disp_step = (r->decimals > 0) ? pow(10.0, -(double)r->decimals) : 1.0;
    }

    lv_obj_t *row = row_container();
    row_name_label(r, row, vesc_config_pool(s_kind, p->longname_off));

    /* Row content width is ~764 px (list 784 - 2*6 pad, row - 2*4 pad); keep the
     * cluster left of ~760 so it clears the right edge and the scrollbar.
     * minus 540..594, value 600..700, plus 704..758. */
    small_step_btn(row, 540, "-", COL_RED, num_dec_cb, r);
    lv_obj_t *val = lv_label_create(row);
    lv_obj_set_pos(val, 600, 10);
    lv_obj_set_width(val, 100);
    lv_obj_set_style_text_color(val, lv_color_hex(COL_CYAN), 0);
    lv_obj_set_style_text_font(val, &lv_font_montserratMedium_16, 0);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, 0);
    r->value_lbl = val;
    small_step_btn(row, 704, "+", COL_CYAN, num_inc_cb, r);

    numeric_refresh(r);
}

static void build_enum_row(vt_row_t *r, const vc_param_t *p)
{
    lv_obj_t *row = row_container();
    row_name_label(r, row, vesc_config_pool(s_kind, p->longname_off));

    /* static scratch: UI runs single-threaded on the LVGL task, and a 512-byte
     * stack buffer here would eat into the 8 KB LVGL task stack. */
    static char opts[512];
    opts[0] = '\0';
    size_t off = 0;
    const vc_table_t *t = vesc_config_table(s_kind);
    if (!t) return;
    for (int i = 0; i < p->enum_count && off < sizeof opts - 1; i++) {
        const char *en = vc_str(t, t->enum_name_offsets[p->enum_off + i]);
        off += snprintf(opts + off, sizeof opts - off, "%s%s", i ? "\n" : "", en);
    }
    if (off == 0) {
        snprintf(opts, sizeof opts, " ");  /* never hand lv_dropdown an empty list */
    }
    lv_obj_t *dd = lv_dropdown_create(row);
    lv_dropdown_set_options(dd, opts);
    lv_obj_set_width(dd, 240);
    lv_obj_set_pos(dd, 510, 0);   /* 510..750, clear of the right edge */
    style_dropdown(dd);
    int sel = vesc_config_get_int(s_kind, r->param_idx);
    if (sel < 0) sel = 0;
    if (sel >= p->enum_count) sel = p->enum_count ? p->enum_count - 1 : 0;
    lv_dropdown_set_selected(dd, sel);
    r->editor = dd;
    lv_obj_add_event_cb(dd, enum_cb, LV_EVENT_VALUE_CHANGED, r);
    mark_row_dirty_color(r);
}

static void build_bool_row(vt_row_t *r, const vc_param_t *p)
{
    lv_obj_t *row = row_container();
    row_name_label(r, row, vesc_config_pool(s_kind, p->longname_off));

    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_set_size(sw, 60, 30);
    lv_obj_set_pos(sw, 690, 6);   /* 690..750, clear of the right edge */
    lv_obj_set_style_bg_color(sw, lv_color_hex(COL_BTN), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(COL_CYAN),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (vesc_config_get_int(s_kind, r->param_idx)) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    r->editor = sw;
    lv_obj_add_event_cb(sw, bool_cb, LV_EVENT_VALUE_CHANGED, r);
    mark_row_dirty_color(r);
}

static void build_bitfield_row(vt_row_t *r, const vc_param_t *p)
{
    lv_obj_t *row = row_container();
    row_name_label(r, row, vesc_config_pool(s_kind, p->longname_off));

    /* switches laid out under the name label, wrapping */
    lv_obj_t *holder = lv_obj_create(row);
    lv_obj_set_pos(holder, 0, 28);
    lv_obj_set_size(holder, 740, LV_SIZE_CONTENT);   /* fit within row content width */
    lv_obj_set_style_bg_opa(holder, 0, 0);
    lv_obj_set_style_border_width(holder, 0, 0);
    lv_obj_set_style_pad_all(holder, 2, 0);
    lv_obj_set_flex_flow(holder, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_clear_flag(holder, LV_OBJ_FLAG_SCROLLABLE);

    const vc_table_t *t = vesc_config_table(s_kind);
    int32_t cur = vesc_config_get_int(s_kind, r->param_idx);
    int n = p->enum_count;
    if (n > VT_MAX_BITS) n = VT_MAX_BITS;
    r->bit_count = n;
    for (int b = 0; b < n; b++) {
        lv_obj_t *cell = lv_obj_create(holder);
        lv_obj_set_size(cell, 180, 36);
        lv_obj_set_style_bg_opa(cell, 0, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_set_style_pad_all(cell, 0, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *sw = lv_switch_create(cell);
        lv_obj_set_size(sw, 44, 22);
        lv_obj_align(sw, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_color(sw, lv_color_hex(COL_BTN), LV_PART_MAIN);
        lv_obj_set_style_bg_color(sw, lv_color_hex(COL_CYAN),
                                  LV_PART_INDICATOR | LV_STATE_CHECKED);
        if (cur & (1 << b)) lv_obj_add_state(sw, LV_STATE_CHECKED);
        r->bits[b] = sw;
        lv_obj_add_event_cb(sw, bitfield_cb, LV_EVENT_VALUE_CHANGED, r);

        lv_obj_t *bl = lv_label_create(cell);
        lv_label_set_text(bl, vc_str(t, t->enum_name_offsets[p->enum_off + b]));
        lv_obj_align(bl, LV_ALIGN_LEFT_MID, 52, 0);
        lv_obj_set_style_text_color(bl, lv_color_hex(COL_DIM), 0);
        lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
    }
    mark_row_dirty_color(r);
}

static void build_string_row(vt_row_t *r, const vc_param_t *p)
{
    lv_obj_t *row = row_container();
    row_name_label(r, row, vesc_config_pool(s_kind, p->longname_off));
    lv_obj_t *val = lv_label_create(row);
    lv_obj_set_pos(val, 360, 6);
    lv_label_set_text(val, "(text - read-only)");
    lv_obj_set_style_text_color(val, lv_color_hex(COL_DIM), 0);
    lv_obj_set_style_text_font(val, &lv_font_montserratMedium_16, 0);
}

static void build_separator(const char *label)
{
    lv_obj_t *sep = lv_label_create(s_list);
    lv_obj_set_width(sep, lv_pct(100));
    lv_label_set_text(sep, label);
    lv_obj_set_style_text_color(sep, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_text_font(sep, &lv_font_montserrat_24, 0);
    lv_obj_set_style_pad_top(sep, 8, 0);
    lv_obj_set_style_pad_bottom(sep, 2, 0);
}

/* ===================== list build ===================== */

static void vt_build_list(void)
{
    if (!s_list || !vesc_config_table(s_kind)) return;
    lv_obj_clean(s_list);
    s_row_count = 0;
    memset(s_rows, 0, sizeof s_rows);

    const vc_group_t *g = vesc_config_group(s_kind, s_group_idx);
    const vc_subgroup_t *sg = g ? vesc_config_subgroup(s_kind, g, s_sub_idx) : NULL;
    if (!sg) return;

    for (int ei = 0; ei < sg->entry_count; ei++) {
        const vc_group_entry_t *e = vesc_config_entry(s_kind, sg, ei);
        if (!e) continue;
        if (e->param_idx < 0) {
            build_separator(vesc_config_pool(s_kind, e->sep_label_off));
            continue;
        }
        if (s_row_count >= VT_MAX_ROWS) {
            build_separator("... (list truncated)");
            break;
        }
        const vc_param_t *p = vesc_config_param(s_kind, e->param_idx);
        if (!p) continue;
        vt_row_t *r = &s_rows[s_row_count++];
        r->param_idx = e->param_idx;
        r->type = p->type;
        switch (p->type) {
        case VC_T_DOUBLE:
        case VC_T_INT:      build_numeric_row(r, p); break;
        case VC_T_ENUM:     build_enum_row(r, p);    break;
        case VC_T_BOOL:     build_bool_row(r, p);    break;
        case VC_T_BITFIELD: build_bitfield_row(r, p); break;
        case VC_T_QSTRING:  build_string_row(r, p);  break;
        default:            s_row_count--;           break;
        }
    }
    lv_obj_scroll_to_y(s_list, 0, LV_ANIM_OFF);
}

/* ===================== selectors ===================== */

static void vt_populate_selectors(void)
{
    if (!vesc_config_ready()) return;
    static char opts[512];
    opts[0] = '\0';

    size_t off = 0;
    uint16_t gc = vesc_config_group_count(s_kind);
    for (int gi = 0; gi < gc && off < sizeof opts - 1; gi++) {
        const vc_group_t *g = vesc_config_group(s_kind, gi);
        off += snprintf(opts + off, sizeof opts - off, "%s%s",
                        gi ? "\n" : "", vesc_config_pool(s_kind, g->name_off));
    }
    lv_dropdown_set_options(s_dd_group, opts);
    if (s_group_idx >= gc) s_group_idx = 0;
    lv_dropdown_set_selected(s_dd_group, s_group_idx);

    const vc_group_t *g = vesc_config_group(s_kind, s_group_idx);
    off = 0;
    int sc = g ? g->sub_count : 0;
    for (int si = 0; si < sc && off < sizeof opts - 1; si++) {
        const vc_subgroup_t *sg = vesc_config_subgroup(s_kind, g, si);
        off += snprintf(opts + off, sizeof opts - off, "%s%s",
                        si ? "\n" : "", vesc_config_pool(s_kind, sg->name_off));
    }
    if (sc == 0) snprintf(opts, sizeof opts, " ");
    lv_dropdown_set_options(s_dd_sub, opts);
    if (s_sub_idx >= sc) s_sub_idx = 0;
    lv_dropdown_set_selected(s_dd_sub, s_sub_idx);
}

/* Write is only safe after a successful read-back of the current kind (else the
 * full-blob SET would overwrite untouched params with local defaults). */
static void vt_update_write_enabled(void)
{
    if (!s_write_btn) return;
    bool can = vesc_config_read_ok(s_kind) && !vesc_config_is_readonly();
    if (can) {
        lv_obj_clear_state(s_write_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(s_write_btn, LV_STATE_DISABLED);
    }
}

static void vt_update_restore_enabled(void)
{
    if (!s_restore_btn) return;
    if (vesc_config_has_backup(s_kind)) {
        lv_obj_clear_state(s_restore_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(s_restore_btn, LV_STATE_DISABLED);
    }
}

static void vt_update_kind_buttons(void)
{
    style_btn(s_btn_motor, s_kind == VC_MCCONF ? COL_ACCENT : COL_BTN);
    style_btn(s_btn_app,   s_kind == VC_APPCONF ? COL_ACCENT : COL_BTN);
    if (s_kind == VC_MCCONF) {
        lv_obj_set_style_text_color(s_btn_motor, lv_color_hex(0x000000), 0);
    }
    if (s_kind == VC_APPCONF) {
        lv_obj_set_style_text_color(s_btn_app, lv_color_hex(0x000000), 0);
    }
}

static void switch_kind(vc_kind_t kind)
{
    if (kind == s_kind) return;
    s_kind = kind;
    s_group_idx = 0;
    s_sub_idx = 0;
    vt_update_kind_buttons();
    vt_populate_selectors();
    vt_build_list();
    vt_update_write_enabled();
    vt_update_restore_enabled();
    if (!s_loaded[kind]) {
        s_retries = 0;
        vt_request_read(kind, false);  /* lazy-load on first switch */
    }
}

/* ===================== event callbacks ===================== */

static void kind_motor_cb(lv_event_t *e) { (void)e; switch_kind(VC_MCCONF); }
static void kind_app_cb(lv_event_t *e)   { (void)e; switch_kind(VC_APPCONF); }

static void group_cb(lv_event_t *e)
{
    (void)e;
    s_group_idx = (int)lv_dropdown_get_selected(s_dd_group);
    s_sub_idx = 0;
    vt_populate_selectors();
    vt_build_list();
}

static void sub_cb(lv_event_t *e)
{
    (void)e;
    s_sub_idx = (int)lv_dropdown_get_selected(s_dd_sub);
    vt_build_list();
}

static void read_cb(lv_event_t *e)    { (void)e; s_retries = 0; vt_request_read(s_kind, false); }
static void write_cb(lv_event_t *e)   { (void)e; s_retries = 0; vt_request_write(); }
static void default_cb(lv_event_t *e) { (void)e; s_retries = 0; vt_request_read(s_kind, true); }

/* ===================== FOC detection (manual + auto) ===================== */

/* manual measurement results (raw SI units) */
static double s_m_r, s_m_l, s_m_lddiff, s_m_lambda, s_m_kp, s_m_ki, s_m_gain;
static uint8_t s_m_hall[8];
static bool   s_m_hall_ok;
static double s_m_enc_off, s_m_enc_ratio;
static int    s_m_enc_inv;
static bool   s_m_enc_ok;
static int    s_tc_us = 1000;
static lv_obj_t *s_foc_result_lbl, *s_tc_lbl;

/* async hand-off from the CAN/timer task to LVGL */
static void detect_apply(void *p);
static void foc_meas_apply(void *p);
static volatile int s_meas_done_type;  /* 1 RL, 2 linkage, 3 encoder, 4 hall */
static volatile int s_meas_done_ok;
static double  s_meas_da, s_meas_db, s_meas_dc;
static uint8_t s_meas_done_hall[8];
static int     s_meas_done_hres;

static void vt_detect_done(int result, void *user)   /* CAN/timer task */
{
    (void)user;
    s_detect_result = result;
    lv_async_call(detect_apply, NULL);
}

static void rl_cb(int ok, double a, double b, double c, void *u)
{ (void)u; s_meas_done_ok=ok; s_meas_da=a; s_meas_db=b; s_meas_dc=c; s_meas_done_type=1; lv_async_call(foc_meas_apply, NULL); }
static void link_cb(int ok, double a, double b, double c, void *u)
{ (void)u; (void)b; (void)c; s_meas_done_ok=ok; s_meas_da=a; s_meas_done_type=2; lv_async_call(foc_meas_apply, NULL); }
static void enc_cb(int ok, double a, double b, double c, void *u)
{ (void)u; s_meas_done_ok=ok; s_meas_da=a; s_meas_db=b; s_meas_dc=c; s_meas_done_type=3; lv_async_call(foc_meas_apply, NULL); }
static void hall_cb(int ok, const uint8_t *table, int res, void *u)
{ (void)u; s_meas_done_ok=ok; if (ok && table) for (int i=0;i<8;i++) s_meas_done_hall[i]=table[i]; s_meas_done_hres=res; s_meas_done_type=4; lv_async_call(foc_meas_apply, NULL); }

static void foc_calc(void)
{
    if (s_m_l > 0.0 && s_tc_us > 0) {
        double bw = 1.0 / ((double)s_tc_us * 1e-6);
        s_m_kp = s_m_l * bw;
        s_m_ki = s_m_r * bw;
    }
    if (s_m_lambda > 1e-10) s_m_gain = 1e-3 / (s_m_lambda * s_m_lambda);
}

static void foc_result_refresh(void)
{
    if (!s_foc_result_lbl) return;
    char b[256];
    snprintf(b, sizeof b,
             "R     : %.2f mOhm\n"
             "L     : %.2f uH\n"
             "Ld-Lq : %.2f uH\n"
             "lambda: %.3f mWb\n"
             "Kp    : %.4f\n"
             "Ki    : %.2f\n"
             "Hall  : %s\n"
             "Enc   : %s",
             s_m_r * 1e3, s_m_l * 1e6, s_m_lddiff * 1e6, s_m_lambda * 1e3,
             s_m_kp, s_m_ki,
             s_m_hall_ok ? "detected" : "--",
             s_m_enc_ok ? "detected" : "--");
    lv_label_set_text(s_foc_result_lbl, b);
}

static void foc_meas_apply(void *p)   /* LVGL task */
{
    (void)p;
    if (!s_alive) return;
    vt_hide_spinner();
    s_req_in_flight = false;
    if (!s_meas_done_ok) { vt_set_status("Measurement timed out", COL_RED); return; }
    switch (s_meas_done_type) {
    case 1:
        s_m_r = s_meas_da; s_m_l = s_meas_db; s_m_lddiff = s_meas_dc;
        foc_calc(); vt_set_status("R/L measured", COL_ACCENT);
        break;
    case 2:
        s_m_lambda = s_meas_da; foc_calc(); vt_set_status("Flux linkage measured", COL_ACCENT);
        break;
    case 3:
        if (s_meas_da > 1000.0) { s_m_enc_ok=false; vt_set_status("Encoder detect failed", COL_RED); }
        else { s_m_enc_off=s_meas_da; s_m_enc_ratio=s_meas_db; s_m_enc_inv=(int)s_meas_dc; s_m_enc_ok=true; vt_set_status("Encoder detected", COL_ACCENT); }
        break;
    case 4:
        if (s_meas_done_hres != 0) { s_m_hall_ok=false; vt_set_status("Hall detect failed", COL_RED); }
        else { for (int i=0;i<8;i++) s_m_hall[i]=s_meas_done_hall[i]; s_m_hall_ok=true; vt_set_status("Hall sensors detected", COL_ACCENT); }
        break;
    }
    foc_result_refresh();
}

static void detect_dlg_close(void)
{
    if (s_detect_dlg) {
        lv_obj_del(s_detect_dlg);
        s_detect_dlg = NULL;
        s_detect_pl_lbl = NULL;
        s_foc_result_lbl = NULL;
        s_tc_lbl = NULL;
    }
}

static void detect_pl_refresh(void)
{ if (s_detect_pl_lbl) { char b[16]; snprintf(b, sizeof b, "%d W", s_detect_power_loss); lv_label_set_text(s_detect_pl_lbl, b); } }
static void detect_pl_inc_cb(lv_event_t *e)
{ (void)e; s_detect_power_loss += 5; if (s_detect_power_loss > 500) s_detect_power_loss = 500; detect_pl_refresh(); }
static void detect_pl_dec_cb(lv_event_t *e)
{ (void)e; s_detect_power_loss -= 5; if (s_detect_power_loss < 5) s_detect_power_loss = 5; detect_pl_refresh(); }
static void detect_cancel_cb(lv_event_t *e) { (void)e; detect_dlg_close(); }

static void tc_refresh(void)
{ if (s_tc_lbl) { char b[16]; snprintf(b, sizeof b, "%d", s_tc_us); lv_label_set_text(s_tc_lbl, b); } }
static void tc_inc_cb(lv_event_t *e)
{ (void)e; s_tc_us += 100; if (s_tc_us > 5000) s_tc_us = 5000; tc_refresh(); foc_calc(); foc_result_refresh(); }
static void tc_dec_cb(lv_event_t *e)
{ (void)e; s_tc_us -= 100; if (s_tc_us < 100) s_tc_us = 100; tc_refresh(); foc_calc(); foc_result_refresh(); }

/* ---- auto (APPLY_ALL_FOC) ---- */
static void detect_run_cb(lv_event_t *e)
{
    (void)e;
    detect_dlg_close();
    if (s_req_in_flight) return;
    s_req_in_flight = true;
    vt_set_status("Auto detect - motor spins...", COL_CYAN);
    vt_show_spinner("Auto detect (R/L/Flux + sensors)...\nMOTOR WILL SPIN");
    vc_result_t r = vesc_config_detect_foc(s_detect_can_other, (double)s_detect_power_loss,
                                           0, 0, 0, 0, vt_detect_done, NULL);
    if (r != VC_OK) {
        s_req_in_flight = false;
        vt_hide_spinner();
        vt_set_status(r == VC_ERR_READONLY ? "Detection needs a real VESC" : "Detect failed",
                      COL_RED);
    }
}

static void detect_apply(void *p)   /* LVGL task */
{
    (void)p;
    if (!s_alive) return;
    vt_hide_spinner();
    s_req_in_flight = false;
    int res = s_detect_result;
    if (res >= 0) {
        vt_set_status("Auto detect OK - reloading config", COL_ACCENT);
        s_retries = 0;
        vt_request_read(VC_MCCONF, false);
    } else {
        const char *m = "Detection failed";
        if (res == VC_DETECT_TIMEOUT) m = "Detection timed out";
        else if (res == -1)   m = "Persistent fault on VESC";
        else if (res == -10)  m = "Flux linkage detect failed";
        else if (res <= -100) m = "Fault during detect - check motor/wiring";
        char buf[56];
        snprintf(buf, sizeof buf, "%s (%d)", m, res);
        vt_set_status(buf, COL_RED);
    }
}

/* ---- manual measurement buttons ---- */
static bool meas_start_guard(void) { return !s_req_in_flight; }

static void meas_rl_btn(lv_event_t *e)
{
    (void)e; if (!meas_start_guard()) return;
    s_req_in_flight = true;
    vt_set_status("Measuring R/L...", COL_CYAN);
    vt_show_spinner("Measuring R / L...\n(motor energizes briefly)");
    if (vesc_config_measure_rl(rl_cb, NULL) != VC_OK) {
        s_req_in_flight = false; vt_hide_spinner(); vt_set_status("R/L request failed", COL_RED);
    }
}

static void meas_lambda_btn(lv_event_t *e)
{
    (void)e; if (!meas_start_guard()) return;
    if (s_m_r <= 0.0 || s_m_l <= 0.0) { vt_set_status("Measure R/L first", COL_ORANGE); return; }
    int idx = vesc_config_find_param(VC_MCCONF, "l_current_max");
    double cur = (idx >= 0) ? vesc_config_get_double(VC_MCCONF, idx) / 3.0 : 10.0;
    if (cur <= 0.0) cur = 10.0;
    s_req_in_flight = true;
    vt_set_status("Measuring flux linkage...", COL_CYAN);
    vt_show_spinner("Measuring flux linkage...\nMOTOR SPINS");
    if (vesc_config_measure_linkage(cur, 2000.0, 0.3, s_m_r, s_m_l, link_cb, NULL) != VC_OK) {
        s_req_in_flight = false; vt_hide_spinner(); vt_set_status("Flux request failed", COL_RED);
    }
}

static void meas_hall_btn(lv_event_t *e)
{
    (void)e; if (!meas_start_guard()) return;
    s_req_in_flight = true;
    vt_set_status("Detecting hall...", COL_CYAN);
    vt_show_spinner("Detecting hall sensors...\n(motor moves)");
    if (vesc_config_measure_hall(10.0, hall_cb, NULL) != VC_OK) {
        s_req_in_flight = false; vt_hide_spinner(); vt_set_status("Hall request failed", COL_RED);
    }
}

static void meas_enc_btn(lv_event_t *e)
{
    (void)e; if (!meas_start_guard()) return;
    s_req_in_flight = true;
    vt_set_status("Detecting encoder...", COL_CYAN);
    vt_show_spinner("Detecting encoder...\nMOTOR SPINS");
    if (vesc_config_measure_encoder(10.0, enc_cb, NULL) != VC_OK) {
        s_req_in_flight = false; vt_hide_spinner(); vt_set_status("Encoder request failed", COL_RED);
    }
}

static void calc_btn(lv_event_t *e)
{ (void)e; foc_calc(); foc_result_refresh(); vt_set_status("Calculated Kp/Ki/observer gain", COL_ACCENT); }

static void apply_manual_btn(lv_event_t *e)
{
    (void)e;
    if (s_req_in_flight) return;
    bool any = false;
    if (s_m_r > 0.0 && s_m_l > 0.0) {
        foc_calc();
        vesc_config_set_double_by_name(VC_MCCONF, "foc_motor_r", s_m_r);
        vesc_config_set_double_by_name(VC_MCCONF, "foc_motor_l", s_m_l);
        vesc_config_set_double_by_name(VC_MCCONF, "foc_motor_ld_lq_diff", s_m_lddiff);
        vesc_config_set_double_by_name(VC_MCCONF, "foc_current_kp", s_m_kp);
        vesc_config_set_double_by_name(VC_MCCONF, "foc_current_ki", s_m_ki);
        any = true;
    }
    if (s_m_lambda > 1e-10) {
        vesc_config_set_double_by_name(VC_MCCONF, "foc_motor_flux_linkage", s_m_lambda);
        vesc_config_set_double_by_name(VC_MCCONF, "foc_observer_gain", s_m_gain * 1e6);
        any = true;
    }
    if (s_m_hall_ok) {
        for (int i = 0; i < 8; i++) {
            char nm[24]; snprintf(nm, sizeof nm, "foc_hall_table__%d", i);
            vesc_config_set_int_by_name(VC_MCCONF, nm, s_m_hall[i]);
        }
        vesc_config_set_int_by_name(VC_MCCONF, "foc_sensor_mode", 2);  /* Hall */
        any = true;
    }
    if (s_m_enc_ok) {
        vesc_config_set_double_by_name(VC_MCCONF, "foc_encoder_offset", s_m_enc_off);
        vesc_config_set_double_by_name(VC_MCCONF, "foc_encoder_ratio", s_m_enc_ratio);
        vesc_config_set_int_by_name(VC_MCCONF, "foc_encoder_inverted", s_m_enc_inv);
        vesc_config_set_int_by_name(VC_MCCONF, "foc_sensor_mode", 1);  /* Encoder */
        any = true;
    }
    if (!any) { vt_set_status("Nothing measured yet", COL_ORANGE); return; }
    detect_dlg_close();
    s_kind = VC_MCCONF;
    if (s_list) vt_build_list();
    vt_request_write();   /* push the applied params to the VESC */
}

/* dialog button helpers (arbitrary y, unlike the row stepper) */
static lv_obj_t *dlg_step_btn(lv_obj_t *parent, int x, int y, const char *g,
                              uint32_t bg, lv_event_cb_t cb)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, 50, 40);
    lv_obj_set_pos(b, x, y);
    style_btn(b, bg);
    lv_obj_t *l = lv_label_create(b); lv_label_set_text(l, g); lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(b, cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    return b;
}

static lv_obj_t *dlg_btn(lv_obj_t *parent, int x, int y, int w, const char *txt,
                         uint32_t bg, lv_event_cb_t cb)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, w, 44);
    lv_obj_set_pos(b, x, y);
    style_btn(b, bg);
    lv_obj_set_style_text_font(b, &lv_font_montserratMedium_16, 0);
    lv_obj_t *l = lv_label_create(b); lv_label_set_text(l, txt); lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

static void detect_cb(lv_event_t *e)
{
    (void)e;
    if (vesc_config_is_emulator()) { vt_set_status("Detection needs a real VESC", COL_ORANGE); return; }
    if (s_detect_dlg || s_req_in_flight) return;

    lv_obj_t *dlg = lv_obj_create(s_screen);
    s_detect_dlg = dlg;
    lv_obj_set_size(dlg, 760, 462);
    lv_obj_center(dlg);
    lv_obj_set_style_bg_color(dlg, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_color(dlg, lv_color_hex(COL_ORANGE), 0);
    lv_obj_set_style_border_width(dlg, 2, 0);
    lv_obj_set_style_radius(dlg, 8, 0);
    lv_obj_set_style_pad_all(dlg, 0, 0);   /* children use absolute coords; theme's 20px card pad would push the bottom/right buttons off-window */
    lv_obj_clear_flag(dlg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(dlg);
    lv_label_set_text(t, "FOC Detection - motor will SPIN");
    lv_obj_set_pos(t, 12, 6);
    lv_obj_set_style_text_color(t, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_24, 0);

    s_foc_result_lbl = lv_label_create(dlg);
    lv_obj_set_pos(s_foc_result_lbl, 12, 44);
    lv_obj_set_width(s_foc_result_lbl, 400);
    lv_obj_set_style_text_color(s_foc_result_lbl, lv_color_hex(COL_CYAN), 0);
    lv_obj_set_style_text_font(s_foc_result_lbl, &lv_font_montserratMedium_16, 0);
    foc_result_refresh();

    /* tc stepper (current-loop time constant, for Kp/Ki) */
    lv_obj_t *tcl = lv_label_create(dlg);
    lv_label_set_text(tcl, "tc (us)"); lv_obj_set_pos(tcl, 12, 302);
    lv_obj_set_style_text_color(tcl, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(tcl, &lv_font_montserratMedium_16, 0);
    dlg_step_btn(dlg, 120, 296, "-", COL_RED, tc_dec_cb);
    s_tc_lbl = lv_label_create(dlg);
    lv_obj_set_pos(s_tc_lbl, 178, 304); lv_obj_set_width(s_tc_lbl, 64);
    lv_obj_set_style_text_color(s_tc_lbl, lv_color_hex(COL_CYAN), 0);
    lv_obj_set_style_text_font(s_tc_lbl, &lv_font_montserratMedium_16, 0);
    lv_obj_set_style_text_align(s_tc_lbl, LV_TEXT_ALIGN_CENTER, 0);
    dlg_step_btn(dlg, 252, 296, "+", COL_CYAN, tc_inc_cb);
    tc_refresh();

    /* power-loss stepper (for the auto path) */
    lv_obj_t *pll = lv_label_create(dlg);
    lv_label_set_text(pll, "Loss (W)"); lv_obj_set_pos(pll, 12, 358);
    lv_obj_set_style_text_color(pll, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(pll, &lv_font_montserratMedium_16, 0);
    dlg_step_btn(dlg, 120, 352, "-", COL_RED, detect_pl_dec_cb);
    s_detect_pl_lbl = lv_label_create(dlg);
    lv_obj_set_pos(s_detect_pl_lbl, 178, 360); lv_obj_set_width(s_detect_pl_lbl, 64);
    lv_obj_set_style_text_color(s_detect_pl_lbl, lv_color_hex(COL_ORANGE), 0);
    lv_obj_set_style_text_font(s_detect_pl_lbl, &lv_font_montserratMedium_16, 0);
    lv_obj_set_style_text_align(s_detect_pl_lbl, LV_TEXT_ALIGN_CENTER, 0);
    dlg_step_btn(dlg, 252, 352, "+", COL_CYAN, detect_pl_inc_cb);
    detect_pl_refresh();

    /* right column: step-by-step manual flow + auto + close */
    dlg_btn(dlg, 430, 44,  300, "1. Measure R / L",          COL_BTN, meas_rl_btn);
    dlg_btn(dlg, 430, 96,  300, "2. Measure Flux (lambda)",  COL_BTN, meas_lambda_btn);
    dlg_btn(dlg, 430, 148, 300, "Detect Hall sensors",       COL_BTN, meas_hall_btn);
    dlg_btn(dlg, 430, 200, 300, "Detect Encoder",            COL_BTN, meas_enc_btn);
    dlg_btn(dlg, 430, 252, 300, "3. Calculate Kp/Ki/gain",   COL_BTN, calc_btn);
    lv_obj_t *ap = dlg_btn(dlg, 430, 304, 300, "4. Apply to VESC", COL_ACCENT, apply_manual_btn);
    lv_obj_set_style_text_color(ap, lv_color_hex(0x000000), 0);
    dlg_btn(dlg, 430, 356, 300, "Auto: Run & Apply (all)",   COL_CYAN, detect_run_cb);
    dlg_btn(dlg, 430, 408, 300, "Close",                     COL_BTN, detect_cancel_cb);
}

static void restore_dlg_close(void)
{
    if (s_restore_dlg) { lv_obj_del(s_restore_dlg); s_restore_dlg = NULL; }
}

static void restore_cancel_cb(lv_event_t *e) { (void)e; restore_dlg_close(); }

static void restore_pick_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    restore_dlg_close();
    if (s_req_in_flight) return;
    vc_result_t r = vesc_config_restore_index(s_kind, idx);
    if (r == VC_OK) {
        vt_build_list();
        vt_update_write_enabled();
        vt_set_status("Backup loaded - press Write to apply", COL_ACCENT);
    } else if (r == VC_ERR_SIGNATURE) {
        vt_set_status("Backup is from another FW version", COL_ORANGE);
    } else {
        vt_set_status("Restore failed", COL_RED);
    }
}

static void restore_cb(lv_event_t *e)
{
    (void)e;
    if (s_req_in_flight || s_restore_dlg) return;
    int cnt = vesc_config_backup_count(s_kind);
    if (cnt <= 0) { vt_set_status("No backup available", COL_DIM); return; }

    lv_obj_t *dlg = lv_obj_create(s_screen);
    s_restore_dlg = dlg;
    lv_obj_set_size(dlg, 440, 420);
    lv_obj_center(dlg);
    lv_obj_set_style_bg_color(dlg, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_color(dlg, lv_color_hex(COL_CYAN), 0);
    lv_obj_set_style_border_width(dlg, 2, 0);
    lv_obj_set_style_radius(dlg, 8, 0);
    lv_obj_set_style_pad_all(dlg, 0, 0);   /* children use absolute coords; theme's 20px card pad would push the bottom/right buttons off-window */
    lv_obj_clear_flag(dlg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(dlg);
    lv_label_set_text(t, s_kind == VC_MCCONF ? "Restore Motor backup"
                                             : "Restore App backup");
    lv_obj_set_pos(t, 12, 8);
    lv_obj_set_style_text_color(t, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_24, 0);

    /* scrollable list of backups, newest first */
    lv_obj_t *list = lv_obj_create(dlg);
    lv_obj_set_pos(list, 10, 46);
    lv_obj_set_size(list, 420, 300);
    lv_obj_set_style_bg_opa(list, 0, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 4, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

    for (int i = 0; i < cnt; i++) {
        uint32_t seq = vesc_config_backup_seq(s_kind, i);
        char lbl[32];
        snprintf(lbl, sizeof lbl, "#%u%s", (unsigned)seq, i == 0 ? "  (newest)" : "");
        lv_obj_t *b = lv_btn_create(list);
        lv_obj_set_width(b, lv_pct(100));
        lv_obj_set_height(b, 44);
        style_btn(b, COL_BTN);
        lv_obj_set_style_text_font(b, &lv_font_montserratMedium_16, 0);
        lv_obj_t *l = lv_label_create(b); lv_label_set_text(l, lbl); lv_obj_center(l);
        lv_obj_add_event_cb(b, restore_pick_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    lv_obj_t *cnl = lv_btn_create(dlg);
    lv_obj_set_pos(cnl, 12, 356);
    lv_obj_set_size(cnl, 416, 50);
    style_btn(cnl, COL_BTN);
    lv_obj_t *cll = lv_label_create(cnl); lv_label_set_text(cll, "Cancel"); lv_obj_center(cll);
    lv_obj_add_event_cb(cnl, restore_cancel_cb, LV_EVENT_CLICKED, NULL);
}

static void leave_to_dashboard(void)
{
    lv_scr_load_anim(guider_ui.dashboard, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

static void discard_msgbox_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_current_target(e);
    uint16_t id = lv_msgbox_get_active_btn(mbox);
    lv_msgbox_close(mbox);
    if (id == 0) {  /* Discard */
        leave_to_dashboard();
    }
}

static void back_cb(lv_event_t *e)
{
    (void)e;
    if (vesc_config_dirty(VC_MCCONF) || vesc_config_dirty(VC_APPCONF)) {
        static const char *btns[] = {"Discard", "Cancel", ""};
        lv_obj_t *mbox = lv_msgbox_create(NULL, "Unsaved changes",
                                          "Discard unsaved edits and leave?", btns, false);
        lv_obj_center(mbox);
        lv_obj_add_event_cb(mbox, discard_msgbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }
    leave_to_dashboard();
}

static void screen_unloaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    s_alive = false;
    if (s_ready_poll) {
        lv_timer_del(s_ready_poll);
        s_ready_poll = NULL;
    }
    if (s_fs_timer) {
        lv_timer_del(s_fs_timer);
        s_fs_timer = NULL;
    }
    if (s_screen) {
        lv_obj_del_async(s_screen);   /* also frees the modals/dialog children */
        s_screen = NULL;
    }
    s_spinner_modal = NULL;
    s_fmt_modal = NULL;
    s_detect_dlg = NULL;
    s_detect_pl_lbl = NULL;
    s_restore_dlg = NULL;
    s_list = NULL;
    s_req_in_flight = false;
}

/* ===================== build & entry ===================== */

static lv_obj_t *make_header_btn(const char *txt, int x, int w, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(s_screen);
    lv_obj_set_pos(btn, x, 8);
    lv_obj_set_size(btn, w, 40);
    style_btn(btn, COL_BTN);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

/* Header tool buttons → sibling screens. Opening either unloads (and thus
 * destroys, via screen_unloaded_cb) this config screen; their Back returns
 * here by rebuilding it through run_vesc_tool_menu(). */
static void lisp_open_cb(lv_event_t *e) { (void)e; show_lisp_editor(); }
static void rt_open_cb(lv_event_t *e)   { (void)e; show_realtime_viewer(); }

static void vt_build_screen(void)
{
    s_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_screen, 800, 480);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_screen, 255, 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* header: back, title, emulator banner */
    make_header_btn("Back", 8, 90, back_cb);

    s_title = lv_label_create(s_screen);
    lv_label_set_text(s_title, "VESC Config");
    lv_obj_set_pos(s_title, 110, 16);
    lv_obj_set_style_text_color(s_title, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_24, 0);

    s_emu_banner = lv_label_create(s_screen);
    lv_label_set_text(s_emu_banner, "");
    lv_obj_set_pos(s_emu_banner, 300, 18);
    lv_obj_set_width(s_emu_banner, 270);   /* shrunk to clear the tool buttons */
    lv_obj_set_style_text_color(s_emu_banner, lv_color_hex(COL_ORANGE), 0);
    lv_obj_set_style_text_font(s_emu_banner, &lv_font_montserratMedium_16, 0);
    lv_obj_set_style_text_align(s_emu_banner, LV_TEXT_ALIGN_RIGHT, 0);

    /* tool buttons: LISP editor + realtime viewer (top-right) */
    make_header_btn("LISP", 576, 100, lisp_open_cb);
    make_header_btn("Realtime", 680, 110, rt_open_cb);

    /* selector row */
    s_btn_motor = lv_btn_create(s_screen);
    lv_obj_set_pos(s_btn_motor, 8, 56);
    lv_obj_set_size(s_btn_motor, 90, 40);
    style_btn(s_btn_motor, COL_ACCENT);
    lv_obj_t *lm = lv_label_create(s_btn_motor);
    lv_label_set_text(lm, "Motor");
    lv_obj_center(lm);
    lv_obj_add_event_cb(s_btn_motor, kind_motor_cb, LV_EVENT_CLICKED, NULL);

    s_btn_app = lv_btn_create(s_screen);
    lv_obj_set_pos(s_btn_app, 104, 56);
    lv_obj_set_size(s_btn_app, 90, 40);
    style_btn(s_btn_app, COL_BTN);
    lv_obj_t *la = lv_label_create(s_btn_app);
    lv_label_set_text(la, "App");
    lv_obj_center(la);
    lv_obj_add_event_cb(s_btn_app, kind_app_cb, LV_EVENT_CLICKED, NULL);

    s_dd_group = lv_dropdown_create(s_screen);
    lv_obj_set_pos(s_dd_group, 204, 56);
    lv_obj_set_size(s_dd_group, 270, 40);
    style_dropdown(s_dd_group);
    lv_obj_add_event_cb(s_dd_group, group_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_dd_sub = lv_dropdown_create(s_screen);
    lv_obj_set_pos(s_dd_sub, 484, 56);
    lv_obj_set_size(s_dd_sub, 300, 40);   /* 484..784, clear of the right edge */
    style_dropdown(s_dd_sub);
    lv_obj_add_event_cb(s_dd_sub, sub_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* parameter list */
    s_list = lv_obj_create(s_screen);
    lv_obj_set_pos(s_list, 8, 104);
    lv_obj_set_size(s_list, 784, 304);
    lv_obj_set_style_bg_color(s_list, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_style_radius(s_list, 6, 0);
    lv_obj_set_style_pad_all(s_list, 6, 0);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(s_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_list, LV_SCROLLBAR_MODE_AUTO);

    /* footer: Read | Write | Defaults | Detect | Restore */
    make_footer_btn("Read", 8, 86, COL_BTN, read_cb);
    s_write_btn = make_footer_btn("Write", 98, 86, COL_CYAN, write_cb);
    lv_obj_add_state(s_write_btn, LV_STATE_DISABLED);  /* enabled only after a read-back */
    s_default_btn = make_footer_btn("Defaults", 188, 92, COL_BTN, default_cb);
    s_detect_btn  = make_footer_btn("Detect", 284, 92, COL_BTN, detect_cb);
    s_restore_btn = make_footer_btn("Restore", 380, 100, COL_BTN, restore_cb);
    lv_obj_add_state(s_restore_btn, LV_STATE_DISABLED);

    s_status = lv_label_create(s_screen);
    lv_obj_set_pos(s_status, 488, 428);
    lv_obj_set_width(s_status, 304);
    lv_label_set_text(s_status, "");
    lv_obj_set_style_text_color(s_status, lv_color_hex(COL_DIM), 0);
    lv_obj_set_style_text_font(s_status, &lv_font_montserratMedium_16, 0);

    lv_obj_add_event_cb(s_screen, screen_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
}

/* Configure the screen once a config table is available (called either
 * immediately on open, or from the readiness poll after a late FW detect). */
static void vt_enter_ready(void)
{
    uint8_t maj = 0, min_ = 0;
    vesc_config_get_fw(&maj, &min_);
    char tbuf[40];
    snprintf(tbuf, sizeof tbuf, "VESC Config  %u.%02u", maj, min_);
    lv_label_set_text(s_title, tbuf);

    if (vesc_config_is_readonly()) {
        lv_label_set_text(s_emu_banner, "Read-only (FW fallback)");
        lv_obj_add_state(s_write_btn, LV_STATE_DISABLED);
        lv_obj_add_state(s_default_btn, LV_STATE_DISABLED);
    }
    vt_update_kind_buttons();
    vt_populate_selectors();
    vt_build_list();
    vt_update_restore_enabled();
    s_retries = 0;
    vt_request_read(s_kind, false);
}

static void ready_poll_cb(lv_timer_t *t)
{
    if (!s_alive) {
        lv_timer_del(t);
        s_ready_poll = NULL;
        return;
    }
    if (vesc_config_ready()) {
        lv_timer_del(t);
        s_ready_poll = NULL;
        vt_enter_ready();
    }
}

static void start_ready_poll(void)
{
    if (!s_ready_poll) {
        s_ready_poll = lv_timer_create(ready_poll_cb, 400, NULL);
    }
}

/* ===================== backup-FS format notice ===================== */

static void fs_hide_format_modal(void)
{
    if (s_fmt_modal) { lv_obj_del(s_fmt_modal); s_fmt_modal = NULL; }
}

static void fs_show_format_modal(void)
{
    if (s_fmt_modal) return;
    s_fmt_modal = lv_obj_create(s_screen);
    lv_obj_set_size(s_fmt_modal, 800, 480);
    lv_obj_set_pos(s_fmt_modal, 0, 0);
    lv_obj_set_style_bg_color(s_fmt_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_fmt_modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_fmt_modal, 0, 0);
    lv_obj_add_flag(s_fmt_modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_fmt_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *l = lv_label_create(s_fmt_modal);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(l, 720);
    lv_label_set_text(l, "Formatting backup storage...\n"
                         "One-time setup, please wait (up to ~1 min).\n"
                         "Do not power off.");
    lv_obj_set_style_text_color(l, lv_color_hex(COL_ORANGE), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l);
    /* Force-draw now: the format that follows starves rendering (flash erase,
     * AUTO_SUSPEND off), so this notice must hit the panel before the freeze. */
    lv_refr_now(NULL);
}

static void fs_timer_cb(lv_timer_t *t)
{
    if (!s_alive) { lv_timer_del(t); s_fs_timer = NULL; return; }
    int st = vesc_config_fs_state();
    if (st == VC_FS_FORMATTING) {
        fs_show_format_modal();
    } else if (st == VC_FS_READY) {
        fs_hide_format_modal();
        vt_update_restore_enabled();
        lv_timer_del(t); s_fs_timer = NULL;
    } else if (st == VC_FS_FAIL) {
        fs_hide_format_modal();
        lv_timer_del(t); s_fs_timer = NULL;
    }
}

static void start_fs_timer(void)
{
    if (!s_fs_timer) s_fs_timer = lv_timer_create(fs_timer_cb, 200, NULL);
}

void run_vesc_tool_menu(void)
{
    if (s_screen) return;  /* re-entrancy guard */

    /* s_kind / s_group_idx / s_sub_idx are NOT reset here: they persist in RAM
     * while the device is powered, so reopening the menu returns to the page you
     * were last editing. (Stale indices are clamped by vt_populate_selectors.) */
    s_loaded[0] = s_loaded[1] = false;
    s_req_in_flight = false;
    s_spinner_modal = NULL;
    s_ready_poll = NULL;
    s_retries = 0;

    vt_build_screen();
    s_alive = true;

    /* Mount (and first-time format) the backup filesystem in the background;
     * the fs timer shows a "Formatting..." notice if a format is needed. */
    vesc_config_fs_ensure();
    start_fs_timer();

    if (vesc_config_is_emulator()) {
        lv_label_set_text(s_emu_banner, "Emulator - local config");
        lv_obj_add_state(s_detect_btn, LV_STATE_DISABLED);  /* no real motor */
    }

    if (vesc_config_ready()) {
        vt_enter_ready();
    } else {
        vt_set_status("VESC not detected - tap Read", COL_ORANGE);
        vesc_config_probe_fw();
        start_ready_poll();
    }

    lv_scr_load_anim(s_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

/* footer button helper — defined after use is fine in C with a prototype */
static lv_obj_t *make_footer_btn(const char *txt, int x, int w, uint32_t bg, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(s_screen);
    lv_obj_set_pos(btn, x, 416);
    lv_obj_set_size(btn, w, 50);
    style_btn(btn, bg);
    lv_obj_set_style_text_font(btn, &lv_font_montserratMedium_16, 0);  /* 5 buttons → narrower */
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

#else  /* !LV_REALDEVICE — desktop simulator placeholder */

static lv_obj_t *s_sim_screen;

/* Destroy only AFTER the load animation has finished and this screen is off
 * screen — deleting it inside the back handler (mid-animation) frees the
 * object out from under lv_scr_load_anim and crashes. Same pattern as the
 * logs screen and the device build's screen_unloaded_cb. */
static void sim_unloaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    if (s_sim_screen) {
        lv_obj_del_async(s_sim_screen);
        s_sim_screen = NULL;
    }
}

static void sim_back_cb(lv_event_t *e)
{
    (void)e;
    lv_scr_load_anim(guider_ui.dashboard, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

void run_vesc_tool_menu(void)
{
    if (s_sim_screen) return;
    s_sim_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_sim_screen, 800, 480);
    lv_obj_set_style_bg_color(s_sim_screen, lv_color_hex(0x07090A), 0);

    lv_obj_t *btn = lv_btn_create(s_sim_screen);
    lv_obj_set_pos(btn, 17, 14);
    lv_obj_set_size(btn, 200, 40);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "Back to dashboard");
    lv_obj_center(bl);
    lv_obj_add_event_cb(btn, sim_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(s_sim_screen);
    lv_label_set_text(lbl, "VESC Config menu is available on the device build.");
    lv_obj_center(lbl);

    lv_obj_add_event_cb(s_sim_screen, sim_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_scr_load_anim(s_sim_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

#endif /* LV_REALDEVICE */
