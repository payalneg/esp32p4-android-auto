#include "uart_link.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

/* UART0 on the standard GPIO 1 (TX) / GPIO 3 (RX) pins. UART2 on this
 * particular D1 Mini clone wasn't reliably driving the GPIO16/17 lines —
 * some clones route those pins to the CH9102 auto-program circuit and
 * the level shifters interfere with our link to P4.
 *
 * Using UART0 means the on-board USB-C/CH9102 debug bridge fights for the
 * same lines whenever the USB-C cable is plugged in. Flash workflow is:
 *   1. unplug the wires going to P4
 *   2. plug USB-C, idf.py flash
 *   3. unplug USB-C, plug wires to P4
 * D1 Mini's ESP_LOG output also goes here by default, so the P4 sees
 * everything we log without any extra glue. */
#define UART_PORT      UART_NUM_0
#define UART_TX_PIN    GPIO_NUM_1
#define UART_RX_PIN    GPIO_NUM_3
#define UART_BAUD      115200

static const char *TAG = "uart_link";

static uart_link_wifi_t s_wifi;
static volatile bool    s_wifi_have;

/* Top-level mode handshake. P4 sends "MODE|AVRCP" or "MODE|AA" on its boot
 * right after the UART is up. main.c blocks on uart_link_wait_mode() until
 * the line lands (or until the timeout, in which case we default to AA so
 * existing pre-AVRCP boards keep working). */
static volatile uart_link_mode_t s_mode = UART_LINK_MODE_NONE;
static EventGroupHandle_t        s_mode_ev;
#define MODE_EV_BIT  (1 << 0)

bool uart_link_have_wifi(void) { return s_wifi_have; }

const uart_link_wifi_t *uart_link_get_wifi(void)
{
    return s_wifi_have ? &s_wifi : NULL;
}

/* Parse one received line.
 *   WIFI|<ssid>|<password>|<bssid>|<ip>|<port>
 * Pipes split fields. Trailing newline already stripped by caller.
 * Stores into s_wifi on success and sets s_wifi_have. */
static void parse_line(char *line)
{
    if (strncmp(line, "MODE|", 5) == 0) {
        const char *m = line + 5;
        uart_link_mode_t new_mode;
        if (strcmp(m, "AVRCP") == 0) {
            new_mode = UART_LINK_MODE_AVRCP;
        } else if (strcmp(m, "AA") == 0) {
            new_mode = UART_LINK_MODE_AA;
        } else {
            ESP_LOGW(TAG, "MODE: unknown value '%s'", m);
            return;
        }
        if (s_mode == UART_LINK_MODE_NONE) {
            s_mode = new_mode;
            if (s_mode_ev) xEventGroupSetBits(s_mode_ev, MODE_EV_BIT);
            ESP_LOGI(TAG, "MODE locked to %s", m);
        } else if (s_mode != new_mode) {
            /* P4 re-sent a different mode after we already committed —
             * the agent stack is already up; ignore and let the next
             * reboot apply the new choice. */
            ESP_LOGW(TAG, "MODE change to %s ignored (already %s)", m,
                     s_mode == UART_LINK_MODE_AVRCP ? "AVRCP" : "AA");
        }
        return;
    }

    if (strncmp(line, "WIFI|", 5) != 0) {
        ESP_LOGW(TAG, "ignoring unknown line: %.40s", line);
        return;
    }

    char *fields[5] = {0};   /* ssid, pass, bssid, ip, port */
    char *p = line + 5;
    int  n = 0;
    fields[n++] = p;
    while (n < 5 && *p) {
        if (*p == '|') {
            *p = '\0';
            fields[n++] = p + 1;
        }
        p++;
    }
    if (n != 5 || !fields[4][0]) {
        ESP_LOGW(TAG, "WIFI line malformed: only %d fields", n);
        return;
    }

    strlcpy(s_wifi.ssid,     fields[0], sizeof(s_wifi.ssid));
    strlcpy(s_wifi.password, fields[1], sizeof(s_wifi.password));
    strlcpy(s_wifi.bssid,    fields[2], sizeof(s_wifi.bssid));
    strlcpy(s_wifi.ip,       fields[3], sizeof(s_wifi.ip));
    s_wifi.port = atoi(fields[4]);
    s_wifi_have = true;

    /* Avoid logging the password in clear text; mask middle bytes. */
    ESP_LOGI(TAG, "got WiFi from P4: ssid='%s' bssid=%s ip=%s port=%d",
             s_wifi.ssid, s_wifi.bssid, s_wifi.ip, s_wifi.port);

    /* Ack so P4 stops the resend loop. */
    uart_link_say("BT:WIFI_RECEIVED");
}

