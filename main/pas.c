#include "pas.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"

#include "ble_cadence_client.h"
#include "vesc_can/vesc_lisp_panel.h"

static const char *TAG = "pas";

#define PAS_NVS_NS        "pas"
#define PAS_NVS_SETTINGS  "settings"
#define PAS_NVS_SENSOR    "sensor"     /* 7 bytes: [addr_type][addr 6] */

#define PAS_TICK_MS       50
#define PAS_STALE_MS      600          /* cadence considered stale past this */

static portMUX_TYPE  s_mux = portMUX_INITIALIZER_UNLOCKED;
static pas_settings_t s_cfg;           /* guarded by s_mux */
static pas_telem_t    s_telem;         /* guarded by s_mux */
static bool           s_started;

static inline uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static inline float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static void apply_defaults(pas_settings_t *s)
{
    s->enabled           = false;
    s->reverse           = false;
    s->level             = 1;
    s->level_count       = 5;
    s->max_current_a     = 15.0f;
    s->mode              = PAS_MODE_SWITCH;
    s->start_delay_ms    = 300;
    s->start_current_pct = 30;
    s->ramp_up_aps       = 8.0f;
    s->stop_delay_ms     = 300;
    s->min_cadence_rpm   = 10;
    s->full_cadence_rpm  = 70;
}

/* ---- NVS ---- */

