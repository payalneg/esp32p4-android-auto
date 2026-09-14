/*
 * Headless driver for the simulator — the part that makes a layout reviewable
 * without a board, a window, or a pair of eyes per screen.
 *
 * Three things live here:
 *
 *   1. Screenshots. lv_snapshot of the active screen, written as a 24-bit BMP
 *      (no image library, and `sips` reads it). With SDL_VIDEODRIVER=dummy
 *      this needs no display at all.
 *
 *   2. Synthetic touch. A pointer input device fed from this file rather than
 *      from the mouse, so a script can tap and swipe. Real LVGL input, so
 *      dropdowns open, lists scroll and buttons fire their callbacks — which
 *      is what "walk the menus" has to mean for the result to be worth
 *      anything.
 *
 *   3. A layout audit. Walks the widget tree of the active screen and reports
 *      what a 480-wide panel breaks: widgets off the edge of the screen, text
 *      wider than the box it sits in, and overlapping labels. The screenshots
 *      show these too, but the audit names the widget and survives diffing
 *      between the 800 and 480 builds.
 *
 * Script language, one command per line or separated by ';':
 *
 *   screen <name>     load a dashboard theme by name, or "settings"
 *   menu <id>         open a code-built screen: trips, vesc, pas, speed,
 *                     lisp, lispedit, rt
 *   tap <x> <y>       tap at a pixel
 *   tap "<text>"      tap the centre of the widget showing that text
 *   swipe <x1> <y1> <x2> <y2> [steps]
 *   wait <ms>
 *   shot <name>       write <out_dir>/<name>.bmp
 *   audit [<label>]   print a layout report for the active screen
 *   tree              print the widget tree with coordinates
 *   tour              every screen above: load, shot, audit
 *
 * SVD_SHOT keeps working as it did: a comma-separated list of themes (or
 * "all"), each loaded and shot.
 */

#include "sim_drive.h"

#include "lvgl.h"
#include "gui_guider.h"
#include "custom.h"
#include "dashboard_theme.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern lv_ui guider_ui;

static const char *s_out_dir = ".";
static int         s_failures;

/* ------------------------------------------------------------------ ticking */

/* Run LVGL for `ms` of simulated time. The tick thread from hal_init() is what
 * advances lv_tick, so this only has to keep calling the handler. */
static void settle(int ms)
{
    for (int i = 0; i < ms / 5; i++) {
        lv_task_handler();
        usleep(5 * 1000);
    }
}

/* --------------------------------------------------------- synthetic touch */

static lv_point_t s_touch;
static bool       s_touch_down;

static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    data->point = s_touch;
    data->state = s_touch_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void touch_init(void)
{
    static bool done;
    if (done) return;
    done = true;
    static lv_indev_drv_t drv;
    lv_indev_drv_init(&drv);
    drv.type = LV_INDEV_TYPE_POINTER;
    drv.read_cb = touch_read;
    lv_indev_drv_register(&drv);
}

/* A tap long enough for the UI's own press handling: the AA touch path uses a
 * 250 ms minimum hold, and some buttons animate on press. */
static void tap_xy(lv_coord_t x, lv_coord_t y)
{
    touch_init();
    s_touch.x = x; s_touch.y = y;
    s_touch_down = true;
    settle(300);
    s_touch_down = false;
    settle(400);            /* release + whatever the click starts */
}

static void swipe_xy(lv_coord_t x1, lv_coord_t y1, lv_coord_t x2, lv_coord_t y2, int steps)
{
    touch_init();
    if (steps < 2) steps = 10;
    s_touch.x = x1; s_touch.y = y1;
    s_touch_down = true;
    settle(60);
    for (int i = 1; i <= steps; i++) {
        s_touch.x = (lv_coord_t)(x1 + (x2 - x1) * i / steps);
        s_touch.y = (lv_coord_t)(y1 + (y2 - y1) * i / steps);
        settle(30);
    }
    s_touch_down = false;
    settle(500);            /* let scroll momentum finish */
}

/* --------------------------------------------------- find a widget by text */

/* The text a widget shows, if it shows any: labels and anything with a label
 * child (buttons), plus textareas and dropdowns. */
static void text_size(lv_obj_t *o, const char *txt, lv_coord_t avail_w, lv_point_t *out);
static void text_ink_area(lv_obj_t *o, const char *txt, lv_area_t *out);

