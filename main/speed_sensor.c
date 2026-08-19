#include "speed_sensor.h"

#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"

#include "ble_speed_client.h"
#include "dev_settings.h"        /* settings_get_wheel_diameter_mm */
#include "trip_log.h"            /* speed provider hook */
#include "vesc_trip_persist.h"   /* trip-reset hook */

static const char *TAG = "spd";

#define SPD_NVS_NS        "spdsns"
#define SPD_NVS_SETTINGS  "settings"
#define SPD_NVS_SENSOR    "sensor"     /* 7 bytes: [addr_type][addr 6] */
#define SPD_NVS_ODO       "odo_m"      /* u32 meters */
#define SPD_NVS_TRIP      "trip_m"     /* u32 meters */

#define SPD_TICK_MS       200
#define SPD_STALE_MS      5000   /* no notification at all past this */
#define SPD_ZERO_MS       3000   /* revs not advancing → wheel stopped */

/* NVS write policy: rides end standing still, so the normal save lands while
 * parked (zero flash-stall exposure); the periodic fallback bounds the loss
 * on a hard power-cut to ~10 min of distance. */
#define SPD_STOP_KMH        0.5f
#define SPD_STOP_HOLD_US    (5LL * 1000000)
#define SPD_SAVE_MIN_M      25.0
#define SPD_SAVE_PERIOD_US  (600LL * 1000000)

#define SPD_SETTINGS_VER 1

/* Persisted settings blob. Load accepts it only when BOTH the length and the
 * version match — pas.c's length-only check silently resets everything when
 * the struct grows; bump the version on any layout change instead. */
typedef struct {
    uint8_t ver;          /* SPD_SETTINGS_VER */
    uint8_t source_ble;   /* 0 = VESC (default), 1 = BLE wheel sensor */
    uint8_t reserved[6];
} speed_settings_t;

/* ---- state (under s_mux) ---- */

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static speed_settings_t s_cfg;
static double   s_odo_m;          /* double: float32 loses meters past ~16 777 km */
static double   s_trip_m;
static uint64_t s_consumed_revs;  /* ble_speed total_revs already integrated */
static float    s_kmh;            /* 0 when stale/stopped */
static bool     s_fresh;
static bool     s_started;

/* Save bookkeeping (task-local timing, meters under s_mux). */
static double  s_saved_odo_m;     /* s_odo_m at the last enqueued save */
static int64_t s_last_save_us;
static int64_t s_stop_since_us;   /* 0 = moving */

/* ---- deferred NVS writer (batt_sav pattern: producers enqueue only, a
 * low-priority task owns every nvs_commit — commits cost ~100 ms on this
 * board and several producers run on the LVGL thread) ---- */

typedef enum {
    SAVE_DIST,          /* odo_m + trip_m */
    SAVE_SETTINGS,
    SAVE_SENSOR,
    SAVE_SENSOR_ERASE,
} save_kind_t;

typedef struct {
    save_kind_t      kind;
    uint32_t         odo_m;
    uint32_t         trip_m;
    speed_settings_t cfg;
    uint8_t          sensor[7];
} save_msg_t;

static QueueHandle_t s_save_q;

