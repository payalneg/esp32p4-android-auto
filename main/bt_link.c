#include "bt_link.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_PORT      UART_NUM_1
/* Swapped from the original 21/22 plan — bench wiring puts P4's TX line
 * on GPIO22 and RX on GPIO21. */
#define UART_TX_PIN    GPIO_NUM_22
#define UART_RX_PIN    GPIO_NUM_21
#define UART_BAUD      115200

/* Heartbeat lines (BT-HB:tick=...) prove the UART link is alive. Once
 * the link is known good they're just noise on the console — drop them
 * silently by default. Flip to 1 to bring them back when debugging. */
#define BT_LINK_LOG_HEARTBEAT 0

/* Forward declarations of state shared between rx_task (consumes acks) and
 * wifi_resend_task (stops once acked). Definitions are further down. */
static volatile bool s_wifi_acked;

static const char *TAG = "bt_link";

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
                        printf("[BT] %s\n", line + 7);
                    } else if (strncmp(line, "BT-HB:", 6) == 0) {
#if BT_LINK_LOG_HEARTBEAT
                        ESP_LOGI(TAG, "[heartbeat] %s", line + 6);
#endif
                        /* else: silently drop, see BT_LINK_LOG_HEARTBEAT. */
                    } else if (strncmp(line, "BT:", 3) == 0) {
                        const char *evt = line + 3;
                        if (strcmp(evt, "WIFI_RECEIVED") == 0) {
                            s_wifi_acked = true;  /* stop resend loop */
                        }
                        ESP_LOGI(TAG, "agent event %s", evt);
                    } else {
                        /* Anything else — most likely an ESP_LOG line that
                         * skipped our tee (vprintf hook didn't catch it).
                         * Treat as a log so the user still sees it. */
                        printf("[BT] %s\n", line);
                    }
                    pos = 0;
                }
            } else if (pos < sizeof(line) - 1) {
                line[pos++] = c;
            } else {
                /* Overlong line — flush partial and reset. */
                line[pos] = '\0';
                printf("[BT?] %s...<truncated>\n", line);
                pos = 0;
            }
        }
    }
}

void bt_link_init(void)
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
    /* Mirror everything the BT agent sends on its side. See rx_task() for
     * how the inbound prefix protocol is decoded. */
    xTaskCreate(rx_task, "bt_link_rx", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "UART1 to BT agent ready (TX=%d, RX=%d, %d baud)",
             UART_TX_PIN, UART_RX_PIN, UART_BAUD);
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
        xTaskCreate(wifi_resend_task, "bt_wifi_resend", 2048, NULL, 4, NULL);
    }
}
