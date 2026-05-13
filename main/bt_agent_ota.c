#include "bt_agent_ota.h"

#include "sdkconfig.h"

#ifndef CONFIG_BT_AGENT_OTA_ENABLED

esp_err_t bt_agent_ota_check_and_update(void) { return ESP_OK; }

#else  /* CONFIG_BT_AGENT_OTA_ENABLED */

#include <stdio.h>
#include <string.h>

#include "bt_agent_fw.h"
#include "bt_link.h"
#include "esp_heap_caps.h"
#include "esp_loader.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp32_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ota_screen.h"

static const char *TAG = "bt_agent_ota";

/* Merged image starts at 0x1000 (bootloader offset on ESP32 classic).
 * We pass `--target-offset 0x1000` to esptool merge_bin so the blob has no
 * leading 4 KiB of padding — and crucially, we do NOT write to 0x0..0x1000
 * which the ROM bootloader rejects on this chip. Without that, the first
 * write succeeds (the leading FF page is silently dropped by the ROM) but
 * the second block returns FLASH_WRITE_FAIL — exactly what we saw. */
#define BT_AGENT_FLASH_ADDR  0x1000

/* esp_loader_flash_write block size — library expects multiples of 1024.
 * 1 KiB chunks: smaller per-block transmission window means a single byte
 * loss only forfeits 1 KiB of progress rather than 4 KiB, and retries
 * are quicker. Combined with the internal-RAM staging below this is the
 * combination that survives end-to-end on this board's wiring. */
#define FLASH_BLOCK_SIZE  (1 * 1024)

/* Briefly let LVGL render a frame while we're otherwise keeping it paused
 * for the duration of the flash. ~40 ms is enough for one render pass at
 * the usual frame rate; we call this only between flash blocks so it
 * cannot interleave with an in-flight ACK from the agent. */
static void lvgl_pulse_if_paused(bool paused)
{
    if (!paused) return;
    if (esp_lv_adapter_resume() != ESP_OK) return;
    vTaskDelay(pdMS_TO_TICKS(40));
    esp_lv_adapter_pause(1000);
}

