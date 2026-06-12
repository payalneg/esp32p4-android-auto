/*
    Copyright 2026 Super VESC Display

    Dashboard theme registry + switcher + the update_*() dispatchers.
    See dashboard_theme.h for the model. This file has NO dependency on the
    device-only main/ code (bsp, ui_mode, music_info_view): the post-switch
    chrome re-attach is delegated to a callback the app registers, so the file
    compiles unchanged in the desktop simulator.
*/

#include "dashboard_theme.h"

#include "gui_guider.h"   /* guider_ui — the active-theme screen slot */
#include "custom.h"       /* the public update_*() prototypes we implement */

#define DASHBOARD_THEME_MAX 8

static const dashboard_theme_t *s_reg[DASHBOARD_THEME_MAX];
static int s_count;
static int s_active = -1;
static dashboard_theme_switch_cb_t s_switch_cb;

void dashboard_theme_register(const dashboard_theme_t *t)
{
    if (!t || s_count >= DASHBOARD_THEME_MAX) return;
    /* Ignore duplicate registrations (custom_init_once can run twice across
     * simulator setup_ui + a later re-entry). */
    for (int i = 0; i < s_count; i++) {
        if (s_reg[i] == t) return;
    }
    s_reg[s_count++] = t;
}

int dashboard_theme_count(void) { return s_count; }

const dashboard_theme_t *dashboard_theme_get(int idx)
{
    return (idx >= 0 && idx < s_count) ? s_reg[idx] : NULL;
}

int dashboard_theme_active_index(void) { return s_active; }

lv_obj_t *dashboard_theme_active_screen(void) { return guider_ui.dashboard_Classic; }

void dashboard_theme_set_switch_cb(dashboard_theme_switch_cb_t cb) { s_switch_cb = cb; }

static const dashboard_theme_ops_t *active_ops(void)
{
    return (s_active >= 0 && s_active < s_count) ? s_reg[s_active]->ops : NULL;
}

static void fire_switch_cb(int idx)
{
    if (!s_switch_cb) return;
    const dashboard_theme_t *t = s_reg[idx];
    s_switch_cb(guider_ui.dashboard_Classic, t->music_tile ? t->music_tile() : NULL);
}

void dashboard_theme_adopt(int idx)
{
    if (s_count == 0) return;
    if (idx < 0 || idx >= s_count) idx = 0;
    s_active = idx;
    /* The generated settings-exit nav loads *guider_ui.dashboard_Classic and rebuilds
     * it only when dashboard_del is set — keep it clear so the active theme
     * survives a Settings round-trip without being clobbered by
     * setup_scr_dashboard. */
    guider_ui.dashboard_Classic_del = false;
    fire_switch_cb(idx);
}

void dashboard_theme_build(int idx)
{
    if (s_count == 0) return;
    if (idx < 0 || idx >= s_count) idx = 0;
    const dashboard_theme_t *t = s_reg[idx];
    if (t->create) guider_ui.dashboard_Classic = t->create();
    dashboard_theme_adopt(idx);
}

