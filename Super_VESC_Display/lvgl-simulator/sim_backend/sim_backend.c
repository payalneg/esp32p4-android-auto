/*
 * Fake firmware backend for the desktop simulator (SIM_SHIM=1).
 *
 * The device-only screens — VESC Config, PAS, Speed sensor, the LISP panel and
 * editor, the realtime viewer — are the ones with the most layout to get
 * wrong, and on the host they used to be replaced by a "this screen is on the
 * device build" placeholder. Here their real code compiles against the
 * firmware's own headers, and this file stands in for the hardware: plausible
 * telemetry, a couple of discoverable BLE sensors, a small LISP panel, and a
 * CAN transport that answers every config request out of the table's own
 * firmware defaults.
 *
 * The point is layout, not fidelity. Anything asynchronous completes from a
 * one-shot LVGL timer rather than instantly, because the screens show a
 * spinner while a request is in flight and an instant callback would hide
 * whatever that state looks like.
 */

#include "lvgl.h"

#include "app_fs.h"
#include "ble_cadence_client.h"
#include "ble_speed_client.h"
#include "pas.h"
#include "speed_sensor.h"
#include "vesc_can/vesc_io_data.h"
#include "vesc_can/vesc_lisp_code.h"
#include "vesc_can/vesc_lisp_panel.h"
#include "vesc_can/vesc_rt_data.h"
#include "vesc_config/vesc_config.h"
#include "vesc_config/vesc_config_serdes.h"
#include "vesc_config/vesc_config_types.h"
#include "vesc_ui.h"
#include "custom.h"
#include "dashboard_theme.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------- helpers */

/* Run fn(arg) after `ms` of LVGL time, once. The screens are written against
 * an asynchronous backend; calling their completion callback inline would
 * re-enter them from inside their own request call. */
typedef void (*sim_later_fn)(void *);

typedef struct {
    sim_later_fn fn;
    void        *arg;
} sim_later_t;

static void sim_later_cb(lv_timer_t *t)
{
    sim_later_t *l = (sim_later_t *)t->user_data;
    sim_later_fn fn = l->fn;
    void *arg = l->arg;
    free(l);
    lv_timer_del(t);
    if (fn) fn(arg);
}

static void sim_later(sim_later_fn fn, void *arg, uint32_t ms)
{
    sim_later_t *l = malloc(sizeof *l);
    if (!l) return;
    l->fn = fn; l->arg = arg;
    lv_timer_t *t = lv_timer_create(sim_later_cb, ms ? ms : 1, l);
    lv_timer_set_repeat_count(t, 1);
}

/* A slow wobble, so live readouts are not all zeroes in a screenshot. */
static float wobble(float base, float amp, float period_s)
{
    float t = (float)lv_tick_get() / 1000.0f;
    return base + amp * sinf(t * 6.2831853f / period_s);
}

/* ------------------------------------------------------------------ app_fs */

void        app_fs_ensure(void) { }
int         app_fs_state(void) { return 1; }        /* APP_FS_MOUNTED */
bool        app_fs_ready(void) { return true; }
const char *app_fs_base(void) { return "/tmp/vescfs"; }

/* ------------------------------------------------------- dev_settings slice */

bool settings_get_vesc_emulator(void) { return false; }

/* ------------------------------------------------------------------ vesc_ui */

lv_obj_t *vesc_ui_get_screen(void)
{
    lv_obj_t *s = dashboard_theme_active_screen();
    return s ? s : lv_scr_act();
}

void vesc_ui_init(void) { }

/* -------------------------------------------------------------- PAS + speed */

static pas_settings_t s_pas = {
    .enabled = true, .reverse = false, .level = 2, .level_count = 5,
    .max_current_a = 42.0f, .mode = 1 /* PROPORTIONAL */,
    .start_delay_ms = 250, .start_current_pct = 40, .ramp_up_aps = 25.0f,
    .stop_delay_ms = 400, .min_cadence_rpm = 20, .full_cadence_rpm = 75,
};
static bool s_pas_bound = true, s_pas_scanning;
static bool s_spd_ble = true, s_spd_bound = true, s_spd_scanning;
static float s_trip_km = 14.2f, s_odo_km = 1832.6f;