static esp_err_t do_flash_image(char *err_buf, size_t err_buf_len,
                                bool lvgl_paused)
{
    const uint8_t *data = bt_agent_fw_data();
    size_t         size = bt_agent_fw_size();
    if (!data || size == 0) {
        ESP_LOGE(TAG, "embedded blob empty — bt_agent_fw.bin missing?");
        snprintf(err_buf, err_buf_len, "blob missing");
        return ESP_ERR_NOT_FOUND;
    }

    /* Initialize the esp_serial_flasher port. It owns the UART for the
     * duration of the flash; bt_link must have already released it. */
    loader_esp32_config_t cfg = {
        .baud_rate         = 115200,
        .uart_port         = BT_AGENT_UART_PORT,
        .uart_rx_pin       = BT_AGENT_UART_RX,
        .uart_tx_pin       = BT_AGENT_UART_TX,
        .reset_trigger_pin = BT_AGENT_RST_PIN,
        .gpio0_trigger_pin = BT_AGENT_IO0_PIN,
    };
    ota_screen_set_status("Connecting to bootloader...");
    lvgl_pulse_if_paused(lvgl_paused);
    if (loader_port_esp32_init(&cfg) != ESP_LOADER_SUCCESS) {
        ESP_LOGE(TAG, "loader_port_esp32_init failed");
        snprintf(err_buf, err_buf_len, "port init failed");
        return ESP_FAIL;
    }

    /* Connect with stub uploader. The plain `esp_loader_connect` talks
     * directly to the mask-ROM bootloader, which on ESP32 classic loses
     * sync deterministically around the 3rd block of a multi-block write
     * (reproducible flash_write @ 0x2000 fail in our setup). The stub is
     * a small program esp_serial_flasher uploads into the target's RAM
     * that re-implements the same protocol over a known-good codepath —
     * also ~5× faster on big writes. */
    esp_loader_connect_args_t connect = ESP_LOADER_CONNECT_DEFAULT();
    if (esp_loader_connect_with_stub(&connect) != ESP_LOADER_SUCCESS) {
        ESP_LOGE(TAG, "esp_loader_connect_with_stub failed");
        snprintf(err_buf, err_buf_len, "ROM/stub sync failed");
        loader_port_esp32_deinit();
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "connected to agent (stub uploaded)");

    /* Bump baud to CONFIG_BT_AGENT_FLASH_BAUD for the actual transfer.
     * Falls back gracefully if the ROM/stub doesn't support it. */
    if (esp_loader_change_transmission_rate(CONFIG_BT_AGENT_FLASH_BAUD) ==
            ESP_LOADER_SUCCESS) {
        loader_port_change_transmission_rate(CONFIG_BT_AGENT_FLASH_BAUD);
        ESP_LOGI(TAG, "baud → %d", CONFIG_BT_AGENT_FLASH_BAUD);
    }

    ota_screen_set_status("Erasing flash...");
    ota_screen_set_progress(0, size);
    lvgl_pulse_if_paused(lvgl_paused);
    if (esp_loader_flash_start(BT_AGENT_FLASH_ADDR, size, FLASH_BLOCK_SIZE)
            != ESP_LOADER_SUCCESS) {
        ESP_LOGE(TAG, "flash_start failed");
        snprintf(err_buf, err_buf_len, "erase failed");
        loader_port_esp32_deinit();
        return ESP_FAIL;
    }

    /* Staging buffer in internal RAM. The decompressed blob lives in PSRAM;
     * feeding a PSRAM pointer to esp_loader_flash_write makes its UART TX
     * stream from PSRAM directly and the slow memory access widens per-byte
     * gaps on the line just enough that ROM/stub occasionally misses our
     * checksum byte and the ACK never comes back. memcpy chunk-at-a-time
     * to DRAM up front, then the actual UART TX is fed from fast memory. */
    uint8_t *staging = heap_caps_malloc(FLASH_BLOCK_SIZE,
                                        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!staging) {
        ESP_LOGE(TAG, "internal-ram staging alloc failed");
        snprintf(err_buf, err_buf_len, "no internal RAM for staging");
        loader_port_esp32_deinit();
        return ESP_ERR_NO_MEM;
    }

    ota_screen_set_status("Writing firmware...");
    lvgl_pulse_if_paused(lvgl_paused);
    size_t   off = 0;
    int      pct_last  = -1;
    int      pct_drawn = 0;
    while (off < size) {
        size_t chunk = (size - off > FLASH_BLOCK_SIZE)
                           ? FLASH_BLOCK_SIZE : (size - off);
        memcpy(staging, data + off, chunk);
        if (esp_loader_flash_write(staging, chunk) != ESP_LOADER_SUCCESS) {
            ESP_LOGE(TAG, "flash_write @ 0x%x failed", (unsigned)off);
            snprintf(err_buf, err_buf_len,
                     "write failed @ 0x%x", (unsigned)off);
            free(staging);
            loader_port_esp32_deinit();
            return ESP_FAIL;
        }
        off += chunk;
        ota_screen_set_progress(off, size);
        int pct = (int)((off * 100) / size);
        if (pct / 10 != pct_last / 10) {
            ESP_LOGI(TAG, "flashed %d%% (%u/%u bytes)",
                     pct, (unsigned)off, (unsigned)size);
            pct_last = pct;
        }
        /* ~5% steps → ~20 short LVGL render windows over the whole flash.
         * Frequent enough that the user sees the bar move, rare enough
         * that the ACK-loss issue (see the pause comment in the caller)
         * stays away. */
        if (pct >= pct_drawn + 5) {
            pct_drawn = pct - (pct % 5);
            lvgl_pulse_if_paused(lvgl_paused);
        }
    }
    free(staging);

    ota_screen_set_status("Verifying…");
    lvgl_pulse_if_paused(lvgl_paused);
    /* Pass true → reboot into the new app. Library handles the reset. */
    if (esp_loader_flash_finish(true) != ESP_LOADER_SUCCESS) {
        ESP_LOGE(TAG, "flash_finish failed");
        snprintf(err_buf, err_buf_len, "verify/finish failed");
        loader_port_esp32_deinit();
        return ESP_FAIL;
    }
    loader_port_esp32_deinit();
    ESP_LOGI(TAG, "flash done, agent rebooting");
    return ESP_OK;
}

