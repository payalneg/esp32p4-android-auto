#include "files_screen.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bsp/esp-bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"

#include "app_fs.h"   /* app_fs_base()/app_fs_ready()/app_fs_state()/app_fs_ensure() */

static const char *TAG = "files";

/* Force-link anchor — see main.c's force_link_main_strongs[] (same
 * weak/strong override pattern as nav_settings_show, sd_info_get, etc).
 * The weak stub for files_screen_show lives in
 * components/vesc_ui/custom.c; this TU's strong impl wins at link time
 * once main.c references the anchor. */
const int files_screen_link_anchor = 1;

#define MAX_PATH        384
#define MAX_NAME        256
#define MAX_ENTRIES     200
#define TEXT_PREVIEW_N  4096

typedef struct {
    char     name[MAX_NAME];
    uint64_t size;
    bool     is_dir;
} entry_t;

typedef enum {
    MOVE_OFF = 0,
    MOVE_PICK_DEST,
} move_mode_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *prev_screen;
    lv_obj_t *title_lbl;        /* "Files" or "Pick destination" */
    lv_obj_t *path_lbl;
    lv_obj_t *btn_close;
    lv_obj_t *btn_move_here;    /* visible only in MOVE_PICK_DEST */
    lv_obj_t *list;             /* scrollable container for rows */

    /* Modal overlay (action sheet / info / rename / text view). At most
     * one alive at a time; tear-down via dismiss_modal(). */
    lv_obj_t *modal;
    lv_obj_t *modal_kb;         /* used by rename modal */
    lv_obj_t *modal_ta;         /* rename textarea */
    lv_obj_t *modal_msg_lbl;    /* status text inside modal */

    /* Browser state. cwd has no trailing slash (except when root equals
     * BSP_SD_MOUNT_POINT — root is whatever BSP_SD_MOUNT_POINT is, with
     * no trailing slash). */
    char cwd[MAX_PATH];

    /* The full path of the entry whose action sheet is currently up
     * (set on row tap, cleared on modal dismiss). */
    char selected_path[MAX_PATH];
    bool selected_is_dir;
    uint64_t selected_size;

    move_mode_t move_mode;
    char        move_src[MAX_PATH];  /* full path of file being moved */
    char        move_src_name[MAX_NAME]; /* basename for "Move here" */

    bool        sd_available;        /* /sdcard mounted → show it at root */
} state_t;

static state_t s_st;

/* ---------- Forward decls ---------- */
static void list_dir_render(void);
static void dismiss_modal(void);
static void open_action_sheet(void);
static void open_info_modal(void);
static void open_rename_modal(void);
static void open_text_view_modal(void);
static void open_image_view_modal(void);
static void open_delete_confirm(void);
static void enter_move_mode(void);
static void exit_move_mode(void);
static void show_toast(const char *msg);

/* True for filenames ending in a common image extension we can hand to
 * esp_lv_decoder (PNG / JPEG variants / BMP via LVGL's built-in). The
 * check is case-insensitive so "FOO.PNG" works. */
static bool is_image_name(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot) return false;
    const char *ext = dot + 1;
    return strcasecmp(ext, "png")  == 0 ||
           strcasecmp(ext, "jpg")  == 0 ||
           strcasecmp(ext, "jpeg") == 0 ||
           strcasecmp(ext, "bmp")  == 0 ||
           strcasecmp(ext, "gif")  == 0;
}

/* ---------- Path helpers ----------
 *
 * The browser presents a synthetic root "/" that lists the mounted drives
 * as two directories: "vescfs" (internal LittleFS, app_fs_base()) and
 * "sdcard" (BSP_SD_MOUNT_POINT, shown only when a card is mounted). Below a
 * drive it's plain absolute POSIX paths. cwd never has a trailing slash. */

#define SYN_ROOT "/"

static const char *vescfs_root(void) { return app_fs_base(); }      /* "/vescfs" */

static bool is_at_root(const char *path)
{
    return strcmp(path, SYN_ROOT) == 0;
}

/* True if path is the /vescfs mount itself or anything under it. */
static bool is_under_vescfs(const char *path)
{
    size_t n = strlen(vescfs_root());
    return strncmp(path, vescfs_root(), n) == 0 &&
           (path[n] == '\0' || path[n] == '/');
}

static void path_join(char *out, size_t out_sz,
                      const char *dir, const char *name)
{
    /* At the synthetic root, dir == "/" — avoid the "//name" double slash.
     * Elsewhere dir has no trailing slash, so "%s/%s" is safe. */
    if (strcmp(dir, SYN_ROOT) == 0) {
        snprintf(out, out_sz, "/%s", name);
    } else {
        snprintf(out, out_sz, "%s/%s", dir, name);
    }
}

static void path_go_up(char *path)
{
    if (is_at_root(path)) return;          /* already at the synthetic root */
    char *slash = strrchr(path, '/');
    if (!slash || slash == path) {
        /* "/vescfs" or "/sdcard" → back up to the synthetic drive list. */
        strcpy(path, SYN_ROOT);
        return;
    }
    *slash = '\0';
}

static void basename_of(const char *path, char *out, size_t out_sz)
{
    const char *slash = strrchr(path, '/');
    const char *n = slash ? slash + 1 : path;
    strncpy(out, n, out_sz - 1);
    out[out_sz - 1] = '\0';
}

