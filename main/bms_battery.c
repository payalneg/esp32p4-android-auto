#include "bms_battery.h"

#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "ble_bms_client.h"
#include "vesc_battery_calc.h"

static const char *TAG = "bms_batt";

#define NVS_NS       "jkbms"
#define NVS_SENSOR   "sensor"     /* 7 bytes: [addr_type][addr 6] */

typedef struct {
    bool    erase;
    uint8_t blob[7];
} nvs_msg_t;

static QueueHandle_t s_q;

/* ---- battery-source provider ---- */

static bool bms_provider(float *soc, float *remaining_ah, float *full_ah)
{
    ble_bms_state_t st;
    ble_bms_get(&st);
    if (!st.connected || !st.fresh || !isfinite(st.soc) ||
        !isfinite(st.full_ah) || st.full_ah <= 0.0f) {
        return false;
    }
    if (soc)          *soc = st.soc;
    if (remaining_ah) *remaining_ah = st.remaining_ah;
    if (full_ah)      *full_ah = st.full_ah;
    return true;
}

/* ---- deferred NVS writer (flash commits never run on the LVGL task) ---- */

static void nvs_task(void *arg)
{
    (void)arg;
    nvs_msg_t m;
    for (;;) {
        if (xQueueReceive(s_q, &m, portMAX_DELAY) != pdTRUE) continue;
        nvs_handle_t h;
        if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
            ESP_LOGW(TAG, "NVS open failed — pairing not saved");
            continue;
        }
        if (m.erase) {
            nvs_erase_key(h, NVS_SENSOR);
        } else {
            nvs_set_blob(h, NVS_SENSOR, m.blob, sizeof(m.blob));
        }
        nvs_commit(h);
        nvs_close(h);
        ESP_LOGI(TAG, "%s saved", m.erase ? "forget" : "pairing");
    }
}

static void queue_save(bool erase, const uint8_t addr[6], uint8_t addr_type)
{
    if (!s_q) return;
    nvs_msg_t m = { .erase = erase };
    if (!erase) {
        m.blob[0] = addr_type;
        memcpy(&m.blob[1], addr, 6);
    }
    if (xQueueSend(s_q, &m, 0) != pdTRUE) {
        ESP_LOGW(TAG, "NVS queue full — pairing change dropped");
    }
}

static bool load_saved(uint8_t addr[6], uint8_t *addr_type)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    uint8_t blob[7];
    size_t len = sizeof(blob);
    bool ok = (nvs_get_blob(h, NVS_SENSOR, blob, &len) == ESP_OK &&
               len == sizeof(blob));
    nvs_close(h);
    if (!ok) return false;
    *addr_type = blob[0];
    memcpy(addr, &blob[1], 6);
    return true;
}

/* ---- public API ---- */

void bms_battery_init(void)
{
    if (!s_q) {
        s_q = xQueueCreate(4, sizeof(nvs_msg_t));
        if (!s_q || xTaskCreate(nvs_task, "bms_nvs", 3072, NULL, 2, NULL) != pdPASS) {
            ESP_LOGE(TAG, "NVS writer init failed — pairing won't persist");
            if (s_q) { vQueueDelete(s_q); s_q = NULL; }
        }
    }

    battery_calc_set_bms_cb(bms_provider);

    uint8_t addr[6];
    uint8_t type = 0;
    if (load_saved(addr, &type)) {
        ESP_LOGI(TAG, "re-binding saved pack %02X:%02X:%02X:%02X:%02X:%02X",
                 addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        ble_bms_bind(addr, type);
    } else {
        ESP_LOGI(TAG, "no pack paired (Settings -> Battery -> Search)");
    }
}

void bms_battery_scan_start(void)
{
    ble_bms_scan_start();
}

void bms_battery_scan_stop(void)
{
    ble_bms_scan_stop();
}

void bms_battery_set_scan_cb(bms_scan_cb_t cb)
{
    ble_bms_set_scan_cb(cb);
}

void bms_battery_select(const uint8_t addr[6], uint8_t addr_type)
{
    queue_save(false, addr, addr_type);
    ble_bms_bind(addr, addr_type);
}

void bms_battery_forget(void)
{
    queue_save(true, NULL, 0);
    ble_bms_forget();
}

bool bms_battery_is_bound(void)
{
    return ble_bms_is_bound();
}

void bms_battery_get(bms_telem_t *out)
{
    if (!out) return;
    ble_bms_state_t st;
    ble_bms_get(&st);

    out->bound           = st.bound;
    out->connected       = st.connected;
    out->scanning        = st.scanning;
    out->fresh           = st.fresh;
    out->has_data        = st.has_data;
    out->soc             = st.soc;
    out->remaining_ah    = st.remaining_ah;
    out->full_ah         = st.full_ah;
    out->voltage         = st.voltage;
    out->current         = st.current;
    out->power           = st.power;
    out->temp1           = st.temp1;
    out->temp2           = st.temp2;
    out->temp_mos        = st.temp_mos;
    out->cell_min        = st.cell_min;
    out->cell_max        = st.cell_max;
    out->cell_delta      = st.cell_delta;
    out->cell_min_index  = st.cell_min_index;
    out->cell_max_index  = st.cell_max_index;
    out->cell_count      = st.cell_count;
    memcpy(out->cells, st.cells, sizeof(out->cells));
    out->cycles          = st.cycles;
    out->soh             = st.soh;
    out->charging_mos    = st.charging_mos;
    out->discharging_mos = st.discharging_mos;
    out->balancing       = st.balancing;
    out->heating         = st.heating;
    memcpy(out->model, st.model, sizeof(out->model));
    out->age_ms          = st.age_ms;
}
