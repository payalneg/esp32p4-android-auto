/* BT agent for ESP32-P4 Android Auto head unit.
 *
 * Pretends to be a Classic Bluetooth car kit (Audio Video / Car Audio device
 * class), accepts pairing from the phone, opens an SPP server on RFCOMM, and
 * runs the Android Auto Wireless Setup protocol (the same WifiStartRequest /
 * WifiInfoRequest / WifiInfoResponse exchange that the WirelessAndroidAuto
 * Dongle does). Once the phone has the WiFi credentials it connects to our
 * shared LAN and finds the P4 head unit by mDNS at port 5288.
 *
 * Why this exists: modern Android Auto (12.7+, "Cakewalk" wireless flow)
 * silently drops projection if the head unit was reached purely via mDNS
 * with no prior BT pairing. Going through the canonical BT-pairing handshake
 * makes the phone treat the HU as a trusted car and skips the new-FRX block. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_hf_client_api.h"
#include "esp_log.h"
#include "esp_sdp_api.h"
#include "esp_spp_api.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "uart_link.h"
#include "wifi_setup_proto.h"

static const char *TAG = "bt_agent";

/* Firmware version reported back to P4 over UART on boot via "BT-VER:<v>".
 * P4's bt_agent_ota compares this against CONFIG_BT_AGENT_FW_VERSION; on
 * mismatch it forces this chip into ROM bootloader and reflashes from the
 * embedded blob. Bump together with the CONFIG_BT_AGENT_FW_VERSION default
 * in main/Kconfig.projbuild on the P4 side any time the agent code changes. */
#define BT_AGENT_FW_VERSION "0.1.1"

/* ---------- User-configurable identity / Wifi creds ---------- */

/* BT device name we expose to the phone. Set to a placeholder until P4
 * publishes its AP SSID, at which point app_main() switches it to match
 * (so the phone sees one identity for both BT and WiFi — easier to pair
 * the right device when there are multiple HUs around). */
#define DEVICE_NAME_INIT "AAHU-init"
#define SPP_SERVER_NAME  "AA-WIRELESS"

/* Android Auto Wireless Service UUID — magic value the phone scans for to
 * decide whether a paired BT device is AA-capable. Without this in our SDP
 * database the phone treats us as a generic Car Audio with no AA support
 * and the auto-setup flow never fires.
 *
 * IMPORTANT: SDP wire format wants the UUID in BIG-ENDIAN (canonical) byte
 * order, not little-endian. (BLE uses LE for advertising data, Classic SDP
 * does NOT.) Putting LE here makes phone-side SDP query interpret our
 * record as a *different* UUID, so RFCOMM connect-by-UUID fails with
 * IOException. Keep this canonical-MSB-first.
 *
 *   4de17a00-52cb-11e6-bdf4-0800200c9a66
 */
static const uint8_t AA_WIRELESS_UUID128_BE[ESP_UUID_LEN_128] = {
    0x4d, 0xe1, 0x7a, 0x00, 0x52, 0xcb, 0x11, 0xe6,
    0xbd, 0xf4, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66,
};

/* WiFi credentials, BSSID, IP and port are no longer hard-coded — they
 * arrive from P4 over UART once its SoftAP is up. See uart_link.[ch].
 * Until the first WIFI|... line is received, we don't push WifiStartRequest
 * to the phone (no point — we'd be lying about an IP we haven't confirmed
 * yet, and a wrong BSSID makes gearhead drop the wireless setup with
 * WIFI_INVALID_BSSID). */

/* ---------- SPP frame format ----------
 *
 * Each message on RFCOMM is:
 *   [length BE16][message_id BE16][payload ...]
 * where length covers the payload only (NOT the 4-byte header itself).
 * Same wire shape as WirelessAndroidAutoDongle's bluetoothProfiles.cpp. */

#define SPP_HDR_LEN      4

static uint32_t g_spp_handle = 0;
static bool     g_wifi_info_sent = false;