esp_err_t bt_agent_ota_check_and_update(void)
{
    /* Mute [BT] forwarding while we wait + flash. A broken agent reboots
     * itself ~9× per second; each iteration emits ~80 bytes of bootloader
     * text → rx_task does ~270 printf/sec → enough to starve IDLE0 on
     * core 0 (TWDT trips at 5 s). Parsing for BT-VER: still runs muted. */
    bt_link_set_quiet(true);

    const char *expected = CONFIG_BT_AGENT_FW_VERSION;
    const char *actual   = bt_agent_wait_version(CONFIG_BT_AGENT_VERSION_TIMEOUT_MS);

    if (actual && strcmp(actual, expected) == 0) {
        ESP_LOGI(TAG, "agent ver=%s matches — OTA skipped", actual);
        bt_link_set_quiet(false);
        return ESP_OK;
    }

    /* Show the overlay before kicking off any UART/GPIO surgery — the user
     * sees "Updating BT agent" instead of staring at a frozen idle screen
     * for the duration of the ~50 s flash. Also unconditionally shown on
     * mismatch so the user always knows when their agent got rewritten. */
    ota_screen_set_title("Updating BT agent");
    ota_screen_show("Don't power off");
    char subtitle[80];
    if (actual) {
        ESP_LOGW(TAG, "agent ver=%s, expected=%s — reflashing", actual, expected);
        snprintf(subtitle, sizeof(subtitle), "Found %s, expected %s",
                 actual, expected);
    } else {
        ESP_LOGW(TAG, "no BT-VER: in %d ms — assuming agent broken, reflashing",
                 CONFIG_BT_AGENT_VERSION_TIMEOUT_MS);
        snprintf(subtitle, sizeof(subtitle), "Agent firmware unresponsive");
    }
    ota_screen_set_status(subtitle);

    bt_link_suspend_for_flash();
    bt_agent_enter_bootloader();
    /* ROM emits ~80 bytes of `ets ...rst:0x3 ...waiting for download` text
     * after the reset edge. If we start SYNC while that prelude is still
     * landing in the UART FIFO, the loader tries to parse it as a SLIP
     * frame and quietly desynchronises — repro: deterministic flash_write
     * @ 0x2000 fail. 500 ms is enough margin for ROM to finish printing
     * and the FIFO to drain before loader_port_esp32_init reinstalls the
     * driver. */
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Pause the LVGL worker for the duration of the flash. Empirically
     * lvgl @ prio 6 on core 0 was preempting our UART RX path often enough
     * during long writes to lose the occasional ACK byte from the agent
     * (repro: flash_write fail around the 80% mark even with stub +
     * retries=10 + DRAM staging). Progress bar stays frozen for ~50 s
     * which is fine — overlay status text is still visible. */
    bool lvgl_paused = (esp_lv_adapter_pause(2000) == ESP_OK);

    char err_msg[64] = "";
    esp_err_t flash_err = do_flash_image(err_msg, sizeof(err_msg), lvgl_paused);

    if (lvgl_paused) esp_lv_adapter_resume();
    bt_link_resume_after_flash();

    if (flash_err != ESP_OK) {
        ESP_LOGE(TAG, "OTA failed (%s) — agent may be unbootable",
                 esp_err_to_name(flash_err));
        char line[96];
        snprintf(line, sizeof(line), "Failed: %s", err_msg);
        ota_screen_set_status_error(line);
        /* Hold the failure on screen long enough for the user to read it.
         * No retry loop — a fresh boot will pick this up again. */
        vTaskDelay(pdMS_TO_TICKS(8000));
        ota_screen_hide();
        bt_link_set_quiet(false);
        return flash_err;
    }

    /* Verify the new firmware came up with the expected version. */
    ota_screen_set_status("Waiting for agent to boot...");
    const char *post = bt_agent_wait_version(CONFIG_BT_AGENT_VERSION_TIMEOUT_MS);
    if (post && strcmp(post, expected) == 0) {
        ESP_LOGI(TAG, "OTA verified, agent ver=%s", post);
        ota_screen_set_status("Done");
        vTaskDelay(pdMS_TO_TICKS(1000));
        ota_screen_hide();
        bt_link_set_quiet(false);
        return ESP_OK;
    }
    ESP_LOGE(TAG, "OTA wrote but agent reports %s (expected %s)",
             post ? post : "<none>", expected);
    char line[96];
    snprintf(line, sizeof(line), "Wrote OK but ver=%s", post ? post : "<none>");
    ota_screen_set_status_error(line);
    vTaskDelay(pdMS_TO_TICKS(8000));
    ota_screen_hide();
    bt_link_set_quiet(false);
    return ESP_FAIL;
}

#endif  /* CONFIG_BT_AGENT_OTA_ENABLED */
