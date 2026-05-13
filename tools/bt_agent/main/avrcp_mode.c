/* AVRCP-only mode — see avrcp_mode.h for the high-level contract.
 *
 * Phones in practice will not open an AVRCP session unless an A2DP link is
 * also established (spec allows it, real-world Android/iOS don't), so we
 * register BOTH A2DP sink and AVRCP CT. The PCM stream from A2DP gets
 * dumped into a no-op data callback — we have no DAC, the bytes are
 * discarded. AVRCP CT then receives track metadata + play state and
 * forwards them to P4 over UART.
 *
 * IMPORTANT: AVRC CT must be init'd BEFORE esp_a2d_sink_init. Bluedroid
 * checks at A2DP-init time whether AVRC is registered, and if not, logs
 * "AVRC not Init, not using it." and never opens an AVRC channel for the
 * A2DP connection — so metadata callbacks silently never fire. Order
 * matters.
 *
 * Audio routing: when the phone first pairs with us it will route media
 * audio to our A2DP sink by default. The phone owner has to open
 * Bluetooth settings → our device → toggle "Media audio" OFF if they
 * want music to stay on their headphones. This is a per-pairing
 * persistent setting and there's no way to suppress it from our side
 * while keeping A2DP registered (we need A2DP registered to keep AVRCP
 * alive).
 *
 * Reference: ESP-IDF examples/bluetooth/bluedroid/classic_bt/a2dp_sink/. */

#include "avrcp_mode.h"

#include <stdio.h>
#include <string.h>

#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "uart_link.h"

static const char *TAG = "avrcp";

/* Device name advertised on the BT scan list. Kept short so it shows up
 * clearly in the phone's "Pair new device" UI. */
#define AVRCP_DEVICE_NAME  "ESP32 Head Unit"

/* Metadata fields cap. AVRCP responses are usually < 100 bytes per field;
 * Sonos / Pandora occasionally send longer titles — keep some headroom. */
#define META_FIELD_MAX 192

static struct {
    char title[META_FIELD_MAX];
    char artist[META_FIELD_MAX];
    char album[META_FIELD_MAX];
    /* Track whether we've seen all three fields after the most recent
     * TRACK_CHANGE notification, so we only push one META line per track
     * instead of three partial updates that would flicker the labels. */
    bool have_title;
    bool have_artist;
    bool have_album;
} s_meta;

static void meta_reset(void)
{
    s_meta.title[0] = s_meta.artist[0] = s_meta.album[0] = '\0';
    s_meta.have_title = s_meta.have_artist = s_meta.have_album = false;
}

static void meta_flush(void)
{
    /* Some phones omit album entirely — don't wait forever for it. Push as
     * soon as we have title + artist, and again later if album shows up. */
    if (s_meta.have_title && s_meta.have_artist) {
        uart_link_send_meta(s_meta.title, s_meta.artist, s_meta.album);
    }
}

/* AVRCP attribute IDs we care about — defined in the spec, esp_avrc_md_attr_mask_t
 * bit positions are the same numeric values minus 1. */
#define ATTR_ID_TITLE   0x01
#define ATTR_ID_ARTIST  0x02
#define ATTR_ID_ALBUM   0x03

static void avrcp_ct_metadata_handler(uint8_t attr_id, const uint8_t *text,
                                      uint16_t text_len)
{
    char *dst = NULL;
    bool *flag = NULL;
    switch (attr_id) {
        case ATTR_ID_TITLE:  dst = s_meta.title;  flag = &s_meta.have_title;  break;
        case ATTR_ID_ARTIST: dst = s_meta.artist; flag = &s_meta.have_artist; break;
        case ATTR_ID_ALBUM:  dst = s_meta.album;  flag = &s_meta.have_album;  break;
        default: return;
    }
    size_t n = text_len < META_FIELD_MAX - 1 ? text_len : META_FIELD_MAX - 1;
    memcpy(dst, text, n);
    dst[n] = '\0';
    *flag = true;
    meta_flush();
}

/* Request title + artist + album for the currently playing track. Fires
 * right after connect and again after each TRACK_CHANGE notification. */
#define AVRCP_TRACK_ATTR_MASK \
    (ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM)

static uint8_t s_tl_meta    = 1;  /* transaction labels — AVRCP requires unique
                                   * per outstanding command; we only have one
                                   * in flight at a time so a static counter is
                                   * fine. */
static uint8_t s_tl_notify  = 2;

static void avrcp_ct_request_track_meta(void)
{
    meta_reset();
    esp_avrc_ct_send_metadata_cmd(s_tl_meta, AVRCP_TRACK_ATTR_MASK);
}