static void format_size(uint64_t bytes, char *out, size_t out_sz)
{
    if (bytes < 1024) {
        snprintf(out, out_sz, "%llu B", (unsigned long long)bytes);
    } else if (bytes < 1024ULL * 1024) {
        snprintf(out, out_sz, "%.1f KiB", (double)bytes / 1024.0);
    } else if (bytes < 1024ULL * 1024 * 1024) {
        snprintf(out, out_sz, "%.1f MiB", (double)bytes / (1024.0 * 1024.0));
    } else {
        snprintf(out, out_sz, "%.2f GiB",
                 (double)bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

/* ---------- Listing ---------- */

static int cmp_entries(const void *a, const void *b)
{
    const entry_t *ea = (const entry_t *)a;
    const entry_t *eb = (const entry_t *)b;
    /* Directories first, then files. strcasecmp inside each group so
     * the order doesn't flip on case differences (FAT is case-
     * insensitive anyway, but the listing comes back in whatever case
     * the entries were created with). */
    if (ea->is_dir != eb->is_dir) return ea->is_dir ? -1 : 1;
    return strcasecmp(ea->name, eb->name);
}

/* Row click handler — user_data points to a strdup'd name (or NULL for
 * the "..", which is identified by its label). Free happens in
 * on_row_delete. */
static void on_row_clicked(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    intptr_t kind = (intptr_t)lv_obj_get_user_data(btn);
    /* Encoding: kind == 0 → ".."; kind == 1 → dir, name in label;
     * kind == 2 → file. The full name lives in a strdup'd buffer
     * attached as the button's first child's user_data? Cleaner: use
     * the trailing label widget's text — readable and we don't have
     * to manage another allocation. */
    if (kind == 0) {
        /* go up */
        path_go_up(s_st.cwd);
        list_dir_render();
        return;
    }
    /* Pull the name from the row's name-label (set in add_row below). */
    const char *name = (const char *)lv_obj_get_user_data(
        lv_obj_get_child(btn, 0));
    if (!name) return;

    char full[MAX_PATH];
    path_join(full, sizeof full, s_st.cwd, name);

    if (kind == 1) {
        /* dir tap */
        if (s_st.move_mode == MOVE_PICK_DEST) {
            /* In move-pick mode, descend just like normal browse — the
             * "Move here" button at the top handles confirming the
             * current directory as destination. */
            strncpy(s_st.cwd, full, sizeof(s_st.cwd) - 1);
            s_st.cwd[sizeof(s_st.cwd) - 1] = '\0';
            list_dir_render();
            return;
        }
        strncpy(s_st.cwd, full, sizeof(s_st.cwd) - 1);
        s_st.cwd[sizeof(s_st.cwd) - 1] = '\0';
        list_dir_render();
        return;
    }

    /* File tap — open action sheet. Move-pick mode shows files greyed
     * out (or filtered out) but if for some reason a file row gets a
     * tap, do nothing. */
    if (s_st.move_mode == MOVE_PICK_DEST) return;

    strncpy(s_st.selected_path, full, sizeof(s_st.selected_path) - 1);
    s_st.selected_path[sizeof(s_st.selected_path) - 1] = '\0';
    s_st.selected_is_dir = false;
    /* Size — stat once now so the action sheet / info can show it
     * without a second syscall. */
    struct stat st;
    if (stat(full, &st) == 0) {
        s_st.selected_size = (uint64_t)st.st_size;
    } else {
        s_st.selected_size = 0;
    }
    open_action_sheet();
}

static void on_row_delete(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    /* The name strdup lives on the first child's user_data. Free here
     * so a list reload (which calls lv_obj_clean on the parent) cleans
     * up properly. */
    lv_obj_t *name_lbl = lv_obj_get_child(btn, 0);
    if (name_lbl) {
        char *nm = (char *)lv_obj_get_user_data(name_lbl);
        if (nm) {
            free(nm);
            lv_obj_set_user_data(name_lbl, NULL);
        }
    }
}

/* kind: 0 = "..", 1 = directory, 2 = file. */
static void add_row(int kind, const char *name, uint64_t size, int y)
{
    lv_obj_t *btn = lv_btn_create(s_st.list);
    lv_obj_set_size(btn, 760, 44);
    lv_obj_set_pos(btn, 0, y);
    lv_obj_set_style_radius(btn, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn,
        (kind == 1) ? lv_color_hex(0x223344) : lv_color_hex(0x2a3440),
        LV_PART_MAIN);
    lv_obj_set_user_data(btn, (void *)(intptr_t)kind);
    lv_obj_add_event_cb(btn, on_row_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn, on_row_delete, LV_EVENT_DELETE, NULL);

    /* Left label = FontAwesome glyph + name. LVGL ships these inside
     * the built-in Montserrat fonts (LV_FONT_FMT_TXT_LARGE region), so
     * no extra font wiring is needed.
     *   LV_SYMBOL_LEFT      ← back arrow for ".."
     *   LV_SYMBOL_DIRECTORY 📁 folder
     *   LV_SYMBOL_FILE      📄 page
     * Store strdup'd name on the label's user_data so the click handler
     * can read it back. */
    lv_obj_t *name_lbl = lv_label_create(btn);
    char display[MAX_NAME + 16];
    const char *icon =
        (kind == 0) ? LV_SYMBOL_LEFT :
        (kind == 1) ? LV_SYMBOL_DIRECTORY :
        (name && is_image_name(name)) ? LV_SYMBOL_IMAGE :
                      LV_SYMBOL_FILE;
    snprintf(display, sizeof display, "%s  %s", icon,
             (kind == 0) ? "Up one level" : name);
    lv_label_set_text(name_lbl, display);
    lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(name_lbl, 550);
    lv_obj_set_pos(name_lbl, 12, 10);
    lv_obj_set_style_text_color(name_lbl, lv_color_white(), 0);

    /* user_data carries the bare name (no prefix) so on_row_clicked can
     * build the full path. ".." doesn't get a name (kind 0 short-circuits). */
    if (kind != 0 && name) {
        lv_obj_set_user_data(name_lbl, strdup(name));
    } else {
        lv_obj_set_user_data(name_lbl, NULL);
    }

    /* Right-side size label for files only. */
    if (kind == 2) {
        lv_obj_t *sz_lbl = lv_label_create(btn);
        char szbuf[32];
        format_size(size, szbuf, sizeof szbuf);
        lv_label_set_text(sz_lbl, szbuf);
        lv_obj_set_style_text_color(sz_lbl, lv_color_hex(0xb0b0b0), 0);
        lv_obj_align(sz_lbl, LV_ALIGN_RIGHT_MID, -12, 0);
    }
}

static void list_dir_render(void)
{
    lv_obj_clean(s_st.list);

    /* Path label & title — refresh on every reload (cwd may have
     * changed, move-mode may have switched). */
    if (s_st.title_lbl) {
        lv_label_set_text(s_st.title_lbl,
            s_st.move_mode == MOVE_PICK_DEST ? "Pick destination" : "Files");
    }
    if (s_st.path_lbl) {
        lv_label_set_text(s_st.path_lbl, s_st.cwd);
    }
    if (s_st.btn_move_here) {
        if (s_st.move_mode == MOVE_PICK_DEST) {
            lv_obj_clear_flag(s_st.btn_move_here, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_st.btn_move_here, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* Synthetic root: list the mounted drives instead of a real directory.
     * No opendir, no ".." (already at the top). */
    if (is_at_root(s_st.cwd)) {
        int yy = 0;
        add_row(1, "vescfs", 0, yy); yy += 48;
        if (s_st.sd_available) { add_row(1, "sdcard", 0, yy); yy += 48; }
        return;
    }

    /* /vescfs is mounted asynchronously (first boot formats, ~1 min). While
     * it's coming up, show status instead of a confusing "cannot open". */
    if (is_under_vescfs(s_st.cwd) && !app_fs_ready()) {
        add_row(0, NULL, 0, 0);   /* ".." back to the drive list */
        lv_obj_t *msg = lv_label_create(s_st.list);
        lv_label_set_text(msg,
            app_fs_state() == APP_FS_FORMATTING
            ? "Preparing storage (first-time format, ~1 min)..."
            : "Storage is mounting...");
        lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(msg, 720);
        lv_obj_set_style_text_color(msg, lv_color_hex(0xe0c060), 0);
        lv_obj_set_pos(msg, 12, 60);
        return;
    }

    DIR *d = opendir(s_st.cwd);
    if (!d) {
        lv_obj_t *err = lv_label_create(s_st.list);
        lv_label_set_text_fmt(err, "Cannot open: %s (errno=%d)",
                              s_st.cwd, errno);
        lv_obj_set_style_text_color(err, lv_color_hex(0xff8080), 0);
        lv_obj_set_pos(err, 12, 12);
        return;
    }

    entry_t *entries = calloc(MAX_ENTRIES, sizeof(entry_t));
    int n = 0;
    int skipped = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 ||
            strcmp(de->d_name, "..") == 0) continue;
        if (n >= MAX_ENTRIES) { skipped++; continue; }
        entry_t *e = &entries[n];
        strncpy(e->name, de->d_name, sizeof(e->name) - 1);
        e->name[sizeof(e->name) - 1] = '\0';
        char full[MAX_PATH];
        path_join(full, sizeof full, s_st.cwd, de->d_name);
        struct stat st;
        if (stat(full, &st) == 0) {
            e->is_dir = S_ISDIR(st.st_mode);
            e->size = (uint64_t)st.st_size;
        } else {
            e->is_dir = false;
            e->size = 0;
        }
        n++;
    }
    closedir(d);

    qsort(entries, n, sizeof(entry_t), cmp_entries);

    int y = 0;
    if (!is_at_root(s_st.cwd)) {
        add_row(0, NULL, 0, y);
        y += 48;
    }
    for (int i = 0; i < n; ++i) {
        if (s_st.move_mode == MOVE_PICK_DEST && !entries[i].is_dir) {
            /* Hide regular files in move-dest picker — only directories
             * are valid destinations. */
            continue;
        }
        add_row(entries[i].is_dir ? 1 : 2, entries[i].name,
                entries[i].size, y);
        y += 48;
    }

    if (skipped > 0) {
        lv_obj_t *more = lv_label_create(s_st.list);
        lv_label_set_text_fmt(more,
            "(+%d more entries — listing capped at %d)",
            skipped, MAX_ENTRIES);
        lv_obj_set_style_text_color(more, lv_color_hex(0xb0b0b0), 0);
        lv_obj_set_pos(more, 12, y + 8);
    }

    free(entries);
}

/* ---------- Modal helpers ---------- */

static void dismiss_modal(void)
{
    /* Keyboard lives directly under s_st.screen (not the modal) so its
     * 200 px height isn't clipped by the modal frame — delete it
     * explicitly here. Order matters: delete kb first while we still
     * have valid pointers, then drop the modal. */
    if (s_st.modal_kb) {
        lv_obj_del(s_st.modal_kb);
        s_st.modal_kb = NULL;
    }
    s_st.modal_ta = NULL;
    s_st.modal_msg_lbl = NULL;
    if (s_st.modal) {
        lv_obj_del(s_st.modal);
        s_st.modal = NULL;
    }
}

static void on_modal_close(lv_event_t *e)
{
    (void)e;
    dismiss_modal();
}

static lv_obj_t *make_modal_button(lv_obj_t *parent, const char *txt,
                                   lv_event_cb_t cb, lv_color_t bg,
                                   int x, int y, int w, int h)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, bg, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, lv_color_white(), 0);
    lv_obj_center(l);
    return btn;
}

static lv_obj_t *make_modal_container(int w, int h)
{
    dismiss_modal();
    lv_obj_t *m = lv_obj_create(s_st.screen);
    lv_obj_set_size(m, w, h);
    lv_obj_center(m);
    lv_obj_set_style_bg_color(m, lv_color_hex(0x1a1f25), LV_PART_MAIN);
    lv_obj_set_style_border_color(m, lv_color_hex(0x4fc3f7), LV_PART_MAIN);
    lv_obj_set_style_border_width(m, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(m, 0, LV_PART_MAIN);
    lv_obj_clear_flag(m, LV_OBJ_FLAG_SCROLLABLE);
    s_st.modal = m;
    return m;
}

/* Quick non-blocking status — small label centered at bottom of screen
 * for ~2 s. Used for "Rename failed" and similar non-fatal feedback. */
static void on_toast_timer(lv_timer_t *t)
{
    lv_obj_t *lbl = (lv_obj_t *)t->user_data;
    if (lbl && lv_obj_is_valid(lbl)) {
        lv_obj_del(lbl);
    }
    lv_timer_del(t);
}

static void show_toast(const char *msg)
{
    lv_obj_t *lbl = lv_label_create(s_st.screen);
    lv_label_set_text(lbl, msg);
    lv_obj_set_style_bg_color(lbl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_70, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_pad_all(lbl, 8, 0);
    lv_obj_set_style_radius(lbl, 6, 0);
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_timer_t *t = lv_timer_create(on_toast_timer, 2000, lbl);
    lv_timer_set_repeat_count(t, 1);
}

/* ---------- Action sheet ---------- */

static void on_act_info(lv_event_t *e)    { (void)e; open_info_modal(); }
static void on_act_open(lv_event_t *e)    { (void)e; open_text_view_modal(); }
static void on_act_image(lv_event_t *e)   { (void)e; open_image_view_modal(); }
static void on_act_rename(lv_event_t *e)  { (void)e; open_rename_modal(); }
static void on_act_move(lv_event_t *e)    { (void)e; enter_move_mode(); }
static void on_act_delete(lv_event_t *e)  { (void)e; open_delete_confirm(); }

static void open_action_sheet(void)
{
    lv_obj_t *m = make_modal_container(520, 460);

    lv_obj_t *title = lv_label_create(m);
    char base[MAX_NAME];
    basename_of(s_st.selected_path, base, sizeof base);
    lv_label_set_text(title, base);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title, 480);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(title, 20, 14);

    int y = 56;
    int bw = 480, bh = 56, gap = 10;
    /* Image files get a dedicated "View image" entry that uses the LVGL
     * decoder; text files fall back to the existing hex/text viewer. */
    if (is_image_name(base)) {
        make_modal_button(m, "View image", on_act_image,
                          lv_color_hex(0x1f6feb), 20, y, bw, bh); y += bh + gap;
    } else {
        make_modal_button(m, "Open as text", on_act_open,
                          lv_color_hex(0x1f6feb), 20, y, bw, bh); y += bh + gap;
    }
    make_modal_button(m, "Rename", on_act_rename,
                      lv_color_hex(0x2a3440), 20, y, bw, bh); y += bh + gap;
    make_modal_button(m, "Move", on_act_move,
                      lv_color_hex(0x2a3440), 20, y, bw, bh); y += bh + gap;
    make_modal_button(m, "Info", on_act_info,
                      lv_color_hex(0x2a3440), 20, y, bw, bh); y += bh + gap;
    make_modal_button(m, "Delete", on_act_delete,
                      lv_color_hex(0xb00020), 20, y, bw, bh); y += bh + gap;

    /* Cancel — bottom corner, slimmer to set it apart from action rows. */
    make_modal_button(m, "Cancel", on_modal_close,
                      lv_color_hex(0x424242), 380, 14, 120, 32);
}

/* ---------- Info modal ---------- */

static void open_info_modal(void)
{
    lv_obj_t *m = make_modal_container(640, 280);

    lv_obj_t *title = lv_label_create(m);
    lv_label_set_text(title, "File info");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(title, 20, 14);

    make_modal_button(m, "Close", on_modal_close,
                      lv_color_hex(0x424242), 500, 14, 120, 36);

    char szbuf[32];
    format_size(s_st.selected_size, szbuf, sizeof szbuf);

    lv_obj_t *info = lv_label_create(m);
    lv_label_set_text_fmt(info,
        "Path:\n  %s\n\nSize: %s (%llu bytes)",
        s_st.selected_path, szbuf,
        (unsigned long long)s_st.selected_size);
    lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(info, 600);
    lv_obj_set_style_text_color(info, lv_color_hex(0xd0d0d0), 0);
    lv_obj_set_pos(info, 20, 68);
}

/* ---------- Rename ---------- */

static void on_rename_save(lv_event_t *e)
{
    (void)e;
    if (!s_st.modal_ta) return;
    const char *new_name = lv_textarea_get_text(s_st.modal_ta);
    if (!new_name || !new_name[0]) {
        lv_label_set_text(s_st.modal_msg_lbl, "Name can't be empty");
        return;
    }
    if (strchr(new_name, '/') != NULL) {
        lv_label_set_text(s_st.modal_msg_lbl, "Name can't contain '/'");
        return;
    }
    /* Compute parent dir from selected_path → join with new_name. */
    char parent[MAX_PATH];
    strncpy(parent, s_st.selected_path, sizeof(parent) - 1);
    parent[sizeof(parent) - 1] = '\0';
    char *slash = strrchr(parent, '/');
    if (slash) *slash = '\0';
    else strncpy(parent, s_st.cwd, sizeof(parent) - 1);

    char dst[MAX_PATH];
    path_join(dst, sizeof dst, parent, new_name);

    if (rename(s_st.selected_path, dst) != 0) {
        ESP_LOGW(TAG, "rename %s -> %s failed: errno=%d",
                 s_st.selected_path, dst, errno);
        lv_label_set_text_fmt(s_st.modal_msg_lbl,
                              "Rename failed (errno %d)", errno);
        return;
    }
    ESP_LOGI(TAG, "renamed %s -> %s", s_st.selected_path, dst);
    dismiss_modal();
    list_dir_render();
    show_toast("Renamed");
}

static void open_rename_modal(void)
{
    lv_obj_t *m = make_modal_container(800, 280);
    /* Top-aligned so the on-screen keyboard at the bottom (200 px) has
     * room without overlapping the textarea. Centered would put the
     * modal's bottom edge at y=380, and the keyboard occupies y=280..480,
     * so we anchor to top instead. */
    lv_obj_align(m, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *title = lv_label_create(m);
    lv_label_set_text(title, "Rename");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(title, 20, 14);

    char base[MAX_NAME];
    basename_of(s_st.selected_path, base, sizeof base);

    lv_obj_t *ta = lv_textarea_create(m);
    lv_obj_set_size(ta, 760, 50);
    lv_obj_set_pos(ta, 20, 60);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, base);
    s_st.modal_ta = ta;

    s_st.modal_msg_lbl = lv_label_create(m);
    lv_label_set_text(s_st.modal_msg_lbl, "");
    lv_obj_set_style_text_color(s_st.modal_msg_lbl,
                                lv_color_hex(0xff7777), 0);
    lv_obj_set_pos(s_st.modal_msg_lbl, 20, 120);

    make_modal_button(m, "Save", on_rename_save,
                      lv_color_hex(0x2e7d32), 460, 200, 140, 50);
    make_modal_button(m, "Cancel", on_modal_close,
                      lv_color_hex(0x424242), 620, 200, 140, 50);

    /* Keyboard owned by the screen (not the modal) so its 200-px height
     * isn't clipped by the modal frame. Destroyed in dismiss_modal via
     * a separate path — store on s_st.modal_kb and delete explicitly. */
    s_st.modal_kb = lv_keyboard_create(s_st.screen);
    lv_obj_set_size(s_st.modal_kb, 800, 200);
    lv_obj_align(s_st.modal_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(s_st.modal_kb, ta);
    /* dismiss_modal() takes care of deleting s_st.modal_kb before the
     * modal itself — see the comment there. */
}

/* ---------- Text view ---------- */

static bool looks_textual(const uint8_t *buf, size_t n)
{
    /* Heuristic: > 5% non-printable in the first chunk → treat as
     * binary and switch to hex view. Tabs/newlines/CR count as
     * printable. */
    if (n == 0) return true;
    size_t bad = 0;
    for (size_t i = 0; i < n; ++i) {
        uint8_t c = buf[i];
        if (c == '\t' || c == '\n' || c == '\r') continue;
        if (c < 0x20 || c == 0x7f) bad++;
    }
    return (bad * 20) < n;  /* < 5% */
}

static void render_as_hex(char *out, size_t out_sz,
                          const uint8_t *buf, size_t n)
{
    size_t off = 0;
    for (size_t i = 0; i < n && off + 80 < out_sz; i += 16) {
        off += snprintf(out + off, out_sz - off, "%04zx  ", i);
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < n) {
                off += snprintf(out + off, out_sz - off, "%02x ",
                                buf[i + j]);
            } else {
                off += snprintf(out + off, out_sz - off, "   ");
            }
        }
        off += snprintf(out + off, out_sz - off, " ");
        for (size_t j = 0; j < 16 && i + j < n; ++j) {
            uint8_t c = buf[i + j];
            off += snprintf(out + off, out_sz - off, "%c",
                            (c >= 0x20 && c < 0x7f) ? c : '.');
        }
        off += snprintf(out + off, out_sz - off, "\n");
    }
}

static void open_text_view_modal(void)
{
    lv_obj_t *m = make_modal_container(780, 460);

    lv_obj_t *title = lv_label_create(m);
    char base[MAX_NAME];
    basename_of(s_st.selected_path, base, sizeof base);
    lv_label_set_text(title, base);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title, 600);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(title, 20, 14);

    make_modal_button(m, "Close", on_modal_close,
                      lv_color_hex(0x424242), 640, 14, 120, 36);

    /* Read first TEXT_PREVIEW_N bytes. esp_vfs_fat hands us POSIX file
     * I/O on the SD card — fopen/fread is fine. */
    FILE *f = fopen(s_st.selected_path, "rb");
    if (!f) {
        lv_obj_t *err = lv_label_create(m);
        lv_label_set_text_fmt(err, "Cannot open: errno=%d", errno);
        lv_obj_set_style_text_color(err, lv_color_hex(0xff8080), 0);
        lv_obj_set_pos(err, 20, 70);
        return;
    }
    uint8_t *raw = malloc(TEXT_PREVIEW_N);
    if (!raw) {
        fclose(f);
        lv_obj_t *err = lv_label_create(m);
        lv_label_set_text(err, "Out of memory");
        lv_obj_set_style_text_color(err, lv_color_hex(0xff8080), 0);
        lv_obj_set_pos(err, 20, 70);
        return;
    }
    size_t got = fread(raw, 1, TEXT_PREVIEW_N, f);
    bool truncated = (got == TEXT_PREVIEW_N);
    fclose(f);

    char *rendered = NULL;
    bool is_hex = !looks_textual(raw, got);
    if (is_hex) {
        /* 16 bytes per line → ceil(got/16) lines × ~75 chars = generous
         * upper bound on a 4 KB preview. */
        size_t cap = (got / 16 + 1) * 80 + 64;
        rendered = malloc(cap);
        if (rendered) {
            render_as_hex(rendered, cap, raw, got);
        }
    } else {
        /* Treat as text — strip NULs (FAT artefacts) and copy as a
         * C-string. lv_textarea limits text to whatever set_text feeds. */
        rendered = malloc(got + 16);
        if (rendered) {
            size_t off = 0;
            for (size_t i = 0; i < got; ++i) {
                uint8_t c = raw[i];
                if (c == 0) continue;
                rendered[off++] = (char)c;
            }
            rendered[off] = '\0';
        }
    }
    free(raw);

    lv_obj_t *ta = lv_textarea_create(m);
    lv_obj_set_size(ta, 740, 350);
    lv_obj_set_pos(ta, 20, 60);
    lv_textarea_set_one_line(ta, false);
    lv_obj_add_state(ta, LV_STATE_DISABLED);  /* read-only display */
    /* Mono-ish look for hex; default font for text. Default is fine. */
    if (rendered) {
        lv_textarea_set_text(ta, rendered);
        free(rendered);
    } else {
        lv_textarea_set_text(ta, "(empty or out of memory)");
    }

    if (truncated) {
        lv_obj_t *tlbl = lv_label_create(m);
        lv_label_set_text_fmt(tlbl,
            "(showing first %d bytes%s)", TEXT_PREVIEW_N,
            is_hex ? " — binary, hex view" : "");
        lv_obj_set_style_text_color(tlbl, lv_color_hex(0xb0b0b0), 0);
        lv_obj_set_pos(tlbl, 20, 420);
    } else if (is_hex) {
        lv_obj_t *tlbl = lv_label_create(m);
        lv_label_set_text(tlbl, "(binary — hex view)");
        lv_obj_set_style_text_color(tlbl, lv_color_hex(0xb0b0b0), 0);
        lv_obj_set_pos(tlbl, 20, 420);
    }
}

/* ---------- Image view ---------- */

static void open_image_view_modal(void)
{
    /* Decoded RGBA can be ~10× the source for PNG; cap by file size so a
     * stray 50 MB PNG can't OOM us. PSRAM has plenty for typical
     * dashcam-style photos, but a hard ceiling keeps things predictable. */
    if (s_st.selected_size > (8U << 20)) {
        dismiss_modal();
        show_toast("Image too large to preview (>8 MiB)");
        return;
    }

    /* LVGL FS_POSIX driver mounts at letter 'S' (see lv_conf.h). Anything
     * after "S:" is fed verbatim to open(2) — we pass the full POSIX path
     * so /sdcard/foo.png becomes S:/sdcard/foo.png. */
    char lvpath[MAX_PATH + 4];
    snprintf(lvpath, sizeof lvpath, "S:%s", s_st.selected_path);

    lv_img_header_t hdr = {0};
    lv_res_t hr = lv_img_decoder_get_info(lvpath, &hdr);
    if (hr != LV_RES_OK || hdr.w == 0 || hdr.h == 0) {
        ESP_LOGW(TAG, "image decoder rejected %s (res=%d)", lvpath, hr);
        dismiss_modal();
        show_toast("Cannot decode image");
        return;
    }

    lv_obj_t *m = make_modal_container(780, 460);

    char base[MAX_NAME];
    basename_of(s_st.selected_path, base, sizeof base);
    char title_buf[MAX_NAME + 32];
    snprintf(title_buf, sizeof title_buf, "%s  (%d\xc3\x97%d)",
             base, (int)hdr.w, (int)hdr.h);

    lv_obj_t *title = lv_label_create(m);
    lv_label_set_text(title, title_buf);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title, 600);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(title, 20, 14);

    make_modal_button(m, "Close", on_modal_close,
                      lv_color_hex(0x424242), 640, 14, 120, 36);

    /* Fit-to-viewport zoom (downscale only — never blow small icons up).
     * 256 = native 1×; smaller = shrink. */
    const int vw = 720, vh = 380;
    int zw = 256 * vw / (int)hdr.w;
    int zh = 256 * vh / (int)hdr.h;
    int zoom = (zw < zh) ? zw : zh;
    if (zoom > 256) zoom = 256;
    int sw = (int)hdr.w * zoom / 256;
    int sh = (int)hdr.h * zoom / 256;

    /* Pivot at (0,0) so scaled bbox lines up with object position — the
     * default centre-pivot otherwise pulls the visible image off-screen
     * when we resize the object to the scaled dimensions. */
    lv_obj_t *img = lv_img_create(m);
    lv_img_set_src(img, lvpath);
    lv_img_set_zoom(img, zoom);
    lv_img_set_pivot(img, 0, 0);
    lv_img_set_antialias(img, true);
    lv_obj_set_size(img, sw, sh);
    lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 64);
}