static esp_err_t spp_send_message(uint16_t msg_id, const uint8_t *payload,
                                  uint16_t payload_len)
{
    if (g_spp_handle == 0) {
        ESP_LOGW(TAG, "spp_send_message: no client connected");
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t buf[SPP_HDR_LEN + 256];
    if (payload_len > sizeof(buf) - SPP_HDR_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }
    buf[0] = (uint8_t)(payload_len >> 8);
    buf[1] = (uint8_t)(payload_len & 0xFF);
    buf[2] = (uint8_t)(msg_id >> 8);
    buf[3] = (uint8_t)(msg_id & 0xFF);
    if (payload_len > 0) {
        memcpy(buf + SPP_HDR_LEN, payload, payload_len);
    }
    ESP_LOGI(TAG, "TX msg=%u payload=%u bytes", msg_id, payload_len);
    return esp_spp_write(g_spp_handle, SPP_HDR_LEN + payload_len, buf);
}

/* Send WifiStartRequest{ip, port} — first message we push to phone after the
 * SPP channel opens. Phone replies with WifiInfoRequest. */
static void send_wifi_start_request(void)
{
    const uart_link_wifi_t *w = uart_link_get_wifi();
    if (!w) {
        ESP_LOGW(TAG, "send_wifi_start_request: no WiFi info from P4 yet");
        return;
    }
    uint8_t pl[64];
    size_t  n = aa_wsm_build_start_request(pl, sizeof(pl), w->ip, w->port);
    ESP_LOGI(TAG, "WifiStartRequest: ip=%s port=%d", w->ip, w->port);
    spp_send_message(AA_WSM_WIFI_START_REQUEST, pl, n);
}

/* Send WifiInfoResponse with our credentials so phone can join the LAN. */
static void send_wifi_info_response(void)
{
    const uart_link_wifi_t *w = uart_link_get_wifi();
    if (!w) {
        ESP_LOGW(TAG, "send_wifi_info_response: no WiFi info from P4 yet");
        return;
    }
    uint8_t pl[160];
    /* DYNAMIC AP type: phone has to actually join the WiFi we describe.
     * STATIC was wrong — it told phone "you should already be on this
     * network" which doesn't apply when we host an AP just for projection. */
    size_t  n = aa_wsm_build_info_response(pl, sizeof(pl),
                                           w->ssid, w->password, w->bssid,
                                           AA_WSEC_WPA2_PERSONAL,
                                           AA_WAPT_DYNAMIC);
    ESP_LOGI(TAG, "WifiInfoResponse: ssid='%s' bssid=%s", w->ssid, w->bssid);
    spp_send_message(AA_WSM_WIFI_INFO_RESPONSE, pl, n);
    g_wifi_info_sent = true;
    uart_link_say("BT:WIFI_INFO_SENT");
}

/* Parse one inbound SPP message. data points at the 4-byte header. */
static void handle_spp_message(const uint8_t *data, uint16_t len)
{
    if (len < SPP_HDR_LEN) {
        ESP_LOGW(TAG, "short frame %u", len);
        return;
    }
    uint16_t plen   = ((uint16_t)data[0] << 8) | data[1];
    uint16_t msg_id = ((uint16_t)data[2] << 8) | data[3];
    if (SPP_HDR_LEN + plen > len) {
        ESP_LOGW(TAG, "truncated: hdr says %u + %u, got %u",
                 SPP_HDR_LEN, plen, len);
        return;
    }
    ESP_LOGI(TAG, "RX msg=%u payload=%u bytes", msg_id, plen);

    switch ((aa_wsm_id_t)msg_id) {
    case AA_WSM_WIFI_INFO_REQUEST:
        /* Phone is asking for SSID/key — respond with WifiInfoResponse. */
        send_wifi_info_response();
        break;
    case AA_WSM_WIFI_START_RESPONSE:
        ESP_LOGI(TAG, "Phone acked WifiStart");
        break;
    case AA_WSM_WIFI_CONNECT_STATUS:
        /* Phone is reporting back whether it managed to join the WiFi. The
         * status enum is in the payload but we don't strictly need to parse
         * it — if WiFi worked phone will TCP-connect to P4 next. */
        ESP_LOGI(TAG, "Phone reports WifiConnectStatus");
        uart_link_say("BT:WIFI_CONNECT_STATUS");
        break;
    case AA_WSM_WIFI_VERSION_REQUEST:
        ESP_LOGW(TAG, "WifiVersionRequest — not implemented (might be OK)");
        break;
    default:
        ESP_LOGW(TAG, "unhandled msg %u", msg_id);
        break;
    }
}

/* ---------- SPP / GAP callbacks ---------- */

static void spp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *p)
{
    switch (event) {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(TAG, "SPP_INIT (waiting for WiFi info from P4 before "
                      "going discoverable)");
        /* DON'T enable discoverable yet — we'd hand the phone an empty
         * WifiInfoResponse if it pairs before P4 publishes its AP creds.
         * app_main() flips us discoverable once uart_link_have_wifi() is
         * true. SPP server still starts here so we're ready to accept the
         * RFCOMM connection the moment a phone shows up. */
        /* Pin RFCOMM to channel 1 and DISABLE the auto SPP (UUID 0x1101)
         * record. Phone-side gearhead resolves the AA Wireless UUID via
         * SDP and tries to RFCOMM-connect at the channel that record
         * points to. If both the auto SPP record and our AA Wireless
         * record advertise channel 1, the SDP resolution returns a
         * conflicting set and phone fails with IOException. With
         * create_spp_record=false, the only record bound to channel 1 is
         * our AA Wireless one — clean route. */
        {
            esp_spp_start_srv_cfg_t cfg = {
                /* SCN 3 to stay clear of HFP (which lands at SCN 2 on this
                 * stack) and avoid any first-channel quirks. */
                .local_scn         = 3,
                .create_spp_record = false,
                /* SEC_AUTHENTICATE was rejecting phone's RFCOMM connect
                 * for the AA UUID even on a bonded link — phone's
                 * createRfcommSocketToServiceRecord() doesn't always
                 * negotiate the auth handshake we expect. SEC_NONE lets
                 * the connection through; the link is already ACL-authed
                 * by virtue of having been paired earlier. */
                .sec_mask          = ESP_SPP_SEC_NONE,
                .role              = ESP_SPP_ROLE_SLAVE,
                .name              = SPP_SERVER_NAME,
            };
            esp_spp_start_srv_with_cfg(&cfg);
        }
        break;

    case ESP_SPP_START_EVT:
        ESP_LOGI(TAG, "SPP server up: status=%d, scn=%d",
                 p->start.status, p->start.scn);
        uart_link_say("BT:READY");
        break;

    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(TAG, "SPP client connected: handle=%lu rem_bda=%02x:%02x:%02x:%02x:%02x:%02x",
                 p->srv_open.handle,
                 p->srv_open.rem_bda[0], p->srv_open.rem_bda[1],
                 p->srv_open.rem_bda[2], p->srv_open.rem_bda[3],
                 p->srv_open.rem_bda[4], p->srv_open.rem_bda[5]);
        g_spp_handle = p->srv_open.handle;
        g_wifi_info_sent = false;
        uart_link_say("BT:CONNECTED");
        /* Push WifiStartRequest right away — phone is waiting for it. */
        send_wifi_start_request();
        break;

    case ESP_SPP_DATA_IND_EVT:
        handle_spp_message(p->data_ind.data, p->data_ind.len);
        break;

    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(TAG, "SPP_CLOSE");
        g_spp_handle = 0;
        uart_link_say("BT:DISCONNECTED");
        break;

    case ESP_SPP_WRITE_EVT:
        if (p->write.status != ESP_SPP_SUCCESS) {
            ESP_LOGW(TAG, "spp write failed: %d", p->write.status);
        }
        break;

    default:
        break;
    }
}

