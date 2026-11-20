#include "boot_guard.h"
#include "boot_guard_policy.h"
#include "boot_guard_settings.h"
#include "ui_mode.h"
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "dev_settings.h"

#define RECORD_MAGIC 0x42474731u
typedef struct {
    uint32_t magic;
    uint8_t hash[32];
    uint8_t failures, armed, forced, safe;
} record_t;
static record_t s_record;
static bool s_recovery;
static atomic_bool s_ready, s_loaded, s_stable, s_failed, s_command_pending;
static atomic_int s_command_result;
static int64_t s_deadline, s_ui, s_ble, s_nav, s_ready_at, s_flash_until;
static bool s_ui_required = true;
static const char *s_stage = "startup";
static uint32_t s_nonce;
static esp_task_wdt_user_handle_t s_wdt;
static QueueHandle_t s_commands;
static SemaphoreHandle_t s_config_lock;
static SemaphoreHandle_t s_record_lock;
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static const char *TAG = "boot_guard";

static esp_err_t save(void)
{
    nvs_handle_t h;
    esp_err_t e = nvs_open("boot_guard", NVS_READWRITE, &h);
    if (e != ESP_OK) return e;
    e = nvs_set_blob(h, "record", &s_record, sizeof(s_record));
    if (e == ESP_OK) e = nvs_commit(h);
    nvs_close(h);
    return e;
}

static void configure_wdt(uint32_t timeout)
{
    esp_task_wdt_config_t config = {
        .timeout_ms = timeout,
        .idle_core_mask = (1u << portNUM_PROCESSORS) - 1,
        .trigger_panic = true,
    };
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    esp_err_t e = esp_task_wdt_reconfigure(&config);
    if (e == ESP_ERR_INVALID_STATE) e = esp_task_wdt_init(&config);
    xSemaphoreGive(s_config_lock);
    ESP_ERROR_CHECK(e);
}

static void command_task(void *arg)
{
    uint8_t op;
    while (xQueueReceive(s_commands, &op, portMAX_DELAY) == pdTRUE) {
        boot_guard_flash_begin();
        xSemaphoreTake(s_record_lock, portMAX_DELAY);
        esp_err_t e = ESP_OK;
        if (op == 2 || op == 3) e = boot_guard_display_settings(false);
        if (op == 4) e = boot_guard_display_settings(true);
        if (e == ESP_OK) {
            record_t old = s_record;
            s_record.forced = op == 1;
            if (op == 2 || op == 3) s_record.safe = 1;
            if (op == 4) s_record.safe = 0;
            s_record.failures = 0;
            s_record.armed = 0;
            e = save();
            if (e == ESP_OK && op == 3) e = fw_slots_boot_other();
            if (e != ESP_OK) {
                s_record = old;
                if (op == 3) s_record.safe = 1;
                save();
            }
        }
        s_command_result = e;
        xSemaphoreGive(s_record_lock);
        ESP_LOGW(TAG, "recovery command %u: %s", op, esp_err_to_name(e));
        if (e == ESP_OK) { vTaskDelay(pdMS_TO_TICKS(500)); esp_restart(); }
        boot_guard_flash_end();
        s_command_pending = false;
    }
}