static const char *widget_text(lv_obj_t *o)
{
    if (lv_obj_check_type(o, &lv_label_class))    return lv_label_get_text(o);
#if LV_USE_TEXTAREA
    if (lv_obj_check_type(o, &lv_textarea_class)) return lv_textarea_get_text(o);
#endif
    return NULL;
}

static lv_obj_t *find_by_text(lv_obj_t *parent, const char *want)
{
    const char *txt = widget_text(parent);
    if (txt && strstr(txt, want)) return parent;
    uint32_t n = lv_obj_get_child_cnt(parent);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *hit = find_by_text(lv_obj_get_child(parent, i), want);
        if (hit) return hit;
    }
    return NULL;
}

/* Tapping a label does nothing; the thing that handles the click is usually its
 * parent button. Walk up to the nearest clickable ancestor. */
static lv_obj_t *clickable_self_or_parent(lv_obj_t *o)
{
    for (lv_obj_t *p = o; p && p != lv_scr_act(); p = lv_obj_get_parent(p)) {
        if (lv_obj_has_flag(p, LV_OBJ_FLAG_CLICKABLE)) return p;
    }
    return o;
}

static bool tap_text(const char *want)
{
    lv_obj_t *hit = find_by_text(lv_scr_act(), want);
    if (!hit) {
        fprintf(stderr, "drive: no widget showing \"%s\"\n", want);
        return false;
    }
    hit = clickable_self_or_parent(hit);
    lv_area_t a;
    lv_obj_get_coords(hit, &a);
    tap_xy((lv_coord_t)((a.x1 + a.x2) / 2), (lv_coord_t)((a.y1 + a.y2) / 2));
    return true;
}

/* ------------------------------------------------------------- screenshots */

/* 24-bit BMP: no image library, and every viewer plus `sips` reads it. */
static bool write_bmp(const lv_img_dsc_t *dsc, const char *path)
{
    if (!dsc || !dsc->data) return false;
    const int w = dsc->header.w, h = dsc->header.h;
    const int row = (w * 3 + 3) & ~3;              /* BMP rows are 4-byte aligned */
    const int img_sz = row * h;

    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint8_t hdr[54] = {0};
    hdr[0] = 'B'; hdr[1] = 'M';
    uint32_t file_sz = 54 + img_sz;  memcpy(hdr + 2, &file_sz, 4);
    uint32_t off = 54;               memcpy(hdr + 10, &off, 4);
    uint32_t dib = 40;               memcpy(hdr + 14, &dib, 4);
    int32_t  iw = w;                 memcpy(hdr + 18, &iw, 4);
    int32_t  ih = h;                 memcpy(hdr + 22, &ih, 4);
    uint16_t planes = 1, bpp = 24;
    memcpy(hdr + 26, &planes, 2);
    memcpy(hdr + 28, &bpp, 2);
    uint32_t raw = img_sz;           memcpy(hdr + 34, &raw, 4);
    fwrite(hdr, 1, sizeof hdr, f);

    uint8_t *line = calloc(1, row);
    if (!line) { fclose(f); return false; }
    for (int y = h - 1; y >= 0; y--) {             /* BMP is bottom-up */
        for (int x = 0; x < w; x++) {
            lv_color_t c = ((const lv_color_t *)dsc->data)[y * w + x];
            line[x * 3 + 0] = (uint8_t)(c.ch.blue  << 3);
            line[x * 3 + 1] = (uint8_t)(c.ch.green << 2);
            line[x * 3 + 2] = (uint8_t)(c.ch.red   << 3);
        }
        fwrite(line, 1, row, f);
    }
    free(line);
    fclose(f);
    return true;
}

/* Grab what the panel would actually show. NOT lv_snapshot of the active
 * screen: the LISP quick-action panel is a drawer on lv_layer_top(), and a
 * screen snapshot leaves it out entirely — the first version of this shot the
 * dashboard underneath and reported it as the panel. Reading the display's
 * own draw buffer after a full refresh includes every layer, which is what a
 * photo of the board would show.
 *
 * full_refresh is forced on in sim_drive_run() so the buffer holds the whole
 * frame rather than the last invalidated strip. */
