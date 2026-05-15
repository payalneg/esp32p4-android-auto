#include <stdio.h>

#include "bsp/esp-bsp.h"
#include "esp_heap_caps.h"
#include "esp_lv_adapter_input.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_rom_sys.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs_flash.h"

/* Constructor that runs LATE in do_global_ctors() (priority 65535 = lowest
 * among priority-tagged, but still before untagged constructors).
 * Prints free MALLOC_CAP_INTERNAL+8BIT heap to bracket who consumed it. */
__attribute__((constructor(65535)))
static void heap_probe_post_priority_ctors(void)
{
    size_t internal8 = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    esp_rom_printf("HEAP_PROBE A (post-priority-ctors) free=%u largest=%u\n",
                   (unsigned)internal8, (unsigned)largest);
}

/* port_start_app_hook is a weak symbol declared in
 * components/freertos/app_startup.c, called from esp_startup_start_app()
 * AFTER xTaskCreatePinnedToCore(main_task) but BEFORE vTaskStartScheduler.
 * That's the last point where we can measure heap before IDLE allocs. */
void port_start_app_hook(void)
{
    size_t internal8 = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    esp_rom_printf("HEAP_PROBE B (pre-scheduler) free=%u largest=%u\n",
                   (unsigned)internal8, (unsigned)largest);
}

#include "aa_overclock.h"
#include "ble_host.h"
#include "ble_nus.h"
#include "bt_agent_ota.h"
#include "bt_link.h"
#include "c6_ota.h"
#include "config.h"
#include "dev_settings.h"
#include "display_init.h"
#include "display_video.h"
#include "h264_pipe.h"
#include "idle_screen.h"
#include "mdns_advertise.h"
#include "now_playing.h"
#include "gui_guider.h"   /* guider_ui — for dashboard_song_title_label */
#include "ota_http.h"
#include "ota_screen.h"
#include "tcp_server.h"
#include "touch_input.h"
#include "ui_mode.h"
#include "vesc_can/comm_can.h"
#include "vesc_battery_calc.h"
#include "vesc_can/vesc_lisp_poll.h"
#include "vesc_can/vesc_rt_data.h"
#include "vesc_sim.h"
#include "vesc_trip_persist.h"
#include "vesc_ui_updater.h"
#include "wifi_manager.h"

static const char *TAG = "main";

/* ---- Custom LVGL touch indev fed by touch_input.c ----
 *
 * BSP auto-installs its own LVGL touch indev that reads GT911 directly. We
 * unregister it and create our own pointer-typed indev that pulls from
 * touch_input's shared state, so:
 *   - touch_input is the single GT911 reader (no I2C race),
 *   - in TOUCH_MODE_AA the indev sees pressed=false (LVGL stops getting
 *     events even though polling continues),
 *   - in TOUCH_MODE_LVGL touches flow into the dashboard normally.
 *
 * LVGL 8.3 indev API: lv_indev_drv_init + lv_indev_drv_register
 * (lv_indev_create / lv_indev_set_* arrived in LVGL 9). */
static lv_indev_drv_t s_lvgl_touch_drv;

static void lvgl_touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    bool pressed = false;
    uint16_t x = 0, y = 0;
    touch_input_lvgl_read(&x, &y, &pressed);
    data->state   = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = x;
    data->point.y = y;
}

static void install_lvgl_touch_indev(void)
{
    /* Drop BSP's auto-installed touch indev (it would race our reader on
     * the same GT911 over I2C). */
    lv_indev_t *bsp_indev = bsp_display_get_input_dev();
    if (bsp_indev) {
        if (esp_lv_adapter_unregister_touch(bsp_indev) == ESP_OK) {
            ESP_LOGI(TAG, "BSP touch indev unregistered");
        } else {
            ESP_LOGW(TAG, "BSP touch indev unregister failed");
        }
    }

    /* lv_indev_drv_register touches LVGL globals — hold the BSP lock. */
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGW(TAG, "lvgl indev: bsp_display_lock failed — skipping");
        return;
    }
    lv_indev_drv_init(&s_lvgl_touch_drv);
    s_lvgl_touch_drv.type    = LV_INDEV_TYPE_POINTER;
    s_lvgl_touch_drv.read_cb = lvgl_touch_read_cb;
    lv_indev_t *indev = lv_indev_drv_register(&s_lvgl_touch_drv);
    bsp_display_unlock();
    if (indev) {
        ESP_LOGI(TAG, "custom LVGL touch indev registered");
    } else {
        ESP_LOGW(TAG, "custom LVGL touch indev registration failed");
    }
}

