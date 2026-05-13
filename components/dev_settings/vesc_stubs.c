#include "vesc_limits.h"

#include "esp_log.h"

static const char *TAG = "vesc_limits_stub";

/* Stub — vesc_app component (planned) will replace these with real CAN
 * round-trips. For now the Settings → "Read limits" / "Apply limits"
 * buttons surface a clear failure to the user instead of silently
 * pretending to work. */

void vesc_limits_init(void) {}

bool vesc_limits_is_valid(void) { return false; }

const vesc_motor_limits_t *vesc_limits_get(void) { return NULL; }

bool vesc_limits_request(uint8_t target_id) {
    ESP_LOGW(TAG, "vesc_limits_request(%u): backend not wired", target_id);
    return false;
}

bool vesc_limits_set_current_max(uint8_t target_id, float motor, float battery) {
    ESP_LOGW(TAG, "vesc_limits_set_current_max(%u, %.1f, %.1f): backend not wired",
             target_id, motor, battery);
    return false;
}

bool vesc_limits_set_speed_max(uint8_t target_id, float erpm) {
    ESP_LOGW(TAG, "vesc_limits_set_speed_max(%u, %.0f): backend not wired",
             target_id, erpm);
    return false;
}