static bool shot(const char *name)
{
    lv_disp_t *disp = lv_disp_get_default();
    if (!disp) return false;

    lv_obj_invalidate(lv_layer_top());
    lv_obj_invalidate(lv_scr_act());
    settle(200);
    lv_refr_now(disp);

    lv_disp_draw_buf_t *db = lv_disp_get_draw_buf(disp);
    lv_coord_t w = lv_disp_get_hor_res(disp), h = lv_disp_get_ver_res(disp);
    if (!db || !db->buf1) return false;

    lv_img_dsc_t dsc = {
        .header = { .w = (uint32_t)w, .h = (uint32_t)h,
                    .cf = LV_IMG_CF_TRUE_COLOR },
        .data = (const uint8_t *)db->buf1,
    };

    char safe[160];
    size_t n = 0;
    for (const char *p = name; *p && n < sizeof safe - 1; p++) {
        safe[n++] = (*p == ' ' || *p == '/') ? '_' : *p;
    }
    safe[n] = 0;
    char path[512];
    snprintf(path, sizeof path, "%s/%s.bmp", s_out_dir, safe);
    bool ok = write_bmp(&dsc, path);
    printf("%s %s (%dx%d)\n", ok ? "wrote" : "FAILED", path, (int)w, (int)h);
    return ok;
}

/* LVGL 8 does not store a class name, so name the few kinds that matter for a
 * layout report. */
static const char *kind_of(lv_obj_t *o)
{
    if (lv_obj_check_type(o, &lv_label_class))    return "label";
    if (lv_obj_check_type(o, &lv_btn_class))      return "button";
#if LV_USE_DROPDOWN
    if (lv_obj_check_type(o, &lv_dropdown_class)) return "dropdown";
#endif
#if LV_USE_SWITCH
    if (lv_obj_check_type(o, &lv_switch_class))   return "switch";
#endif
#if LV_USE_TEXTAREA
    if (lv_obj_check_type(o, &lv_textarea_class)) return "textarea";
#endif
#if LV_USE_SLIDER
    if (lv_obj_check_type(o, &lv_slider_class))   return "slider";
#endif
#if LV_USE_BAR
    if (lv_obj_check_type(o, &lv_bar_class))      return "bar";
#endif
#if LV_USE_IMG
    if (lv_obj_check_type(o, &lv_img_class))      return "image";
#endif
#if LV_USE_LIST
    if (lv_obj_check_type(o, &lv_list_class))     return "list";
#endif
#if LV_USE_KEYBOARD
    if (lv_obj_check_type(o, &lv_keyboard_class)) return "keyboard";
#endif
    return "obj";
}

/* ------------------------------------------------------------ layout audit */

#define AUDIT_MAX_TEXTS 256

typedef struct {
    lv_obj_t  *obj;
    lv_area_t  area;
    const char *text;
} audit_text_t;

static audit_text_t s_texts[AUDIT_MAX_TEXTS];
static int          s_text_count;
/* LEDs and images: a caption sliding under the next indicator's dot is the
 * failure that turned the 480 status bar into mush, and comparing text against
 * text alone cannot see it. */
static audit_text_t s_icons[AUDIT_MAX_TEXTS];
static int          s_icon_count;
static int          s_issues;

static const char *short_text(const char *t, char *buf, size_t cap)
{
    size_t n = 0;
    for (; *t && n < cap - 1; t++) {
        buf[n++] = (*t == '\n') ? ' ' : *t;   /* keep the report one line deep */
    }
    buf[n] = 0;
    return buf;
}

/* How wide/tall this label's text wants to be. */
static void text_size(lv_obj_t *o, const char *txt, lv_coord_t avail_w,
                      lv_point_t *out)
{
    const lv_font_t *font = lv_obj_get_style_text_font(o, LV_PART_MAIN);
    lv_coord_t ls = lv_obj_get_style_text_letter_space(o, LV_PART_MAIN);
    lv_coord_t lsp = lv_obj_get_style_text_line_space(o, LV_PART_MAIN);
    lv_txt_get_size(out, txt, font, ls, lsp, avail_w, LV_TEXT_FLAG_NONE);
}

/* Does an ancestor clip this widget, and is that ancestor a scroll container
 * (where being out of view is the point, not a bug)? */
static bool clipped_by_parent(lv_obj_t *o, lv_area_t *out_parent, bool *scrollable)
{
    lv_obj_t *p = lv_obj_get_parent(o);
    if (!p) return false;
    lv_area_t self, par;
    text_ink_area(o, widget_text(o), &self);
    lv_obj_get_coords(p, &par);
    *scrollable = lv_obj_has_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    *out_parent = par;
    return self.x1 < par.x1 || self.x2 > par.x2 || self.y1 < par.y1 || self.y2 > par.y2;
}

