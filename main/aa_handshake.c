#include "aa_handshake.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "aa_frame.h"
#include "aa_tls.h"
#include "esp_log.h"

static const char *TAG = "aa_hs";

/* AAP version negotiation. Tried 1.7 first (modern), but gearhead then
 * classifies our HU as "Cakewalk-compatible" and routes setup through a
 * new wireless flow we don't implement — FRX gets skipped, projection
 * stalls at PROJECTION_WINDOW_MANAGER_STARTING. Drop to 1.1 (the aasdk
 * legacy default) to opt out of Cakewalk and trigger the legacy FRX
 * "Add this car" UI on the phone. */
#define AA_VERSION_MAJOR 1
#define AA_VERSION_MINOR 1

/* AuthCompleteIndication{ status = Status::OK (0) } in protobuf wire format:
 *   field 1 (varint): tag = (1 << 3) | 0 = 0x08, value = 0x00. */
static const uint8_t AUTH_COMPLETE_OK[] = { 0x08, 0x00 };

static esp_err_t do_version_handshake(int sock)
{
    uint8_t body[4] = {
        (uint8_t)(AA_VERSION_MAJOR >> 8), (uint8_t)(AA_VERSION_MAJOR & 0xFF),
        (uint8_t)(AA_VERSION_MINOR >> 8), (uint8_t)(AA_VERSION_MINOR & 0xFF),
    };
    esp_err_t err = aa_frame_send_plain(sock, AA_CHANNEL_CONTROL,
                                        AA_MSG_VERSION_REQUEST, body, sizeof(body));
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG, "VersionRequest sent");

    aa_channel_id_t ch;
    uint8_t flags;
    uint8_t payload[64];
    size_t  plen;
    err = aa_frame_recv(sock, &ch, &flags, payload, sizeof(payload), &plen);
    if (err != ESP_OK) return err;
    if (ch != AA_CHANNEL_CONTROL || plen < 8) {
        ESP_LOGE(TAG, "unexpected reply: ch %d len %u", ch, (unsigned)plen);
        return ESP_FAIL;
    }
    uint16_t msg_id = ((uint16_t)payload[0] << 8) | payload[1];
    if (msg_id != AA_MSG_VERSION_RESPONSE) {
        ESP_LOGE(TAG, "expected VersionResponse, got 0x%04x", msg_id);
        return ESP_FAIL;
    }
    uint16_t major = ((uint16_t)payload[2] << 8) | payload[3];
    uint16_t minor = ((uint16_t)payload[4] << 8) | payload[5];
    uint16_t status = ((uint16_t)payload[6] << 8) | payload[7];
    ESP_LOGI(TAG, "VersionResponse: %u.%u status %u", major, minor, status);
    if (status != 0) {
        ESP_LOGE(TAG, "version mismatch");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* Each pass-through buffer is 4 KiB which is enough to hold a single TLS
 * record fragment; the whole handshake fits in a few records. */
#define HS_BUF_SIZE 4096

static esp_err_t do_tls_handshake(int sock, aa_tls_t *tls,
                                  uint8_t *out_buf, uint8_t *rx_buf)
{
    size_t out_len;
    bool   done = false;
    int    iter = 0;

    while (!done) {
        iter++;
        esp_err_t err = aa_tls_handshake_step(tls, out_buf, HS_BUF_SIZE, &out_len, &done);
        if (err != ESP_OK) return err;

        if (out_len > 0) {
            ESP_LOGI(TAG, "TLS step %d: tx %u bytes%s", iter, (unsigned)out_len,
                     done ? " (final)" : "");
            err = aa_frame_send_plain(sock, AA_CHANNEL_CONTROL,
                                      AA_MSG_SSL_HANDSHAKE, out_buf, out_len);
            if (err != ESP_OK) return err;
        }

        if (done) break;

        aa_channel_id_t ch;
        uint8_t flags;
        size_t  plen;
        err = aa_frame_recv(sock, &ch, &flags, rx_buf, HS_BUF_SIZE, &plen);
        if (err != ESP_OK) return err;
        if (ch != AA_CHANNEL_CONTROL || plen < 2) {
            ESP_LOGE(TAG, "unexpected during TLS: ch %d len %u", ch, (unsigned)plen);
            return ESP_FAIL;
        }
        uint16_t msg_id = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
        if (msg_id != AA_MSG_SSL_HANDSHAKE) {
            ESP_LOGE(TAG, "expected SSL_HANDSHAKE, got 0x%04x", msg_id);
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "TLS step %d: rx %u bytes", iter, (unsigned)(plen - 2));
        err = aa_tls_feed_rx(tls, rx_buf + 2, plen - 2);
        if (err != ESP_OK) return err;
    }

    ESP_LOGI(TAG, "TLS handshake complete");
    return ESP_OK;
}

esp_err_t aa_handshake_run(int sock, aa_tls_t *tls)
{
    esp_err_t err = do_version_handshake(sock);
    if (err != ESP_OK) return err;

    err = aa_tls_init(tls);
    if (err != ESP_OK) return err;

    /* Two 4 KiB pass-through buffers live on the heap — too big for the stack. */
    uint8_t *out_buf = malloc(HS_BUF_SIZE);
    uint8_t *rx_buf  = malloc(HS_BUF_SIZE);
    if (!out_buf || !rx_buf) {
        free(out_buf); free(rx_buf);
        aa_tls_deinit(tls);
        return ESP_ERR_NO_MEM;
    }

    err = do_tls_handshake(sock, tls, out_buf, rx_buf);
    free(out_buf); free(rx_buf);
    if (err != ESP_OK) {
        aa_tls_deinit(tls);
        return err;
    }

    err = aa_frame_send_plain(sock, AA_CHANNEL_CONTROL,
                              AA_MSG_AUTH_COMPLETE,
                              AUTH_COMPLETE_OK, sizeof(AUTH_COMPLETE_OK));
    if (err != ESP_OK) {
        aa_tls_deinit(tls);
        return err;
    }
    ESP_LOGI(TAG, "AuthComplete sent — auth done");
    return ESP_OK;
}