void pas_init(void) { }
void pas_get_settings(pas_settings_t *out) { if (out) *out = s_pas; }
void pas_set_settings(const pas_settings_t *s) { if (s) s_pas = *s; }
void pas_set_enabled(bool en) { s_pas.enabled = en; }
void pas_set_level(uint8_t lvl) { s_pas.level = lvl; }
void pas_sensor_select(const uint8_t addr[6], uint8_t t) { (void)addr; (void)t; s_pas_bound = true; }
void pas_sensor_forget(void) { s_pas_bound = false; }

void pas_get_telem(pas_telem_t *out)
{
    if (!out) return;
    out->centi_rpm       = (int16_t)(wobble(6200.0f, 1800.0f, 9.0f));
    out->sensor_bound    = s_pas_bound;
    out->sensor_connected = s_pas_bound;
    out->scanning        = s_pas_scanning;
    out->battery         = 78;
    out->assist_a        = wobble(18.0f, 8.0f, 7.0f);
}

void speed_sensor_init(void) { }
bool speed_source_is_ble(void) { return s_spd_ble; }
void speed_sensor_set_source(bool ble) { s_spd_ble = ble; }
float speed_sensor_get_kmh(void) { return wobble(24.0f, 9.0f, 11.0f); }
bool speed_sensor_is_fresh(void) { return s_spd_bound; }
float speed_sensor_get_trip_km(void) { return s_trip_km; }
float speed_sensor_get_odometer_km(void) { return s_odo_km; }
void speed_sensor_trip_reset(void) { s_trip_km = 0.0f; }
void speed_sensor_select(const uint8_t a[6], uint8_t t) { (void)a; (void)t; s_spd_bound = true; }
void speed_sensor_forget(void) { s_spd_bound = false; }

void speed_sensor_get_telem(speed_telem_t *out)
{
    if (!out) return;
    out->source_ble       = s_spd_ble;
    out->sensor_bound     = s_spd_bound;
    out->sensor_connected = s_spd_bound;
    out->scanning         = s_spd_scanning;
    out->fresh            = s_spd_bound;
    out->kmh              = speed_sensor_get_kmh();
    out->trip_km          = s_trip_km;
    out->odo_km           = s_odo_km;
    out->battery          = 64;
}

/* ------------------------------------------------- BLE sensor scans (fake) */

/* Two candidates with names of very different length: the short one checks the
 * layout, the long one checks what the list does when a name does not fit. */
typedef struct { const char *name; int8_t rssi; uint8_t addr0; } fake_dev_t;
static const fake_dev_t s_fake_devs[] = {
    { "BK6LS-Cadence",                   -54, 0xA1 },
    { "Magene S3+ Speed/Cadence Sensor", -77, 0xB2 },
};

static ble_cadence_scan_cb_t s_cad_cb;
static ble_speed_scan_cb_t   s_spd_cb;

static void cad_scan_emit(void *unused)
{
    (void)unused;
    for (unsigned i = 0; i < sizeof s_fake_devs / sizeof s_fake_devs[0]; i++) {
        uint8_t addr[6] = { s_fake_devs[i].addr0, 0x22, 0x33, 0x44, 0x55, 0x66 };
        if (s_cad_cb) s_cad_cb(addr, 0, s_fake_devs[i].name, s_fake_devs[i].rssi);
    }
    s_pas_scanning = false;
}

static void spd_scan_emit(void *unused)
{
    (void)unused;
    for (unsigned i = 0; i < sizeof s_fake_devs / sizeof s_fake_devs[0]; i++) {
        uint8_t addr[6] = { s_fake_devs[i].addr0, 0x22, 0x33, 0x44, 0x55, 0x77 };
        if (s_spd_cb) s_spd_cb(addr, 0, s_fake_devs[i].name, s_fake_devs[i].rssi);
    }
    s_spd_scanning = false;
}

void ble_cadence_client_init(void) { }
void ble_cadence_on_ble_sync(uint8_t t) { (void)t; }
void ble_cadence_scan_start(void) { s_pas_scanning = true; sim_later(cad_scan_emit, NULL, 700); }
void ble_cadence_scan_stop(void) { s_pas_scanning = false; }
void ble_cadence_set_scan_cb(ble_cadence_scan_cb_t cb) { s_cad_cb = cb; }
void ble_cadence_bind(const uint8_t a[6], uint8_t t) { (void)a; (void)t; s_pas_bound = true; }
void ble_cadence_forget(void) { s_pas_bound = false; }
bool ble_cadence_is_bound(void) { return s_pas_bound; }