/* Where the glyphs actually land inside a label's box.
 *
 * A label's box is not its text: the big Antonio readouts sit centred in boxes
 * far wider than the digits, and the bottom row of the Cockpit theme hangs a
 * 60 px box off the panel while the text inside it stays visible. Comparing
 * boxes reported both as faults, which buries the real ones. So measure the
 * text and place it in the box the way LVGL will draw it. */
static void text_ink_area(lv_obj_t *o, const char *txt, lv_area_t *out)
{
    lv_obj_get_coords(o, out);
    if (!txt || !*txt || !lv_obj_check_type(o, &lv_label_class)) return;

    const lv_coord_t w = lv_area_get_width(out);
    lv_point_t need;
    text_size(o, txt, w, &need);            /* wrapped to the box width */
    if (need.x > w) need.x = w;

    lv_text_align_t al = lv_obj_get_style_text_align(o, LV_PART_MAIN);
    lv_coord_t slack = w - need.x;
    if (slack > 0) {
        if (al == LV_TEXT_ALIGN_CENTER)     { out->x1 += slack / 2; out->x2 -= slack - slack / 2; }
        else if (al == LV_TEXT_ALIGN_RIGHT) { out->x1 += slack; }
        else                                { out->x2 -= slack; }
    }
    /* LVGL draws a label's text from the top of its box down. */
    if (need.y < lv_area_get_height(out)) out->y2 = out->y1 + need.y;
}

/* The part of a widget that is actually painted: its own area clipped by every
 * ancestor. Without this the overlap pass reports a list item scrolled up
 * behind the header as overlapping the Back button — they share coordinates,
 * but the list clips it and nobody ever sees it. */
static bool visible_area(lv_obj_t *o, lv_area_t *out)
{
    text_ink_area(o, widget_text(o), out);
    for (lv_obj_t *p = lv_obj_get_parent(o); p; p = lv_obj_get_parent(p)) {
        lv_area_t par;
        lv_obj_get_coords(p, &par);
        if (!_lv_area_intersect(out, out, &par)) return false;   /* fully hidden */
    }
    return true;
}

static void audit_obj(lv_obj_t *o, lv_coord_t scr_w, lv_coord_t scr_h)
{
    if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) return;

    lv_area_t box;
    lv_obj_get_coords(o, &box);
    const char *txt = widget_text(o);
    lv_area_t a;
    text_ink_area(o, txt, &a);      /* a == box for anything but a label */
    const lv_coord_t w = lv_area_get_width(&a), h = lv_area_get_height(&a);
    char buf[96];

    /* 1. Off the panel. Exempt along an axis some ancestor scrolls on — a row
     *    below the fold of a list is content, not a bug. The ancestor need not
     *    be the direct parent: these screens are list -> row -> label. */
    if (w > 0 && h > 0) {
        bool scroll_ver = false, scroll_hor = false;
        for (lv_obj_t *p = lv_obj_get_parent(o); p; p = lv_obj_get_parent(p)) {
            if (!lv_obj_has_flag(p, LV_OBJ_FLAG_SCROLLABLE)) continue;
            lv_dir_t dir = lv_obj_get_scroll_dir(p);
            if (dir & LV_DIR_VER) scroll_ver = true;
            if (dir & LV_DIR_HOR) scroll_hor = true;
        }
        bool off_x = a.x1 < 0 || a.x2 > scr_w - 1;
        bool off_y = a.y1 < 0 || a.y2 > scr_h - 1;
        if ((off_x && !scroll_hor) || (off_y && !scroll_ver)) {
            printf("  off-screen  %-22s [%4d,%4d %4dx%4d]  %s\n",
                   kind_of(o), a.x1, a.y1, w, h,
                   txt ? short_text(txt, buf, sizeof buf) : "");
            s_issues++;
        }
    }

    /* 2. Text that does not fit its box. This is the 480 port's whole problem:
     *    a caption laid out for 800 px either gets clipped or wraps into the
     *    row below it. */
    if (txt && *txt && lv_obj_check_type(o, &lv_label_class) && w > 0) {
        lv_label_long_mode_t mode = lv_label_get_long_mode(o);
        lv_point_t need;
        text_size(o, txt, LV_COORD_MAX, &need);
        if (need.x > w) {
            if (mode == LV_LABEL_LONG_WRAP) {
                lv_point_t wrapped;
                text_size(o, txt, w, &wrapped);
                /* A label sized to one line that now needs two. */
                if (wrapped.y > h) {
                    printf("  wraps       %-22s [%4d,%4d %4dx%4d] needs %dpx: %s\n",
                           "label", a.x1, a.y1, w, h, need.x,
                           short_text(txt, buf, sizeof buf));
                    s_issues++;
                }
            } else {
                printf("  clipped     %-22s [%4d,%4d %4dx%4d] needs %dpx: %s\n",
                       "label", a.x1, a.y1, w, h, need.x,
                       short_text(txt, buf, sizeof buf));
                s_issues++;
            }
        }
    }