static void nvs_save_settings(const pas_settings_t *s)
{
    nvs_handle_t h;
    if (nvs_open(PAS_NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_blob(h, PAS_NVS_SETTINGS, s, sizeof(*s));
    nvs_commit(h);
    nvs_close(h);
}

static void nvs_save_sensor(const uint8_t addr[6], uint8_t addr_type)
{
    nvs_handle_t h;
    if (nvs_open(PAS_NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    uint8_t blob[7];
    blob[0] = addr_type;
    memcpy(&blob[1], addr, 6);
    nvs_set_blob(h, PAS_NVS_SENSOR, blob, sizeof(blob));
    nvs_commit(h);
    nvs_close(h);
}

static void nvs_erase_sensor(void)
{
    nvs_handle_t h;
    if (nvs_open(PAS_NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_erase_key(h, PAS_NVS_SENSOR);
    nvs_commit(h);
    nvs_close(h);
}

/* Load settings + bound sensor from NVS. Binds the sensor (so it reconnects on
 * boot) if one was saved. */
static void nvs_load(pas_settings_t *s)
{
    apply_defaults(s);
    nvs_handle_t h;
    if (nvs_open(PAS_NVS_NS, NVS_READONLY, &h) != ESP_OK) return;

    size_t len = sizeof(*s);
    pas_settings_t tmp;
    if (nvs_get_blob(h, PAS_NVS_SETTINGS, &tmp, &len) == ESP_OK &&
        len == sizeof(*s)) {
        *s = tmp;
    }

    uint8_t blob[7];
    size_t blen = sizeof(blob);
    if (nvs_get_blob(h, PAS_NVS_SENSOR, blob, &blen) == ESP_OK &&
        blen == sizeof(blob)) {
        ble_cadence_bind(&blob[1], blob[0]);
        ESP_LOGI(TAG, "restored bound sensor from NVS");
    }
    nvs_close(h);
}

/* ---- control loop ---- */

static void pas_task(void *arg)
{
    (void)arg;
    /* Per-loop state (task-local). */
    float    out_a       = 0.0f;
    bool     engaged     = false;
    uint32_t first_pedal_ms = 0;
    uint32_t last_pedal_ms  = 0;

    for (;;) {
        const float dt = PAS_TICK_MS / 1000.0f;
        uint32_t now = now_ms();

        pas_settings_t cfg;
        portENTER_CRITICAL(&s_mux);
        cfg = s_cfg;
        portEXIT_CRITICAL(&s_mux);

        ble_cadence_state_t cad;
        ble_cadence_get(&cad);

        float raw_rpm   = cad.centi_rpm / 100.0f;
        float effective = cfg.reverse ? -raw_rpm : raw_rpm;  /* forward → + */
        bool  stale     = !cad.connected || cad.age_ms > PAS_STALE_MS;
        bool  pedaling  = cfg.enabled && !stale &&
                          effective >= (float)cfg.min_cadence_rpm;

        if (pedaling) {
            if (first_pedal_ms == 0) first_pedal_ms = now;
            last_pedal_ms = now;
            if (!engaged && (now - first_pedal_ms) >= cfg.start_delay_ms) {
                engaged = true;
            }
        } else {
            first_pedal_ms = 0;
        }

        /* Hold assist through the stop-delay window after pedaling stops. */
        bool assisting = cfg.enabled && engaged &&
                         (pedaling || (now - last_pedal_ms) < cfg.stop_delay_ms);

        float target = 0.0f;
        if (assisting) {
            uint8_t lc = cfg.level_count ? cfg.level_count : 1;
            float level_frac = clampf((float)cfg.level / (float)lc, 0.0f, 1.0f);
            if (cfg.mode == PAS_MODE_PROPORTIONAL) {
                float span = (float)cfg.full_cadence_rpm -
                             (float)cfg.min_cadence_rpm;
                float cad_frac = span > 1.0f
                    ? clampf((effective - (float)cfg.min_cadence_rpm) / span,
                             0.0f, 1.0f)
                    : 1.0f;
                target = cad_frac * level_frac * cfg.max_current_a;
            } else {
                target = level_frac * cfg.max_current_a;
            }
        } else {
            engaged = false;
        }

        /* Initial-kick floor: on the first engaged tick start at start_current_pct
         * of the target rather than ramping from 0, for a natural pickup. */
        if (assisting && out_a < target) {
            float floor_a = target * (cfg.start_current_pct / 100.0f);
            if (out_a < floor_a) out_a = floor_a;
        }

        /* Slew-rate limit toward the target (both directions). */
        float max_step = cfg.ramp_up_aps * dt;
        if (max_step <= 0.0f) {
            out_a = target;
        } else if (out_a < target) {
            out_a += max_step;
            if (out_a > target) out_a = target;
        } else if (out_a > target) {
            out_a -= max_step;
            if (out_a < target) out_a = target;
        }
        out_a = clampf(out_a, 0.0f, cfg.max_current_a);

        if (!cfg.enabled) {
            out_a = 0.0f;
            engaged = false;
        }

        /* Forward to the LISP arbiter (0 when disabled / not assisting → coast,
         * and keeps the watchdog fed). */
        vesc_lisp_panel_set_pas(out_a);

        portENTER_CRITICAL(&s_mux);
        s_telem.centi_rpm        = cad.centi_rpm;
        s_telem.sensor_bound     = cad.bound;
        s_telem.sensor_connected = cad.connected;
        s_telem.scanning         = cad.scanning;
        s_telem.battery          = cad.battery;
        s_telem.assist_a         = out_a;
        portEXIT_CRITICAL(&s_mux);

        vTaskDelay(pdMS_TO_TICKS(PAS_TICK_MS));
    }
}

/* ---- public API ---- */

void pas_init(void)
{
    if (s_started) return;
    pas_settings_t cfg;
    nvs_load(&cfg);
    portENTER_CRITICAL(&s_mux);
    s_cfg = cfg;
    memset(&s_telem, 0, sizeof(s_telem));
    s_telem.battery = 0xFF;
    portEXIT_CRITICAL(&s_mux);

    s_started = true;
    xTaskCreate(pas_task, "pas", 3072, NULL, 5, NULL);
    ESP_LOGI(TAG, "PAS started (enabled=%d level=%u/%u max=%.1fA mode=%u)",
             cfg.enabled, cfg.level, cfg.level_count, cfg.max_current_a,
             cfg.mode);
}

void pas_get_settings(pas_settings_t *out)
{
    if (!out) return;
    portENTER_CRITICAL(&s_mux);
    *out = s_cfg;
    portEXIT_CRITICAL(&s_mux);
}

void pas_set_settings(const pas_settings_t *s)
{
    if (!s) return;
    pas_settings_t v = *s;
    if (v.level_count == 0) v.level_count = 1;
    if (v.level > v.level_count) v.level = v.level_count;
    portENTER_CRITICAL(&s_mux);
    s_cfg = v;
    portEXIT_CRITICAL(&s_mux);
    nvs_save_settings(&v);
    ESP_LOGI(TAG, "settings updated (enabled=%d level=%u/%u)", v.enabled,
             v.level, v.level_count);
}

void pas_set_enabled(bool enabled)
{
    pas_settings_t v;
    portENTER_CRITICAL(&s_mux);
    s_cfg.enabled = enabled;
    v = s_cfg;
    portEXIT_CRITICAL(&s_mux);
    nvs_save_settings(&v);
    ESP_LOGI(TAG, "PAS %s", enabled ? "ENABLED" : "disabled");
}

void pas_set_level(uint8_t level)
{
    pas_settings_t v;
    portENTER_CRITICAL(&s_mux);
    if (s_cfg.level_count && level > s_cfg.level_count) level = s_cfg.level_count;
    s_cfg.level = level;
    v = s_cfg;
    portEXIT_CRITICAL(&s_mux);
    nvs_save_settings(&v);
}

void pas_sensor_select(const uint8_t addr[6], uint8_t addr_type)
{
    nvs_save_sensor(addr, addr_type);
    ble_cadence_bind(addr, addr_type);
}

void pas_sensor_forget(void)
{
    nvs_erase_sensor();
    ble_cadence_forget();
}

void pas_get_telem(pas_telem_t *out)
{
    if (!out) return;
    portENTER_CRITICAL(&s_mux);
    *out = s_telem;
    portEXIT_CRITICAL(&s_mux);
}