void ble_cadence_get(ble_cadence_state_t *out)
{
    if (!out) return;
    out->bound     = s_pas_bound;
    out->connected = s_pas_bound;
    out->scanning  = s_pas_scanning;
    out->centi_rpm = (int16_t)wobble(6200.0f, 1800.0f, 9.0f);
    out->age_ms    = 120;
    out->battery   = 78;
}

void ble_speed_client_init(void) { }
void ble_speed_on_ble_sync(uint8_t t) { (void)t; }
void ble_speed_scan_start(void) { s_spd_scanning = true; sim_later(spd_scan_emit, NULL, 700); }
void ble_speed_scan_stop(void) { s_spd_scanning = false; }
void ble_speed_set_scan_cb(ble_speed_scan_cb_t cb) { s_spd_cb = cb; }
void ble_speed_bind(const uint8_t a[6], uint8_t t) { (void)a; (void)t; s_spd_bound = true; }
void ble_speed_forget(void) { s_spd_bound = false; }
bool ble_speed_is_bound(void) { return s_spd_bound; }

void ble_speed_get(ble_speed_state_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof *out);
    out->bound     = s_spd_bound;
    out->connected = s_spd_bound;
    out->scanning  = s_spd_scanning;
}

/* ------------------------------------------------------ VESC live telemetry */

static vesc_setup_values_t s_rt;

const vesc_setup_values_t *vesc_rt_data_get_latest(void)
{
    s_rt.temp_mos       = wobble(46.0f, 6.0f, 13.0f);
    s_rt.temp_motor     = wobble(58.0f, 8.0f, 17.0f);
    s_rt.current_motor  = wobble(34.0f, 22.0f, 6.0f);
    s_rt.current_in     = wobble(18.0f, 10.0f, 6.0f);
    s_rt.duty_now       = wobble(0.42f, 0.2f, 8.0f);
    s_rt.rpm            = wobble(3800.0f, 900.0f, 10.0f);
    s_rt.speed          = wobble(6.8f, 2.4f, 11.0f);          /* m/s */
    s_rt.v_in           = wobble(49.6f, 1.4f, 21.0f);
    s_rt.battery_level  = 0.63f;
    s_rt.amp_hours      = 8.64f;
    s_rt.watt_hours     = 412.0f;
    s_rt.tachometer_abs = s_trip_km * 1000.0f;
    s_rt.fault_code     = 0;
    s_rt.vesc_id        = 10;
    s_rt.num_vescs      = 1;
    s_rt.battery_wh     = 1080.0f;
    s_rt.odometer       = (uint32_t)(s_odo_km * 1000.0f);
    s_rt.uptime_ms      = lv_tick_get();
    s_rt.rx_time        = lv_tick_get();
    return &s_rt;
}

bool     vesc_rt_data_is_fresh(void) { return true; }
void     vesc_rt_data_init(uint8_t id, uint32_t ms) { (void)id; (void)ms; }
void     vesc_rt_data_start(void) { }
void     vesc_rt_data_stop(void) { }
void     vesc_rt_data_request(void) { }
float    vesc_rt_data_get_speed_kmh(void) { return speed_sensor_get_kmh(); }
float    vesc_rt_data_get_trip_km(void) { return s_trip_km; }
float    vesc_rt_data_get_odometer_km(void) { return s_odo_km; }
float    vesc_rt_data_get_efficiency_whkm(void) { return 28.4f; }
float    vesc_rt_data_get_amp_hours(void) { return 8.64f; }
uint32_t vesc_rt_data_get_uptime_ms(void) { return lv_tick_get(); }

static vesc_io_data_t s_io;
static bool s_io_active;

void vesc_io_data_init(uint8_t id, uint32_t ms) { (void)id; (void)ms; }
void vesc_io_data_set_active(bool on) { s_io_active = on; }
bool vesc_io_data_is_active(void) { return s_io_active; }
bool vesc_io_data_is_fresh(void) { return s_io_active; }

