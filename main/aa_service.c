#include "aa_service.h"

#include <stdlib.h>
#include <string.h>

#include "aa_frame.h"
#include "aa_proto.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

static const char *TAG = "aa_svc";

/* Control-channel message ids beyond what's already in aa_frame.h */
#define AA_MSG_SD_REQUEST          0x0005
#define AA_MSG_SD_RESPONSE         0x0006
#define AA_MSG_CHANNEL_OPEN_REQ    0x0007
#define AA_MSG_CHANNEL_OPEN_RESP   0x0008
#define AA_MSG_PING_REQUEST        0x000b
#define AA_MSG_PING_RESPONSE       0x000c
#define AA_MSG_NAV_FOCUS_REQ       0x000d
#define AA_MSG_NAV_FOCUS_RESP      0x000e
#define AA_MSG_AUDIO_FOCUS_REQ     0x0012
#define AA_MSG_AUDIO_FOCUS_RESP    0x0013

/* Per-channel message ids (HU_*_CHANNEL_MESSAGE in headunit's hu_aap.h).
 * The same numeric tag means different things on different channel kinds —
 * dispatch must check the channel id first, then the message id. */
#define AA_MSG_AV_MEDIA_SETUP_REQ   0x8000
#define AA_MSG_AV_MEDIA_START_REQ   0x8001
#define AA_MSG_AV_MEDIA_SETUP_RESP  0x8003
#define AA_MSG_AV_VIDEO_FOCUS_REQ   0x8007
#define AA_MSG_AV_VIDEO_FOCUS_IND   0x8008
#define AA_MSG_SENSOR_START_REQ     0x8001
#define AA_MSG_SENSOR_START_RESP    0x8002
#define AA_MSG_SENSOR_EVENT         0x8003
#define AA_MSG_INPUT_BINDING_REQ    0x8002
#define AA_MSG_INPUT_BINDING_RESP   0x8003

/* Buffer sizes — generous since these live on the heap, not the stack. */
#define CIPHER_BUF_SIZE     16384   /* one TLS record max */
#define PLAIN_BUF_SIZE      16384
#define SCRATCH_SIZE        8192    /* protobuf scratch */

/* ---------- Service Discovery response builder ---------- */

/* AVChannel{stream_type=VIDEO, available_while_in_call=true,
 *           video_configs=[{resolution=720p, fps=30, dpi=160}]}
 * → bytes.
 *
 * Trying enum=2 (1280x720) on the off chance modern gearhead refuses to
 * project to 480p heads — most certified wireless car units advertise
 * 720p+, so this might be the "I'll talk to you" tier. We'll downscale
 * the decoded frames to the 800x480 panel via the ESP32-P4 PPA. */
static size_t build_video_av_channel(uint8_t *out, size_t cap)
{
    /* Inner VideoConfig submessage. */
    uint8_t vcfg[32];
    size_t  vp = 0;
    pb_w_uint32(vcfg, sizeof(vcfg), &vp, 1, 2);   /* resolution = 1280x720 */
    pb_w_uint32(vcfg, sizeof(vcfg), &vp, 2, 1);   /* fps = 30 */
    pb_w_uint32(vcfg, sizeof(vcfg), &vp, 3, 0);   /* margin_width */
    pb_w_uint32(vcfg, sizeof(vcfg), &vp, 4, 0);   /* margin_height */
    pb_w_uint32(vcfg, sizeof(vcfg), &vp, 5, 160); /* dpi — typical 720p car */

    size_t pos = 0;
    pb_w_uint32(out, cap, &pos, 1, 3);           /* stream_type = VIDEO */
    pb_w_bool  (out, cap, &pos, 5, true);        /* available_while_in_call */
    pb_w_submsg(out, cap, &pos, 4, vcfg, vp);    /* video_configs (repeated) */
    return pos;
}

/* AVChannel for an audio stream (audio_type 1=SPEECH, 2=SYSTEM, 3=MEDIA).
 * MEDIA uses 48 kHz stereo to match headunit-revived; SPEECH/SYSTEM are
 * 16 kHz mono. */
static size_t build_audio_av_channel(uint8_t *out, size_t cap, uint32_t audio_type)
{
    uint8_t acfg[16];
    size_t  ap = 0;
    if (audio_type == 3 /* MEDIA */) {
        pb_w_uint32(acfg, sizeof(acfg), &ap, 1, 48000); /* sample_rate */
        pb_w_uint32(acfg, sizeof(acfg), &ap, 2, 16);    /* bit_depth */
        pb_w_uint32(acfg, sizeof(acfg), &ap, 3, 2);     /* channel_count */
    } else {
        pb_w_uint32(acfg, sizeof(acfg), &ap, 1, 16000);
        pb_w_uint32(acfg, sizeof(acfg), &ap, 2, 16);
        pb_w_uint32(acfg, sizeof(acfg), &ap, 3, 1);
    }

    size_t pos = 0;
    pb_w_uint32(out, cap, &pos, 1, 1);               /* stream_type = AUDIO */
    pb_w_uint32(out, cap, &pos, 2, audio_type);      /* audio_type */
    pb_w_submsg(out, cap, &pos, 3, acfg, ap);        /* audio_configs */
    pb_w_bool  (out, cap, &pos, 5, true);            /* available_while_in_call */
    return pos;
}

/* InputStreamChannel for the microphone. Required by modern Android Auto —
 * without a MIC channel the phone won't progress past Service Discovery. */