static void save_task(void *arg)
{
    (void)arg;
    save_msg_t m;
    for (;;) {
        if (xQueueReceive(s_save_q, &m, portMAX_DELAY) != pdTRUE) continue;
        nvs_handle_t h;
        if (nvs_open(SPD_NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
            ESP_LOGW(TAG, "NVS open failed — save skipped");
            continue;
        }
        switch (m.kind) {
        case SAVE_DIST:
            nvs_set_u32(h, SPD_NVS_ODO, m.odo_m);
            nvs_set_u32(h, SPD_NVS_TRIP, m.trip_m);
            break;
        case SAVE_SETTINGS:
            nvs_set_blob(h, SPD_NVS_SETTINGS, &m.cfg, sizeof(m.cfg));
            break;
        case SAVE_SENSOR:
            nvs_set_blob(h, SPD_NVS_SENSOR, m.sensor, sizeof(m.sensor));
            break;
        case SAVE_SENSOR_ERASE:
            nvs_erase_key(h, SPD_NVS_SENSOR);
            break;
        }
        nvs_commit(h);
        nvs_close(h);
        ESP_LOGD(TAG, "saved kind=%d", (int)m.kind);
    }
}

static void enqueue(const save_msg_t *m)
{
    if (!s_save_q || xQueueSend(s_save_q, m, 0) != pdTRUE) {
        ESP_LOGW(TAG, "save queue unavailable — save skipped");
    }
}

static void enqueue_dist_locked_snapshot(void)
{
    save_msg_t m = { .kind = SAVE_DIST };
    portENTER_CRITICAL(&s_mux);
    m.odo_m  = (uint32_t)s_odo_m;
    m.trip_m = (uint32_t)s_trip_m;
    s_saved_odo_m = s_odo_m;
    portEXIT_CRITICAL(&s_mux);
    s_last_save_us = esp_timer_get_time();
    enqueue(&m);
}

/* ---- NVS load (boot only, before the tasks start) ---- */

static void nvs_load(void)
{
    s_cfg.ver = SPD_SETTINGS_VER;
    s_cfg.source_ble = 0;
    memset(s_cfg.reserved, 0, sizeof(s_cfg.reserved));

    nvs_handle_t h;
    if (nvs_open(SPD_NVS_NS, NVS_READONLY, &h) != ESP_OK) return;

    speed_settings_t tmp;
    size_t len = sizeof(tmp);
    if (nvs_get_blob(h, SPD_NVS_SETTINGS, &tmp, &len) == ESP_OK &&
        len == sizeof(tmp) && tmp.ver == SPD_SETTINGS_VER) {
        s_cfg = tmp;
    }

    uint32_t v;
    if (nvs_get_u32(h, SPD_NVS_ODO, &v) == ESP_OK)  s_odo_m  = (double)v;
    if (nvs_get_u32(h, SPD_NVS_TRIP, &v) == ESP_OK) s_trip_m = (double)v;
    s_saved_odo_m = s_odo_m;

    uint8_t blob[7];
    size_t blen = sizeof(blob);
    if (nvs_get_blob(h, SPD_NVS_SENSOR, blob, &blen) == ESP_OK &&
        blen == sizeof(blob)) {
        ble_speed_bind(&blob[1], blob[0]);
        ESP_LOGI(TAG, "restored bound sensor from NVS");
    }
    nvs_close(h);
}

/* ---- integrator task ---- */

static void spd_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SPD_TICK_MS));

        ble_speed_state_t st;
        ble_speed_get(&st);

        /* Circumference from the shared wheel-diameter setting (RAM-cached
         * getter, cheap at 5 Hz; live changes apply on the next tick). */
        double circ_m = M_PI * (double)settings_get_wheel_diameter_mm() / 1000.0;

        bool fresh = st.connected && st.age_ms < SPD_STALE_MS;
        float kmh = 0.0f;
        if (fresh && st.rev_age_ms < SPD_ZERO_MS && st.last_delta_1024 > 0) {
            kmh = (float)((double)st.last_delta_revs * circ_m * 1024.0 /
                          (double)st.last_delta_1024 * 3.6);
        }

        int64_t now = esp_timer_get_time();
        double since_save;
        portENTER_CRITICAL(&s_mux);
        uint64_t delta = st.total_revs - s_consumed_revs;
        s_consumed_revs = st.total_revs;
        if (delta) {
            double m = (double)delta * circ_m;
            s_odo_m  += m;
            s_trip_m += m;
        }
        s_kmh   = kmh;
        s_fresh = fresh;
        since_save = s_odo_m - s_saved_odo_m;
        portEXIT_CRITICAL(&s_mux);

        /* Save policy: on a ride stop (speed low for a while) once at least
         * SPD_SAVE_MIN_M accumulated since the last save — since_save drops
         * to 0 after the save, so a long stop writes exactly once — plus a
         * periodic fallback while moving. */
        if (kmh < SPD_STOP_KMH) {
            if (s_stop_since_us == 0) s_stop_since_us = now;
        } else {
            s_stop_since_us = 0;
        }
        if (since_save >= SPD_SAVE_MIN_M &&
            ((s_stop_since_us != 0 &&
              now - s_stop_since_us >= SPD_STOP_HOLD_US) ||
             now - s_last_save_us >= SPD_SAVE_PERIOD_US)) {
            enqueue_dist_locked_snapshot();
        }
    }
}

/* ---- trip_log speed provider ---- */