#if LV_USE_DROPDOWN
    /* 2b. A dropdown clips its own text, inside the widget, where no overflow
     *     or overlap check can see it — the selected option just ends
     *     mid-word. The text is drawn by the dropdown, not by a child label,
     *     so it needs its own pass. */
    if (lv_obj_check_type(o, &lv_dropdown_class) && w > 0) {
        /* A dropdown with a static text (lv_dropdown_set_text) shows that,
         * never the selected option — measuring the option would report the
         * file picker in the LISP editor, which only ever says "Open...". */
        char sel[96];
        const char *fixed = lv_dropdown_get_text(o);
        if (fixed) {
            snprintf(sel, sizeof sel, "%s", fixed);
        } else {
            lv_dropdown_get_selected_str(o, sel, sizeof sel);
        }
        if (sel[0]) {
            lv_point_t need;
            text_size(o, sel, LV_COORD_MAX, &need);
            /* The arrow symbol and the widget padding are not text room. */
            lv_coord_t room = lv_obj_get_content_width(o) - 26;
            if (need.x > room) {
                printf("  dd clipped  %-22s [%4d,%4d %4dx%4d] needs %dpx: %s\n",
                       "dropdown", a.x1, a.y1, w, h, need.x + 26, sel);
                s_issues++;
            }
        }
    }
#endif

    /* 3. Clipped by a non-scrolling parent: the widget fits the panel but not
     *    the card it was put in. */
    if (txt && *txt && w > 0 && h > 0) {
        lv_area_t par; bool scrollable = false;
        if (clipped_by_parent(o, &par, &scrollable) && !scrollable) {
            printf("  cut by card %-22s [%4d,%4d %4dx%4d] parent [%4d,%4d %4dx%4d]  %s\n",
                   "label", a.x1, a.y1, w, h,
                   par.x1, par.y1, lv_area_get_width(&par), lv_area_get_height(&par),
                   short_text(txt, buf, sizeof buf));
            s_issues++;
        }
    }

    /* Collect visible texts for the overlap pass — the painted area, not the
     * nominal one. */
    if (txt && *txt && w > 0 && h > 0 && s_text_count < AUDIT_MAX_TEXTS) {
        lv_area_t vis;
        if (visible_area(o, &vis)) {
            s_texts[s_text_count].obj  = o;
            s_texts[s_text_count].area = vis;
            s_texts[s_text_count].text = txt;
            s_text_count++;
        }
    }

    /* Opaque widgets that can bury a caption: indicators AND controls. A
     * dropdown sized for an 800-wide row is drawn after the caption and simply
     * paints over it on a 480-wide one — no clipping, no overflow, nothing the
     * text-vs-text pass can see. Bars and sliders are deliberately absent:
     * the dashboard themes park readouts on top of them by design. */
    if (!txt && w > 0 && h > 0 && s_icon_count < AUDIT_MAX_TEXTS) {
        bool is_icon = false;
#if LV_USE_LED
        if (lv_obj_check_type(o, &lv_led_class)) is_icon = true;
#endif
#if LV_USE_IMG
        if (lv_obj_check_type(o, &lv_img_class)) is_icon = true;
#endif
#if LV_USE_DROPDOWN
        if (lv_obj_check_type(o, &lv_dropdown_class)) is_icon = true;
#endif
#if LV_USE_SWITCH
        if (lv_obj_check_type(o, &lv_switch_class)) is_icon = true;
#endif
#if LV_USE_TEXTAREA
        if (lv_obj_check_type(o, &lv_textarea_class)) is_icon = true;
#endif
        if (lv_obj_check_type(o, &lv_btn_class)) is_icon = true;
        lv_area_t vis;
        if (is_icon && visible_area(o, &vis)) {
            s_icons[s_icon_count].obj  = o;
            s_icons[s_icon_count].area = vis;
            s_icons[s_icon_count].text = kind_of(o);
            s_icon_count++;
        }
    }

    uint32_t n = lv_obj_get_child_cnt(o);
    for (uint32_t i = 0; i < n; i++) audit_obj(lv_obj_get_child(o, i), scr_w, scr_h);
}

