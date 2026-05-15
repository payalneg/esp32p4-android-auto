#include "dev_settings.h"

#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "vesc_battery_calc.h"

static const char *TAG = "dev_settings";
#define NS "vesc_cfg"

/* RAM cache — populated once on settings_init(). Setters update both NVS
 * and cache, getters return cache. The cache is the source of truth at
 * runtime; NVS is only re-read on cold boot. */
static struct {
    bool                 loaded;
    uint8_t              target_vesc_id;
    can_speed_t          can_speed;
    uint8_t              brightness;
    uint8_t              controller_id;
    float                battery_capacity;
    battery_calc_mode_t  battery_calc_mode;
    bool                 show_fps;
    uint16_t             wheel_diameter_mm;
    uint8_t              motor_poles;
    connection_mode_t    connection_mode;
    float                power_max_kw;
    bool                 vesc_emulator;
} s_cache;

/* Coerce a raw NVS byte to a valid enum value. CONN_ANDROID_AUTO is the
 * historical default — devices that never touched the new dropdown keep
 * doing AA. */
static connection_mode_t sanitize_connection_mode(uint8_t v) {
    switch (v) {
        case CONN_AVRCP:        return CONN_AVRCP;
        case CONN_ANDROID_AUTO: return CONN_ANDROID_AUTO;
        case CONN_CARPLAY:      return CONN_CARPLAY;
        default:                return CONN_ANDROID_AUTO;
    }
}

static settings_can_speed_cb_t     s_can_speed_cb;
static settings_brightness_cb_t    s_brightness_cb;
static settings_target_id_cb_t     s_target_id_cb;
static settings_controller_id_cb_t s_controller_id_cb;

/* Validate a kbps value (read from NVS / passed by callers). Bad values
 * fall back to the Kconfig default. The UI dropdown can only produce one
 * of these four, but NVS contents could be anything if the partition was
 * tampered with or migrated from an older firmware. */
static can_speed_t sanitize_can_speed(uint16_t v) {
    switch (v) {
        case 125:  return CAN_SPEED_125_KBPS;
        case 250:  return CAN_SPEED_250_KBPS;
        case 500:  return CAN_SPEED_500_KBPS;
        case 1000: return CAN_SPEED_1000_KBPS;
        default:   return (can_speed_t)CONFIG_VESC_CAN_SPEED_KBPS;
    }
}

static void load_from_nvs(void) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        /* Namespace missing on first boot — leave cache at defaults. */
        ESP_LOGI(TAG, "no NVS namespace yet, using defaults");
        return;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open RO failed: %s — using defaults",
                 esp_err_to_name(err));
        return;
    }

    uint8_t  u8;
    uint16_t u16;

    if (nvs_get_u8 (h, "tgt_vesc_id", &u8 ) == ESP_OK) s_cache.target_vesc_id    = u8;
    if (nvs_get_u16(h, "can_kbps",    &u16) == ESP_OK) s_cache.can_speed         = sanitize_can_speed(u16);
    if (nvs_get_u8 (h, "brightness",  &u8 ) == ESP_OK) s_cache.brightness        = u8;
    if (nvs_get_u8 (h, "ctrl_id",     &u8 ) == ESP_OK) s_cache.controller_id     = u8;
    if (nvs_get_u8 (h, "batt_calc",   &u8 ) == ESP_OK) s_cache.battery_calc_mode = (battery_calc_mode_t)u8;
    if (nvs_get_u8 (h, "show_fps",    &u8 ) == ESP_OK) s_cache.show_fps          = (u8 != 0);
    if (nvs_get_u8 (h, "vesc_sim",    &u8 ) == ESP_OK) s_cache.vesc_emulator     = (u8 != 0);
    if (nvs_get_u16(h, "wheel_mm",    &u16) == ESP_OK) s_cache.wheel_diameter_mm = u16;
    if (nvs_get_u8 (h, "motor_poles", &u8 ) == ESP_OK) s_cache.motor_poles       = u8;
    if (nvs_get_u8 (h, "conn_mode",   &u8 ) == ESP_OK) s_cache.connection_mode   = sanitize_connection_mode(u8);

    /* float via blob — NVS has no native float type. */
    size_t sz = sizeof(float);
    float  fv;
    if (nvs_get_blob(h, "batt_cap", &fv, &sz) == ESP_OK && sz == sizeof(float)) {
        s_cache.battery_capacity = fv;
    }
    sz = sizeof(float);
    if (nvs_get_blob(h, "pwr_max", &fv, &sz) == ESP_OK && sz == sizeof(float)) {
        s_cache.power_max_kw = fv;
    }

    nvs_close(h);
}

static esp_err_t open_rw(nvs_handle_t *h) {
    esp_err_t err = nvs_open(NS, NVS_READWRITE, h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open RW failed: %s", esp_err_to_name(err));
    }
    return err;
}