static trip_speed_prov_res_t speed_provider(float *kmh)
{
    portENTER_CRITICAL(&s_mux);
    bool  active = s_cfg.source_ble != 0;
    bool  fresh  = s_fresh;
    float v      = s_kmh;
    portEXIT_CRITICAL(&s_mux);
    if (!active) return TRIP_SPEED_PROV_INACTIVE;
    if (!fresh)  return TRIP_SPEED_PROV_STALE;
    *kmh = v;
    return TRIP_SPEED_PROV_FRESH;
}

/* ---- public API ---- */

void speed_sensor_init(void)
{
    if (s_started) return;
    nvs_load();
    s_last_save_us = esp_timer_get_time();

    s_save_q = xQueueCreate(4, sizeof(save_msg_t));
    xTaskCreate(save_task, "spd_sav", 4096, NULL, 2, NULL);
    xTaskCreate(spd_task, "spd", 3072, NULL, 5, NULL);
    s_started = true;

    trip_log_set_speed_provider(speed_provider);
    trip_persist_add_reset_cb(speed_sensor_trip_reset);

    ESP_LOGI(TAG, "speed sensor started (source=%s, odo=%.1f km)",
             s_cfg.source_ble ? "BLE" : "VESC", s_odo_m / 1000.0);
}

bool speed_source_is_ble(void)
{
    portENTER_CRITICAL(&s_mux);
    bool v = s_cfg.source_ble != 0;
    portEXIT_CRITICAL(&s_mux);
    return v;
}

void speed_sensor_set_source(bool ble)
{
    save_msg_t m = { .kind = SAVE_SETTINGS };
    portENTER_CRITICAL(&s_mux);
    s_cfg.source_ble = ble ? 1 : 0;
    m.cfg = s_cfg;
    portEXIT_CRITICAL(&s_mux);
    enqueue(&m);
    ESP_LOGI(TAG, "speed source: %s", ble ? "BLE sensor" : "VESC");
}

float speed_sensor_get_kmh(void)
{
    portENTER_CRITICAL(&s_mux);
    float v = s_kmh;
    portEXIT_CRITICAL(&s_mux);
    return v;
}

bool speed_sensor_is_fresh(void)
{
    portENTER_CRITICAL(&s_mux);
    bool v = s_fresh;
    portEXIT_CRITICAL(&s_mux);
    return v;
}

float speed_sensor_get_trip_km(void)
{
    portENTER_CRITICAL(&s_mux);
    double m = s_trip_m;
    portEXIT_CRITICAL(&s_mux);
    return (float)(m / 1000.0);
}

float speed_sensor_get_odometer_km(void)
{
    portENTER_CRITICAL(&s_mux);
    double m = s_odo_m;
    portEXIT_CRITICAL(&s_mux);
    return (float)(m / 1000.0);
}

void speed_sensor_trip_reset(void)
{
    portENTER_CRITICAL(&s_mux);
    s_trip_m = 0.0;
    portEXIT_CRITICAL(&s_mux);
    enqueue_dist_locked_snapshot();   /* persist the zero immediately */
    ESP_LOGI(TAG, "trip reset");
}

void speed_sensor_select(const uint8_t addr[6], uint8_t addr_type)
{
    save_msg_t m = { .kind = SAVE_SENSOR };
    m.sensor[0] = addr_type;
    memcpy(&m.sensor[1], addr, 6);
    enqueue(&m);
    ble_speed_bind(addr, addr_type);
}

void speed_sensor_forget(void)
{
    save_msg_t m = { .kind = SAVE_SENSOR_ERASE };
    enqueue(&m);
    ble_speed_forget();
}

void speed_sensor_get_telem(speed_telem_t *out)
{
    if (!out) return;
    ble_speed_state_t st;
    ble_speed_get(&st);
    portENTER_CRITICAL(&s_mux);
    out->source_ble = s_cfg.source_ble != 0;
    out->fresh      = s_fresh;
    out->kmh        = s_kmh;
    out->trip_km    = (float)(s_trip_m / 1000.0);
    out->odo_km     = (float)(s_odo_m / 1000.0);
    portEXIT_CRITICAL(&s_mux);
    out->sensor_bound     = st.bound;
    out->sensor_connected = st.connected;
    out->scanning         = st.scanning;
    out->battery          = st.battery;
}