static size_t build_mic_input_channel(uint8_t *out, size_t cap)
{
    uint8_t acfg[12];
    size_t  ap = 0;
    pb_w_uint32(acfg, sizeof(acfg), &ap, 1, 16000);  /* sample_rate */
    pb_w_uint32(acfg, sizeof(acfg), &ap, 2, 16);     /* bit_depth */
    pb_w_uint32(acfg, sizeof(acfg), &ap, 3, 1);      /* channel_count = mono */

    size_t pos = 0;
    pb_w_uint32(out, cap, &pos, 1, 1);               /* stream_type = AUDIO */
    pb_w_submsg(out, cap, &pos, 2, acfg, ap);        /* audio_config (singular) */
    pb_w_bool  (out, cap, &pos, 3, false);           /* available_while_in_call */
    return pos;
}

/* SensorChannel{sensors=[DRIVING_STATUS, NIGHT_DATA, LOCATION]} —
 * matches headunit-revived's set. Phone sometimes refuses to proceed
 * past Service Discovery if LOCATION isn't advertised. */
static size_t build_sensor_channel(uint8_t *out, size_t cap)
{
    static const uint32_t TYPES[] = { 13 /* DRIVING_STATUS */,
                                       10 /* NIGHT_DATA */,
                                        1 /* LOCATION */ };
    size_t pos = 0;
    for (size_t i = 0; i < sizeof(TYPES)/sizeof(TYPES[0]); i++) {
        uint8_t s[4];
        size_t  sp = 0;
        pb_w_uint32(s, sizeof(s), &sp, 1, TYPES[i]);
        pb_w_submsg(out, cap, &pos, 1, s, sp);
    }
    return pos;
}

/* InputChannel{supported_keycodes=[...], touch_screen_config={800,480}}.
 * Gearhead expects at least a few keycodes, otherwise it may decline to
 * open the input channel.
 *
 * Keycode values come from aasdk's ButtonCodeEnum.proto. We advertise the
 * essentials: ENTER, BACK, UP/DOWN/LEFT/RIGHT, SCROLL_WHEEL. */
static const uint32_t INPUT_KEYCODES[] = {
    0x13, /* UP */
    0x14, /* DOWN */
    0x15, /* LEFT */
    0x16, /* RIGHT */
    0x17, /* CENTER (enter) */
    0x04, /* BACK */
    0x52, /* MENU */
    0x54, /* PHONE_HOOK_END */
    0x55, /* PLAY/PAUSE */
    0x57, /* NEXT */
    0x58, /* PREV */
};

static size_t build_input_channel(uint8_t *out, size_t cap)
{
    size_t pos = 0;
    /* supported_keycodes — repeated uint32, field 1, varint each. */
    for (size_t i = 0; i < sizeof(INPUT_KEYCODES)/sizeof(INPUT_KEYCODES[0]); i++) {
        pb_w_uint32(out, cap, &pos, 1, INPUT_KEYCODES[i]);
    }

    uint8_t ts[16];
    size_t  tp = 0;
    pb_w_uint32(ts, sizeof(ts), &tp, 1, 800);
    pb_w_uint32(ts, sizeof(ts), &tp, 2, 480);
    pb_w_submsg(out, cap, &pos, 2, ts, tp);    /* touch_screen_config */
    return pos;
}

/* One ChannelDescriptor with id + nested feature submessage at given field. */
static size_t build_channel_descriptor(uint8_t *out, size_t cap,
                                       uint32_t channel_id,
                                       uint32_t feature_field,
                                       const uint8_t *feature, size_t feature_len)
{
    size_t pos = 0;
    pb_w_uint32(out, cap, &pos, 1, channel_id);
    pb_w_submsg(out, cap, &pos, feature_field, feature, feature_len);
    return pos;
}

/* Minimal 1x1 transparent PNG — 67 bytes. Used as placeholder head-unit
 * icon while we don't have artwork. Phone's SDR request showed it sends
 * three PNG icons (32×32, 64×64, 128×128); newer AAP looks like it
 * expects the head unit to respond with the same shape. */
__attribute__((unused))
static const uint8_t TINY_PNG[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
    0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
    0x89, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x44, 0x41,
    0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
    0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
    0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
    0x42, 0x60, 0x82,
};

/* Build a "field 6" sub-message containing our HU UUID. */
__attribute__((unused))
static size_t build_uuid_submsg(uint8_t *out, size_t cap)
{
    size_t pos = 0;
    /* Make up a stable UUID — the actual content doesn't matter to gearhead
     * as far as we know, but it has to be present and well-formed. */
    pb_w_string(out, cap, &pos, 1,
                "00000000-0000-0000-0000-deadbeef0001");
    return pos;
}

/* Mode A: legacy aasdk-style SDR response — the one openauto/headunit-revived
 * use, with a list of channel descriptors and head-unit info strings.
 * Mode B: mirror the new-AAP format we observed in the request — 3 icons,
 * name, brand, UUID submsg. Used as a diagnostic to see if gearhead 1.7
 * is rejecting us because it parses our response with a different proto. */
#define SDR_MODE_LEGACY  1
#define SDR_MODE_NEW     2
#define SDR_MODE         SDR_MODE_LEGACY

