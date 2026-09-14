/*
 * VESC link settings screen (show_vesc_link_settings).
 *
 * Everything about reaching the motor controller lives here, because all of it
 * is one decision with several parts: which wire, which node, and — on BLE —
 * how the request is addressed.
 *
 *   - the transport selector, applied immediately (main.c swaps the transport
 *     on a timer task; the dashboard reconnects by itself);
 *   - which controller to poll: Target VESC ID, and the optional second head.
 *     These used to sit on the main Settings screen among the display and
 *     battery options, where they read as unrelated numbers;
 *   - the CAN bus parameters — this display's own node id and the bus speed —
 *     which only mean anything in CAN mode and are labelled as such;
 *   - adapter pairing: "Search" starts the BLE central scan
 *     (ble_vesc_scan_start), advertisers carrying the Nordic UART Service
 *     stream in via the scan callback and become tappable rows; tapping pairs
 *     that adapter (vesc_ble_link_select). "Forget" unpairs;
 *   - live link state — connected / round trip / requests answered — so a
 *     pairing can be verified on the spot, and a link that connects but
 *     carries nothing says so instead of leaving a blank dashboard;
 *   - addressing, which decides whether payloads get wrapped in
 *     COMM_FORWARD_CAN. The default asks the link rather than guessing; see
 *     main/vesc_ble_link.c.
 *
 * The scan callback fires on the NimBLE host task, so it only stashes the hit
 * and marshals the list rebuild onto the LVGL task with lv_async_call — same
 * shape as speed_screen.c / pas_screen.c.
 *
 * Device-only (needs the BLE backend in main/); the simulator gets a stub.
 */
#include "lvgl.h"
#include "ui_geom.h"
#include "custom.h"

extern lv_ui guider_ui;

#ifdef LV_REALDEVICE

#include "freertos/FreeRTOS.h"
#include "ble_vesc_client.h"
#include "vesc_ble_link.h"
#include "settings_wrapper.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---- palette (matches speed_screen.c / the rest of the UI) ---- */
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

static lv_obj_t   *s_screen;
static lv_timer_t *s_timer;
static bool        s_alive;

/* live labels */
static lv_obj_t *s_status_lbl, *s_rt_lbl, *s_drops_lbl, *s_mtu_lbl;
static lv_obj_t *s_peer_lbl, *s_answer_lbl;

/* NVS commits stall the LVGL thread for ~100 ms, and these are +/- buttons the
 * user taps in bursts — so every tap only updates the cache (and hot-applies),
 * and one debounce timer writes them out once the tapping stops. Also flushed
 * when the screen closes, in case the timer never fires. */
#define PERSIST_DEBOUNCE_MS 800
static lv_timer_t *s_persist_timer;
static bool        s_dirty_target, s_dirty_ctrl, s_dirty_head2;

static void persist_now(void)
{
    if (s_dirty_target) { settings_wrapper_persist_target_vesc_id(); s_dirty_target = false; }
    if (s_dirty_ctrl)   { settings_wrapper_persist_controller_id();  s_dirty_ctrl   = false; }
    if (s_dirty_head2)  { settings_wrapper_persist_second_head_id(); s_dirty_head2  = false; }
}

static void persist_timer_cb(lv_timer_t *t)
{
    (void)t;
    persist_now();
    if (s_persist_timer) { lv_timer_del(s_persist_timer); s_persist_timer = NULL; }
}

static void persist_soon(void)
{
    if (s_persist_timer) lv_timer_del(s_persist_timer);
    s_persist_timer = lv_timer_create(persist_timer_cb, PERSIST_DEBOUNCE_MS, NULL);
}

/* scan results (written on the NimBLE host task, read on the LVGL task) */
static portMUX_TYPE s_hit_mux = portMUX_INITIALIZER_UNLOCKED;
static scanhit_t    s_hits[MAX_SCAN];
static volatile int s_hit_count;
static int          s_built_count;
static lv_obj_t    *s_scan_list;

/* ---- widget builders (same shapes as speed_screen.c) ---- */