void dashboard_theme_set(int idx)
{
    if (idx < 0 || idx >= s_count) return;
    if (idx == s_active) return;

    const dashboard_theme_t *old     = (s_active >= 0) ? s_reg[s_active] : NULL;
    lv_obj_t                *old_scr  = guider_ui.dashboard_Classic;
    /* Are we looking at the dashboard right now? (false when the switch is
     * triggered from the Settings screen, which is the usual path.) */
    bool was_loaded = (old_scr && lv_scr_act() == old_scr);

    const dashboard_theme_t *t = s_reg[idx];
    lv_obj_t *new_scr = t->create ? t->create() : NULL;
    guider_ui.dashboard_Classic     = new_scr;
    guider_ui.dashboard_Classic_del = false;
    s_active = idx;

    if (was_loaded && new_scr) {
        /* Direct switch while the dashboard is on screen — load the rebuilt
         * one now. The Settings path skips this; the new screen shows when the
         * user taps "exit", via the generated nav that loads guider_ui.dashboard_Classic. */
        lv_scr_load(new_scr);
        lv_obj_invalidate(new_scr);
    }

    /* Re-attach external chrome (the music tile) to the new screen's widgets
     * BEFORE freeing the old screen: the app's hook stops the music poller and
     * drops its references first, so nothing can touch the about-to-be-freed
     * old tile, then re-attaches onto the new theme's tile (or tears down when
     * the new theme has none). */
    fire_switch_cb(idx);

    /* Tear down the previous theme. The new screen is already the active slot
     * (and loaded, if it was on screen), so deleting the old screen here can
     * never delete the currently-shown screen. */
    if (old && old->destroy) old->destroy();
    if (old_scr && old_scr != new_scr) lv_obj_del(old_scr);
}

/* ============ update_*() dispatchers ==================================== *
 * The canonical data feed (vesc_ui_updater) and the demo loop call these;
 * each forwards to the active theme, skipping fields the theme doesn't draw. */

#define DISPATCH0(fn, member) \
    void fn(void) { const dashboard_theme_ops_t *o = active_ops(); if (o && o->member) o->member(); }
#define DISPATCH1(fn, member, t0) \
    void fn(t0 a0) { const dashboard_theme_ops_t *o = active_ops(); if (o && o->member) o->member(a0); }
#define DISPATCH2(fn, member, t0, t1) \
    void fn(t0 a0, t1 a1) { const dashboard_theme_ops_t *o = active_ops(); if (o && o->member) o->member(a0, a1); }
#define DISPATCH3(fn, member, t0, t1, t2) \
    void fn(t0 a0, t1 a1, t2 a2) { const dashboard_theme_ops_t *o = active_ops(); if (o && o->member) o->member(a0, a1, a2); }

DISPATCH1(update_current,               current,               float)
DISPATCH1(update_speed,                 speed,                 float)
DISPATCH1(update_battery_proc,          battery_proc,          float)
DISPATCH1(update_battery_voltage,       battery_voltage,       float)
DISPATCH1(update_battery_temp,          battery_temp,          float)
DISPATCH1(update_temp_fet,              temp_fet,              float)
DISPATCH1(update_temp_motor,            temp_motor,            float)
DISPATCH1(update_trip,                  trip,                  float)
DISPATCH1(update_range,                 range,                 float)
DISPATCH1(update_odometer,              odometer,              float)
DISPATCH1(update_amp_hours,             amp_hours,             float)
DISPATCH1(update_uptime,                uptime,                uint32_t)
DISPATCH1(update_fps,                   fps,                   int)
DISPATCH1(update_ble_status,            ble_status,            bool)
DISPATCH1(update_esc_connection_status, esc_connection_status, bool)
DISPATCH1(update_cruise_control_status, cruise_control_status, bool)
DISPATCH1(update_cruise_speed,          cruise_speed,          float)
DISPATCH1(update_mode_text,             mode_text,             uint8_t)
DISPATCH1(update_navigation_text,       navigation_text,       const char *)
DISPATCH1(update_music_text,            music_text,            const char *)
DISPATCH0(dashboard_units_changed,      units_changed)
DISPATCH0(hide_cur_time,                hide_cur_time)
DISPATCH0(hide_mode_text,               hide_mode_text)
DISPATCH2(update_cur_time_hm,           cur_time_hm,           int, int)
DISPATCH3(update_cur_time,              cur_time,              int, int, int)

void update_navigation_icon(const uint8_t *img_data, uint32_t data_size,
                            uint16_t width, uint16_t height, lv_img_cf_t color_format)
{
    const dashboard_theme_ops_t *o = active_ops();
    if (o && o->navigation_icon) o->navigation_icon(img_data, data_size, width, height, color_format);
}