#if SDR_MODE == SDR_MODE_NEW
static esp_err_t build_sd_response(uint8_t *out, size_t cap, size_t *out_len)
{
    size_t pos = 0;
    uint8_t scratch[256];
    size_t  sl;

    /* HU identity — same shape gearhead 1.7 sent us in its request. */
    pb_w_bytes(out, cap, &pos, 1, TINY_PNG, sizeof(TINY_PNG));   /* icon 32 */
    pb_w_bytes(out, cap, &pos, 2, TINY_PNG, sizeof(TINY_PNG));   /* icon 64 */
    pb_w_bytes(out, cap, &pos, 3, TINY_PNG, sizeof(TINY_PNG));   /* icon 128 */
    pb_w_string(out, cap, &pos, 4, "ESP32-P4 AA");                /* name */
    pb_w_string(out, cap, &pos, 5, "espressif");                  /* brand */
    {
        uint8_t uu[64];
        size_t  ul = build_uuid_submsg(uu, sizeof(uu));
        pb_w_submsg(out, cap, &pos, 6, uu, ul);                   /* device id */
    }

    /* Channels — guess at field 7 in the new proto. Order: input,
     * sensor, video, audio*. Same channel descriptor layout as legacy
     * (channel_id + nested feature submsg). */
    sl = build_input_channel(scratch, sizeof(scratch));
    {
        uint8_t cd[128];
        size_t  cdp = build_channel_descriptor(cd, sizeof(cd), 1, 4, scratch, sl);
        pb_w_submsg(out, cap, &pos, 7, cd, cdp);
    }
    sl = build_sensor_channel(scratch, sizeof(scratch));
    {
        uint8_t cd[128];
        size_t  cdp = build_channel_descriptor(cd, sizeof(cd), 2, 2, scratch, sl);
        pb_w_submsg(out, cap, &pos, 7, cd, cdp);
    }
    sl = build_video_av_channel(scratch, sizeof(scratch));
    {
        uint8_t cd[128];
        size_t  cdp = build_channel_descriptor(cd, sizeof(cd), 3, 3, scratch, sl);
        pb_w_submsg(out, cap, &pos, 7, cd, cdp);
    }
    sl = build_audio_av_channel(scratch, sizeof(scratch), 3 /* MEDIA */);
    {
        uint8_t cd[128];
        size_t  cdp = build_channel_descriptor(cd, sizeof(cd), 4, 3, scratch, sl);
        pb_w_submsg(out, cap, &pos, 7, cd, cdp);
    }
    sl = build_audio_av_channel(scratch, sizeof(scratch), 1 /* SPEECH */);
    {
        uint8_t cd[128];
        size_t  cdp = build_channel_descriptor(cd, sizeof(cd), 5, 3, scratch, sl);
        pb_w_submsg(out, cap, &pos, 7, cd, cdp);
    }
    sl = build_audio_av_channel(scratch, sizeof(scratch), 2 /* SYSTEM */);
    {
        uint8_t cd[128];
        size_t  cdp = build_channel_descriptor(cd, sizeof(cd), 6, 3, scratch, sl);
        pb_w_submsg(out, cap, &pos, 7, cd, cdp);
    }

    /* Legacy car-info strings, shifted up to fields 8-15 to leave 7 for
     * channels. Most will be ignored if the new proto numbers them
     * differently — they cost nothing to include. */
    pb_w_string(out, cap, &pos,  8, "Universal");      /* car_model */
    pb_w_string(out, cap, &pos,  9, "2026");           /* car_year */
    pb_w_string(out, cap, &pos, 10, "0001");           /* car_serial */
    pb_w_bool  (out, cap, &pos, 11, true);             /* lhd / driver_pos */
    pb_w_string(out, cap, &pos, 12, "1");              /* sw_build */
    pb_w_string(out, cap, &pos, 13, "1.0");            /* sw_version */
    pb_w_bool  (out, cap, &pos, 14, false);            /* native_media_during_vr */
    pb_w_bool  (out, cap, &pos, 15, false);            /* hide_clock */

    *out_len = pos;
    return ESP_OK;
}

#endif /* SDR_MODE == SDR_MODE_NEW */

#if SDR_MODE == SDR_MODE_LEGACY
static esp_err_t build_sd_response(uint8_t *out, size_t cap, size_t *out_len)
{
    /* Build each channel descriptor first, then assemble the top-level. */
    uint8_t scratch[256];
    size_t  sl;

    uint8_t *p = out;
    size_t   pos = 0;

    /* Top-level scalars. Field numbers match aasdk's ServiceDiscoveryResponse
     * proto. Modern gearhead's *request* uses fields 1-3 for icons and 4-6 for
     * name/brand/UUID — that's a different proto from our response, so the
     * field numbers don't conflict. We respond with the legacy aasdk schema. */
    pb_w_string(p, cap, &pos, 2,  "ESP32-P4 AA");      /* head_unit_name */
    pb_w_string(p, cap, &pos, 3,  "Universal");        /* car_model */
    pb_w_string(p, cap, &pos, 4,  "2026");             /* car_year */
    pb_w_string(p, cap, &pos, 5,  "0001");             /* car_serial */
    pb_w_bool  (p, cap, &pos, 6,  true);               /* left_hand_drive / driver_pos */
    pb_w_string(p, cap, &pos, 7,  "espressif");        /* manufacturer / make */
    pb_w_string(p, cap, &pos, 8,  "ESP32-P4");         /* model */
    pb_w_string(p, cap, &pos, 9,  "1");                /* sw_build */
    pb_w_string(p, cap, &pos, 10, "1.0");              /* sw_version */
    pb_w_bool  (p, cap, &pos, 11, false);              /* native media during VR */
    pb_w_bool  (p, cap, &pos, 12, false);              /* hide_clock */

    /* Input channel (id=1) — gearhead opens this first, advertise first. */
    sl = build_input_channel(scratch, sizeof(scratch));
    {
        uint8_t cd[128];
        size_t cdp = build_channel_descriptor(cd, sizeof(cd), 1, 4, scratch, sl);
        pb_w_submsg(p, cap, &pos, 1, cd, cdp);
    }

    /* Sensor channel (id=2) */
    sl = build_sensor_channel(scratch, sizeof(scratch));
    {
        uint8_t cd[256];
        size_t cdp = build_channel_descriptor(cd, sizeof(cd), 2, 2, scratch, sl);
        pb_w_submsg(p, cap, &pos, 1, cd, cdp);
    }

    /* Video channel (id=3) */
    sl = build_video_av_channel(scratch, sizeof(scratch));
    {
        uint8_t cd[256];
        size_t cdp = build_channel_descriptor(cd, sizeof(cd), 3, 3, scratch, sl);
        pb_w_submsg(p, cap, &pos, 1, cd, cdp);
    }

    /* Media audio (id=4) */
    sl = build_audio_av_channel(scratch, sizeof(scratch), 3 /* MEDIA */);
    {
        uint8_t cd[256];
        size_t cdp = build_channel_descriptor(cd, sizeof(cd), 4, 3, scratch, sl);
        pb_w_submsg(p, cap, &pos, 1, cd, cdp);
    }

    /* Speech audio (id=5) */
    sl = build_audio_av_channel(scratch, sizeof(scratch), 1 /* SPEECH */);
    {
        uint8_t cd[256];
        size_t cdp = build_channel_descriptor(cd, sizeof(cd), 5, 3, scratch, sl);
        pb_w_submsg(p, cap, &pos, 1, cd, cdp);
    }

    /* System audio (id=6) — beeps, system sounds. openauto advertises this
     * via SystemAudioService; without it gearhead may treat the head unit
     * as audio-incomplete and stall before MediaStartIndication. */
    sl = build_audio_av_channel(scratch, sizeof(scratch), 2 /* SYSTEM */);
    {
        uint8_t cd[256];
        size_t cdp = build_channel_descriptor(cd, sizeof(cd), 6, 3, scratch, sl);
        pb_w_submsg(p, cap, &pos, 1, cd, cdp);
    }

    /* Microphone — InputStreamChannel sits at field 5 of ChannelDescriptor.
     * Headunit-revived assigns id=7 to MIC; we follow the same numbering so
     * that gearhead's expected channel layout matches. */
    sl = build_mic_input_channel(scratch, sizeof(scratch));
    {
        uint8_t cd[128];
        size_t cdp = build_channel_descriptor(cd, sizeof(cd), 7, 5, scratch, sl);
        pb_w_submsg(p, cap, &pos, 1, cd, cdp);
    }

    *out_len = pos;
    return ESP_OK;
}
#endif /* SDR_MODE == SDR_MODE_LEGACY */