/* ---------- Delete confirm ---------- */

static void on_delete_yes(lv_event_t *e)
{
    (void)e;
    if (unlink(s_st.selected_path) != 0) {
        ESP_LOGW(TAG, "unlink %s failed: errno=%d",
                 s_st.selected_path, errno);
        dismiss_modal();
        char toast[80];
        snprintf(toast, sizeof toast, "Delete failed (errno %d)", errno);
        show_toast(toast);
        return;
    }
    ESP_LOGI(TAG, "deleted %s", s_st.selected_path);
    dismiss_modal();
    list_dir_render();
    show_toast("Deleted");
}

static void open_delete_confirm(void)
{
    lv_obj_t *m = make_modal_container(560, 240);

    lv_obj_t *title = lv_label_create(m);
    lv_label_set_text(title, "Delete?");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(title, 20, 14);

    lv_obj_t *info = lv_label_create(m);
    char base[MAX_NAME];
    basename_of(s_st.selected_path, base, sizeof base);
    lv_label_set_text_fmt(info, "Permanently delete:\n  %s", base);
    lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(info, 520);
    lv_obj_set_style_text_color(info, lv_color_hex(0xd0d0d0), 0);
    lv_obj_set_pos(info, 20, 60);

    make_modal_button(m, "Delete", on_delete_yes,
                      lv_color_hex(0xb00020), 20, 170, 240, 56);
    make_modal_button(m, "Cancel", on_modal_close,
                      lv_color_hex(0x424242), 300, 170, 240, 56);
}