/* No log tee — UART0 already carries ESP_LOG output via the IDF console
 * pipeline, so anything we'd write through a vprintf hook would just
 * double up on the wire. P4 receives the raw log lines and just prints
 * them with `[BT]` prefix on its own console. */

/* Heartbeat task — every second push a recognizable line out so the P4 can
 * confirm the wires/baud/pins are good before we get into BT debugging. */
static void heartbeat_task(void *arg)
{
    uint32_t n = 0;
    while (1) {
        char line[64];
        int  len = snprintf(line, sizeof(line),
                            "BT-HB:tick=%lu uptime_ms=%lu\n",
                            (unsigned long)n,
                            (unsigned long)(xTaskGetTickCount() *
                                            portTICK_PERIOD_MS));
        if (len > 0) {
            uart_write_bytes(UART_PORT, line, len);
        }
        n++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Drain UART, accumulate bytes into a line buffer, hand off complete
 * lines (terminated by \n or \r\n) to parse_line. Lines longer than
 * the buffer are dropped with a warning — P4 only sends ~160 bytes. */
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
                    parse_line(line);
                    pos = 0;
                }
            } else if (pos < sizeof(line) - 1) {
                line[pos++] = c;
            } else {
                ESP_LOGW(TAG, "rx line overflow, dropping");
                pos = 0;
            }
        }
    }
}

uart_link_mode_t uart_link_wait_mode(uint32_t timeout_ms)
{
    if (!s_mode_ev) {
        ESP_LOGW(TAG, "wait_mode called before init — defaulting to AA");
        return UART_LINK_MODE_AA;
    }
    EventBits_t bits = xEventGroupWaitBits(s_mode_ev, MODE_EV_BIT,
                                           pdFALSE, pdTRUE,
                                           pdMS_TO_TICKS(timeout_ms));
    if (!(bits & MODE_EV_BIT)) {
        ESP_LOGW(TAG, "no MODE from P4 within %u ms — defaulting to AA",
                 (unsigned)timeout_ms);
        return UART_LINK_MODE_AA;
    }
    return s_mode;
}

/* Replace any '|' in `src` with a space when copying — keeps the META line
 * format unambiguous without needing escape parsing on P4. Truncates to
 * `dstsize - 1` chars. Returns chars written (excl. NUL). */
static size_t sanitize_field(char *dst, size_t dstsize, const char *src)
{
    if (!dst || dstsize == 0) return 0;
    size_t n = 0;
    if (!src) { dst[0] = '\0'; return 0; }
    while (src[n] && n < dstsize - 1) {
        char c = src[n];
        dst[n] = (c == '|') ? ' ' : c;
        n++;
    }
    dst[n] = '\0';
    return n;
}

void uart_link_send_meta(const char *title, const char *artist, const char *album)
{
    char t[160], a[160], al[160];
    sanitize_field(t,  sizeof(t),  title);
    sanitize_field(a,  sizeof(a),  artist);
    sanitize_field(al, sizeof(al), album);
    char line[520];
    int n = snprintf(line, sizeof(line), "META|%s|%s|%s\n", t, a, al);
    if (n <= 0 || n >= (int)sizeof(line)) {
        ESP_LOGW(TAG, "send_meta: line too long, dropped");
        return;
    }
    uart_write_bytes(UART_PORT, line, (size_t)n);
}

void uart_link_send_state(bool playing)
{
    const char *s = playing ? "STATE|playing\n" : "STATE|paused\n";
    uart_write_bytes(UART_PORT, s, strlen(s));
}

void uart_link_init(void)
{
    if (!s_mode_ev) s_mode_ev = xEventGroupCreate();

    /* On UART0 the IDF console may already have configured baud/pins.
     * Install the IDF UART driver to get an RX ring buffer (we need to
     * read P4's WIFI line). The console's printf path is unaffected — it
     * still writes via esp_rom_printf, which works whether or not the
     * driver is installed. */
    if (!uart_is_driver_installed(UART_PORT)) {
        ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 2048, 2048, 0, NULL, 0));
    }
    /* Re-pin to the standard UART0 pads. uart_set_pin tolerates being
     * called more than once. */
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(rx_task, "uart_rx", 4096, NULL, 5, NULL);
    xTaskCreate(heartbeat_task, "uart_hb", 2048, NULL, 4, NULL);

    ESP_LOGI(TAG, "UART0 ready (TX=%d, RX=%d, %d baud)",
             UART_TX_PIN, UART_RX_PIN, UART_BAUD);
}

void uart_link_say(const char *line)
{
    size_t len = strlen(line);
    uart_write_bytes(UART_PORT, line, len);
    uart_write_bytes(UART_PORT, "\n", 1);
}