/* ---------- Encrypted send/recv helpers ---------- */

static esp_err_t send_encrypted(int sock, aa_tls_t *tls,
                                aa_channel_id_t channel,
                                uint16_t msg_id,
                                const uint8_t *body, size_t body_len,
                                uint8_t *cipher_buf, size_t cipher_cap)
{
    /* plaintext = [msg_id BE16][body] */
    uint8_t *plain = malloc(2 + body_len);
    if (!plain) return ESP_ERR_NO_MEM;
    plain[0] = (uint8_t)(msg_id >> 8);
    plain[1] = (uint8_t)(msg_id & 0xFF);
    if (body_len) memcpy(plain + 2, body, body_len);

    size_t cipher_len = 0;
    esp_err_t err = aa_tls_encrypt(tls, plain, 2 + body_len,
                                   cipher_buf, cipher_cap, &cipher_len);
    free(plain);
    if (err != ESP_OK) return err;

    /* aasdk's "MessageType" field in the flag byte:
     *   - SPECIFIC (0) for everything on the Control channel
     *     (Version, SD, AuthComplete, AudioFocus, NavFocus, Ping...)
     *   - CONTROL  (4) for channel-management messages on AV/Sensor/Input
     *     channels (ChannelOpenResponse, AVSetupResponse, SensorStart...)
     *   - SPECIFIC (0) for data on those channels (video frames etc.)
     *
     * Misnomer alert: the bit is named "CONTROL" but it does NOT mean
     * "this is on the control channel" — it means the opposite. */
    uint8_t flags = AA_FRAME_FLAG_BULK | AA_FRAME_FLAG_ENCRYPT;
    if (channel != AA_CHANNEL_CONTROL) flags |= AA_FRAME_FLAG_CONTROL;
    ESP_LOGI(TAG, "tx ch=%d msg=0x%04x plain=%u cipher=%u",
             channel, msg_id, (unsigned)body_len, (unsigned)cipher_len);
    return aa_frame_send_raw(sock, channel, flags, cipher_buf, cipher_len);
}

/* Decrypt one received frame's payload. Returns the inner [msg_id][body]. */
static esp_err_t recv_decrypted(int sock, aa_tls_t *tls,
                                aa_channel_id_t *out_ch,
                                uint8_t *cipher_buf, size_t cipher_cap,
                                uint8_t *plain_buf, size_t plain_cap,
                                size_t *plain_len)
{
    uint8_t flags;
    size_t cipher_len;
    esp_err_t err = aa_frame_recv(sock, out_ch, &flags,
                                  cipher_buf, cipher_cap, &cipher_len);
    if (err != ESP_OK) return err;
    if (!(flags & AA_FRAME_FLAG_ENCRYPT)) {
        ESP_LOGW(TAG, "unexpected plaintext frame post-auth (flags 0x%02x)", flags);
        return ESP_ERR_INVALID_STATE;
    }
    return aa_tls_decrypt(tls, cipher_buf, cipher_len, plain_buf, plain_cap, plain_len);
}

/* ---------- Message handlers ---------- */