static void gap_callback(esp_bt_gap_cb_event_t event,
                         esp_bt_gap_cb_param_t *p)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (p->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "auth ok with '%s'", p->auth_cmpl.device_name);
            uart_link_say("BT:PAIRED");
        } else {
            ESP_LOGW(TAG, "auth failed status=%d", p->auth_cmpl.stat);
        }
        break;

    case ESP_BT_GAP_PIN_REQ_EVT:
        /* Legacy PIN — fall back to '0000'. Modern phones use SSP and never
         * hit this path, but harmless to handle. */
        ESP_LOGI(TAG, "PIN request, replying '0000'");
        {
            esp_bt_pin_code_t pin = { '0', '0', '0', '0' };
            esp_bt_gap_pin_reply(p->pin_req.bda, true, 4, pin);
        }
        break;

    case ESP_BT_GAP_CFM_REQ_EVT:
        /* Numeric comparison (Just Works variant of SSP). Auto-accept. */
        ESP_LOGI(TAG, "SSP confirm req num=%lu — accepting",
                 (unsigned long)p->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(p->cfm_req.bda, true);
        break;

    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "SSP passkey: %lu", (unsigned long)p->key_notif.passkey);
        break;

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(TAG, "mode change: %d", p->mode_chg.mode);
        break;

    default:
        break;
    }
}

/* ---------- SDP: advertise the AA Wireless Service UUID ---------- */

static int g_aa_sdp_handle = -1;