/* ---------- Move ---------- */

static void on_move_here(lv_event_t *e)
{
    (void)e;
    if (s_st.move_mode != MOVE_PICK_DEST) return;
    char dst[MAX_PATH];
    path_join(dst, sizeof dst, s_st.cwd, s_st.move_src_name);
    if (strcmp(dst, s_st.move_src) == 0) {
        show_toast("Source and destination are the same");
        exit_move_mode();
        list_dir_render();
        return;
    }
    if (rename(s_st.move_src, dst) != 0) {
        ESP_LOGW(TAG, "move %s -> %s failed: errno=%d",
                 s_st.move_src, dst, errno);
        char toast[96];
        snprintf(toast, sizeof toast, "Move failed (errno %d)", errno);
        show_toast(toast);
        exit_move_mode();
        list_dir_render();
        return;
    }
    ESP_LOGI(TAG, "moved %s -> %s", s_st.move_src, dst);
    exit_move_mode();
    list_dir_render();
    show_toast("Moved");
}

static void enter_move_mode(void)
{
    /* Stash source path + basename, then re-render — list_dir_render
     * filters out files and the title flips to "Pick destination". */
    strncpy(s_st.move_src, s_st.selected_path, sizeof(s_st.move_src) - 1);
    s_st.move_src[sizeof(s_st.move_src) - 1] = '\0';
    basename_of(s_st.move_src, s_st.move_src_name, sizeof(s_st.move_src_name));
    s_st.move_mode = MOVE_PICK_DEST;
    dismiss_modal();
    /* Start the picker from the source's parent directory so the user
     * sees siblings first — typical case is "move file A into folder B
     * that lives next to A". */
    strncpy(s_st.cwd, s_st.move_src, sizeof(s_st.cwd) - 1);
    s_st.cwd[sizeof(s_st.cwd) - 1] = '\0';
    path_go_up(s_st.cwd);
    list_dir_render();
}