/* Re-assembled VESC packets land here. Forwards to the RT-data parser,
 * (if enabled) the LISP poll parser, and the BLE NUS bridge so VESC Tool
 * over BLE sees CAN responses. All three filter / gate on their own state
 * (RT/LISP on the leading COMM_PACKET_ID byte, NUS on connection state)
 * so the fan-out is unconditional. */
static void vesc_packet_dispatch(const uint8_t *data, unsigned int len)
{
    vesc_rt_data_process_response(data, len);
#if CONFIG_VESC_CAN_LISP_POLL_ENABLE
    vesc_lisp_poll_process_response(data, len);
#endif
    ble_nus_forward_response(data, (uint16_t)len);
}

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

/* settings_set_can_speed → here. Re-arms the TWAI driver with the new
 * baud rate, keeping the current controller_id from settings. */
static void on_can_speed_changed(int new_kbps)
{
    if (comm_can_reinit(settings_get_controller_id(), new_kbps) != ESP_OK) {
        ESP_LOGW(TAG, "comm_can_reinit(%d kbps) failed", new_kbps);
    }
}

/* settings_set_screen_brightness → here. bsp_display_brightness_set is
 * a thin LEDC duty wrapper, safe to call from the UI task. */
static void on_brightness_changed(uint8_t pct)
{
    bsp_display_brightness_set(pct);
}

/* settings_set_controller_id → here. Reinit TWAI so STATUS_* frames go
 * out under the new ID. Speed comes back from settings (single source). */
static void on_controller_id_changed(uint8_t new_id)
{
    if (comm_can_reinit(new_id, (int)settings_get_can_speed()) != ESP_OK) {
        ESP_LOGW(TAG, "comm_can_reinit(ctrl=%u) failed", new_id);
    }
}

/* settings_set_target_vesc_id → here. Both pollers store the target ID
 * statically; vesc_*_init is documented as safe to call repeatedly so
 * we just re-init them and the next poll cycle hits the new node. */
