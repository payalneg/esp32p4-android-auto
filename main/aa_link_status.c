#include "aa_link_status.h"

#include <string.h>

#include "bt_link.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "idle_screen.h"

static const char *TAG = "aa_link";

/* Paging + SPP + Wi-Fi join + TCP + handshake takes 5-15 s when it works;
 * gearhead's own retries (3 x 2 s TCP timeouts, then a WPP restart) fit
 * inside this too. Past it, nothing is coming. */
#define AA_LINK_CONNECTING_TIMEOUT_MS  40000

static aa_link_state_t    s_state = AA_LINK_DISCONNECTED;
static portMUX_TYPE       s_lock  = portMUX_INITIALIZER_UNLOCKED;
static esp_timer_handle_t s_timeout;

static const char *state_name(aa_link_state_t s)
{
    switch (s) {
    case AA_LINK_CONNECTING: return "Connecting";
    case AA_LINK_CONNECTED:  return "Connected";
    default:                 return "Disconnected";
    }
}

static idle_screen_state_t to_idle(aa_link_state_t s)
{
    switch (s) {
    case AA_LINK_CONNECTING: return IDLE_STATE_CONNECTING;
    case AA_LINK_CONNECTED:  return IDLE_STATE_CONNECTED;
    default:                 return IDLE_STATE_DISCONNECTED;
    }
}

aa_link_state_t aa_link_status_get(void)
{
    portENTER_CRITICAL(&s_lock);
    aa_link_state_t s = s_state;
    portEXIT_CRITICAL(&s_lock);
    return s;
}

void aa_link_status_set(aa_link_state_t state, const char *detail)
{
    portENTER_CRITICAL(&s_lock);
    s_state = state;
    portEXIT_CRITICAL(&s_lock);
    ESP_LOGI(TAG, "%s — %s", state_name(state), detail ? detail : "");
    idle_screen_set_state(to_idle(state), detail);
    if (s_timeout) {
        esp_timer_stop(s_timeout);
        if (state == AA_LINK_CONNECTING) {
            esp_timer_start_once(s_timeout,
                                 (uint64_t)AA_LINK_CONNECTING_TIMEOUT_MS * 1000);
        }
    }
}

/* A progress report moves Disconnected/Connecting forward; it never demotes
 * a live session (the agent's SPP/Wi-Fi chatter can trail the handshake). */
static void progress(const char *detail)
{
    if (aa_link_status_get() != AA_LINK_CONNECTED) {
        aa_link_status_set(AA_LINK_CONNECTING, detail);
    }
}

static void timeout_cb(void *arg)
{
    (void)arg;
    if (aa_link_status_get() == AA_LINK_CONNECTING) {
        aa_link_status_set(AA_LINK_DISCONNECTED,
                           "No answer from the phone. Tap Connect to retry");
    }
}

/* BT:<event> lines from the agent (see tools/bt_agent/main/uart_link.c). */
static void on_bt_event(const char *evt)
{
    if (strcmp(evt, "CONNECTED") == 0) {            /* phone opened AA Wireless SPP */
        progress("Phone found, starting Wi-Fi setup...");
    } else if (strcmp(evt, "WIFI_CONNECT_STATUS") == 0) {
        progress("Phone joining Wi-Fi...");
    } else if (strcmp(evt, "PAIRED") == 0) {
        progress("Paired, waiting for Android Auto...");
    }
    /* DISCONNECTED (SPP closed) is not a verdict: gearhead closes the channel
     * after a completed setup too. ON_AIR/OFF_AIR/BOOT/READY carry no news
     * about the phone. The Connecting timeout covers a setup that stalls. */
}

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)data;
    if (id == WIFI_EVENT_AP_STACONNECTED) {
        progress("Phone on Wi-Fi, waiting for Android Auto...");
    }
}

void aa_link_status_init(void)
{
    if (!s_timeout) {
        const esp_timer_create_args_t args = {
            .callback = timeout_cb,
            .name     = "aa_link_to",
        };
        if (esp_timer_create(&args, &s_timeout) != ESP_OK) {
            ESP_LOGW(TAG, "timeout timer create failed — Connecting may stick");
        }
    }
    bt_link_set_event_cb(on_bt_event);
    esp_err_t e = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED,
                                             on_wifi_event, NULL);
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "WIFI_EVENT handler: %s (STA join won't show)", esp_err_to_name(e));
    }
}