const vesc_io_data_t *vesc_io_data_get_latest(void)
{
    s_io.adc1         = wobble(0.45f, 0.45f, 5.0f);
    s_io.adc1_voltage = 0.8f + s_io.adc1 * 2.4f;
    s_io.adc2         = wobble(0.20f, 0.20f, 9.0f);
    s_io.adc2_voltage = 0.8f + s_io.adc2 * 2.4f;
    s_io.ppm          = wobble(0.0f, 0.8f, 7.0f);
    s_io.ppm_ms       = 1.5f + s_io.ppm * 0.5f;
    s_io.adc_rx_time  = lv_tick_get();
    s_io.ppm_rx_time  = lv_tick_get();
    return &s_io;
}

/* ------------------------------------------------------------- LISP panel */

/* One of each control type, with the longest label the LISP lint allows, so
 * the panel's own geometry gets exercised rather than a best case. */
static vlp_model_t s_panel = {
    .version = VLP_PROTO_VER,
    .count   = 6,
    .ctrl = {
        { .id = 1, .type = VLP_CTRL_TOGGLE, .label = "Cruise control",      .value = 1 },
        { .id = 2, .type = VLP_CTRL_TOGGLE, .label = "Lights",              .value = 0 },
        { .id = 3, .type = VLP_CTRL_BUTTON, .label = "Reset trip counters" },
        { .id = 4, .type = VLP_CTRL_NUMBER, .label = "Assist level",
          .vmin = 0, .vmax = 5, .vstep = 1, .value = 3, .suffix = "" },
        { .id = 5, .type = VLP_CTRL_NUMBER, .label = "Current limit",
          .vmin = 10, .vmax = 120, .vstep = 5, .value = 85, .suffix = " A" },
        { .id = 6, .type = VLP_CTRL_LABEL,  .label = "Motor temperature",
          .value = 58.4f, .suffix = " \xC2\xB0""C" },
    },
    .ui_epoch = 1, .state_epoch = 1,
};

bool vesc_lisp_panel_get_model(vlp_model_t *out)
{
    if (!out) return false;
    s_panel.ctrl[5].value = wobble(58.0f, 8.0f, 17.0f);
    s_panel.state_epoch++;
    *out = s_panel;
    return true;
}

void vesc_lisp_panel_send_action(uint8_t ctrl_id, float value)
{
    for (int i = 0; i < s_panel.count; i++) {
        if (s_panel.ctrl[i].id != ctrl_id) continue;
        if (s_panel.ctrl[i].type != VLP_CTRL_BUTTON) s_panel.ctrl[i].value = value;
        s_panel.state_epoch++;
        break;
    }
}

void vesc_lisp_panel_init(uint8_t id) { (void)id; }
void vesc_lisp_panel_set_target(uint8_t id) { (void)id; }
void vesc_lisp_panel_set_enabled(bool en) { (void)en; }
void vesc_lisp_panel_polls_pause(bool p) { (void)p; }
void vesc_lisp_panel_request_ui(void) { }
void vesc_lisp_panel_request_state(void) { }
void vesc_lisp_panel_request_dash(void) { }
void vesc_lisp_panel_set_pas(float a) { (void)a; }
bool vesc_lisp_panel_get_dash(vlp_dash_t *out) { if (out) memset(out, 0, sizeof *out); return false; }

/* --------------------------------------------------------- LISP code store */

static const char *k_fake_lisp =
    "; Demo script served by the simulator's fake VESC.\n"
    "; Long enough to need scrolling, and with the kind of line lengths the\n"
    "; real main.lisp has, which is what the editor's layout has to survive.\n"
    "(define version 6)\n"
    "\n"
    "@const-start\n"
    "(defun clamp (x lo hi) (if (< x lo) lo (if (> x hi) hi x)))\n"
    "\n"
    "(defun throttle-current (adc)\n"
    "    (let ((scaled (* adc (conf-get 'l-current-max))))\n"
    "        (clamp scaled 0 (conf-get 'l-current-max))))\n"
    "\n"
    "(defun motor-control-loop ()\n"
    "    (loopwhile t\n"
    "        (progn\n"
    "            (set-current (max (throttle-current (get-adc 0)) pas-amps))\n"
    "            (sleep 0.05))))\n"
    "@const-end\n"
    "\n"
    "(spawn motor-control-loop)\n";

typedef struct {
    vlc_progress_cb_t     progress;
    vlc_read_done_cb_t    read_done;
    vlc_upload_done_cb_t  up_done;
    void                 *user;
    uint32_t              total;
} lisp_job_t;

static bool s_lisp_busy;