static void log_sd_request(const uint8_t *body, size_t len)
{
    /* Walk fields, print everything we can recognise. The body is much
     * bigger than the documented 2-field aasdk proto — modern gearhead
     * stuffs a lot of capability data in here, hence the verbose dump. */
    size_t pos = 0;
    pb_field_t f;
    ESP_LOGI(TAG, "ServiceDiscoveryRequest %u bytes:", (unsigned)len);
    while (pb_read_field(body, len, &pos, &f)) {
        if (f.wire == 0) {
            ESP_LOGI(TAG, "  field %u (varint) = %llu", (unsigned)f.field, (unsigned long long)f.varint);
        } else if (f.wire == 2) {
            /* Try to print as ASCII if printable, otherwise hex. */
            bool printable = true;
            for (size_t i = 0; i < f.len && i < 80; i++) {
                if (f.p[i] < 0x20 || f.p[i] >= 0x7F) { printable = false; break; }
            }
            if (printable && f.len < 96) {
                char tmp[96];
                memcpy(tmp, f.p, f.len);
                tmp[f.len] = '\0';
                ESP_LOGI(TAG, "  field %u (str %u) = \"%s\"", (unsigned)f.field, (unsigned)f.len, tmp);
            } else {
                ESP_LOGI(TAG, "  field %u (bytes %u):", (unsigned)f.field, (unsigned)f.len);
                ESP_LOG_BUFFER_HEXDUMP(TAG, f.p, f.len > 64 ? 64 : f.len, ESP_LOG_INFO);
            }
        }
    }
}

static esp_err_t handle_sd_request(int sock, aa_tls_t *tls,
                                   const uint8_t *body, size_t body_len,
                                   uint8_t *cipher_buf, size_t cipher_cap,
                                   uint8_t *scratch, size_t scratch_cap)
{
    log_sd_request(body, body_len);

    size_t resp_len;
    esp_err_t err = build_sd_response(scratch, scratch_cap, &resp_len);
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG, "ServiceDiscoveryResponse %u bytes", (unsigned)resp_len);

    return send_encrypted(sock, tls, AA_CHANNEL_CONTROL, AA_MSG_SD_RESPONSE,
                          scratch, resp_len, cipher_buf, cipher_cap);
}

static esp_err_t handle_channel_open(int sock, aa_tls_t *tls,
                                     aa_channel_id_t reply_channel,
                                     const uint8_t *body, size_t body_len,
                                     uint8_t *cipher_buf, size_t cipher_cap)
{
    /* ChannelOpenRequest{ priority=1, channel_id=2 } — log and ack.
     * Response goes back on the SAME channel, with the CONTROL flag set
     * (handled by send_encrypted automatically for non-control channels). */
    size_t pos = 0;
    pb_field_t f;
    uint32_t prio = 0, ch = 0xFF;
    while (pb_read_field(body, body_len, &pos, &f)) {
        if (f.wire == 0) {
            if (f.field == 1) prio = (uint32_t)f.varint;
            else if (f.field == 2) ch = (uint32_t)f.varint;
        }
    }
    ESP_LOGI(TAG, "ChannelOpenRequest channel=%u priority=%u (replying on ch=%d)",
             (unsigned)ch, (unsigned)prio, reply_channel);

    /* ChannelOpenResponse{ status = OK (0) } — field 1, varint. */
    uint8_t resp[4];
    size_t  rp = 0;
    pb_w_uint32(resp, sizeof(resp), &rp, 1, 0 /* STATUS_OK */);
    esp_err_t err = send_encrypted(sock, tls, reply_channel, AA_MSG_CHANNEL_OPEN_RESP,
                                   resp, rp, cipher_buf, cipher_cap);
    if (err != ESP_OK) return err;

    /* Sensor channel: emit DRIVING_STATUS=UNRESTRICTED immediately, mirroring
     * headunit-revived's hu_handle_ChannelOpenRequest behaviour. Some gearhead
     * versions read this *before* sending SensorStartRequest and will stall
     * if it isn't already there. */
    if ((int)reply_channel == 2 /* sensor */) {
        uint8_t evt[8];
        size_t  ep = 0;
        uint8_t inner[4];
        size_t  ip = 0;
        pb_w_uint32(inner, sizeof(inner), &ip, 1, 0 /* UNRESTRICTED */);
        pb_w_submsg(evt, sizeof(evt), &ep, 13 /* driving_status */, inner, ip);
        ESP_LOGI(TAG, "SensorEvent ch=2 DRIVING_STATUS=UNRESTRICTED (post-open)");
        err = send_encrypted(sock, tls, reply_channel, AA_MSG_SENSOR_EVENT,
                             evt, ep, cipher_buf, cipher_cap);
    }
    return err;
}

static esp_err_t handle_ping(int sock, aa_tls_t *tls,
                             const uint8_t *body, size_t body_len,
                             uint8_t *cipher_buf, size_t cipher_cap)
{
    /* PingRequest has a timestamp (int64, field 1). Echo it back. */
    size_t pos = 0;
    pb_field_t f;
    uint64_t ts = 0;
    while (pb_read_field(body, body_len, &pos, &f)) {
        if (f.wire == 0 && f.field == 1) ts = f.varint;
    }
    uint8_t resp[16];
    size_t  rp = 0;
    pb_w_uint64(resp, sizeof(resp), &rp, 1, ts);
    return send_encrypted(sock, tls, AA_CHANNEL_CONTROL, AA_MSG_PING_RESPONSE,
                          resp, rp, cipher_buf, cipher_cap);
}

/* Send a ChannelOpenRequest on a given channel id. Some newer AAP
 * implementations have the head-unit *initiate* channel opens; we try this
 * once after AudioFocus to see if gearhead is waiting on us. */
__attribute__((unused))
static esp_err_t send_channel_open_request(int sock, aa_tls_t *tls,
                                           uint32_t target_channel,
                                           uint8_t *cipher_buf, size_t cipher_cap)
{
    /* ChannelOpenRequest{ priority=0, channel_id=target } */
    uint8_t body[16];
    size_t  bp = 0;
    pb_w_uint32(body, sizeof(body), &bp, 1, 0);              /* priority */
    pb_w_uint32(body, sizeof(body), &bp, 2, target_channel); /* id */

    /* Sent on the channel being opened. */
    return send_encrypted(sock, tls, (aa_channel_id_t)target_channel,
                          AA_MSG_CHANNEL_OPEN_REQ,
                          body, bp, cipher_buf, cipher_cap);
}