static void sdp_callback(esp_sdp_cb_event_t event, esp_sdp_cb_param_t *p)
{
    switch (event) {
    case ESP_SDP_INIT_EVT: {
        ESP_LOGI(TAG, "SDP init: status=%d", p->init.status);
        if (p->init.status != ESP_SDP_SUCCESS) break;

        /* Create a RAW SDP record carrying the AA Wireless UUID. The first
         * attempt with a long service_name and length=strlen tripped over
         * BT_LOG "Invalid server name!" — turns out the bluedroid stack
         * dislikes the combination. Use a short ASCII name with explicit
         * +1 for the trailing null and the call goes through. */
        static char svc_name[] = "AA";
        esp_bluetooth_sdp_record_t rec = {0};
        rec.hdr.type                  = ESP_SDP_TYPE_RAW;
        rec.hdr.uuid.len              = ESP_UUID_LEN_128;
        memcpy(rec.hdr.uuid.uuid.uuid128, AA_WIRELESS_UUID128_BE,
               ESP_UUID_LEN_128);
        rec.hdr.service_name          = svc_name;
        rec.hdr.service_name_length   = sizeof(svc_name); /* incl. NUL */
        rec.hdr.rfcomm_channel_number = 3;  /* must match SPP local_scn */
        rec.hdr.l2cap_psm             = -1;
        rec.hdr.profile_version       = 0x0102;
        esp_err_t err = esp_sdp_create_record(&rec);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "esp_sdp_create_record failed: %s",
                     esp_err_to_name(err));
        }
        break;
    }
    case ESP_SDP_CREATE_RECORD_COMP_EVT:
        ESP_LOGI(TAG, "AA Wireless SDP record created: handle=%d, status=%d",
                 p->create_record.record_handle, p->create_record.status);
        if (p->create_record.status == ESP_SDP_SUCCESS) {
            g_aa_sdp_handle = p->create_record.record_handle;
        }
        break;
    case ESP_SDP_REMOVE_RECORD_COMP_EVT:
        ESP_LOGI(TAG, "SDP record removed: handle=%d, status=%d",
                 p->remove_record.record_handle, p->remove_record.status);
        break;
    default:
        break;
    }
}

/* ---------- HFP-HF: pretend to be a hands-free car kit ----------
 *
 * We don't actually route call audio anywhere — the goal is to make the
 * phone treat us like a real wireless car kit (HFP advertised) so that
 * after pairing it auto-reconnects on BT toggle and gearhead's BTCapsStore
 * check fires on a bonded ACL_CONNECTED.
 *
 * The callback just logs events; we don't respond to phone calls. */
static void hf_client_callback(esp_hf_client_cb_event_t event,
                               esp_hf_client_cb_param_t *p)
{
    switch (event) {
    case ESP_HF_CLIENT_CONNECTION_STATE_EVT:
        ESP_LOGI(TAG, "HFP conn state: %d (peer features 0x%lx)",
                 p->conn_stat.state, (unsigned long)p->conn_stat.peer_feat);
        break;
    case ESP_HF_CLIENT_AUDIO_STATE_EVT:
        ESP_LOGI(TAG, "HFP audio state: %d", p->audio_stat.state);
        break;
    default:
        break;
    }
}

/* ---------- Init ---------- */

