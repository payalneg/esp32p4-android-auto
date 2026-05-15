#include "bt_link.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"


#define UART_PORT      ((uart_port_t)BT_AGENT_UART_PORT)
#define UART_TX_PIN    BT_AGENT_UART_TX
#define UART_RX_PIN    BT_AGENT_UART_RX
#define UART_BAUD      115200

static const char *TAG = "bt_link";

/* Both RST and IO0 are push-pull: this board has no external pull-ups on
 * either line, so an open-drain release would leave the ESP32 reading
 * floating values — often low, which means stuck in download mode (IO0)
 * or held in reset (RST). Driving the rails actively makes the strap state
 * deterministic. The CH2104 USB-UART is unplugged in production; during
 * dev with USB attached, keep the agent's USB cable disconnected while the
 * P4 drives these lines to avoid driver contention. */
static void bt_agent_strap_init(void)
{
    static bool inited;
    if (inited) return;

    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BT_AGENT_RST_PIN) |
                        (1ULL << BT_AGENT_IO0_PIN),
        .mode         = GPIO_MODE_OUTPUT,           /* push-pull */
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    /* Idle: both actively driven high — IO0 high = normal-boot strap,
     * RST high = chip running. */
    gpio_set_level(BT_AGENT_IO0_PIN, 1);
    gpio_set_level(BT_AGENT_RST_PIN, 1);
    inited = true;
}

void bt_agent_reset_to_app(void)
{
    bt_agent_strap_init();
    /* Release IO0 first so the ESP32 reads HIGH on the next reset edge. */
    gpio_set_level(BT_AGENT_IO0_PIN, 1);
    /* Drive RST low for 50 ms (datasheet says ~10 ms is enough, give margin). */
    gpio_set_level(BT_AGENT_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(BT_AGENT_RST_PIN, 1);
    /* ROM bootloader needs ~100-200 ms to validate the app image and jump
     * into it. Wait so the BT agent's first ESP_LOG line lands after we've
     * finished UART setup, not during it. */
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "BT agent reset (IO0 high → normal boot)");
}