static void lisp_read_finish(void *p)
{
    lisp_job_t *j = p;
    s_lisp_busy = false;
    if (j->progress) j->progress(j->user, j->total, j->total);
    if (j->read_done) j->read_done(j->user, VLC_OK, k_fake_lisp, j->total);
    free(j);
}

static void lisp_upload_finish(void *p)
{
    lisp_job_t *j = p;
    s_lisp_busy = false;
    if (j->progress) j->progress(j->user, j->total, j->total);
    if (j->up_done) j->up_done(j->user, VLC_OK);
    free(j);
}

void vesc_lisp_code_init(uint8_t id) { (void)id; }
void vesc_lisp_code_set_target(uint8_t id) { (void)id; }
bool vesc_lisp_code_busy(void) { return s_lisp_busy; }
void vesc_lisp_code_set_running(bool run) { (void)run; }
bool vesc_lisp_code_repl(const char *e, uint32_t l) { (void)e; (void)l; return true; }

bool vesc_lisp_code_read(vlc_progress_cb_t progress, vlc_read_done_cb_t done, void *user)
{
    if (s_lisp_busy) return false;
    lisp_job_t *j = calloc(1, sizeof *j);
    if (!j) return false;
    s_lisp_busy = true;
    j->progress = progress; j->read_done = done; j->user = user;
    j->total = (uint32_t)strlen(k_fake_lisp);
    if (progress) progress(user, 0, j->total);
    sim_later(lisp_read_finish, j, 400);
    return true;
}

bool vesc_lisp_code_upload(const char *code, uint32_t len, bool run_after,
                           vlc_progress_cb_t progress, vlc_upload_done_cb_t done,
                           void *user)
{
    (void)code; (void)run_after;
    if (s_lisp_busy) return false;
    lisp_job_t *j = calloc(1, sizeof *j);
    if (!j) return false;
    s_lisp_busy = true;
    j->progress = progress; j->up_done = done; j->user = user;
    j->total = len ? len : 1;
    if (progress) progress(user, 0, j->total);
    sim_later(lisp_upload_finish, j, 500);
    return true;
}

/* ------------------------------------------------- fake CAN config transport
 *
 * vct_* is the whole boundary between the config runtime and the CAN bus, so
 * faking these seven functions gets the firmware's real runtime, real
 * parameter tables and real (de)serializer running on the host. A read is
 * answered out of the table's own firmware defaults — the same numbers a
 * freshly flashed VESC would report — and a write is accepted.
 */

#include "vesc_can/buffer.h"
#include "vesc_can/vesc_datatypes.h"
#include "vesc_config_priv.h"

/* The firmware version the fake VESC reports. 7.00 is what the tables call the
 * newest baked version, and what Alexey's controllers run. */
#define SIM_FW_MAJOR 7
#define SIM_FW_MINOR 0

/* One request in flight, like the real transport. */
static bool s_vct_busy;

typedef struct {
    vct_cb_t          cb;
    vct_fw_cb_t       fw_cb;
    vct_detect_cb_t   det_cb;
    vct_raw_cb_t      raw_cb;
    void             *user;
    const vc_table_t *table;
    vc_value_t       *vals_out;
    uint8_t           cmd;
    uint8_t           payload[16];
    unsigned int      payload_len;
} vct_job_t;

static void vct_finish_get(void *p)
{
    vct_job_t *j = p;
    s_vct_busy = false;
    if (j->table && j->vals_out) vesc_config_load_defaults(j->table, j->vals_out);
    if (j->cb) j->cb(VC_OK, j->user);
    free(j);
}

static void vct_finish_plain(void *p)
{
    vct_job_t *j = p;
    s_vct_busy = false;
    if (j->cb) j->cb(VC_OK, j->user);
    free(j);
}

static void vct_finish_fw(void *p)
{
    vct_job_t *j = p;
    s_vct_busy = false;
    if (j->fw_cb) j->fw_cb(SIM_FW_MAJOR, SIM_FW_MINOR, true, j->user);
    free(j);
}

static void vct_finish_detect(void *p)
{
    vct_job_t *j = p;
    s_vct_busy = false;
    if (j->det_cb) j->det_cb(1, j->user);   /* one motor detected */
    free(j);
}

