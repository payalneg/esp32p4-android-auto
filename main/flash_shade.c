#include "flash_shade.h"

#include <stdatomic.h>
#include <stdint.h>

#include "bsp/esp-bsp.h"
#include "dev_settings.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "flash_shade";

/* How long the backlight stays off after the last flash op. The panel runs
 * at ~60 Hz, so this is several full scan-outs — enough for the frame that
 * was torn by the stall to have been replaced by a clean one. Back-to-back
 * ops (an NVS commit is a handful of page writes, an OTA thousands) re-arm
 * this and the backlight stays off until the burst is over. */
#define FLASH_SHADE_HOLD_MS  80

/* Writes shorter than this are left alone. A flash page program is ~0.3 ms
 * of stall — a dozen scan lines out of 480, invisible in practice — and the
 * small writers are frequent: the trip log appends a 64-byte record every
 * 10 s, NVS writes 32-byte entries. Blanking the panel for 80 ms on each of
 * those is far worse than the tear it hides. What tears visibly is a sector
 * erase (30-100 ms, several whole frames) or a multi-page write (OTA and
 * littlefs write 4 KB at a time, ~6 ms = a third of a frame) — those are
 * shaded. */
#define FLASH_SHADE_MIN_WRITE  1024

static esp_timer_handle_t s_restore;
static atomic_bool        s_armed;
static bool               s_shaded;           /* under s_lock */
static portMUX_TYPE       s_lock = portMUX_INITIALIZER_UNLOCKED;

/* The flag and the LEDC write move together under the spinlock, so a write
 * that begins while the restore timer is firing cannot end up lit: either it
 * sees the backlight already off, or it turns it off after the restore. */
static void restore_cb(void *arg)
{
    (void)arg;
    portENTER_CRITICAL(&s_lock);
    s_shaded = false;
    bsp_display_brightness_set(settings_get_screen_brightness());
    portEXIT_CRITICAL(&s_lock);
    ESP_LOGI(TAG, "backlight back");
}

void flash_shade_arm(void)
{
    if (!s_restore) {
        const esp_timer_create_args_t args = {
            .callback = restore_cb,
            .name     = "flash_shade",
        };
        if (esp_timer_create(&args, &s_restore) != ESP_OK) {
            ESP_LOGW(TAG, "timer create failed — flash ops will show on the panel");
            return;
        }
    }
    atomic_store(&s_armed, true);
    ESP_LOGI(TAG, "armed: backlight off during flash write/erase, back %d ms after the last",
             FLASH_SHADE_HOLD_MS);
}

/* The wrappers run in whatever task asked for the flash op. Stay out of the
 * way when that is not a normal task context — the panic path, or anything
 * before the scheduler — where neither LEDC nor esp_timer may be touched. */
static bool usable(void)
{
    return atomic_load(&s_armed) && !xPortInIsrContext() &&
           xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

static void shade_begin(void)
{
    if (!usable()) return;
    portENTER_CRITICAL(&s_lock);
    if (!s_shaded) {
        s_shaded = true;
        bsp_display_brightness_set(0);
    }
    portEXIT_CRITICAL(&s_lock);
}

static void shade_end(void)
{
    if (!usable()) return;
    /* stop() on an idle timer just says so; both calls are µs. A race
     * between two tasks here only shortens or lengthens the hold slightly. */
    esp_timer_stop(s_restore);
    esp_timer_start_once(s_restore, (uint64_t)FLASH_SHADE_HOLD_MS * 1000);
}

/* ---- linker --wrap interposers (see main/CMakeLists.txt) ---- */

esp_err_t __real_esp_flash_write(esp_flash_t *chip, const void *buffer,
                                 uint32_t address, uint32_t length);
esp_err_t __real_esp_flash_erase_region(esp_flash_t *chip, uint32_t start, uint32_t len);
esp_err_t __real_esp_flash_erase_chip(esp_flash_t *chip);

esp_err_t __wrap_esp_flash_write(esp_flash_t *chip, const void *buffer,
                                 uint32_t address, uint32_t length)
{
    if (length < FLASH_SHADE_MIN_WRITE) {
        return __real_esp_flash_write(chip, buffer, address, length);
    }
    ESP_LOGI(TAG, "write 0x%06x +%u -> backlight off", (unsigned)address, (unsigned)length);
    shade_begin();
    esp_err_t r = __real_esp_flash_write(chip, buffer, address, length);
    shade_end();
    return r;
}

esp_err_t __wrap_esp_flash_erase_region(esp_flash_t *chip, uint32_t start, uint32_t len)
{
    ESP_LOGI(TAG, "erase 0x%06x +%u -> backlight off", (unsigned)start, (unsigned)len);
    shade_begin();
    esp_err_t r = __real_esp_flash_erase_region(chip, start, len);
    shade_end();
    return r;
}

esp_err_t __wrap_esp_flash_erase_chip(esp_flash_t *chip)
{
    shade_begin();
    esp_err_t r = __real_esp_flash_erase_chip(chip);
    shade_end();
    return r;
}
