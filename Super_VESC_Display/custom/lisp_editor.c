/*
 * LISP script editor (show_lisp_editor).
 *
 * A self-contained LVGL screen — created on demand, destroyed on unload, like
 * the VESC Tool config menu — for editing LispBM scripts on the device:
 *   - multi-line textarea + on-screen keyboard for editing,
 *   - save/load scripts to the local littlefs (.lisp files under /vescfs/lisp),
 *   - Read VESC: pull the code currently stored on the VESC into the editor,
 *   - Upload+Run: erase+write the buffer to the VESC and start it,
 *   - Stop: stop the running script.
 *
 * VESC transfer is async (vesc_lisp_code, runs on its own worker task); the
 * editor polls a small lv_timer for progress/completion so nothing touches
 * LVGL from the CAN/worker thread. An s_alive guard stops a late completion
 * from touching a destroyed screen. Keyboard show/hide is paired with a
 * textarea resize so the focused field is never hidden behind the keyboard.
 *
 * Device-only (needs vesc_can + app_fs); the desktop simulator gets a
 * placeholder so the build still links.
 */
#include "lvgl.h"
#include "custom.h"

extern lv_ui guider_ui;

#ifdef LV_REALDEVICE

#include "vesc_can/vesc_lisp_code.h"
#include "app_fs.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ---- palette (matches the rest of the UI) ---- */
#define COL_BG        0x07090A
#define COL_PANEL     0x12181C
#define COL_BTN       0x2a3440
#define COL_ACCENT    0xB6FF2E
#define COL_CYAN      0x00a9ff
#define COL_ORANGE    0xffa500
#define COL_RED       0xFF3B30
#define COL_TEXT      0xFFFFFF
#define COL_DIM       0x8A9499

#define LISP_DIR        "/vescfs/lisp"
#define LISP_MAX_EDIT   16384

static lv_obj_t  *s_screen;
static lv_obj_t  *s_ta;        /* code editor */
static lv_obj_t  *s_fname;     /* one-line filename field */
static lv_obj_t  *s_kb;        /* on-screen keyboard */
static lv_obj_t  *s_dd;        /* file picker dropdown */
static lv_obj_t  *s_status;
static lv_timer_t *s_op_timer; /* polls the async VESC op */
static lv_timer_t *s_fs_timer; /* one-shot: refresh list once FS is ready */
static bool       s_alive;

/* async op state (volatiles written by the vesc_lisp_code worker task) */
static volatile bool     s_op_active;
static volatile bool     s_op_finished;
static volatile int      s_op_result;   /* vlc_result_t */
static volatile uint32_t s_op_done, s_op_total;
static volatile int      s_op_kind;     /* 1 = upload, 2 = read */
static char  * volatile  s_op_code;     /* read result (heap); applied+freed by timer */

static void set_status(const char *txt, uint32_t color)
{
    if (!s_status) return;
    lv_label_set_text(s_status, txt);
    lv_obj_set_style_text_color(s_status, lv_color_hex(color), 0);
}

/* ---- keyboard ---- */

static void show_kb(lv_obj_t *ta)
{
    lv_keyboard_set_textarea(s_kb, ta);
    lv_obj_move_foreground(s_kb);
    lv_obj_clear_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(s_ta, 784, 150);   /* shrink editor so it stays visible */
}

static void hide_kb(void)
{
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(s_ta, 784, 344);   /* restore full editor height */
}

static void ta_focus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) show_kb(ta);
}

static void kb_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) hide_kb();
}

/* ---- littlefs file ops ---- */

static void refresh_file_list(void)
{
    if (!s_dd) return;
    static char opts[1024];
    size_t used = 0;
    int    count = 0;

    if (app_fs_ready()) {
        DIR *d = opendir(LISP_DIR);
        if (d) {
            struct dirent *e;
            while ((e = readdir(d)) != NULL) {
                const char *n = e->d_name;
                size_t l = strlen(n);
                if (l > 5 && strcmp(n + l - 5, ".lisp") == 0) {
                    int w = snprintf(opts + used, sizeof(opts) - used, "%s%s",
                                     count ? "\n" : "", n);
                    if (w < 0 || used + (size_t)w >= sizeof(opts)) break;
                    used += (size_t)w;
                    count++;
                }
            }
            closedir(d);
        }
    }
    if (count == 0) snprintf(opts, sizeof opts, "(no scripts)");
    lv_dropdown_set_options(s_dd, opts);
}

