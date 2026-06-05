/*
 * Realtime data viewer (show_realtime_viewer).
 *
 * A self-contained LVGL screen — created on demand, destroyed on unload, like
 * the VESC Tool config menu — that shows live VESC telemetry plus the decoded
 * ADC and PPM inputs. Telemetry comes from vesc_rt_data (already polled at
 * 100 ms); ADC/PPM come from vesc_io_data, which only polls while this screen
 * is open (set_active true/false on enter/leave). A lv_timer refreshes the
 * value labels ~5x/s by reading the shared snapshots — no CAN marshalling
 * needed since the snapshots are updated on the CAN task and only read here.
 *
 * Device-only (needs vesc_can); the desktop simulator gets a placeholder.
 */
#include "lvgl.h"
#include "custom.h"

extern lv_ui guider_ui;

#ifdef LV_REALDEVICE

#include "vesc_can/vesc_rt_data.h"
#include "vesc_can/vesc_io_data.h"

#include <stdio.h>

/* ---- palette (matches the rest of the UI) ---- */
#define COL_BG        0x07090A
#define COL_PANEL     0x12181C
#define COL_BTN       0x2a3440
#define COL_ACCENT    0xB6FF2E
#define COL_CYAN      0x00a9ff
#define COL_TEXT      0xFFFFFF
#define COL_DIM       0x8A9499

typedef enum {
    RT_VIN, RT_BATT, RT_IIN, RT_IMOTOR, RT_POWER, RT_DUTY, RT_RPM, RT_SPEED,
    RT_TFET, RT_TMOT, RT_AH, RT_WH, RT_FAULT,
    RT_ADC1, RT_ADC1V, RT_ADC2, RT_ADC2V,
    RT_PPM, RT_PPMMS,
    RT_COUNT
} rt_field_t;

static lv_obj_t  *s_screen;
static lv_obj_t  *s_val[RT_COUNT];
static lv_timer_t *s_timer;
static bool       s_alive;

static void back_cb(lv_event_t *e)
{
    (void)e;
    /* Return to the VESC Tool config menu (rebuilt from preserved state). */
    run_vesc_tool_menu();
}

static void screen_unloaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_UNLOADED) return;
    s_alive = false;
    vesc_io_data_set_active(false);
    if (s_timer) { lv_timer_del(s_timer); s_timer = NULL; }
    if (s_screen) { lv_obj_del_async(s_screen); s_screen = NULL; }
    for (int i = 0; i < RT_COUNT; i++) s_val[i] = NULL;
}

static lv_obj_t *add_row(lv_obj_t *list, const char *name)
{
    lv_obj_t *row = lv_obj_create(list);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 34);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 2, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *n = lv_label_create(row);
    lv_label_set_text(n, name);
    lv_obj_align(n, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_text_color(n, lv_color_hex(COL_DIM), 0);

    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, "--");
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(COL_TEXT), 0);
    return v;
}