/* A settings row is a caption on the left and one control on the right, and
 * the two have to share a width that is 800 px on the head units and 480 on
 * the ESP32-S3 board. Fixed pixels for the control plus a free-running caption
 * worked at 800 and collapsed at 480: the caption simply carried on under the
 * control, which — being created later — drew over it. Two shares that add up
 * to less than the row, and a caption that knows where it has to stop. */
#define ROW_NAME_PCT  46
#define ROW_CTRL_PCT  50

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
        lv_obj_set_width(n, lv_pct(ROW_NAME_PCT));
        lv_label_set_long_mode(n, LV_LABEL_LONG_DOT);
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

static void add_hint(lv_obj_t *list, const char *txt)
{
    lv_obj_t *s = lv_label_create(list);
    lv_label_set_text(s, txt);
    lv_obj_set_style_text_color(s, lv_color_hex(COL_DIM), 0);
    lv_obj_set_width(s, lv_pct(100));
    lv_label_set_long_mode(s, LV_LABEL_LONG_WRAP);
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

/* ---- number rows (Target VESC ID, Display CAN ID, second head ID) ----
 *
 * A label between two buttons. Deliberately not the hold-to-repeat stepper
 * from the main Settings screen: these are node ids in a range of a couple of
 * hundred that get set once, and each step hot-applies a transport reconfig. */

typedef struct {
    lv_obj_t *val;
    uint8_t   value;
    uint8_t   min, max;
    void    (*apply)(uint8_t v);
} numrow_t;

static numrow_t s_target_row, s_ctrl_row, s_head2_row;

static void numrow_refresh(numrow_t *r)
{
    if (!r->val) return;
    char buf[8];
    snprintf(buf, sizeof buf, "%u", (unsigned)r->value);
    lv_label_set_text(r->val, buf);
}

static void numrow_step(lv_event_t *e)
{
    numrow_t *r = (numrow_t *)lv_event_get_user_data(e);
    int delta = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    int v = (int)r->value + delta;
    if (v < r->min) v = r->max;          /* wrap: 254 ids is a long way to tap */
    if (v > r->max) v = r->min;
    r->value = (uint8_t)v;
    numrow_refresh(r);
    if (r->apply) r->apply(r->value);
}

static void numrow_add(lv_obj_t *list, const char *name, numrow_t *r,
                       uint8_t value, uint8_t min, uint8_t max,
                       void (*apply)(uint8_t))
{
    r->value = value;
    r->min   = min;
    r->max   = max;
    r->apply = apply;

    lv_obj_t *row = row_base(list, name, 52);

    static const int dx[2]    = { -1, +1 };
    static const char *gl[2]  = { "-", "+" };
    lv_obj_t *btn[2];
    for (int i = 0; i < 2; i++) {
        btn[i] = lv_btn_create(row);
        lv_obj_set_size(btn[i], 56, 44);
        lv_obj_set_style_bg_color(btn[i], lv_color_hex(COL_BTN), 0);
        lv_obj_set_style_radius(btn[i], 8, 0);
        lv_obj_set_user_data(btn[i], (void *)(intptr_t)dx[i]);
        lv_obj_t *l = lv_label_create(btn[i]);
        lv_label_set_text(l, gl[i]);
        lv_obj_set_style_text_font(l, UI_FONT_MENU, 0);
        lv_obj_center(l);
        lv_obj_add_event_cb(btn[i], numrow_step, LV_EVENT_CLICKED, r);
    }
    lv_obj_align(btn[0], LV_ALIGN_RIGHT_MID, -132, 0);
    lv_obj_align(btn[1], LV_ALIGN_RIGHT_MID, -8, 0);

    r->val = lv_label_create(row);
    lv_obj_align(r->val, LV_ALIGN_RIGHT_MID, -76, 0);
    lv_obj_set_style_text_color(r->val, lv_color_hex(COL_CYAN), 0);
    lv_obj_set_style_text_font(r->val, UI_FONT_MENU, 0);
    numrow_refresh(r);
}

static void apply_target_id(uint8_t v)
{
    settings_wrapper_set_target_vesc_id_volatile(v);
    s_dirty_target = true;
    persist_soon();
}

static void apply_ctrl_id(uint8_t v)
{
    settings_wrapper_set_controller_id_volatile(v);
    s_dirty_ctrl = true;
    persist_soon();
}

static void apply_head2_id(uint8_t v)
{
    settings_wrapper_set_second_head_id_volatile(v);
    s_dirty_head2 = true;
    persist_soon();
}

/* ---- transport selector ---- */

static void link_dd_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    settings_wrapper_set_vesc_link_ble(lv_dropdown_get_selected(dd) == 1);
}