void bt_agent_enter_bootloader(void)
{
    bt_agent_strap_init();
    /* Hold IO0 low BEFORE pulling RST so the rising edge of EN sees IO0=0. */
    gpio_set_level(BT_AGENT_IO0_PIN, 0);
    gpio_set_level(BT_AGENT_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(BT_AGENT_RST_PIN, 1);
    /* ROM samples strap pins right after EN goes high. ~50 ms is plenty
     * of margin (datasheet specifies 1 ms). */
    vTaskDelay(pdMS_TO_TICKS(50));
    /* Release IO0 — ROM has already latched its boot mode by now. Leaving
     * it driven low would also work, but releasing lets the agent's own
     * code use IO0 for whatever it likes once the OTA is done. */
    gpio_set_level(BT_AGENT_IO0_PIN, 1);
    ESP_LOGI(TAG, "BT agent forced into ROM bootloader (IO0 low @ reset)");
}

/* Heartbeat lines (BT-HB:tick=...) prove the UART link is alive. Once
 * the link is known good they're just noise on the console — drop them
 * silently by default. Flip to 1 to bring them back when debugging. */
#define BT_LINK_LOG_HEARTBEAT 0

/* Forward declarations of state shared between rx_task (consumes acks) and
 * wifi_resend_task (stops once acked). Definitions are further down. */
static volatile bool s_wifi_acked;

/* When true, rx_task drops all forwarding (printf/ESP_LOG) but keeps
 * parsing for BT-VER:. Toggled by bt_link_set_quiet() during OTA flow
 * to avoid hammering the console while the agent boot-loops. */
static volatile bool s_quiet;

void bt_link_set_quiet(bool quiet)
{
    s_quiet = quiet;
}

/* Last `BT-VER:<version>` value parsed from the agent's UART stream.
 * 64 bytes is plenty for semver + build tag + git short hash. The event
 * group below pulses VERSION_BIT when a new value lands so callers can
 * wait without polling. */
#define VERSION_BUF_LEN  64
static char         s_agent_ver[VERSION_BUF_LEN];
static EventGroupHandle_t s_agent_ev;
#define AGENT_EV_VERSION_BIT  (1 << 0)

const char *bt_agent_get_version(void)
{
    return s_agent_ver[0] ? s_agent_ver : NULL;
}

const char *bt_agent_wait_version(uint32_t timeout_ms)
{
    if (!s_agent_ev) return NULL;
    EventBits_t bits = xEventGroupWaitBits(s_agent_ev, AGENT_EV_VERSION_BIT,
                                           pdFALSE, pdTRUE,
                                           pdMS_TO_TICKS(timeout_ms));
    return (bits & AGENT_EV_VERSION_BIT) ? s_agent_ver : NULL;
}

/* RX task: reads everything the BT agent sends us and prints it on this
 * board's console. Lines are categorised by prefix:
 *   BT-LOG:<...>  → forwarded as `[BT]<...>` so all BT-agent ESP_LOG output
 *                   shows up in the P4's USB-C capture (no need to USB-cable
 *                   the D1 Mini separately for monitor).
 *   BT:<event>    → state events (READY, PAIRED, CONNECTED, …) — printed
 *                   with a clearer prefix so they stand out.
 *   anything else → printed verbatim with `[BT?] ` prefix. */
static void rx_task(void *arg)
{
    char line[256];
    size_t pos = 0;
    while (1) {
        uint8_t buf[64];
        int n = uart_read_bytes(UART_PORT, buf, sizeof(buf), pdMS_TO_TICKS(200));
        if (n <= 0) continue;
        for (int i = 0; i < n; i++) {
            char c = (char)buf[i];
            if (c == '\n' || c == '\r') {
                if (pos > 0) {
                    line[pos] = '\0';
                    if (strncmp(line, "BT-LOG:", 7) == 0) {
                        if (!s_quiet) printf("[BT] %s\n", line + 7);
                    } else if (strncmp(line, "BT-HB:", 6) == 0) {
#if BT_LINK_LOG_HEARTBEAT
                        if (!s_quiet) ESP_LOGI(TAG, "[heartbeat] %s", line + 6);
#endif
                        /* else: silently drop, see BT_LINK_LOG_HEARTBEAT. */
                    } else if (strncmp(line, "BT:", 3) == 0) {
                        const char *evt = line + 3;
                        if (strcmp(evt, "WIFI_RECEIVED") == 0) {
                            s_wifi_acked = true;  /* stop resend loop */
                        }
                        if (!s_quiet) ESP_LOGI(TAG, "agent event %s", evt);
                    } else if (strncmp(line, "BT-VER:", 7) == 0) {
                        /* Always parse — wait_version() depends on this even
                         * when forwarding is muted. */
                        strncpy(s_agent_ver, line + 7, sizeof(s_agent_ver) - 1);
                        s_agent_ver[sizeof(s_agent_ver) - 1] = '\0';
                        if (s_agent_ev) {
                            xEventGroupSetBits(s_agent_ev, AGENT_EV_VERSION_BIT);
                        }
                        if (!s_quiet) ESP_LOGI(TAG, "agent version: %s", s_agent_ver);
                    } else {
                        /* Anything else — most likely an ESP_LOG line that
                         * skipped our tee (vprintf hook didn't catch it).
                         * Treat as a log so the user still sees it. */
                        if (!s_quiet) printf("[BT] %s\n", line);
                    }
                    pos = 0;
                }
            } else if (pos < sizeof(line) - 1) {
                line[pos++] = c;
            } else {
                /* Overlong line — flush partial and reset. */
                line[pos] = '\0';
                if (!s_quiet) printf("[BT?] %s...<truncated>\n", line);
                pos = 0;
            }
        }
    }
}

/* Pull these out so suspend/resume can re-run the same setup without
 * duplicating the config struct. */
static TaskHandle_t s_rx_task;

static void bt_link_uart_install(void)
{
    const uart_config_t cfg = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    /* 1 KiB rings each way — plenty for short setup messages. */
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 1024, 1024, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void bt_link_init(void)
{
    if (!s_agent_ev) s_agent_ev = xEventGroupCreate();

    /* Force the agent into normal-boot mode before bringing UART up.
     * The D1 Mini sometimes powers up with IO0 held low (CH2104 transient
     * + missing pull-up depending on the board revision) and stays stuck
     * in download mode (boot:0x3) — explicit IO0=high + RST pulse from
     * here guarantees we always start with the user app. */
    bt_agent_reset_to_app();

    bt_link_uart_install();
    /* Mirror everything the BT agent sends on its side. See rx_task() for
     * how the inbound prefix protocol is decoded. */
    xTaskCreatePinnedToCore(rx_task, "bt_link_rx", 4096, NULL, 5, &s_rx_task, 0);

    ESP_LOGI(TAG, "UART1 to BT agent ready (TX=%d, RX=%d, %d baud)",
             UART_TX_PIN, UART_RX_PIN, UART_BAUD);
}

void bt_link_suspend_for_flash(void)
{
    if (s_rx_task) {
        vTaskDelete(s_rx_task);
        s_rx_task = NULL;
    }
    /* uart_driver_delete is safe even if rx_task was mid-read — its blocking
     * read returns ESP_FAIL and we've already dropped the task. */
    uart_driver_delete(UART_PORT);
    ESP_LOGI(TAG, "UART1 released for flasher");
}

void bt_link_resume_after_flash(void)
{
    /* esp_serial_flasher leaves IO0 released after esp_loader_flash_finish,
     * but to be safe we explicitly reset back to app mode here too. */
    bt_agent_reset_to_app();
    bt_link_uart_install();
    /* Clear any version observed before flash — agent app may emit a new
     * BT-VER: that should be visible to a follow-up wait_version call. */
    s_agent_ver[0] = '\0';
    if (s_agent_ev) xEventGroupClearBits(s_agent_ev, AGENT_EV_VERSION_BIT);
    xTaskCreatePinnedToCore(rx_task, "bt_link_rx", 4096, NULL, 5, &s_rx_task, 0);
    ESP_LOGI(TAG, "UART1 reattached after flasher");
}

/* Pre-built WIFI line, kept around so the resend task can re-emit it. */
static char            s_wifi_line[256];
static size_t          s_wifi_len;
static char            s_wifi_log_summary[160];

static void wifi_resend_task(void *arg)
{
    /* Burst the first few sends close together to cover D1 Mini's boot
     * latency, then back off. Stops as soon as the BT agent acks. */
    int delay_ms = 500;
    while (s_wifi_len > 0 && !s_wifi_acked) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        if (s_wifi_acked) break;
        uart_write_bytes(UART_PORT, s_wifi_line, s_wifi_len);
        if (delay_ms < 5000) delay_ms *= 2;  /* 0.5 → 1 → 2 → 4 → 5 s */
    }
    vTaskDelete(NULL);
}

void bt_link_publish_wifi(const char *ssid, const char *password,
                          const char *bssid, const char *ip, int port)
{
    int n = snprintf(s_wifi_line, sizeof(s_wifi_line),
                     "WIFI|%s|%s|%s|%s|%d\n",
                     ssid, password, bssid, ip, port);
    if (n <= 0 || n >= (int)sizeof(s_wifi_line)) {
        ESP_LOGE(TAG, "publish_wifi: line too long (%d)", n);
        return;
    }
    s_wifi_len = (size_t)n;
    snprintf(s_wifi_log_summary, sizeof(s_wifi_log_summary),
             "WIFI|%s|***|%s|%s|%d", ssid, bssid, ip, port);

    /* First send right now (covers the case where D1 Mini was already up). */
    uart_write_bytes(UART_PORT, s_wifi_line, s_wifi_len);
    ESP_LOGI(TAG, "→ BT agent: %s", s_wifi_log_summary);

    /* Background resender for cases where D1 Mini boots later, missed the
     * first packet, or had a flaky wire. Cheap — just one task slot. */
    static bool task_started;
    if (!task_started) {
        task_started = true;
        xTaskCreatePinnedToCore(wifi_resend_task, "bt_wifi_resend", 2048, NULL, 4, NULL, 0);
    }
}