static void commit(nvs_handle_t h) {
    esp_err_t err = nvs_commit(h);
    if (err != ESP_OK) ESP_LOGW(TAG, "nvs_commit failed: %s", esp_err_to_name(err));
    nvs_close(h);
}

void settings_init(void) {
    if (s_cache.loaded) return;

    /* Defaults — applied if NVS read misses any key. CAN-related ones come
     * from Kconfig so users who never touch the UI keep current behaviour. */
    s_cache.target_vesc_id    = CONFIG_VESC_CAN_TARGET_ID;
    s_cache.can_speed         = (can_speed_t)CONFIG_VESC_CAN_SPEED_KBPS;
    s_cache.brightness        = 80;
    s_cache.controller_id     = CONFIG_VESC_CAN_CONTROLLER_ID;
    s_cache.battery_capacity  = 15.0f;
    s_cache.battery_calc_mode = BATTERY_CALC_MODE_DIRECT;
    s_cache.show_fps          = true;
    s_cache.wheel_diameter_mm = 200;
    s_cache.motor_poles       = 7;
    s_cache.connection_mode   = CONN_ANDROID_AUTO;
    s_cache.power_max_kw      = 4.5f;
    s_cache.vesc_emulator     = false;

    load_from_nvs();
    s_cache.loaded = true;
    ESP_LOGI(TAG, "loaded: can=%d kbps brightness=%u%% target_id=%u ctrl_id=%u",
             (int)s_cache.can_speed, s_cache.brightness,
             s_cache.target_vesc_id, s_cache.controller_id);
}

/* ---------------- getters ---------------- */

uint8_t             settings_get_target_vesc_id(void)    { return s_cache.target_vesc_id; }
can_speed_t         settings_get_can_speed(void)         { return s_cache.can_speed; }
uint8_t             settings_get_screen_brightness(void) { return s_cache.brightness; }
uint8_t             settings_get_controller_id(void)     { return s_cache.controller_id; }
float               settings_get_battery_capacity(void)  { return s_cache.battery_capacity; }
battery_calc_mode_t settings_get_battery_calc_mode(void) { return s_cache.battery_calc_mode; }
bool                settings_get_show_fps(void)          { return s_cache.show_fps; }
uint16_t            settings_get_wheel_diameter_mm(void) { return s_cache.wheel_diameter_mm; }
uint8_t             settings_get_motor_poles(void)       { return s_cache.motor_poles; }
connection_mode_t   settings_get_connection_mode(void)   { return s_cache.connection_mode; }
float               settings_get_power_max_kw(void)      { return s_cache.power_max_kw; }
bool                settings_get_vesc_emulator(void)     { return s_cache.vesc_emulator; }

/* ---------------- setters ---------------- */

void settings_set_target_vesc_id(uint8_t id) {
    if (s_cache.target_vesc_id == id) return;
    s_cache.target_vesc_id = id;
    nvs_handle_t h;
    if (open_rw(&h) == ESP_OK) {
        nvs_set_u8(h, "tgt_vesc_id", id);
        commit(h);
    }
    if (s_target_id_cb) s_target_id_cb(id);
}

void settings_set_can_speed(can_speed_t speed) {
    can_speed_t s = sanitize_can_speed((uint16_t)speed);
    if (s_cache.can_speed == s) return;
    s_cache.can_speed = s;
    nvs_handle_t h;
    if (open_rw(&h) == ESP_OK) {
        nvs_set_u16(h, "can_kbps", (uint16_t)s);
        commit(h);
    }
    if (s_can_speed_cb) s_can_speed_cb((int)s);
}

void settings_set_screen_brightness(uint8_t brightness) {
    if (brightness > 100) brightness = 100;
    if (s_cache.brightness == brightness) return;
    s_cache.brightness = brightness;
    nvs_handle_t h;
    if (open_rw(&h) == ESP_OK) {
        nvs_set_u8(h, "brightness", brightness);
        commit(h);
    }
    if (s_brightness_cb) s_brightness_cb(brightness);
}

void settings_set_controller_id(uint8_t id) {
    if (s_cache.controller_id == id) return;
    s_cache.controller_id = id;
    nvs_handle_t h;
    if (open_rw(&h) == ESP_OK) {
        nvs_set_u8(h, "ctrl_id", id);
        commit(h);
    }
    if (s_controller_id_cb) s_controller_id_cb(id);
}

void settings_set_battery_capacity(float capacity) {
    if (s_cache.battery_capacity == capacity) return;
    s_cache.battery_capacity = capacity;
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_blob(h, "batt_cap", &capacity, sizeof(capacity));
    commit(h);
    /* Smart calc keeps a separate remaining-Ah counter; capacity change must
     * trigger a reset so the new full = new capacity. */
    battery_calc_capacity_changed();
}

void settings_set_battery_calc_mode(battery_calc_mode_t mode) {
    if (s_cache.battery_calc_mode == mode) return;
    s_cache.battery_calc_mode = mode;
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_u8(h, "batt_calc", (uint8_t)mode);
    commit(h);
}