static void init_bt(void)
{
    /* Free BLE memory; we only need Classic. */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_callback));

    /* Friendly name shown on the phone during pairing. Placeholder for now
     * — app_main() updates it to the WiFi SSID once P4 publishes one, so
     * BT and WiFi share the same identity. */
    ESP_ERROR_CHECK(esp_bt_gap_set_device_name(DEVICE_NAME_INIT));

    /* SSP "just works" — accept pairing without user input on our side. */
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap));

    /* SPP. Use enhanced init so we can ask for ERTM (more reliable RFCOMM
     * over noisy links). */
    ESP_ERROR_CHECK(esp_spp_register_callback(spp_callback));
    esp_spp_cfg_t spp_cfg = {
        .mode              = ESP_SPP_MODE_CB,
        .enable_l2cap_ertm = true,
        .tx_buffer_size    = 0,  /* default */
    };
    ESP_ERROR_CHECK(esp_spp_enhanced_init(&spp_cfg));

    /* HFP Hands-Free role. Phone advertises HFP-AG (it has the audio source
     * for calls); we register HFP-HF (the receiver / car kit). After pair-
     * ing, phone connects HFP automatically — that gives us a "useful"
     * profile and triggers the second ACL_CONNECTED gearhead needs. */
    ESP_ERROR_CHECK(esp_hf_client_register_callback(hf_client_callback));
    ESP_ERROR_CHECK(esp_hf_client_init());

    /* Register the Android Auto Wireless UUID via SDP so the phone realises
     * we're an AA-capable head unit and offers wireless setup. The SPP-
     * default record (UUID 0x1101) stays in place too — we want both. */
    ESP_ERROR_CHECK(esp_sdp_register_callback(sdp_callback));
    ESP_ERROR_CHECK(esp_sdp_init());

    /* Push EIR (Extended Inquiry Response) so the phone sees our UUIDs at
     * scan time, BEFORE pairing. include_uuid=true makes the stack pull
     * UUIDs from the SDP records we registered (SPP + the AA Wireless raw
     * record). Some phones use EIR UUIDs to decide which "Set up Android
     * Auto" prompts to show. */
    {
        esp_bt_eir_data_t eir = {
            .fec_required    = false,
            .include_txpower = true,
            .include_uuid    = true,
            .include_name    = true,
            .flag            = ESP_BT_EIR_FLAG_GEN_DISC,
        };
        esp_err_t e = esp_bt_gap_config_eir_data(&eir);
        if (e != ESP_OK) {
            ESP_LOGW(TAG, "config_eir_data: %s", esp_err_to_name(e));
        }
    }

    /* Set Class of Device LAST — SPP/SDP init may overwrite COD bits during
     * their setup, so calling set_cod here ensures our values stick. Use
     * ESP_BT_INIT_COD which fully overwrites major+minor+service so the
     * default Uncategorized doesn't bleed through.
     *
     * Phone-side BluetoothPairingRequest checks COD to decide whether it
     * can bond without the "Pair with X?" dialog. With our previous Uncate-
     * gorized COD the phone showed a dialog, the user had to tap, and by
     * the time SSP completed gearhead had already given up and disconnected
     * (CAR.BTCapsStore: NOT_BONDED check fired ~1 s after ACL_CONNECTED).
     *
     * Setting COD = Audio/Video (major=4) + Car Audio (minor=8) + Audio
     * service bit pushes the phone into Just-Works auto-bond. */
    {
        esp_bt_cod_t cod = {
            .reserved_2 = 0,
            .minor      = 0x08,    /* Car Audio                    */
            .major      = 0x04,    /* Audio/Video                  */
            .service    = 0x100,   /* Audio service bit            */
            .reserved_8 = 0,
        };
        esp_err_t e = esp_bt_gap_set_cod(cod, ESP_BT_INIT_COD);
        if (e != ESP_OK) {
            ESP_LOGW(TAG, "set_cod: %s", esp_err_to_name(e));
        }
        /* Read-back to verify the value was actually applied. Useful when a
         * lower stack layer keeps overwriting it. */
        esp_bt_cod_t got;
        if (esp_bt_gap_get_cod(&got) == ESP_OK) {
            ESP_LOGI(TAG, "COD readback: major=0x%02x minor=0x%02x service=0x%03x",
                     (unsigned)got.major, (unsigned)got.minor,
                     (unsigned)got.service);
        }
    }

    ESP_LOGI(TAG, "BT classic + SPP + SDP up (name='%s', Car Audio). Will go "
                  "discoverable once WiFi creds arrive from P4.",
             DEVICE_NAME_INIT);
}

void app_main(void)
{
    /* NVS holds bonding info between reboots. Without this, every reboot the
     * phone would have to re-pair from scratch. */
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        r = nvs_flash_init();
    }
    ESP_ERROR_CHECK(r);

    uart_link_init();
    uart_link_say("BT:BOOT");
    uart_link_say("BT-VER:" BT_AGENT_FW_VERSION);

    init_bt();

    /* Block until P4 publishes WiFi creds via UART. Until then we keep BT
     * non-discoverable to avoid handing a phone an incomplete (or empty)
     * WifiInfoResponse mid-pair. Loop is cheap — just polls a flag set by
     * the UART RX task. */
    int waited_s = 0;
    while (!uart_link_have_wifi()) {
        if (waited_s % 5 == 0) {
            ESP_LOGI(TAG, "waiting for WiFi info from P4 (%ds)...", waited_s);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        waited_s++;
    }
    const uart_link_wifi_t *w = uart_link_get_wifi();
    ESP_LOGI(TAG, "WiFi creds ready (ssid=%s, bssid=%s, ip=%s:%d) — going "
                  "discoverable",
             w->ssid, w->bssid, w->ip, w->port);

    /* Rename BT to match the SSID so the phone sees a single identity for
     * pairing + WiFi join. Both end up named e.g. "AAHU-A5F1". */
    esp_err_t e = esp_bt_gap_set_device_name(w->ssid);
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "set_device_name(%s) failed: %s",
                 w->ssid, esp_err_to_name(e));
    }

    /* OK to advertise now: any phone that pairs will get the real
     * credentials in WifiInfoResponse and can join our AP. */
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    uart_link_say("BT:DISCOVERABLE");
}