static void vct_finish_raw(void *p)
{
    vct_job_t *j = p;
    s_vct_busy = false;
    if (j->raw_cb) j->raw_cb(1, j->payload, j->payload_len, j->user);
    free(j);
}

static vct_job_t *vct_job_new(void *user)
{
    if (s_vct_busy) return NULL;
    vct_job_t *j = calloc(1, sizeof *j);
    if (!j) return NULL;
    s_vct_busy = true;
    j->user = user;
    return j;
}

void vct_init(void) { }
bool vct_busy(void) { return s_vct_busy; }
void vesc_config_set_target(uint8_t controller_id) { (void)controller_id; }

bool vesc_config_transport_process_response(const uint8_t *d, unsigned int l)
{
    (void)d; (void)l;
    return false;
}

vc_result_t vct_get(const vc_table_t *t, bool defaults, vc_value_t *vals_out,
                    vct_cb_t cb, void *user)
{
    (void)defaults;
    vct_job_t *j = vct_job_new(user);
    if (!j) return VC_ERR_BUSY;
    j->cb = cb; j->table = t; j->vals_out = vals_out;
    sim_later(vct_finish_get, j, 350);      /* long enough to see the spinner */
    return VC_OK;
}

vc_result_t vct_set(const vc_table_t *t, const vc_value_t *vals,
                    vct_cb_t cb, void *user)
{
    (void)t; (void)vals;
    vct_job_t *j = vct_job_new(user);
    if (!j) return VC_ERR_BUSY;
    j->cb = cb;
    sim_later(vct_finish_plain, j, 350);
    return VC_OK;
}

vc_result_t vct_probe_fw(vct_fw_cb_t cb, void *user)
{
    vct_job_t *j = vct_job_new(user);
    if (!j) return VC_ERR_BUSY;
    j->fw_cb = cb;
    sim_later(vct_finish_fw, j, 200);
    return VC_OK;
}

vc_result_t vct_detect_foc(bool detect_can, double max_power_loss, double min_current_in,
                           double max_current_in, double openloop_rpm, double sl_erpm,
                           vct_detect_cb_t cb, void *user)
{
    (void)detect_can; (void)max_power_loss; (void)min_current_in;
    (void)max_current_in; (void)openloop_rpm; (void)sl_erpm;
    vct_job_t *j = vct_job_new(user);
    if (!j) return VC_ERR_BUSY;
    j->det_cb = cb;
    sim_later(vct_finish_detect, j, 1500);  /* a real detection spins the motor */
    return VC_OK;
}

vc_result_t vct_request_raw(uint8_t cmd, const uint8_t *body, unsigned int body_len,
                            unsigned int timeout_ms, vct_raw_cb_t cb, void *user)
{
    (void)body; (void)body_len; (void)timeout_ms;
    vct_job_t *j = vct_job_new(user);
    if (!j) return VC_ERR_BUSY;
    j->raw_cb = cb;
    j->cmd = cmd;

    /* Plausible measurements, in the wire scaling the runtime expects. */
    int32_t ind = 0;
    switch (cmd) {
    case COMM_DETECT_MOTOR_R_L:
        buffer_append_int32(j->payload, (int32_t)(0.0185 * 1e6), &ind);   /* 18.5 mOhm */
        buffer_append_int32(j->payload, (int32_t)(24.0 * 1e3), &ind);     /* 24 uH */
        buffer_append_int32(j->payload, (int32_t)(1.5 * 1e3), &ind);      /* Ld-Lq */
        break;
    case COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP:
        buffer_append_int32(j->payload, (int32_t)(0.0081 * 1e7), &ind);   /* 8.1 mWb */
        break;
    case COMM_DETECT_ENCODER:
        buffer_append_int32(j->payload, (int32_t)(142.5 * 1e6), &ind);    /* offset */
        buffer_append_int32(j->payload, (int32_t)(7.0 * 1e6), &ind);      /* ratio */
        j->payload[ind++] = 0;                                            /* inverted */
        break;
    case COMM_DETECT_HALL_FOC: {
        static const uint8_t hall[9] = { 255, 1, 3, 2, 5, 6, 4, 255, 0 };
        memcpy(j->payload, hall, sizeof hall);
        ind = (int32_t)sizeof hall;
        break;
    }
    default:
        break;
    }
    j->payload_len = (unsigned int)ind;
    sim_later(vct_finish_raw, j, 900);
    return VC_OK;
}