void settings_set_show_fps(bool show) {
    if (s_cache.show_fps == show) return;
    s_cache.show_fps = show;
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_u8(h, "show_fps", show ? 1 : 0);
    commit(h);
}

void settings_set_wheel_diameter_mm(uint16_t diameter_mm) {
    if (s_cache.wheel_diameter_mm == diameter_mm) return;
    s_cache.wheel_diameter_mm = diameter_mm;
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_u16(h, "wheel_mm", diameter_mm);
    commit(h);
}

void settings_set_motor_poles(uint8_t poles) {
    if (s_cache.motor_poles == poles) return;
    s_cache.motor_poles = poles;
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_u8(h, "motor_poles", poles);
    commit(h);
}

void settings_set_connection_mode(connection_mode_t mode) {
    mode = sanitize_connection_mode((uint8_t)mode);
    if (s_cache.connection_mode == mode) return;
    s_cache.connection_mode = mode;
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_u8(h, "conn_mode", (uint8_t)mode);
    commit(h);
    /* No hot-apply callback — change requires reboot to re-init the stack
     * cleanly. The UI shows a "restart now?" msgbox after this returns. */
}

void settings_set_power_max_kw(float power_max_kw) {
    if (power_max_kw < 0.1f)  power_max_kw = 0.1f;
    if (power_max_kw > 100.0f) power_max_kw = 100.0f;
    if (s_cache.power_max_kw == power_max_kw) return;
    s_cache.power_max_kw = power_max_kw;
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_blob(h, "pwr_max", &power_max_kw, sizeof(power_max_kw));
    commit(h);
}

void settings_set_vesc_emulator(bool on) {
    if (s_cache.vesc_emulator == on) return;
    s_cache.vesc_emulator = on;
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_u8(h, "vesc_sim", on ? 1 : 0);
    commit(h);
}

/* ---------- debounced (volatile + persist) variants ----------
 * Volatile setters mirror their full counterparts EXCEPT they skip the
 * nvs_set/commit pair. Hot-apply callbacks still fire so the UI gets a
 * live preview (e.g. brightness changes the backlight while dragging).
 * Persist functions just flush the current cache value to NVS — they do
 * not re-fire callbacks. */

void settings_set_target_vesc_id_volatile(uint8_t id) {
    if (s_cache.target_vesc_id == id) return;
    s_cache.target_vesc_id = id;
    if (s_target_id_cb) s_target_id_cb(id);
}

void settings_persist_target_vesc_id(void) {
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_u8(h, "tgt_vesc_id", s_cache.target_vesc_id);
    commit(h);
}

void settings_set_screen_brightness_volatile(uint8_t brightness) {
    if (brightness > 100) brightness = 100;
    if (s_cache.brightness == brightness) return;
    s_cache.brightness = brightness;
    if (s_brightness_cb) s_brightness_cb(brightness);
}

void settings_persist_screen_brightness(void) {
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_u8(h, "brightness", s_cache.brightness);
    commit(h);
}

void settings_set_controller_id_volatile(uint8_t id) {
    if (s_cache.controller_id == id) return;
    s_cache.controller_id = id;
    if (s_controller_id_cb) s_controller_id_cb(id);
}

void settings_persist_controller_id(void) {
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    nvs_set_u8(h, "ctrl_id", s_cache.controller_id);
    commit(h);
}

void settings_set_battery_capacity_volatile(float capacity) {
    if (s_cache.battery_capacity == capacity) return;
    s_cache.battery_capacity = capacity;
    /* Smart-calc reset is a heavy-ish operation but still cheap compared to
     * a flash commit — keep it on the volatile path so the calc tracks the
     * live UI value, otherwise it'd reset 800 ms after the user lets go. */
    battery_calc_capacity_changed();
}

void settings_persist_battery_capacity(void) {
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    float v = s_cache.battery_capacity;
    nvs_set_blob(h, "batt_cap", &v, sizeof(v));
    commit(h);
}

void settings_set_power_max_kw_volatile(float power_max_kw) {
    if (power_max_kw < 0.1f)  power_max_kw = 0.1f;
    if (power_max_kw > 100.0f) power_max_kw = 100.0f;
    s_cache.power_max_kw = power_max_kw;
}

void settings_persist_power_max_kw(void) {
    nvs_handle_t h;
    if (open_rw(&h) != ESP_OK) return;
    float v = s_cache.power_max_kw;
    nvs_set_blob(h, "pwr_max", &v, sizeof(v));
    commit(h);
}

void settings_register_can_speed_cb(settings_can_speed_cb_t cb)         { s_can_speed_cb     = cb; }
void settings_register_brightness_cb(settings_brightness_cb_t cb)       { s_brightness_cb    = cb; }
void settings_register_target_id_cb(settings_target_id_cb_t cb)         { s_target_id_cb     = cb; }
void settings_register_controller_id_cb(settings_controller_id_cb_t cb) { s_controller_id_cb = cb; }