/* Map channel id → channel kind. Tied to the channel layout in build_sd_response:
 *   1 = touch input, 2 = sensor, 3 = video AV, 4 = audio media AV,
 *   5 = audio speech AV, 6 = audio system AV, 7 = mic input-stream. */
typedef enum {
    CH_KIND_UNKNOWN = 0,
    CH_KIND_TOUCH,
    CH_KIND_SENSOR,
    CH_KIND_AV_VIDEO,
    CH_KIND_AV_AUDIO,
    CH_KIND_MIC,
} ch_kind_t;

static ch_kind_t channel_kind(aa_channel_id_t ch)
{
    switch ((int)ch) {
    case 1: return CH_KIND_TOUCH;
    case 2: return CH_KIND_SENSOR;
    case 3: return CH_KIND_AV_VIDEO;
    case 4: return CH_KIND_AV_AUDIO;
    case 5: return CH_KIND_AV_AUDIO;
    case 6: return CH_KIND_AV_AUDIO;
    case 7: return CH_KIND_MIC;
    default: return CH_KIND_UNKNOWN;
    }
}

/* SensorStartRequest{ type, refresh_interval } → SensorStartResponse{ status=OK }
 * followed by an initial SensorEvent for the requested sensor type.
 *
 * The follow-up event mirrors openauto's SensorService::onSensorStartRequest:
 * gearhead won't start projection until it has seen a baseline DRIVING_STATUS
 * (=UNRESTRICTED) and NIGHT_MODE (=false). Without these the connection sits
 * idle on keepalive pings — exactly what we observed before this fix. */
static esp_err_t handle_sensor_start(int sock, aa_tls_t *tls,
                                     aa_channel_id_t ch,
                                     const uint8_t *body, size_t body_len,
                                     uint8_t *cipher_buf, size_t cipher_cap)
{
    size_t pos = 0;
    pb_field_t f;
    uint32_t type = 0;
    uint64_t interval = 0;
    while (pb_read_field(body, body_len, &pos, &f)) {
        if (f.wire == 0 && f.field == 1) type = (uint32_t)f.varint;
        else if (f.wire == 0 && f.field == 2) interval = f.varint;
    }
    ESP_LOGI(TAG, "SensorStartRequest ch=%d type=%u interval=%llu",
             ch, (unsigned)type, (unsigned long long)interval);

    uint8_t resp[4];
    size_t  rp = 0;
    pb_w_uint32(resp, sizeof(resp), &rp, 1, 0 /* STATUS_OK */);
    esp_err_t err = send_encrypted(sock, tls, ch, AA_MSG_SENSOR_START_RESP,
                                   resp, rp, cipher_buf, cipher_cap);
    if (err != ESP_OK) return err;

    /* SensorEvent for the type just started. Field numbers come from
     * hu.proto: driving_status=13, night_mode=10. */
    uint8_t evt[16];
    size_t  ep = 0;
    if (type == 13 /* DRIVING_STATUS */) {
        uint8_t inner[4];
        size_t  ip = 0;
        pb_w_uint32(inner, sizeof(inner), &ip, 1, 0 /* UNRESTRICTED */);
        pb_w_submsg(evt, sizeof(evt), &ep, 13, inner, ip);
    } else if (type == 10 /* NIGHT_DATA */) {
        uint8_t inner[4];
        size_t  ip = 0;
        pb_w_bool(inner, sizeof(inner), &ip, 1, false);
        pb_w_submsg(evt, sizeof(evt), &ep, 10, inner, ip);
    } else if (type == 1 /* LOCATION */) {
        /* Initial dummy LocationData. openauto skips this (relies on real
         * GPS), but our gearhead seems to wait ~2s after SensorStart(type=1)
         * before continuing — likely it expects at least one location fix.
         * Send accuracy=10000 m to mark "approximate / no real GPS yet". */
        uint8_t loc[16];
        size_t  lp = 0;
        pb_w_uint32(loc, sizeof(loc), &lp, 4, 10000); /* accuracy in mm? mm-deg? phone ignores rough fixes */
        pb_w_submsg(evt, sizeof(evt), &ep, 1 /* location_data */, loc, lp);
    } else {
        return ESP_OK;
    }
    ESP_LOGI(TAG, "SensorEvent ch=%d type=%u (initial)", ch, (unsigned)type);
    return send_encrypted(sock, tls, ch, AA_MSG_SENSOR_EVENT,
                          evt, ep, cipher_buf, cipher_cap);
}

/* BindingRequest{ scan_codes[] } → BindingResponse{ status=OK }. */
static esp_err_t handle_input_binding(int sock, aa_tls_t *tls,
                                      aa_channel_id_t ch,
                                      const uint8_t *body, size_t body_len,
                                      uint8_t *cipher_buf, size_t cipher_cap)
{
    /* scan_codes is `repeated int32` — wire type 0 (one varint per code) or
     * wire type 2 if packed. Just count for the log. */
    size_t pos = 0;
    pb_field_t f;
    int n_codes = 0;
    while (pb_read_field(body, body_len, &pos, &f)) {
        if (f.field != 1) continue;
        if (f.wire == 0) n_codes++;
        else if (f.wire == 2) n_codes += (int)f.len; /* upper bound for packed */
    }
    ESP_LOGI(TAG, "BindingRequest ch=%d scan_codes~%d", ch, n_codes);

    uint8_t resp[4];
    size_t  rp = 0;
    pb_w_uint32(resp, sizeof(resp), &rp, 1, 0 /* STATUS_OK */);
    return send_encrypted(sock, tls, ch, AA_MSG_INPUT_BINDING_RESP,
                          resp, rp, cipher_buf, cipher_cap);
}