static bool is_ancestor(lv_obj_t *maybe_parent, lv_obj_t *o)
{
    for (lv_obj_t *p = lv_obj_get_parent(o); p; p = lv_obj_get_parent(p)) {
        if (p == maybe_parent) return true;
    }
    return false;
}

static void audit(const char *label)
{
    settle(300);
    lv_coord_t w = lv_disp_get_hor_res(NULL), h = lv_disp_get_ver_res(NULL);
    printf("\n=== audit %s (%dx%d) ===\n", label ? label : "screen", w, h);
    s_text_count = 0;
    s_icon_count = 0;
    s_issues = 0;
    audit_obj(lv_scr_act(), w, h);

    /* Overlapping text: two labels sharing pixels. Arithmetic that fits every
     * label into the panel can still stack two of them on each other, and that
     * is the failure that looks worst on a photo of the board. */
    for (int i = 0; i < s_text_count; i++) {
        for (int j = i + 1; j < s_text_count; j++) {
            lv_obj_t *a = s_texts[i].obj, *b = s_texts[j].obj;
            if (is_ancestor(a, b) || is_ancestor(b, a)) continue;
            lv_area_t inter;
            if (!_lv_area_intersect(&inter, &s_texts[i].area, &s_texts[j].area)) continue;
            /* Ignore a sliver: rounding and letter spacing produce 1-2 px
             * touches that nobody can see. */
            if (lv_area_get_width(&inter) < 3 || lv_area_get_height(&inter) < 3) continue;
            char b1[48], b2[48];
            printf("  overlap     %dx%d px: \"%s\" / \"%s\"\n",
                   lv_area_get_width(&inter), lv_area_get_height(&inter),
                   short_text(s_texts[i].text, b1, sizeof b1),
                   short_text(s_texts[j].text, b2, sizeof b2));
            s_issues++;
        }
    }
    /* Text under an icon. Same idea as above, across the two lists: a label
     * whose glyphs land on a LED or an image is unreadable even though both
     * are inside the screen and neither is clipped. */
    for (int i = 0; i < s_text_count; i++) {
        for (int j = 0; j < s_icon_count; j++) {
            lv_obj_t *t = s_texts[i].obj, *ic = s_icons[j].obj;
            if (is_ancestor(t, ic) || is_ancestor(ic, t)) continue;
            /* Text drawn before the control is behind it; text drawn after sits
             * on top and is readable (a value label over its own row). Only the
             * first case is a fault. */
            if (lv_obj_get_index(t) > lv_obj_get_index(ic) &&
                lv_obj_get_parent(t) == lv_obj_get_parent(ic)) continue;
            /* Grown by 2 px: text touching an indicator dot reads as badly as
             * text under it, and the 480 status bar leaves exactly 1 px. */
            lv_area_t grown = s_icons[j].area;
            grown.x1 -= 2; grown.x2 += 2; grown.y1 -= 2; grown.y2 += 2;
            lv_area_t inter;
            if (!_lv_area_intersect(&inter, &s_texts[i].area, &grown)) continue;
            if (lv_area_get_height(&inter) < 3) continue;
            char b1[48];
            printf("  buried      %dx%d px: \"%s\" under %s [%4d,%4d]\n",
                   lv_area_get_width(&inter), lv_area_get_height(&inter),
                   short_text(s_texts[i].text, b1, sizeof b1),
                   s_icons[j].text, s_icons[j].area.x1, s_icons[j].area.y1);
            s_issues++;
        }
    }

    printf("--- %d issue%s on %s\n", s_issues, s_issues == 1 ? "" : "s",
           label ? label : "screen");
}

/* ------------------------------------------------------------- widget tree */