static void exit_move_mode(void)
{
    s_st.move_mode = MOVE_OFF;
    s_st.move_src[0] = '\0';
    s_st.move_src_name[0] = '\0';
}

/* ---------- Close + tear-down ---------- */

static void tear_down(void)
{
    dismiss_modal();
    if (s_st.prev_screen) {
        lv_scr_load(s_st.prev_screen);
    }
    if (s_st.screen) {
        lv_obj_del(s_st.screen);
    }
    memset(&s_st, 0, sizeof(s_st));
}

static void on_close(lv_event_t *e)
{
    (void)e;
    if (s_st.move_mode == MOVE_PICK_DEST) {
        /* In move mode, Close acts as Cancel-the-move rather than
         * leaving the screen entirely. Less surprising than losing the
         * user's place after they hit Move by accident. */
        exit_move_mode();
        list_dir_render();
        return;
    }
    tear_down();
}

/* ---------- Public entry ---------- */

void files_screen_show(void)
{
    if (s_st.screen) {
        ESP_LOGW(TAG, "already visible — ignoring duplicate open");
        return;
    }

    /* Try to mount the microSD (idempotent — ESP_ERR_INVALID_STATE if it
     * is already mounted). Done before taking the LVGL lock so a slow/absent
     * card can't stall rendering. No card / mount failure → /sdcard is simply
     * omitted from the synthetic root. Also kick the internal LittleFS mount
     * (async) so /vescfs becomes browsable. */
    esp_err_t sderr = bsp_sdcard_mount();
    bool sd_ok = (sderr == ESP_OK || sderr == ESP_ERR_INVALID_STATE);
    if (!sd_ok) {
        ESP_LOGI(TAG, "no SD card (mount rc=0x%x) — /sdcard hidden", sderr);
    }
    app_fs_ensure();

    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGE(TAG, "display lock timeout");
        return;
    }

    s_st.sd_available = sd_ok;

    /* Start at the synthetic root listing the mounted drives. We don't try
     * to remember the last cwd between opens — predictable behaviour beats
     * "where did it leave me?" in a head-unit context. */
    strncpy(s_st.cwd, SYN_ROOT, sizeof(s_st.cwd) - 1);
    s_st.cwd[sizeof(s_st.cwd) - 1] = '\0';
    s_st.move_mode = MOVE_OFF;

    s_st.prev_screen = lv_scr_act();

    s_st.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_st.screen, lv_color_hex(0x101418), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_st.screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(s_st.screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Header row: title left, "Move here" (hidden until move-pick mode),
     * Close right. */
    s_st.title_lbl = lv_label_create(s_st.screen);
    lv_label_set_text(s_st.title_lbl, "Files");
    lv_obj_set_style_text_color(s_st.title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_st.title_lbl, &lv_font_montserrat_32, 0);
    lv_obj_set_pos(s_st.title_lbl, 20, 8);

    s_st.btn_move_here = lv_btn_create(s_st.screen);
    lv_obj_set_size(s_st.btn_move_here, 160, 40);
    lv_obj_set_pos(s_st.btn_move_here, 460, 8);
    lv_obj_set_style_bg_color(s_st.btn_move_here,
                              lv_color_hex(0x2e7d32), 0);
    lv_obj_add_event_cb(s_st.btn_move_here, on_move_here,
                        LV_EVENT_CLICKED, NULL);
    {
        lv_obj_t *l = lv_label_create(s_st.btn_move_here);
        lv_label_set_text(l, "Move here");
        lv_obj_set_style_text_color(l, lv_color_white(), 0);
        lv_obj_center(l);
    }
    lv_obj_add_flag(s_st.btn_move_here, LV_OBJ_FLAG_HIDDEN);

    s_st.btn_close = lv_btn_create(s_st.screen);
    lv_obj_set_size(s_st.btn_close, 110, 40);
    lv_obj_set_pos(s_st.btn_close, 670, 8);
    lv_obj_set_style_bg_color(s_st.btn_close, lv_color_hex(0x424242), 0);
    lv_obj_add_event_cb(s_st.btn_close, on_close, LV_EVENT_CLICKED, NULL);
    {
        lv_obj_t *l = lv_label_create(s_st.btn_close);
        lv_label_set_text(l, "Close");
        lv_obj_set_style_text_color(l, lv_color_white(), 0);
        lv_obj_center(l);
    }

    /* Path label — full-width, second row. LONG_DOT keeps deep paths
     * from wrapping; the user can still navigate, just can't see the
     * full string at once. */
    s_st.path_lbl = lv_label_create(s_st.screen);
    lv_label_set_text(s_st.path_lbl, s_st.cwd);
    lv_obj_set_style_text_color(s_st.path_lbl, lv_color_hex(0x9fc4ff), 0);
    lv_label_set_long_mode(s_st.path_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(s_st.path_lbl, 760);
    lv_obj_set_pos(s_st.path_lbl, 20, 56);

    /* List container: scrollable, vertical. Row spacing is 48 px; rows
     * are sized by add_row. */
    s_st.list = lv_obj_create(s_st.screen);
    lv_obj_set_size(s_st.list, 800, 480 - 90);
    lv_obj_set_pos(s_st.list, 0, 90);
    lv_obj_set_style_bg_opa(s_st.list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_st.list, 0, 0);
    lv_obj_set_style_pad_all(s_st.list, 20, 0);
    lv_obj_set_scroll_dir(s_st.list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_st.list, LV_SCROLLBAR_MODE_AUTO);

    list_dir_render();

    lv_scr_load(s_st.screen);
    bsp_display_unlock();
    ESP_LOGI(TAG, "shown");
}
