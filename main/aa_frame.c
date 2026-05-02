#include "aa_frame.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "lwip/sockets.h"

static const char *TAG = "aa_frame";

/* Diagnostic: log every chunk of bytes coming off the wire so we can see
 * whether gearhead is sending anything at all when our parser is blocked
 * waiting for a full frame. */
/* Per-recv chunk logging + hex dump. Useful when bringing up TLS/framing,
 * but at video rates each frame produces 3-5 log lines that take ~10 ms each
 * over the 115200 console — that alone caps throughput at ~10 fps even when
 * the protocol is fine. Flip to 1 only when actively debugging framing. */
#define AA_FRAME_LOG_RAW 0

static int recv_exact(int sock, uint8_t *buf, size_t len)
{
    size_t got = 0;
    while (got < len) {
        int n = recv(sock, buf + got, len - got, 0);
        if (n == 0) {
            return 0;
        }
        if (n < 0) {
            ESP_LOGW(TAG, "recv errno %d", errno);
            return n;
        }
#if AA_FRAME_LOG_RAW
        ESP_LOGI(TAG, "raw recv %d bytes (need %u, have %u)",
                 n, (unsigned)len, (unsigned)(got + n));
        ESP_LOG_BUFFER_HEXDUMP(TAG, buf + got, n > 32 ? 32 : n, ESP_LOG_INFO);
#endif
        got += n;
    }
    return got;
}

esp_err_t aa_frame_send_raw(int sock, aa_channel_id_t channel,
                            uint8_t flags,
                            const uint8_t *payload, size_t payload_len)
{
    if (payload_len > 0xFFFF) {
        return ESP_ERR_INVALID_SIZE;
    }
    size_t total = 4 + payload_len;
    uint8_t hdr[4];
    hdr[0] = (uint8_t)channel;
    hdr[1] = flags;
    hdr[2] = (uint8_t)(payload_len >> 8);
    hdr[3] = (uint8_t)(payload_len & 0xFF);

    uint8_t stackbuf[512];
    uint8_t *frame = stackbuf;
    if (total > sizeof(stackbuf)) {
        frame = malloc(total);
        if (!frame) return ESP_ERR_NO_MEM;
    }
    memcpy(frame, hdr, 4);
    if (payload_len) {
        memcpy(frame + 4, payload, payload_len);
    }
    int n = send(sock, frame, total, 0);
    if (frame != stackbuf) free(frame);
    if (n != (int)total) {
        ESP_LOGE(TAG, "send failed: %d errno %d", n, errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t aa_frame_send_plain(int sock, aa_channel_id_t channel,
                              uint16_t msg_id,
                              const uint8_t *body, size_t body_len)
{
    /* msg_id(2) + body */
    if (body_len > 0xFFFF - 2) {
        return ESP_ERR_INVALID_SIZE;
    }
    size_t payload_len = 2 + body_len;
    uint8_t stackbuf[512];
    uint8_t *payload = stackbuf;
    if (payload_len > sizeof(stackbuf)) {
        payload = malloc(payload_len);
        if (!payload) return ESP_ERR_NO_MEM;
    }
    payload[0] = (uint8_t)(msg_id >> 8);
    payload[1] = (uint8_t)(msg_id & 0xFF);
    if (body_len) {
        memcpy(payload + 2, body, body_len);
    }
    /* PLAIN, SPECIFIC, BULK */
    esp_err_t err = aa_frame_send_raw(sock, channel, AA_FRAME_FLAG_BULK,
                                      payload, payload_len);
    if (payload != stackbuf) free(payload);
    return err;
}

esp_err_t aa_frame_recv(int sock,
                        aa_channel_id_t *out_channel,
                        uint8_t *out_flags,
                        uint8_t *out_payload, size_t out_capacity,
                        size_t *out_payload_len)
{
    uint8_t hdr[2];
    int n = recv_exact(sock, hdr, 2);
    if (n == 0) return ESP_ERR_INVALID_STATE;   /* peer closed */
    if (n < 0) return ESP_FAIL;

    aa_channel_id_t channel = (aa_channel_id_t)hdr[0];
    uint8_t flags = hdr[1];

    /* Wireless Helper sends 16 bytes of 0xFF as a "client gone" sentinel
     * when gearhead drops the proxy. Detect early and return EOF-like. */
    if (hdr[0] == 0xFF && hdr[1] == 0xFF) {
        ESP_LOGW(TAG, "magic-garbage from peer — wireless helper disconnected");
        return ESP_ERR_INVALID_STATE;
    }

    /* FIRST-without-LAST uses an EXTENDED size field: 2-byte fragment size
     * followed by 4-byte total message size = 6 bytes. BULK/MIDDLE/LAST use
     * a SHORT 2-byte size field. Earlier this was 4 bytes which fell apart
     * the moment phone fragmented an H.264 keyframe — we'd swallow only 4
     * of the 6 size bytes, leaving 2 bytes of "total size" stuck at the
     * start of the next frame, garbling every subsequent header. */
    bool is_first_only = (flags & AA_FRAME_FLAG_FIRST) &&
                         !(flags & AA_FRAME_FLAG_LAST);
    size_t size_field_len = is_first_only ? 6 : 2;

    uint8_t size_buf[6];
    n = recv_exact(sock, size_buf, size_field_len);
    if (n == 0) return ESP_ERR_INVALID_STATE;
    if (n < 0) return ESP_FAIL;

    size_t payload_len = ((size_t)size_buf[0] << 8) | size_buf[1];
    /* size_buf[2..5] of FIRST is the 4-byte total message size — useful for
     * pre-sizing the assembly buffer; we don't need it here. */

    if (payload_len > out_capacity) {
        ESP_LOGE(TAG, "payload %u > capacity %u", (unsigned)payload_len, (unsigned)out_capacity);
        return ESP_ERR_NO_MEM;
    }
    if (payload_len) {
        n = recv_exact(sock, out_payload, payload_len);
        if (n == 0) return ESP_ERR_INVALID_STATE;
        if (n < 0) return ESP_FAIL;
    }
    if (out_channel) *out_channel = channel;
    if (out_flags) *out_flags = flags;
    if (out_payload_len) *out_payload_len = payload_len;
    return ESP_OK;
}