static void tree_obj(lv_obj_t *o, int depth)
{
    lv_area_t a;
    lv_obj_get_coords(o, &a);
    const char *txt = widget_text(o);
    char buf[64];
    printf("%*s%-16s [%4d,%4d %4dx%4d]%s%s\n", depth * 2, "",
           kind_of(o), a.x1, a.y1, lv_area_get_width(&a), lv_area_get_height(&a),
           lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN) ? " HIDDEN " : " ",
           txt ? short_text(txt, buf, sizeof buf) : "");
    uint32_t n = lv_obj_get_child_cnt(o);
    for (uint32_t i = 0; i < n; i++) tree_obj(lv_obj_get_child(o, i), depth + 1);
}

/* --------------------------------------------------------- loading screens */

static bool load_screen(const char *name)
{
    if (strcmp(name, "settings") == 0) {
        setup_scr_settings(&guider_ui);
        lv_scr_load(guider_ui.settings);
        settle(600);
        return true;
    }
    for (int i = 0; i < dashboard_theme_count(); i++) {
        const dashboard_theme_t *t = dashboard_theme_get(i);
        if (!t || !t->name || strcmp(t->name, name) != 0) continue;
        dashboard_theme_set(i);
        lv_obj_t *scr = dashboard_theme_active_screen();
        if (scr) lv_scr_load(scr);
        settle(900);                 /* themes start timers on load */
        return true;
    }
    fprintf(stderr, "drive: no screen or theme called \"%s\"\n", name);
    return false;
}

/* The code-built screens. Each one pushes itself with lv_scr_load_anim, so
 * settle long enough for the 200 ms transition plus the first data pass. */
static bool open_menu(const char *id)
{
    /* The LISP panel is a drawer over the dashboard and refuses to open on top
     * of anything else (lisp_panel.c checks lv_scr_act()), so put the
     * dashboard back first — otherwise the tour silently shoots the previous
     * screen and calls it the panel. */
    if (strcmp(id, "lisp") == 0) {
        lv_obj_t *dash = dashboard_theme_active_screen();
        if (dash && lv_scr_act() != dash) { lv_scr_load(dash); settle(700); }
    }

    if      (strcmp(id, "trips")    == 0) show_trips_statistics();
    else if (strcmp(id, "vesc")     == 0) run_vesc_tool_menu();
    else if (strcmp(id, "pas")      == 0) show_pas_settings();
    else if (strcmp(id, "speed")    == 0) show_speed_settings();
    else if (strcmp(id, "lisp")     == 0) show_lisp_panel();
    else if (strcmp(id, "lispedit") == 0) show_lisp_editor();
    else if (strcmp(id, "rt")       == 0) show_realtime_viewer();
    else {
        fprintf(stderr, "drive: unknown menu \"%s\"\n", id);
        return false;
    }
    settle(1200);
    return true;
}

/* ------------------------------------------------------------------- tour */

/* Everything reachable, shot and audited. The scroll passes matter: half of
 * these screens are lists taller than the panel, and the bottom of a list is
 * where a 480-high layout runs out of room. */
static void tour(void)
{
    for (int i = 0; i < dashboard_theme_count(); i++) {
        const dashboard_theme_t *t = dashboard_theme_get(i);
        if (!t || !t->name) continue;
        if (!load_screen(t->name)) { s_failures++; continue; }
        char nm[128];
        snprintf(nm, sizeof nm, "dash_%s", t->name);
        if (!shot(nm)) s_failures++;
        audit(nm);
    }

    static const struct { const char *id, *name; int scrolls; } menus[] = {
        { "settings", "settings", 4 },
        { "trips",    "trips",    2 },
        { "vesc",     "vescconf", 3 },
        { "pas",      "pas",      3 },
        { "speed",    "speed",    2 },
        { "lisp",     "lisppanel",1 },
        { "lispedit", "lispedit", 1 },
        { "rt",       "realtime", 1 },
    };

    lv_coord_t w = lv_disp_get_hor_res(NULL), h = lv_disp_get_ver_res(NULL);

    for (unsigned m = 0; m < sizeof menus / sizeof menus[0]; m++) {
        bool ok = (strcmp(menus[m].id, "settings") == 0)
                      ? load_screen("settings")
                      : open_menu(menus[m].id);
        if (!ok) { s_failures++; continue; }

        char nm[128];
        snprintf(nm, sizeof nm, "%s", menus[m].name);
        if (!shot(nm)) s_failures++;
        audit(nm);

        for (int s = 1; s < menus[m].scrolls; s++) {
            /* Drag up through the middle of the screen: scrolls the list down
             * one screenful wherever the content is scrollable. */
            swipe_xy((lv_coord_t)(w / 2), (lv_coord_t)(h * 4 / 5),
                     (lv_coord_t)(w / 2), (lv_coord_t)(h / 6), 12);
            snprintf(nm, sizeof nm, "%s_scroll%d", menus[m].name, s);
            if (!shot(nm)) s_failures++;
            audit(nm);
        }
    }
}