static void on_target_id_changed(uint8_t new_id)
{
    vesc_rt_data_init(new_id, CONFIG_VESC_CAN_RT_INTERVAL_MS);
#if CONFIG_VESC_CAN_LISP_POLL_ENABLE
    vesc_lisp_poll_init(new_id, CONFIG_VESC_CAN_LISP_INTERVAL_MS);
#endif
    ESP_LOGI(TAG, "VESC target ID → %u", new_id);
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-P4 Android Auto boot, aa_submode=%d", CONNECTION_MODE);
    ESP_LOGI(TAG, "HEAP_PROBE: app_main INTERNAL+8BIT free=%u largest=%u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

    init_nvs();
    /* Settings cache is now ready for both the UI (settings_ui_init pulls
     * via settings_wrapper_init → settings_init) and our callback wiring
     * below. settings_init is idempotent. */
    settings_init();

    /* Smart battery calc needs NVS open before its first percentage call.
     * settings_init's nvs_flash_init has already run; battery_calc keeps its
     * own namespace ("battery_calc") for the remaining-Ah counter. */
    battery_calc_init();
    /* trip_persist tracks trip / Ah / uptime across VESC reboots — same NVS
     * pattern as battery_calc, separate namespace. The dashboard reset icon
     * eventually flows here via battery_calc_reset_trip_and_ah(). */
    trip_persist_init();

    /* Top-level connection mode picked by the user in Settings. Read once
     * here and reused throughout app_main — the value is stable for the
     * lifetime of the boot (changing it in the UI triggers esp_restart). */
    const connection_mode_t conn_mode = settings_get_connection_mode();
    ESP_LOGI(TAG, "connection_mode = %s",
             conn_mode == CONN_AVRCP        ? "AVRCP"        :
             conn_mode == CONN_ANDROID_AUTO ? "ANDROID_AUTO" :
             conn_mode == CONN_CARPLAY      ? "CARPLAY"      : "?");

    /* Bump CPU to 400 MHz before any peripheral / WiFi init so APB ratio
     * stays consistent. No-op unless CONFIG_AA_OVERCLOCK_400 is set. */
    aa_overclock_400mhz_apply();

    if (display_init() != ESP_OK) {
        ESP_LOGW(TAG, "display init failed — UI disabled");
    }
    /* display_init lights the backlight at 100% — apply the saved brightness
     * once the panel is up so user preference takes hold before any UI
     * draws. Brightness callback persists future changes from the slider. */
    bsp_display_brightness_set(settings_get_screen_brightness());
    settings_register_brightness_cb(on_brightness_changed);
    /* idle first, ota second so the OTA overlay sits on top in z-order
     * (children of lv_scr_act() are stacked in creation order). */
    idle_screen_init();
    ota_screen_init();

    /* Replace BSP's auto-installed LVGL touch indev with our own that reads
     * from touch_input's shared state. Single GT911 reader for both AA and
     * VESC modes — must run before ui_mode_init so the dashboard sees touch
     * from frame 1. */
    install_lvgl_touch_indev();

    /* Build the VESC dashboard offscreen and arm the 3-finger gesture
     * (works in both AA and VESC modes, even before phone connects).
     *
     * super_vesc_ui_init() walks ~1700 lines of GUI Guider widget creation
     * (~750 lv_obj_set_style_* calls) under the BSP lock on prio-1 main task
     * → IDLE0 starves for ~5 s and CONFIG_ESP_TASK_WDT_TIMEOUT_S=5 fires.
     * Detach IDLE0 from TWDT just for this one-shot init. */
    TaskHandle_t idle0 = xTaskGetIdleTaskHandleForCore(0);
    bool wdt_paused = (idle0 && esp_task_wdt_delete(idle0) == ESP_OK);
    esp_err_t ui_err = ui_mode_init();
    if (wdt_paused) {
        esp_task_wdt_add(idle0);
    }
    if (ui_err == ESP_OK) {
        /* 3-finger gesture toggles between VESC dashboard and AA projection.
         * Only meaningful when the AA stack is up and an AA screen exists —
         * in AVRCP/CarPlay there's nothing to switch to, so leave the
         * gesture callback NULL and the swipe is silently ignored. The GT911
         * polling task still starts so LVGL touch keeps working. */
        if (conn_mode == CONN_ANDROID_AUTO) {
            touch_input_set_gesture_cb(ui_mode_toggle);
        }
        touch_input_start(NULL, NULL);
    }

    /* VESC CAN bring-up. Independent from the AA pipeline — runs the
     * second the dashboard is alive so RT data starts streaming even
     * before WiFi is up. The decode-side handler routes reassembled
     * VESC packets to vesc_rt_data (and vesc_lisp_poll if enabled). */
    int     can_kbps = (int)settings_get_can_speed();
    uint8_t ctrl_id  = settings_get_controller_id();
    uint8_t tgt_id   = settings_get_target_vesc_id();

    if (settings_get_vesc_emulator()) {
        /* Synthetic source — runs a scripted drive cycle and injects into
         * vesc_rt_data. No CAN driver, no real polling. */
        vesc_rt_data_init(tgt_id, CONFIG_VESC_CAN_RT_INTERVAL_MS);
        vesc_sim_start();
        ESP_LOGW(TAG, "VESC EMULATOR active — no real CAN");
        vesc_ui_updater_start();
    } else if (comm_can_start(CONFIG_VESC_CAN_TX_GPIO, CONFIG_VESC_CAN_RX_GPIO,
                              ctrl_id, can_kbps) == ESP_OK) {
        vesc_rt_data_init(tgt_id, CONFIG_VESC_CAN_RT_INTERVAL_MS);
#if CONFIG_VESC_CAN_LISP_POLL_ENABLE
        vesc_lisp_poll_init(tgt_id, CONFIG_VESC_CAN_LISP_INTERVAL_MS);
#endif
        comm_can_set_packet_handler(vesc_packet_dispatch);
        vesc_rt_data_start();
        vesc_rt_data_start_task();
#if CONFIG_VESC_CAN_LISP_POLL_ENABLE
        /* start() only flips the active flag. The pumping happens inside the
         * single CAN-polling task spawned by vesc_rt_data_start_task() above —
         * see rt_task() in vesc_rt_data.c. */
        vesc_lisp_poll_start();
#endif
        ESP_LOGI(TAG, "VESC CAN ready, polling target ID %u (own ID %u)",
                 tgt_id, ctrl_id);
        vesc_ui_updater_start();
        /* Hook the setters that drive the CAN bus to live reconfig.
         * Registered after the driver is up so callbacks can never run
         * before the first comm_can_start. */
        settings_register_can_speed_cb(on_can_speed_changed);
        settings_register_controller_id_cb(on_controller_id_changed);
        settings_register_target_id_cb(on_target_id_changed);
    } else {
        ESP_LOGW(TAG, "VESC CAN init failed — dashboard will show no data");
    }

#if CONFIG_C6_OTA_ENABLED
    c6_ota_status_t ota = c6_ota_check_and_update();
    if (ota == C6_OTA_STATUS_UPDATED) {
        ESP_LOGW(TAG, "C6 updated — restarting host to resync");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    } else if (ota == C6_OTA_STATUS_FAILED) {
        ESP_LOGE(TAG, "C6 OTA failed — proceeding with current slave fw");
    }
    /* If OTA showed itself, drop it now that we're past the update phase. */
    ota_screen_hide();
#endif

    /* NimBLE host on top of C6's BT controller (ESP-Hosted VHCI). Starts
     * advertising NUS so VESC Tool can connect over BLE and talk to the
     * VESC controller via the CAN bridge in vesc_packet_dispatch.
     *
     * ble_nus_init brings up the outbound ring buffer + TX task BEFORE
     * NimBLE so the first reply that lands during VESC Tool's handshake
     * already has somewhere to queue without back-pressuring the CAN
     * task. */
    ble_nus_init();
    if (ble_host_init() != ESP_OK) {
        ESP_LOGW(TAG, "BLE host init failed — VESC Tool over BLE unavailable");
    }

    if (conn_mode == CONN_CARPLAY) {
        /* CarPlay placeholder — no stack to bring up yet. Show the idle
         * screen with a "coming soon" message and let the rest of the
         * firmware (VESC dashboard, BLE NUS, CAN poller) keep running. */
        idle_screen_show("CarPlay", "Coming soon — not yet implemented");
        ESP_LOGI(TAG, "CarPlay mode — nothing else to start");
        return;
    }

    if (conn_mode == CONN_AVRCP) {
        /* AVRCP mode — WROOM does all Classic BT, P4 just renders now-
         * playing metadata. We don't need the AA stack, but we do bring up
         * the SoftAP solely so scripts/ota_push.sh can reach the HTTP OTA
         * endpoint at http://192.168.4.1/ota. WROOM-side WiFi creds are
         * NOT published — the agent stays in AVRCP profile and doesn't
         * try to push them into the AA Wireless setup channel. */
        now_playing_init();
        /* Bind the GUI Guider dashboard widget. Only title for now; pass
         * NULL for artist/album/play_icon — now_playing_set_track ignores
         * unbound fields. attach_labels is safe before bt_link is up. */
        now_playing_attach_labels(guider_ui.dashboard_song_title_label,
                                  NULL, NULL, NULL);
        bt_link_init();
        bt_agent_ota_check_and_update();
        bt_link_publish_mode(CONN_AVRCP);
        /* No idle screen — VESC dashboard is already on screen from
         * ui_mode_init above and stays there for the whole session. */
        idle_screen_hide();

        /* OTA-only WiFi: SoftAP up, no AA TCP server, no mDNS, no creds
         * to WROOM. ota_http_start runs even if AP_START hasn't fired
         * yet — httpd binds to 0.0.0.0:80, lwIP will start accepting as
         * soon as the AP netif has its address. */
        ESP_LOGI(TAG, "AVRCP: starting WiFi for OTA");
        esp_err_t werr = wifi_manager_start();
        if (werr != ESP_OK) {
            ESP_LOGE(TAG, "AVRCP: wifi_manager_start: %s",
                     esp_err_to_name(werr));
        } else {
            esp_err_t rerr = wifi_manager_wait_ready(10000);
            ESP_LOGI(TAG, "AVRCP: wifi_manager_wait_ready=%s",
                     esp_err_to_name(rerr));
        }
        esp_err_t herr = ota_http_start();
        ESP_LOGI(TAG, "AVRCP: ota_http_start=%s", esp_err_to_name(herr));

        ESP_LOGI(TAG, "AVRCP mode ready — waiting for phone to pair with the BT agent");
        return;
    }

    /* conn_mode == CONN_ANDROID_AUTO from here on. */
    idle_screen_show("Android Auto", "Initialising Wi-Fi...");

#if CONNECTION_MODE == MODE_WIRELESS_HELPER
    ESP_ERROR_CHECK(wifi_manager_start());
    if (wifi_manager_wait_ready(30000) != ESP_OK) {
        ESP_LOGE(TAG, "wifi setup failed, halting");
        idle_screen_show("Android Auto", "Wi-Fi setup failed");
        return;
    }

    const wifi_ap_info_t *ap = wifi_manager_get_ap_info();
    if (ap) {
        ESP_LOGI(TAG, "AP \"%s\" psk \"%s\" bssid %s ch %u",
                 ap->ssid, ap->password, ap->bssid_str, (unsigned)ap->channel);
    }

    ESP_ERROR_CHECK(mdns_advertise_start());
    ESP_ERROR_CHECK(tcp_server_start(AA_TCP_PORT));
    /* Plain HTTP OTA server — phone joins the SoftAP for AA anyway, so
     * scripts/ota_push.sh can hit http://<gw>/ota from a laptop on the
     * same AP. No-op when CONFIG_OTA_HTTP_ENABLED is unset. */
    ota_http_start();

    /* Display sink first — it captures the panel handle from BSP and waits
     * idle until first frame. Then the H.264 pipe; push() is a no-op until
     * the ring buffer is allocated, so it must be ready before the first
     * AVMediaIndication arrives. */
    if (display_video_init() != ESP_OK) {
        ESP_LOGW(TAG, "video sink failed — frames will be decoded but not shown");
    }
    if (h264_pipe_init() != ESP_OK) {
        ESP_LOGW(TAG, "H.264 decoder failed to start — video will be silent");
    }

    /* Hand off the AP credentials + our IP to the external D1 Mini ESP32
     * BT agent over UART1 (P4 GPIO 21/22 ↔ D1 Mini GPIO 16/17). The agent
     * uses these in the AA Wireless setup protocol so the phone joins our
     * SoftAP and connects back to TCP at the IP/port below.
     *
     * AP mode only — STA bench builds skip this since the dev's existing
     * laptop network already has its own credentials/topology. */
    if (ap) {
        /* bt_link_init drives BT_AGENT_RST/IO0 itself before bringing the
         * UART up — we no longer need to babysit the CH2104 auto-reset
         * transients with a blind delay here. */
        bt_link_init();
        /* If CONFIG_BT_AGENT_OTA_ENABLED, compare BT-VER: against expected
         * and reflash on mismatch / silence. No-op when disabled. */
        bt_agent_ota_check_and_update();
        /* Tell the agent we want the AA Wireless SPP path (not AVRCP).
         * Agent reads this once on boot and locks into one profile set. */
        bt_link_publish_mode(CONN_ANDROID_AUTO);
        esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        esp_netif_ip_info_t ap_ip = {0};
        if (ap_netif) esp_netif_get_ip_info(ap_netif, &ap_ip);
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ap_ip.ip));
        bt_link_publish_wifi(ap->ssid, ap->password, ap->bssid_str,
                             ip_str, AA_TCP_PORT);
    }

    /* Compose a one-line status with our IP for the idle screen.
     * AP mode shows the SSID; STA mode shows the joined network IP. */
    char status_line[80];
    esp_netif_ip_info_t ip_info = {0};
    if (ap) {
        esp_netif_t *n = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (n) esp_netif_get_ip_info(n, &ip_info);
        snprintf(status_line, sizeof(status_line),
                 "AP %s | %d.%d.%d.%d | port %d",
                 ap->ssid, IP2STR(&ip_info.ip), AA_TCP_PORT);
    } else {
        esp_netif_t *n = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (n) esp_netif_get_ip_info(n, &ip_info);
        snprintf(status_line, sizeof(status_line),
                 "%d.%d.%d.%d | port %d",
                 IP2STR(&ip_info.ip), AA_TCP_PORT);
    }
    idle_screen_show("Waiting for phone", status_line);

    ESP_LOGI(TAG, "head unit ready, waiting for Wireless Helper");
#elif CONNECTION_MODE == MODE_BT_CLASSIC
#error "MODE_BT_CLASSIC: not implemented yet (Stage 1 covers Mode B only)"
#else
#error "CONNECTION_MODE not set"
#endif
}