static void load_file(const char *name)
{
    char path[160];
    snprintf(path, sizeof path, "%s/%s", LISP_DIR, name);
    FILE *f = fopen(path, "r");
    if (!f) { set_status("Open failed", COL_RED); return; }
    static char buf[LISP_MAX_EDIT];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';
    lv_textarea_set_text(s_ta, buf);
    lv_textarea_set_text(s_fname, name);
    set_status("Loaded from storage", COL_DIM);
}

static void save_file(void)
{
    if (!app_fs_ready()) { set_status("Storage not ready", COL_ORANGE); return; }

    const char *name = lv_textarea_get_text(s_fname);
    if (!name || !name[0]) { set_status("Enter a file name", COL_ORANGE); return; }

    char fname[96];
    snprintf(fname, sizeof fname, "%s", name);
    size_t l = strlen(fname);
    if (l < 5 || strcmp(fname + l - 5, ".lisp") != 0) {
        if (l + 5 < sizeof fname) strcat(fname, ".lisp");
    }

    mkdir(LISP_DIR, 0775);   /* ok if it already exists */
    char path[160];
    snprintf(path, sizeof path, "%s/%s", LISP_DIR, fname);

    const char *code = lv_textarea_get_text(s_ta);
    FILE *f = fopen(path, "w");
    if (!f) { set_status("Save failed", COL_RED); return; }
    fwrite(code, 1, strlen(code), f);
    fclose(f);

    lv_textarea_set_text(s_fname, fname);
    refresh_file_list();
    set_status("Saved", COL_ACCENT);
}

static void dd_cb(lv_event_t *e)
{
    (void)e;
    char sel[96];
    lv_dropdown_get_selected_str(s_dd, sel, sizeof sel);
    if (sel[0] == '(') return;   /* "(no scripts)" placeholder */
    load_file(sel);
}

static void new_cb(lv_event_t *e)
{
    (void)e;
    lv_textarea_set_text(s_ta, "");
    lv_textarea_set_text(s_fname, "");
    set_status("New script", COL_DIM);
}

static void save_cb(lv_event_t *e) { (void)e; save_file(); }

/* ---- async VESC transfer ---- */

static const char *result_str(int kind, int res)
{
    if (res == VLC_OK)          return kind == 1 ? "Uploaded & running" : "Read from VESC";
    if (res == VLC_ERR_TIMEOUT) return "VESC timeout (check CAN)";
    if (res == VLC_ERR_BUSY)    return "VESC busy";
    if (res == VLC_ERR_NOMEM)   return "Out of memory";
    if (res == VLC_ERR_EMPTY)   return "No code stored on VESC";
    return "Failed";
}

static void prog_cb(void *u, uint32_t done, uint32_t total)
{
    (void)u;
    s_op_done  = done;
    s_op_total = total;
}

static void upload_done_cb(void *u, vlc_result_t res)
{
    (void)u;
    s_op_result   = res;
    s_op_finished = true;
}

static void read_done_cb(void *u, vlc_result_t res, const char *code, uint32_t len)
{
    (void)u;
    if (res == VLC_OK && code) {
        char *c = malloc(len + 1);
        if (c) { memcpy(c, code, len); c[len] = '\0'; }
        s_op_code = c;
    }
    s_op_result   = res;
    s_op_finished = true;
}