/* ----------------------------------------------------------- script parser */

/* Splits off the next whitespace-delimited word, honouring "quotes" so a tap
 * can name a caption with spaces in it. Advances *pp. */
static bool next_word(char **pp, char *out, size_t cap)
{
    char *p = *pp;
    while (*p == ' ' || *p == '\t') p++;
    if (!*p) { *pp = p; return false; }
    size_t n = 0;
    if (*p == '"') {
        p++;
        while (*p && *p != '"' && n < cap - 1) out[n++] = *p++;
        if (*p == '"') p++;
    } else {
        while (*p && *p != ' ' && *p != '\t' && n < cap - 1) out[n++] = *p++;
    }
    out[n] = 0;
    *pp = p;
    return n > 0;
}

static void run_line(char *line)
{
    char cmd[32];
    char *p = line;
    if (!next_word(&p, cmd, sizeof cmd)) return;
    if (cmd[0] == '#') return;

    char a1[160], a2[32], a3[32], a4[32], a5[32];

    if (strcmp(cmd, "screen") == 0) {
        if (next_word(&p, a1, sizeof a1) && !load_screen(a1)) s_failures++;
    } else if (strcmp(cmd, "menu") == 0) {
        if (next_word(&p, a1, sizeof a1) && !open_menu(a1)) s_failures++;
    } else if (strcmp(cmd, "shot") == 0) {
        if (next_word(&p, a1, sizeof a1) && !shot(a1)) s_failures++;
    } else if (strcmp(cmd, "audit") == 0) {
        audit(next_word(&p, a1, sizeof a1) ? a1 : NULL);
    } else if (strcmp(cmd, "tree") == 0) {
        tree_obj(lv_scr_act(), 0);
    } else if (strcmp(cmd, "wait") == 0) {
        if (next_word(&p, a1, sizeof a1)) settle(atoi(a1));
    } else if (strcmp(cmd, "tap") == 0) {
        if (!next_word(&p, a1, sizeof a1)) return;
        if (isdigit((unsigned char)a1[0]) && next_word(&p, a2, sizeof a2)) {
            tap_xy((lv_coord_t)atoi(a1), (lv_coord_t)atoi(a2));
        } else if (!tap_text(a1)) {
            s_failures++;
        }
    } else if (strcmp(cmd, "swipe") == 0) {
        if (next_word(&p, a1, sizeof a1) && next_word(&p, a2, sizeof a2) &&
            next_word(&p, a3, sizeof a3) && next_word(&p, a4, sizeof a4)) {
            int steps = next_word(&p, a5, sizeof a5) ? atoi(a5) : 10;
            swipe_xy((lv_coord_t)atoi(a1), (lv_coord_t)atoi(a2),
                     (lv_coord_t)atoi(a3), (lv_coord_t)atoi(a4), steps);
        }
    } else if (strcmp(cmd, "tour") == 0) {
        tour();
    } else {
        fprintf(stderr, "drive: unknown command \"%s\"\n", cmd);
        s_failures++;
    }
}

int sim_drive_run(const char *script, const char *out_dir)
{
    s_out_dir = (out_dir && *out_dir) ? out_dir : ".";
    s_failures = 0;

    /* A screenshot reads the draw buffer, so every refresh has to paint the
     * whole screen into it, not just the invalidated area. */
    lv_disp_t *disp = lv_disp_get_default();
    if (disp && disp->driver) disp->driver->full_refresh = 1;

    /* custom_init() has just built the default dashboard and its timers have
     * not run yet. */
    settle(600);
    touch_init();

    char *buf = strdup(script ? script : "");
    if (!buf) return 1;
    char *save = buf;
    for (char *line = buf; line && *line; ) {
        char *end = line;
        while (*end && *end != '\n' && *end != ';') end++;
        char tmp = *end;
        *end = 0;
        run_line(line);
        line = tmp ? end + 1 : NULL;
    }
    free(save);
    return s_failures ? 1 : 0;
}