static void add_section(lv_obj_t *list, const char *txt)
{
    lv_obj_t *s = lv_label_create(list);
    lv_label_set_text(s, txt);
    lv_obj_set_style_text_color(s, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_text_font(s, &lv_font_montserratMedium_16, 0);
    lv_obj_set_style_pad_top(s, 6, 0);
}

static void set_val(rt_field_t f, bool ok, const char *fmt, double val)
{
    if (f >= RT_COUNT || !s_val[f]) return;
    if (!ok) { lv_label_set_text(s_val[f], "--"); return; }
    char b[32];
    snprintf(b, sizeof b, fmt, val);
    lv_label_set_text(s_val[f], b);
}

static void update_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_alive) return;

    const vesc_setup_values_t *d = vesc_rt_data_get_latest();
    bool fresh = vesc_rt_data_is_fresh();

    set_val(RT_VIN,    fresh, "%.1f V",    d->v_in);
    set_val(RT_BATT,   fresh, "%.0f %%",   d->battery_level * 100.0);
    set_val(RT_IIN,    fresh, "%.1f A",    d->current_in);
    set_val(RT_IMOTOR, fresh, "%.1f A",    d->current_motor);
    set_val(RT_POWER,  fresh, "%.0f W",    (double)d->current_in * d->v_in);
    set_val(RT_DUTY,   fresh, "%.1f %%",   d->duty_now * 100.0);
    set_val(RT_RPM,    fresh, "%.0f",      d->rpm);
    set_val(RT_SPEED,  fresh, "%.1f km/h", d->speed * 3.6);
    set_val(RT_TFET,   fresh, "%.1f C",    d->temp_mos);
    set_val(RT_TMOT,   fresh, "%.1f C",    d->temp_motor);
    set_val(RT_AH,     fresh, "%.3f Ah",   d->amp_hours);
    set_val(RT_WH,     fresh, "%.1f Wh",   d->watt_hours);
    set_val(RT_FAULT,  fresh, "%.0f",      (double)d->fault_code);

    const vesc_io_data_t *io = vesc_io_data_get_latest();
    bool iofresh = vesc_io_data_is_fresh();

    set_val(RT_ADC1,   iofresh, "%.2f",   io->adc1);
    set_val(RT_ADC1V,  iofresh, "%.2f V", io->adc1_voltage);
    set_val(RT_ADC2,   iofresh, "%.2f",   io->adc2);
    set_val(RT_ADC2V,  iofresh, "%.2f V", io->adc2_voltage);
    set_val(RT_PPM,    iofresh, "%.2f",    io->ppm);
    set_val(RT_PPMMS,  iofresh, "%.2f ms", io->ppm_ms);
}

void show_realtime_viewer(void)
{
    if (s_screen) return;  /* re-entrancy guard */

    s_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_screen, 800, 480);
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
    lv_label_set_text(title, "Realtime");
    lv_obj_set_pos(title, 110, 16);
    lv_obj_set_style_text_color(title, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);

    /* scrollable value list */
    lv_obj_t *list = lv_obj_create(s_screen);
    lv_obj_set_pos(list, 8, 56);
    lv_obj_set_size(list, 784, 416);
    lv_obj_set_style_bg_color(list, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_radius(list, 6, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

    add_section(list, "Telemetry");
    s_val[RT_VIN]    = add_row(list, "Input voltage");
    s_val[RT_BATT]   = add_row(list, "Battery");
    s_val[RT_IIN]    = add_row(list, "Current in");
    s_val[RT_IMOTOR] = add_row(list, "Current motor");
    s_val[RT_POWER]  = add_row(list, "Power");
    s_val[RT_DUTY]   = add_row(list, "Duty cycle");
    s_val[RT_RPM]    = add_row(list, "ERPM");
    s_val[RT_SPEED]  = add_row(list, "Speed");
    s_val[RT_TFET]   = add_row(list, "Temp FET");
    s_val[RT_TMOT]   = add_row(list, "Temp motor");
    s_val[RT_AH]     = add_row(list, "Used Ah");
    s_val[RT_WH]     = add_row(list, "Used Wh");
    s_val[RT_FAULT]  = add_row(list, "Fault code");

    add_section(list, "ADC inputs");
    s_val[RT_ADC1]  = add_row(list, "ADC1 level");
    s_val[RT_ADC1V] = add_row(list, "ADC1 voltage");
    s_val[RT_ADC2]  = add_row(list, "ADC2 level");
    s_val[RT_ADC2V] = add_row(list, "ADC2 voltage");

    add_section(list, "PPM / servo");
    s_val[RT_PPM]   = add_row(list, "PPM level");
    s_val[RT_PPMMS] = add_row(list, "PPM pulse");

    lv_obj_add_event_cb(s_screen, screen_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    s_alive = true;

    /* Turn on the ADC/PPM poller and start refreshing labels. */
    vesc_io_data_set_active(true);
    s_timer = lv_timer_create(update_cb, 200, NULL);
    update_cb(s_timer);

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

void show_realtime_viewer(void)
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
    lv_label_set_text(lbl, "Realtime viewer is available on the device build.");
    lv_obj_center(lbl);

    lv_obj_add_event_cb(s_sim_screen, sim_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_scr_load_anim(s_sim_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

#endif /* LV_REALDEVICE */
