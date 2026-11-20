/* Run production backup/reset against memory NVS, including settings writes
 * that survive BEFORE commit (real NVS can persist individual set operations).
 * No reimplementation of recovery logic in the test. */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "nvs.h"
#include "../../main/boot_guard_settings.h"

static const char *keys[] = {"brightness", "show_fps", "dash_theme", "splash_loops",
    "disp_flip", "music_wdg", "music_btn", "aa_autocon", "phone_scr", "tgt_vesc_id"};
typedef struct { bool present[10]; uint8_t value[10]; } config_t;
static config_t durable, pending;
static unsigned char blob[64], pending_blob[64];
static size_t blob_size, pending_size;
static bool fail_backup_commit;
static int fail_cfg_write = -1;
static int key_index(const char *key) {
    for (int i = 0; i < 10; ++i) if (!strcmp(key, keys[i])) return i;
    assert(!"Unexpected key: recovery must not touch other configuration");
    return 0;
}
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle) {
    (void)mode;
    if (!strcmp(name, "boot_guard")) {
        *handle = 1; pending_size = blob_size; memcpy(pending_blob, blob, blob_size);
    } else {
        assert(!strcmp(name, "vesc_cfg")); *handle = 2; pending = durable;
    }
    return ESP_OK;
}
void nvs_close(nvs_handle_t h) { (void)h; }
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *n) {
    assert(h == 1 && !strcmp(key, "display_backup"));
    if (!blob_size) return ESP_ERR_NVS_NOT_FOUND;
    if (*n < blob_size) return ESP_ERR_INVALID_SIZE;
    memcpy(out, blob, blob_size); *n = blob_size; return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *value, size_t n) {
    assert(h == 1 && !strcmp(key, "display_backup") && n <= sizeof(blob));
    memcpy(pending_blob, value, n); pending_size = n; return ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t h, const char *key, uint8_t *out) {
    assert(h == 2); int i = key_index(key);
    if (!durable.present[i]) return ESP_ERR_NVS_NOT_FOUND;
    *out = durable.value[i]; return ESP_OK;
}
esp_err_t nvs_set_u8(nvs_handle_t h, const char *key, uint8_t value) {
    assert(h == 2); int i = key_index(key);
    if (fail_cfg_write == 0) return ESP_FAIL;
    if (fail_cfg_write > 0) --fail_cfg_write;
    pending.present[i] = durable.present[i] = true;
    pending.value[i] = durable.value[i] = value; return ESP_OK;
}
esp_err_t nvs_erase_key(nvs_handle_t h, const char *key) {
    assert(h == 2); int i = key_index(key);
    if (!pending.present[i]) return ESP_ERR_NVS_NOT_FOUND;
    pending.present[i] = durable.present[i] = false;
    pending.value[i] = durable.value[i] = 0; return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h) {
    if (h == 1) {
        if (fail_backup_commit) return ESP_FAIL;
        memcpy(blob, pending_blob, pending_size); blob_size = pending_size;
    } else { assert(h == 2); durable = pending; }
    return ESP_OK;
}
static void fixture(void) {
    memset(&durable, 0, sizeof(durable));
    for (int i = 0; i < 10; ++i) {
        durable.present[i] = (i % 2) == 0 || i == 9;
        if (durable.present[i]) durable.value[i] = 10 + i;
    }
    blob_size = 0; fail_backup_commit = false; fail_cfg_write = -1;
}
static void same(config_t expected) {
    for (int i = 0; i < 10; ++i) {
        assert(durable.present[i] == expected.present[i]);
        if (expected.present[i]) assert(durable.value[i] == expected.value[i]);
    }
}
int main(void) {
    fixture(); config_t original = durable;
    assert(boot_guard_display_settings(true) == ESP_ERR_NOT_FOUND);
    same(original);
    assert(boot_guard_display_settings(false) == ESP_OK);
    assert(blob_size > 0);
    assert(durable.value[0] == 80 && durable.value[9] == original.value[9]);
    for (int i = 1; i < 9; ++i) assert(durable.present[i] && durable.value[i] == 0);
    /* A second reset must not replace the original backup with defaults. */
    assert(boot_guard_display_settings(false) == ESP_OK);
    assert(boot_guard_display_settings(true) == ESP_OK); same(original);
    assert(boot_guard_display_settings(true) == ESP_OK); same(original);

    fixture(); original = durable; fail_backup_commit = true;
    assert(boot_guard_display_settings(false) != ESP_OK); same(original);
    assert(blob_size == 0); /* cannot modify preferences before durable backup */

    fixture(); original = durable; fail_cfg_write = 3;
    assert(boot_guard_display_settings(false) != ESP_OK);
    assert(blob_size > 0); /* backup survives interrupted reset */
    fail_cfg_write = -1;
    assert(boot_guard_display_settings(false) == ESP_OK);
    assert(boot_guard_display_settings(true) == ESP_OK); same(original);

    fixture(); original = durable; blob_size = 2; memset(blob, 0, sizeof(blob));
    assert(boot_guard_display_settings(false) != ESP_OK); same(original);
    assert(boot_guard_display_settings(true) != ESP_OK); same(original);
    puts("boot guard NVS: PASS (backup, defaults, repeat, restore, missing keys, unrelated keys, write failures, corrupt backup)");
    return 0;
}