static void avrcp_ct_subscribe(void)
{
    /* event_parameter is "playback interval seconds" for play-position;
     * 0 is fine for the events we care about. */
    esp_avrc_ct_send_register_notification_cmd(s_tl_notify,
                                               ESP_AVRC_RN_TRACK_CHANGE, 0);
    esp_avrc_ct_send_register_notification_cmd(s_tl_notify + 1,
                                               ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
}

static void avrcp_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *p)
{
    switch (event) {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
        ESP_LOGI(TAG, "AVRCP CT connection: connected=%d", p->conn_stat.connected);
        if (p->conn_stat.connected) {
            uart_link_say("BT:AVRCP_CONNECTED");
            /* Phone usually has a track loaded by the time it opens AVRCP
             * — fetch metadata right away rather than waiting for a track
             * change that may not come. */
            avrcp_ct_request_track_meta();
            avrcp_ct_subscribe();
        } else {
            uart_link_say("BT:AVRCP_DISCONNECTED");
            meta_reset();
            uart_link_send_meta("", "", "");
            uart_link_send_state(false);
        }
        break;

    case ESP_AVRC_CT_METADATA_RSP_EVT: {
        /* One callback per attribute in the response — Bluedroid splits the
         * AVRCP GetElementAttributes response into per-attr events. */
        avrcp_ct_metadata_handler(p->meta_rsp.attr_id,
                                  p->meta_rsp.attr_text,
                                  p->meta_rsp.attr_length);
        break;
    }

    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        if (p->change_ntf.event_id == ESP_AVRC_RN_TRACK_CHANGE) {
            ESP_LOGI(TAG, "AVRCP TRACK_CHANGE — refetching metadata");
            avrcp_ct_request_track_meta();
            /* Re-subscribe (notifications are one-shot in AVRCP 1.x). */
            esp_avrc_ct_send_register_notification_cmd(s_tl_notify,
                                                       ESP_AVRC_RN_TRACK_CHANGE, 0);
        } else if (p->change_ntf.event_id == ESP_AVRC_RN_PLAY_STATUS_CHANGE) {
            /* event_parameter.playback is the new play_status enum. */
            uint8_t st = p->change_ntf.event_parameter.playback;
            bool playing = (st == ESP_AVRC_PLAYBACK_PLAYING);
            ESP_LOGI(TAG, "AVRCP PLAY_STATUS_CHANGE = %u (playing=%d)", st, playing);
            uart_link_send_state(playing);
            esp_avrc_ct_send_register_notification_cmd(s_tl_notify + 1,
                                                       ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
        }
        break;

    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
        ESP_LOGI(TAG, "AVRCP remote features: 0x%x", (unsigned)p->rmt_feats.feat_mask);
        break;

    default:
        break;
    }
}

/* A2DP data callback — invoked from a high-priority Bluedroid task with raw
 * decoded PCM. We have no DAC; return immediately so the bytes are
 * discarded. Registering an A2DP sink is required to keep AVRCP alive
 * (see file-top comment) — the data path itself is just ballast. */
static void a2d_data_cb(const uint8_t *data, uint32_t len)
{
    (void)data;
    (void)len;
}

static void a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *p)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        ESP_LOGI(TAG, "A2DP conn state=%d", p->conn_stat.state);
        if (p->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
            uart_link_say("BT:A2DP_CONNECTED");
        } else if (p->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
            uart_link_say("BT:A2DP_DISCONNECTED");
        }
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        ESP_LOGD(TAG, "A2DP audio state=%d", p->audio_stat.state);
        break;
    default:
        break;
    }
}

static void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *p)
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
    case ESP_BT_GAP_CFM_REQ_EVT:
        /* Just Works confirmation. */
        esp_bt_gap_ssp_confirm_reply(p->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "SSP passkey: %lu", (unsigned long)p->key_notif.passkey);
        break;
    default:
        break;
    }
}

void avrcp_mode_run(void)
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_cb));
    ESP_ERROR_CHECK(esp_bt_gap_set_device_name(AVRCP_DEVICE_NAME));

    /* Just-Works SSP — accept pairing without UI on our side. */
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap));

    /* AVRCP Controller FIRST. Bluedroid checks at esp_a2d_sink_init time
     * whether AVRC is already registered — if not, it logs "AVRC not Init,
     * not using it." and silently never opens an AVRC channel for the A2DP
     * connection, so metadata callbacks never fire. */
    ESP_ERROR_CHECK(esp_avrc_ct_register_callback(avrcp_ct_cb));
    ESP_ERROR_CHECK(esp_avrc_ct_init());

    /* A2DP Sink — purely to keep AVRCP alive. Data callback installed
     * before init so we don't miss the very first PCM packet (it would
     * assert otherwise). */
    ESP_ERROR_CHECK(esp_a2d_register_callback(a2d_cb));
    ESP_ERROR_CHECK(esp_a2d_sink_register_data_callback(a2d_data_cb));
    ESP_ERROR_CHECK(esp_a2d_sink_init());

    /* Class of Device: Audio/Video, minor "Car audio" (0x08). We're a head
     * unit displaying track info, not a speaker — phones use COD to decide
     * whether to offer "Media audio" routing. Car audio keeps us off the
     * audio-output list so the user's headphones aren't displaced. */
    esp_bt_cod_t cod = {
        .reserved_2 = 0,
        .minor      = 0x08,    /* Car audio */
        .major      = 0x04,    /* Audio/Video */
        .service    = 0x100,   /* Audio service bit (still required so the
                                * phone exposes AVRCP CT controls) */
        .reserved_8 = 0,
    };
    esp_bt_gap_set_cod(cod, ESP_BT_INIT_COD);

    /* EIR with our UUIDs + name, then go discoverable. Unlike the AA path,
     * we have nothing to wait for from P4 — phone can pair immediately. */
    esp_bt_eir_data_t eir = {
        .fec_required    = false,
        .include_txpower = true,
        .include_uuid    = true,
        .include_name    = true,
        .flag            = ESP_BT_EIR_FLAG_GEN_DISC,
    };
    esp_bt_gap_config_eir_data(&eir);

    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    uart_link_say("BT:AVRCP_READY");
    ESP_LOGI(TAG, "AVRCP mode up, name='%s' — phone can pair now", AVRCP_DEVICE_NAME);

    /* Nothing more to do on this task — events flow through the BT/A2DP/
     * AVRCP callbacks. Park forever; no AA polling loop needed. */
    for (;;) {
        vTaskDelay(portMAX_DELAY);
    }
}