static void op_timer_cb(lv_timer_t *t)
{
    if (!s_alive) { lv_timer_del(t); s_op_timer = NULL; return; }

    if (!s_op_finished) {
        char b[64];
        const char *verb = (s_op_kind == 1) ? "Uploading" : "Reading";
        if (s_op_total) {
            snprintf(b, sizeof b, "%s %u%%", verb,
                     (unsigned)((uint64_t)s_op_done * 100 / s_op_total));
        } else {
            snprintf(b, sizeof b, "%s...", verb);
        }
        set_status(b, COL_CYAN);
        return;
    }

    /* finished */
    if (s_op_kind == 2 && s_op_result == VLC_OK && s_op_code) {
        lv_textarea_set_text(s_ta, s_op_code);
    }
    if (s_op_code) { free(s_op_code); s_op_code = NULL; }
    set_status(result_str(s_op_kind, s_op_result),
               s_op_result == VLC_OK ? COL_ACCENT : COL_RED);
    s_op_active = false;
    lv_timer_del(t);
    s_op_timer = NULL;
}

static void start_op_timer(void)
{
    if (!s_op_timer) s_op_timer = lv_timer_create(op_timer_cb, 150, NULL);
}

static void read_vesc_cb(lv_event_t *e)
{
    (void)e;
    if (s_op_active || vesc_lisp_code_busy()) { set_status("Busy", COL_ORANGE); return; }
    s_op_kind = 2; s_op_finished = false; s_op_result = 0;
    s_op_done = 0; s_op_total = 0; s_op_code = NULL; s_op_active = true;
    if (!vesc_lisp_code_read(prog_cb, read_done_cb, NULL)) {
        s_op_active = false;
        set_status("Cannot start read", COL_RED);
        return;
    }
    set_status("Reading from VESC...", COL_CYAN);
    start_op_timer();
}

static void upload_cb(lv_event_t *e)
{
    (void)e;
    if (s_op_active || vesc_lisp_code_busy()) { set_status("Busy", COL_ORANGE); return; }
    const char *code = lv_textarea_get_text(s_ta);
    uint32_t len = code ? (uint32_t)strlen(code) : 0;
    if (len == 0) { set_status("Nothing to upload", COL_ORANGE); return; }

    s_op_kind = 1; s_op_finished = false; s_op_result = 0;
    s_op_done = 0; s_op_total = 0; s_op_active = true;
    if (!vesc_lisp_code_upload(code, len, true, prog_cb, upload_done_cb, NULL)) {
        s_op_active = false;
        set_status("Busy or script too large", COL_RED);
        return;
    }
    set_status("Uploading...", COL_CYAN);
    start_op_timer();
}

static void stop_cb(lv_event_t *e)
{
    (void)e;
    if (vesc_lisp_code_busy()) { set_status("Busy", COL_ORANGE); return; }
    vesc_lisp_code_set_running(false);
    set_status("Stop sent", COL_DIM);
}

/* ---- navigation / lifecycle ---- */

static void back_cb(lv_event_t *e)
{
    (void)e;
    run_vesc_tool_menu();   /* return to the VESC Tool config menu */
}

static void fs_timer_cb(lv_timer_t *t)
{
    if (!s_alive) { lv_timer_del(t); s_fs_timer = NULL; return; }
    if (app_fs_ready()) {
        refresh_file_list();
        lv_timer_del(t);
        s_fs_timer = NULL;
    }
}

static void screen_unloaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    s_alive = false;
    if (s_op_timer) { lv_timer_del(s_op_timer); s_op_timer = NULL; }
    if (s_fs_timer) { lv_timer_del(s_fs_timer); s_fs_timer = NULL; }
    if (s_op_code)  { free(s_op_code); s_op_code = NULL; }
    if (s_screen)   { lv_obj_del_async(s_screen); s_screen = NULL; }
    s_ta = s_fname = s_kb = s_dd = s_status = NULL;
}

