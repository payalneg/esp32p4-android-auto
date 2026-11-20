#include "boot_guard_settings.h"
#include "nvs.h"
#include <stdint.h>
#define RECORD_MAGIC 0x42474731u

/* Only display keys. Keep a durable backup BEFORE changing the shared NVS. */
static const char *const display_keys[] = {
    "brightness", "show_fps", "dash_theme", "splash_loops", "disp_flip",
    "music_wdg", "music_btn", "aa_autocon", "phone_scr",
};
typedef struct { uint32_t magic, present; uint8_t values[9]; } backup_t;
/* Compile-time guard also works with the small Windows host-test compiler. */
typedef char backup_key_count_must_match[(sizeof(display_keys) / sizeof(display_keys[0]) == 9) ? 1 : -1];
esp_err_t boot_guard_display_settings(bool restore)
{
    nvs_handle_t guard, cfg;
    esp_err_t e = nvs_open("boot_guard", NVS_READWRITE, &guard);
    if (e != ESP_OK) return e;
    e = nvs_open("vesc_cfg", NVS_READWRITE, &cfg);
    if (e != ESP_OK) { nvs_close(guard); return e; }
    backup_t backup = { .magic = RECORD_MAGIC };
    size_t n = sizeof(backup);
    e = nvs_get_blob(guard, "display_backup", &backup, &n);
    bool exists = e == ESP_OK && n == sizeof(backup) && backup.magic == RECORD_MAGIC;
    if (!exists && restore) { e = ESP_ERR_NOT_FOUND; goto out; }
    if (!exists) {
        if (e != ESP_ERR_NVS_NOT_FOUND) {
            if (e == ESP_OK) e = ESP_ERR_INVALID_SIZE;
            goto out;
        }
        backup = (backup_t){ .magic = RECORD_MAGIC };
        for (unsigned i = 0; i < sizeof(display_keys)/sizeof(display_keys[0]); ++i) {
            e = nvs_get_u8(cfg, display_keys[i], &backup.values[i]);
            if (e == ESP_OK) backup.present |= 1u << i;
            else if (e != ESP_ERR_NVS_NOT_FOUND) goto out;
        }
        e = nvs_set_blob(guard, "display_backup", &backup, sizeof(backup));
        if (e == ESP_OK) e = nvs_commit(guard);
        if (e != ESP_OK) goto out;
    }
    for (unsigned i = 0; i < sizeof(display_keys)/sizeof(display_keys[0]); ++i) {
        if (restore && !(backup.present & (1u << i))) {
            e = nvs_erase_key(cfg, display_keys[i]);
            if (e == ESP_ERR_NVS_NOT_FOUND) e = ESP_OK;
        } else {
            e = nvs_set_u8(cfg, display_keys[i], restore ? backup.values[i] : (i == 0 ? 80 : 0));
        }
        if (e != ESP_OK) goto out;
    }
    e = nvs_commit(cfg);
out:
    nvs_close(cfg);
    nvs_close(guard);
    return e;
}