static void route_dd_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    settings_wrapper_set_vesc_ble_route((uint8_t)lv_dropdown_get_selected(dd));
}

static void can_speed_dd_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    settings_wrapper_set_can_speed_index((uint8_t)lv_dropdown_get_selected(dd));
}

static void head2_sw_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    settings_wrapper_set_second_head_enabled(lv_obj_has_state(sw, LV_STATE_CHECKED));
}

/* ---- adapter pairing ---- */

static void hit_select_cb(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (i < 0 || i >= s_built_count) return;
    vesc_ble_link_select(s_hits[i].addr, s_hits[i].type);
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
    ble_vesc_scan_start();
}

static void forget_cb(lv_event_t *e) { (void)e; vesc_ble_link_forget(); }

/* ---- live updater ---- */

static void update_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_alive) return;

    vesc_ble_telem_t tel;
    vesc_ble_link_get_telem(&tel);

    const char *txt;
    uint32_t    col;
    if (tel.ready)          { txt = "Connected";    col = COL_ACCENT; }
    else if (tel.connected) { txt = "Linking...";   col = COL_AMBER; }
    else if (tel.scanning)  { txt = "Searching..."; col = COL_CYAN; }
    else if (tel.bound)     { txt = "Reconnecting..."; col = COL_AMBER; }
    else                    { txt = "Not paired";   col = COL_DIM; }
    if (s_status_lbl) {
        lv_label_set_text(s_status_lbl, txt);
        lv_obj_set_style_text_color(s_status_lbl, lv_color_hex(col), 0);
    }

    char buf[32];
    if (s_rt_lbl) {
        if (tel.ready && tel.last_rt_ms) {
            snprintf(buf, sizeof buf, "%u ms", (unsigned)tel.last_rt_ms);
        } else {
            snprintf(buf, sizeof buf, "--");
        }
        lv_label_set_text(s_rt_lbl, buf);
    }
    if (s_drops_lbl) {
        /* Link drops / unanswered requests / motor setpoints dropped for
         * being out of date. The last one growing means the link cannot keep
         * up with pedal assist. */
        snprintf(buf, sizeof buf, "%lu / %lu / %lu",
                 (unsigned long)tel.reconnects, (unsigned long)tel.timeouts,
                 (unsigned long)tel.stale_drops);
        lv_label_set_text(s_drops_lbl, buf);
    }
    if (s_mtu_lbl) {
        if (tel.mtu) snprintf(buf, sizeof buf, "%u B", (unsigned)tel.mtu);
        else         snprintf(buf, sizeof buf, "--");
        lv_label_set_text(s_mtu_lbl, buf);
    }

    /* Requests answered. This is the line that tells a connected-but-dead link
     * apart from a working one, which is the failure the addressing setting
     * causes and the one that used to look exactly like a broken dashboard. */
    if (s_answer_lbl) {
        snprintf(buf, sizeof buf, "%lu / %lu",
                 (unsigned long)tel.replies, (unsigned long)tel.requests);
        lv_label_set_text(s_answer_lbl, buf);
        uint32_t c = COL_DIM;
        if (tel.requests >= 4) c = tel.replies ? COL_ACCENT : COL_RED;
        lv_obj_set_style_text_color(s_answer_lbl, lv_color_hex(c), 0);
    }

    /* What the link found on the other end, and what it does about it. */
    if (s_peer_lbl) {
        char pbuf[96];
        uint32_t c = COL_DIM;
        uint8_t target = settings_wrapper_get_target_vesc_id();
        switch (tel.peer) {
        case VESC_BLE_PEER_CONTROLLER:
            if (!tel.wrapping) {
                snprintf(pbuf, sizeof pbuf, "controller, node %u", (unsigned)tel.peer_id);
                c = COL_ACCENT;
            } else if (target != tel.peer_id) {
                snprintf(pbuf, sizeof pbuf, "controller %u, target %u behind it",
                         (unsigned)tel.peer_id, (unsigned)target);
                c = COL_ACCENT;
            } else {
                /* The one combination that cannot work: we are wired to the
                 * target itself and still sending through its CAN bus, which
                 * means addressing it to itself. Only a manual override can
                 * produce this. */
                snprintf(pbuf, sizeof pbuf, "node %u - but routed via CAN, set Automatic",
                         (unsigned)tel.peer_id);
                c = COL_RED;
            }
            break;
        case VESC_BLE_PEER_BRIDGE:
            snprintf(pbuf, sizeof pbuf, "adapter - going on over CAN");
            c = COL_ACCENT;
            break;
        case VESC_BLE_PEER_SILENT:
            snprintf(pbuf, sizeof pbuf, "answers nothing (%u tries)",
                     (unsigned)tel.probe_tries);
            c = COL_RED;
            break;
        default:
            snprintf(pbuf, sizeof pbuf, tel.ready ? "asking..." : "--");
            break;
        }
        lv_label_set_text(s_peer_lbl, pbuf);
        lv_obj_set_style_text_color(s_peer_lbl, lv_color_hex(c), 0);
    }
}