/* MediaSetupRequest{ type } → MediaSetupResponse{ media_status=MEDIA_STATUS_2,
 * max_unacked=1, configs=[0] }.
 *
 * `configs` selects which of our advertised configs the channel will use —
 * we only have one (index 0) so that's what we ack. openauto's VideoService
 * AND every AudioService both add_configs(0) unconditionally; without it
 * gearhead waits and just keepalive-pings.
 *
 * For the video channel we also follow openauto's flow: immediately after
 * the setup response, push VideoFocusIndication{FOCUSED, unrequested=false}
 * so gearhead knows the head unit has the screen ready. Without this push
 * gearhead never sends MediaStartIndication and projection never begins. */
static esp_err_t handle_av_setup(int sock, aa_tls_t *tls,
                                 aa_channel_id_t ch,
                                 const uint8_t *body, size_t body_len,
                                 uint8_t *cipher_buf, size_t cipher_cap)
{
    size_t pos = 0;
    pb_field_t f;
    uint32_t type = 0;
    while (pb_read_field(body, body_len, &pos, &f)) {
        if (f.wire == 0 && f.field == 1) type = (uint32_t)f.varint;
    }
    ESP_LOGI(TAG, "MediaSetupRequest ch=%d type=%u", ch, (unsigned)type);

    uint8_t resp[12];
    size_t  rp = 0;
    pb_w_uint32(resp, sizeof(resp), &rp, 1, 2 /* MEDIA_STATUS_2 */);
    pb_w_uint32(resp, sizeof(resp), &rp, 2, 1 /* max_unacked */);
    pb_w_uint32(resp, sizeof(resp), &rp, 3, 0 /* configs[0] = use config 0 */);
    esp_err_t err = send_encrypted(sock, tls, ch, AA_MSG_AV_MEDIA_SETUP_RESP,
                                   resp, rp, cipher_buf, cipher_cap);
    if (err != ESP_OK) return err;

    /* Audio media channel (ch=4): right after MediaSetupResponse, push an
     * UNSOLICITED AudioFocusResponse{state=GAIN} on the control channel.
     *
     * Phone-side gearhead log evidence: after the initial AudioFocusRequest
     * type=RELEASE → state=LOSS exchange, gearhead logs
     *   "CAR.AUDIO.CHANNEL: disabling stream: MEDIA, had focus: false"
     * and the projection stalls at PROJECTION_WINDOW_MANAGER_STARTING.
     *
     * Mazda/openauto get away without this because their AudioManager
     * actually grants focus asynchronously and the AudioFocusHappened
     * callback then sends the GAIN response. We have no real audio manager,
     * so we emit it ourselves once the audio media channel is configured —
     * at that point the head unit is "ready to play media". gearhead checks
     * `unsolicitedResponse=true` and re-enables the MEDIA stream. */
    if ((int)ch == 4 /* audio media */) {
        uint8_t af[4];
        size_t  ap = 0;
        pb_w_uint32(af, sizeof(af), &ap, 1, 1 /* AUDIO_FOCUS_STATE_GAIN */);
        ESP_LOGI(TAG, "AudioFocusResponse GAIN (unsolicited, post-media-setup)");
        err = send_encrypted(sock, tls, AA_CHANNEL_CONTROL,
                             AA_MSG_AUDIO_FOCUS_RESP,
                             af, ap, cipher_buf, cipher_cap);
        if (err != ESP_OK) return err;
    }

    /* Video channel: push VideoFocus{mode=FOCUSED, unrequested=true}.
     * `unrequested=true` because the HU is initiating focus on its own,
     * not in response to a phone-side VideoFocusRequest — matches
     * Mazda/Desktop's MediaSetupComplete→VideoFocusHappened(HEADUNIT) path. */
    if (channel_kind(ch) == CH_KIND_AV_VIDEO) {
        uint8_t vf[8];
        size_t  vp = 0;
        pb_w_uint32(vf, sizeof(vf), &vp, 1, 1 /* VIDEO_FOCUS_MODE_FOCUSED */);
        pb_w_bool  (vf, sizeof(vf), &vp, 2, true);
        ESP_LOGI(TAG, "VideoFocus ch=%d FOCUSED unrequested=true (proactive)", ch);
        err = send_encrypted(sock, tls, ch, AA_MSG_AV_VIDEO_FOCUS_IND,
                             vf, vp, cipher_buf, cipher_cap);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

static esp_err_t handle_audio_focus(int sock, aa_tls_t *tls,
                                    const uint8_t *body, size_t body_len,
                                    uint8_t *cipher_buf, size_t cipher_cap)
{
    /* AudioFocusType: NONE=0, GAIN=1, GAIN_TRANSIENT=2, GAIN_NAVI=3, RELEASE=4
     * AudioFocusState: NONE=0, GAIN=1, ..., LOSS=3, ... */
    size_t pos = 0;
    pb_field_t f;
    uint32_t type = 0;
    while (pb_read_field(body, body_len, &pos, &f)) {
        if (f.wire == 0 && f.field == 1) type = (uint32_t)f.varint;
    }

    /* RELEASE(4) → LOSS(3), else → GAIN(1). Confirmed by experiment:
     * answering GAIN to a RELEASE makes gearhead spam AudioFocusRequest at
     * ~30 ms intervals because the response contradicts the request's intent.
     * LOSS keeps the phone quiet — that's the wire-correct mapping. */
    uint32_t state = (type == 4) ? 3 /* LOSS */ : 1 /* GAIN */;
    ESP_LOGI(TAG, "AudioFocusRequest type=%u → state=%u", (unsigned)type, (unsigned)state);

    uint8_t resp[4];
    size_t  rp = 0;
    pb_w_uint32(resp, sizeof(resp), &rp, 1, state);
    return send_encrypted(sock, tls, AA_CHANNEL_CONTROL,
                          AA_MSG_AUDIO_FOCUS_RESP,
                          resp, rp, cipher_buf, cipher_cap);
}

static esp_err_t handle_nav_focus(int sock, aa_tls_t *tls,
                                  const uint8_t *body, size_t body_len,
                                  uint8_t *cipher_buf, size_t cipher_cap)
{
    (void)body; (void)body_len;
    /* NavigationFocusResponse{ type = 2 (NAVIGATION) } — close enough for handshake. */
    uint8_t resp[4];
    size_t  rp = 0;
    pb_w_uint32(resp, sizeof(resp), &rp, 1, 2);
    return send_encrypted(sock, tls, AA_CHANNEL_CONTROL, AA_MSG_NAV_FOCUS_RESP,
                          resp, rp, cipher_buf, cipher_cap);
}

/* ---------- Main loop ---------- */

/* Send a PingRequest with current timestamp on the control channel. Used
 * as a liveness probe — if gearhead is still listening it should answer
 * with PingResponse echoing the timestamp. */
static esp_err_t send_ping(int sock, aa_tls_t *tls,
                           uint8_t *cipher_buf, size_t cipher_cap)
{
    uint64_t now_us = (uint64_t)esp_timer_get_time();
    uint8_t body[16];
    size_t  bp = 0;
    pb_w_uint64(body, sizeof(body), &bp, 1, now_us);
    return send_encrypted(sock, tls, AA_CHANNEL_CONTROL, AA_MSG_PING_REQUEST,
                          body, bp, cipher_buf, cipher_cap);
}

esp_err_t aa_service_run(int sock, aa_tls_t *tls)
{
    uint8_t *cipher  = malloc(CIPHER_BUF_SIZE);
    uint8_t *plain   = malloc(PLAIN_BUF_SIZE);
    uint8_t *scratch = malloc(SCRATCH_SIZE);
    if (!cipher || !plain || !scratch) {
        free(cipher); free(plain); free(scratch);
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = ESP_OK;

    while (true) {
        aa_channel_id_t ch;
        size_t plen;
        err = recv_decrypted(sock, tls, &ch, cipher, CIPHER_BUF_SIZE,
                             plain, PLAIN_BUF_SIZE, &plen);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "recv_decrypted: %s", esp_err_to_name(err));
            break;
        }
        if (plen < 2) {
            ESP_LOGW(TAG, "short message %u on ch %d", (unsigned)plen, ch);
            continue;
        }
        uint16_t msg_id = ((uint16_t)plain[0] << 8) | plain[1];
        const uint8_t *body = plain + 2;
        size_t body_len = plen - 2;
        ESP_LOGI(TAG, "rx ch=%d msg=0x%04x len=%u", ch, msg_id, (unsigned)body_len);

        if (ch == AA_CHANNEL_CONTROL) {
            switch (msg_id) {
            case AA_MSG_SD_REQUEST:
                err = handle_sd_request(sock, tls, body, body_len,
                                        cipher, CIPHER_BUF_SIZE,
                                        scratch, SCRATCH_SIZE);
                break;
            case AA_MSG_PING_REQUEST:
                err = handle_ping(sock, tls, body, body_len,
                                  cipher, CIPHER_BUF_SIZE);
                break;
            case AA_MSG_AUDIO_FOCUS_REQ:
                err = handle_audio_focus(sock, tls, body, body_len,
                                         cipher, CIPHER_BUF_SIZE);
                break;
            case AA_MSG_NAV_FOCUS_REQ:
                err = handle_nav_focus(sock, tls, body, body_len,
                                       cipher, CIPHER_BUF_SIZE);
                break;
            default:
                ESP_LOGW(TAG, "control msg 0x%04x — not handled yet", msg_id);
                break;
            }
        } else if (msg_id == AA_MSG_CHANNEL_OPEN_REQ) {
            /* ChannelOpenRequest is sent on the channel being opened —
             * we reply on the same channel. */
            err = handle_channel_open(sock, tls, ch, body, body_len,
                                      cipher, CIPHER_BUF_SIZE);
        } else {
            /* Per-channel setup messages. The 0x80xx tag namespace overlaps
             * across channel kinds, so we dispatch on (kind, msg_id). */
            ch_kind_t kind = channel_kind(ch);
            bool handled = false;
            if (kind == CH_KIND_SENSOR && msg_id == AA_MSG_SENSOR_START_REQ) {
                err = handle_sensor_start(sock, tls, ch, body, body_len,
                                          cipher, CIPHER_BUF_SIZE);
                handled = true;
            } else if (kind == CH_KIND_TOUCH && msg_id == AA_MSG_INPUT_BINDING_REQ) {
                err = handle_input_binding(sock, tls, ch, body, body_len,
                                           cipher, CIPHER_BUF_SIZE);
                handled = true;
            } else if ((kind == CH_KIND_AV_VIDEO || kind == CH_KIND_AV_AUDIO) &&
                       msg_id == AA_MSG_AV_MEDIA_SETUP_REQ) {
                err = handle_av_setup(sock, tls, ch, body, body_len,
                                      cipher, CIPHER_BUF_SIZE);
                handled = true;
            }
            if (!handled) {
                ESP_LOGW(TAG, "ch %d msg 0x%04x — not handled yet", ch, msg_id);
                ESP_LOG_BUFFER_HEXDUMP(TAG, body, body_len > 32 ? 32 : body_len, ESP_LOG_DEBUG);
            }
        }

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "handler failed: %s", esp_err_to_name(err));
            break;
        }
    }

    free(cipher); free(plain); free(scratch);
    return err;
}