static lv_obj_t *mk_btn(int x, int y, int w, int h, uint32_t bg,
                        const char *txt, lv_event_cb_t cb)
{
    lv_obj_t *b = lv_btn_create(s_screen);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
    lv_obj_set_style_radius(b, 6, 0);
    lv_obj_set_style_text_font(b, &lv_font_montserratMedium_16, 0);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

void show_lisp_editor(void)
{
    if (s_screen) return;  /* re-entrancy guard */

    s_op_active = false;
    s_op_timer  = NULL;
    s_fs_timer  = NULL;
    s_op_code   = NULL;

    s_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_screen, 800, 480);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_screen, 255, 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* header */
    lv_obj_t *back = lv_btn_create(s_screen);
    lv_obj_set_pos(back, 8, 8);
    lv_obj_set_size(back, 90, 36);
    lv_obj_set_style_bg_color(back, lv_color_hex(COL_BTN), 0);
    lv_obj_set_style_radius(back, 6, 0);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "Back");
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, "LISP");
    lv_obj_set_pos(title, 110, 12);
    lv_obj_set_style_text_color(title, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);

    /* toolbar: file picker + actions + filename field */
    s_dd = lv_dropdown_create(s_screen);
    lv_obj_set_pos(s_dd, 8, 52);
    lv_obj_set_size(s_dd, 170, 40);
    lv_dropdown_set_text(s_dd, "Open...");
    lv_obj_add_event_cb(s_dd, dd_cb, LV_EVENT_VALUE_CHANGED, NULL);

    mk_btn(182, 52, 60,  40, COL_BTN,    "New",        new_cb);
    mk_btn(246, 52, 60,  40, COL_BTN,    "Save",       save_cb);
    mk_btn(310, 52, 92,  40, COL_BTN,    "Read VESC",  read_vesc_cb);
    mk_btn(406, 52, 120, 40, COL_CYAN,   "Upload+Run", upload_cb);
    mk_btn(530, 52, 60,  40, COL_RED,    "Stop",       stop_cb);

    s_fname = lv_textarea_create(s_screen);
    lv_textarea_set_one_line(s_fname, true);
    lv_textarea_set_placeholder_text(s_fname, "name.lisp");
    lv_obj_set_pos(s_fname, 594, 52);
    lv_obj_set_size(s_fname, 198, 40);
    lv_obj_add_event_cb(s_fname, ta_focus_cb, LV_EVENT_ALL, NULL);

    /* code editor */
    s_ta = lv_textarea_create(s_screen);
    lv_textarea_set_one_line(s_ta, false);
    lv_textarea_set_placeholder_text(s_ta, "(print \"hello\")");
    lv_obj_set_pos(s_ta, 8, 96);
    lv_obj_set_size(s_ta, 784, 344);
    lv_obj_set_style_bg_color(s_ta, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_text_color(s_ta, lv_color_hex(COL_TEXT), 0);
    lv_obj_add_event_cb(s_ta, ta_focus_cb, LV_EVENT_ALL, NULL);

    /* status line (under the editor; hidden behind the keyboard when open) */
    s_status = lv_label_create(s_screen);
    lv_obj_set_pos(s_status, 10, 448);
    lv_obj_set_width(s_status, 782);
    lv_label_set_text(s_status, "");
    lv_obj_set_style_text_color(s_status, lv_color_hex(COL_DIM), 0);
    lv_obj_set_style_text_font(s_status, &lv_font_montserratMedium_16, 0);

    /* keyboard (created last so it draws on top); hidden until a field is focused */
    s_kb = lv_keyboard_create(s_screen);
    lv_obj_set_size(s_kb, 800, 230);
    lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_kb, kb_cb, LV_EVENT_ALL, NULL);

    lv_obj_add_event_cb(s_screen, screen_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    s_alive = true;

    /* Mount the backup FS in the background and list scripts once it's up. */
    app_fs_ensure();
    refresh_file_list();
    if (!app_fs_ready()) s_fs_timer = lv_timer_create(fs_timer_cb, 300, NULL);

    lv_scr_load_anim(s_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

#else  /* !LV_REALDEVICE — desktop simulator placeholder */

static lv_obj_t *s_sim_screen;

static void sim_unloaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    if (s_sim_screen) { lv_obj_del_async(s_sim_screen); s_sim_screen = NULL; }
}

static void sim_back_cb(lv_event_t *e)
{
    (void)e;
    lv_scr_load_anim(guider_ui.dashboard, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

void show_lisp_editor(void)
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
    lv_label_set_text(lbl, "LISP editor is available on the device build.");
    lv_obj_center(lbl);

    lv_obj_add_event_cb(s_sim_screen, sim_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_scr_load_anim(s_sim_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

#endif /* LV_REALDEVICE */