static void guard_task(void *arg)
{
    for (;;) {
        int64_t now = esp_timer_get_time();
        portENTER_CRITICAL(&s_lock);
        bool expired_flash = s_flash_until && now >= s_flash_until;
        if (expired_flash) s_flash_until = 0;
        bool overdue = boot_guard_due(now, s_deadline, s_ready, s_ui_required ? s_ui : 0, s_ble);
        if (s_ready && now >= s_deadline && s_nav && now - s_nav > 15000000) overdue = true;
        bool flash = s_flash_until > now;
        bool ready = s_ready;
        int64_t ready_at = s_ready_at;
        const char *stage = s_stage;
        portEXIT_CRITICAL(&s_lock);
        if (expired_flash) {
            configure_wdt(15000);
            s_failed = true;
            ESP_LOGE(TAG, "flash maintenance exceeded 120 seconds");
        }
        if (overdue && !flash && !s_failed) {
            s_failed = true;
            ESP_LOGE(TAG, "heartbeat expired in %s; waiting for hardware watchdog", stage);
        }
        if (!s_failed) ESP_ERROR_CHECK(esp_task_wdt_reset_user(s_wdt));
        if (!s_failed && !flash && !s_command_pending && s_loaded && ready && !s_recovery && !s_stable &&
            now - ready_at >= 60000000 && xSemaphoreTake(s_record_lock, 0) == pdTRUE) {
            s_record.failures = 0;
            if (save() == ESP_OK) s_stable = true;
            xSemaphoreGive(s_record_lock);
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void boot_guard_start(void)
{
    s_config_lock = xSemaphoreCreateMutex();
    s_record_lock = xSemaphoreCreateMutex();
    s_commands = xQueueCreate(1, sizeof(uint8_t));
    ESP_ERROR_CHECK(s_config_lock && s_record_lock && s_commands ? ESP_OK : ESP_ERR_NO_MEM);
    s_nonce = esp_random();
    s_deadline = esp_timer_get_time() + 180000000;
    configure_wdt(15000);
    ESP_ERROR_CHECK(esp_task_wdt_add_user("application_health", &s_wdt));
    ESP_ERROR_CHECK(xTaskCreate(guard_task, "boot_guard", 3072, NULL, 4, NULL) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(command_task, "recovery", 4096, NULL, 3, NULL) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}

void boot_guard_load(void)
{
    nvs_handle_t h;
    size_t n = sizeof(s_record);
    if (nvs_open("boot_guard", NVS_READONLY, &h) == ESP_OK) {
        if (nvs_get_blob(h, "record", &s_record, &n) != ESP_OK || n != sizeof(s_record))
            memset(&s_record, 0, sizeof(s_record));
        nvs_close(h);
    }
    const esp_app_desc_t *app = esp_app_get_description();
    if (s_record.magic != RECORD_MAGIC) memset(&s_record, 0, sizeof(s_record));
    bool same = s_record.magic == RECORD_MAGIC && !memcmp(s_record.hash, app->app_elf_sha256, 32);
    esp_reset_reason_t r = esp_reset_reason();
    bool fault = r == ESP_RST_PANIC || r == ESP_RST_INT_WDT || r == ESP_RST_TASK_WDT ||
                 r == ESP_RST_WDT || r == ESP_RST_CPU_LOCKUP;
    s_record.failures = boot_guard_failures(s_record.failures, same, s_record.armed, fault);
    /* A successfully selected new image leaves forced recovery, retaining the safe-display preference. */
    if (!same) s_record.forced = 0;
    s_record.magic = RECORD_MAGIC;
    memcpy(s_record.hash, app->app_elf_sha256, 32);
    s_record.armed = 1;
    s_recovery = s_record.forced || s_record.failures >= 3;
    if (s_recovery) s_record.forced = 1;
    esp_err_t saved = save();
    if (saved != ESP_OK) {
        /* Broken/full settings storage must not trap the user in a panic loop. */
        s_recovery = true;
        s_record.forced = 1;
        ESP_LOGE(TAG, "cannot persist boot state (%s); starting recovery", esp_err_to_name(saved));
    }
    s_loaded = true;
    ESP_LOGW(TAG, "failures=%u recovery=%d safe_display=%u", s_record.failures, s_recovery, s_record.safe);
}

bool boot_guard_recovery(void) { return s_recovery; }
bool boot_guard_safe_display(void) { return s_record.safe; }
void boot_guard_stage(const char *stage)
{
    portENTER_CRITICAL(&s_lock);
    s_stage = stage;
    s_deadline = esp_timer_get_time() + 180000000;
    portEXIT_CRITICAL(&s_lock);
}
void boot_guard_ready(void)
{
    portENTER_CRITICAL(&s_lock);
    s_ready_at = esp_timer_get_time();
    s_deadline = s_ready_at + 15000000;
    s_stage = s_recovery ? "recovery" : "running";
    s_ready = true;
    portEXIT_CRITICAL(&s_lock);
}
void boot_guard_ui_tick(void)
{
    portENTER_CRITICAL(&s_lock);
    s_ui = esp_timer_get_time();
    portEXIT_CRITICAL(&s_lock);
}
void boot_guard_ble_tick(void)
{
    portENTER_CRITICAL(&s_lock);
    s_ble = esp_timer_get_time();
    portEXIT_CRITICAL(&s_lock);
}
void boot_guard_ui_required(bool required)
{
    portENTER_CRITICAL(&s_lock);
    /* An old decoder frame must not disable monitoring after AA was left. */
    if (!required && ui_mode_get() != UI_MODE_AA) {
        portEXIT_CRITICAL(&s_lock);
        return;
    }
    if (required && !s_ui_required) s_ui = esp_timer_get_time();
    s_ui_required = required;
    portEXIT_CRITICAL(&s_lock);
}
void boot_guard_nav_tick(void)
{
    portENTER_CRITICAL(&s_lock);
    s_nav = esp_timer_get_time();
    portEXIT_CRITICAL(&s_lock);
}
void boot_guard_flash_begin(void)
{
    configure_wdt(120000);
    portENTER_CRITICAL(&s_lock);
    s_flash_until = esp_timer_get_time() + 120000000;
    portEXIT_CRITICAL(&s_lock);
}
void boot_guard_flash_end(void)
{
    portENTER_CRITICAL(&s_lock);
    s_flash_until = 0;
    s_deadline = esp_timer_get_time() + 15000000;
    portEXIT_CRITICAL(&s_lock);
    configure_wdt(15000);
}
int boot_guard_status(char *buf, size_t capacity)
{
    char other[48] = "";
    fw_slots_other_label(other, sizeof(other));
    const esp_partition_t *running = esp_ota_get_running_partition();
    /* Version strings originate in another image, so keep JSON well formed. */
    for (char *p = other; *p; ++p) if ((unsigned char)*p < 32 || *p == '"' || *p == '\\') *p = '?';
    xSemaphoreTake(s_record_lock, portMAX_DELAY);
    int n = snprintf(buf, capacity,
        "{\"v\":1,\"recovery\":%s,\"safe\":%s,\"failures\":%u,\"reason\":%d,\"slot\":\"%s\",\"other\":\"%.32s\",\"nonce\":%lu,\"error\":%d}",
        s_recovery ? "true" : "false", s_record.safe ? "true" : "false",
        s_record.failures, esp_reset_reason(), running ? running->label : "?", other, (unsigned long)s_nonce, (int)s_command_result);
    xSemaphoreGive(s_record_lock);
    return n;
}
esp_err_t boot_guard_command(const uint8_t *data, size_t size)
{
    uint8_t op;
    if (s_failed || !boot_guard_decode_command(data, size, s_nonce, s_recovery, &op)) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&s_lock);
    bool flashing = s_flash_until != 0;
    portEXIT_CRITICAL(&s_lock);
    if (flashing) return ESP_ERR_INVALID_STATE;
    if (atomic_exchange(&s_command_pending, true)) return ESP_ERR_INVALID_STATE;
    s_command_result = ESP_OK;
    if (xQueueSend(s_commands, &op, 0) == pdTRUE) return ESP_OK;
    s_command_pending = false;
    return ESP_ERR_TIMEOUT;
}