/* ---- screen ---- */

static void back_cb(lv_event_t *e)
{
    (void)e;
    lv_scr_load_anim(guider_ui.settings, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

static void screen_unloaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    s_alive = false;
    ble_vesc_set_scan_cb(NULL);
    if (s_timer) { lv_timer_del(s_timer); s_timer = NULL; }
    /* Flush before the debounce timer dies with the screen, or the last few
     * taps would be live in RAM and gone after a reboot. */
    if (s_persist_timer) { lv_timer_del(s_persist_timer); s_persist_timer = NULL; }
    persist_now();
    if (s_screen) { lv_obj_del_async(s_screen); s_screen = NULL; }
    s_scan_list  = NULL;
    s_status_lbl = NULL;
    s_rt_lbl     = NULL;
    s_drops_lbl  = NULL;
    s_mtu_lbl    = NULL;
    s_peer_lbl   = NULL;
    s_answer_lbl = NULL;
    s_target_row.val = NULL;
    s_ctrl_row.val   = NULL;
    s_head2_row.val  = NULL;
}

void show_vesc_link_settings(void)
{
    if (s_screen) return;  /* re-entrancy guard */

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
    lv_label_set_text(title, "VESC Link");
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

    /* ---- Transport ---- */
    add_section(list, "Connection");
    lv_obj_t *lrow = row_base(list, "Talk to the VESC over", 50);
    lv_obj_t *dd = lv_dropdown_create(lrow);
    lv_dropdown_set_options(dd, "CAN bus\nBluetooth");
    lv_obj_set_width(dd, lv_pct(ROW_CTRL_PCT));
    lv_obj_align(dd, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_dropdown_set_selected(dd, settings_wrapper_get_vesc_link_ble() ? 1 : 0);
    lv_obj_add_event_cb(dd, link_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);
    add_hint(list, "Bluetooth needs no CAN wiring: this display connects over "
                   "the Nordic UART Service the same way VESC Tool does - to a "
                   "VESC Express, or straight to a BLE module on the "
                   "controller's own UART.");

    /* ---- Which controller ---- */
    add_section(list, "Controller");
    numrow_add(list, "Target VESC ID", &s_target_row,
               settings_wrapper_get_target_vesc_id(), 1, 254, apply_target_id);

    lv_obj_t *h2row = row_base(list, "Second head", 50);
    lv_obj_t *h2sw = lv_switch_create(h2row);
    lv_obj_align(h2sw, LV_ALIGN_RIGHT_MID, -8, 0);
    if (settings_wrapper_get_second_head_enabled()) {
        lv_obj_add_state(h2sw, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(h2sw, head2_sw_cb, LV_EVENT_VALUE_CHANGED, NULL);

    numrow_add(list, "Second head ID", &s_head2_row,
               settings_wrapper_get_second_head_id(), 1, 254, apply_head2_id);
    add_hint(list, "The dashboard polls the target; a second head only adds its "
                   "temperatures. With it on, the dashboard treats a silent "
                   "second head as a dead link - so leave it off unless there "
                   "really are two controllers.");

    /* ---- CAN bus ---- */
    add_section(list, "CAN bus");
    numrow_add(list, "This display's CAN ID", &s_ctrl_row,
               settings_wrapper_get_controller_id(), 1, 254, apply_ctrl_id);

    lv_obj_t *csrow = row_base(list, "Bus speed (kbps)", 50);
    lv_obj_t *csdd = lv_dropdown_create(csrow);
    lv_dropdown_set_options(csdd, "125\n250\n500\n1000");
    lv_obj_set_width(csdd, lv_pct(ROW_CTRL_PCT));
    lv_obj_align(csdd, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_dropdown_set_selected(csdd, settings_wrapper_get_can_speed_index());
    lv_obj_add_event_cb(csdd, can_speed_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);
    add_hint(list, "Used only in CAN mode. The display's own ID has to be free "
                   "on the bus - VESC Tool lists it there as "
                   "'Super VESC Display'.");

    /* ---- Adapter ---- */
    add_section(list, "Bluetooth adapter");

    lv_obj_t *strow = row_base(list, "Status", 40);
    s_status_lbl = live_cell(strow, COL_DIM, NULL);
    lv_obj_t *anrow = row_base(list, "Answered / sent", 40);
    s_answer_lbl = live_cell(anrow, COL_DIM, NULL);
    lv_obj_t *prow = row_base(list, "Other end", 40);
    s_peer_lbl = live_cell(prow, COL_DIM, NULL);
    lv_obj_t *rtrow = row_base(list, "Round trip", 40);
    s_rt_lbl = live_cell(rtrow, COL_CYAN, NULL);
    lv_obj_t *dprow = row_base(list, "Drops / timeouts / stale", 40);
    s_drops_lbl = live_cell(dprow, COL_TEXT, NULL);
    lv_obj_t *mturow = row_base(list, "Packet size (MTU)", 40);
    s_mtu_lbl = live_cell(mturow, COL_TEXT, NULL);

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

    add_hint(list, "An adapter that does not advertise its name shows as '?'. "
                   "Only one device can hold a VESC Express link at a time - "
                   "disconnect VESC Tool on the phone first.");

    /* ---- Addressing ---- */
    add_section(list, "Bluetooth addressing");
    lv_obj_t *rtrow2 = row_base(list, "Reach the controller", 50);
    lv_obj_t *rdd = lv_dropdown_create(rtrow2);
    lv_dropdown_set_options(rdd, "Automatic\nDirect\nVia the adapter");
    lv_obj_set_width(rdd, lv_pct(ROW_CTRL_PCT));
    lv_obj_align(rdd, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_dropdown_set_selected(rdd, settings_wrapper_get_vesc_ble_route());
    lv_obj_add_event_cb(rdd, route_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);

    add_hint(list, "Automatic asks the device on the other end which CAN node "
                   "it is, then sends straight to it and routes anything else "
                   "through COMM_FORWARD_CAN. Pick a fixed mode only if that "
                   "answer is wrong - 'Other end' above shows what it found.");

    add_hint(list, "The quick-action drawer and the cruise/profile readout "
                   "need the LISP script from this firmware's lisp/main.lisp "
                   "(2026-09 or newer) on the controller - older scripts only "
                   "answer over CAN.");

    lv_obj_add_event_cb(s_screen, screen_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    s_alive = true;

    /* Receive scan hits while this screen is open, and refresh live values. */
    ble_vesc_set_scan_cb(scan_cb);
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

void show_vesc_link_settings(void)
{
    if (s_sim_screen) return;
    s_sim_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_sim_screen, UI_W, UI_H);
    lv_obj_set_style_bg_color(s_sim_screen, lv_color_hex(0x07090A), 0);

    lv_obj_t *btn = lv_btn_create(s_sim_screen);
    lv_obj_set_pos(btn, 17, 14);
    lv_obj_set_size(btn, 120, 40);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "Back");
    lv_obj_center(bl);
    lv_obj_add_event_cb(btn, sim_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(s_sim_screen);
    lv_label_set_text(lbl, "VESC link settings are available on the device build.");
    lv_obj_center(lbl);

    lv_obj_add_event_cb(s_sim_screen, sim_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_scr_load_anim(s_sim_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

#endif /* LV_REALDEVICE */
