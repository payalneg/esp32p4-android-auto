#include "screen_capture.h"

#include <stdio.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "img_jpeg.h"
#include "log_capture.h"
#include "lvgl.h"

static const char *TAG = "screen_cap";

esp_err_t screen_capture_to_file(const char *path, int quality)
{
    if (!path) return ESP_ERR_INVALID_ARG;
    if (quality < 1 || quality > 100) quality = 80;

    lv_disp_t *disp = lv_disp_get_default();
    if (!disp) {
        ESP_LOGW(TAG, "no display — nothing to capture");
        return ESP_ERR_INVALID_STATE;
    }
    const int w = lv_disp_get_hor_res(disp);
    const int h = lv_disp_get_ver_res(disp);
    const size_t raw_bytes = (size_t)w * (size_t)h * 2;

    /* Both buffers live only for this call. The panel-sized one is why: 768 KB
     * on the wide head units, 460 KB on the square board — worth borrowing for
     * a moment, not worth holding between shots. */
    uint8_t *raw = img_jpeg_alloc_rgb565(raw_bytes);
    if (!raw) {
        ESP_LOGE(TAG, "no PSRAM for a %dx%d frame (%u B)", w, h,
                 (unsigned)raw_bytes);
        return ESP_ERR_NO_MEM;
    }
    /* JPEG of a dashboard is far smaller than the frame, but a screen full of
     * map detail compresses poorly — give the encoder half the raw size before
     * it has to give up. */
    const size_t jpg_cap = raw_bytes / 2;
    uint8_t *jpg = img_jpeg_alloc_encoded(jpg_cap);
    if (!jpg) {
        heap_caps_free(raw);
        ESP_LOGE(TAG, "no PSRAM for the encoder output (%u B)",
                 (unsigned)jpg_cap);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = ESP_FAIL;
    lv_img_dsc_t dsc;
    memset(&dsc, 0, sizeof dsc);

    /* lv_snapshot re-renders the object tree through LVGL's own draw path, so
     * the lock is held for that and released before the encode — the renderer
     * is not kept waiting while we compress. */
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGW(TAG, "display busy — no shot");
        err = ESP_ERR_TIMEOUT;
        goto out;
    }
    lv_res_t res = lv_snapshot_take_to_buf(lv_scr_act(), LV_IMG_CF_TRUE_COLOR,
                                           &dsc, raw, raw_bytes);
    bsp_display_unlock();
    if (res != LV_RES_OK) {
        ESP_LOGW(TAG, "snapshot failed");
        goto out;
    }

    size_t jpg_len = 0;
    if (!img_jpeg_encode_rgb565(raw, (uint16_t)dsc.header.w,
                                (uint16_t)dsc.header.h, quality,
                                jpg, jpg_cap, &jpg_len) || jpg_len == 0) {
        ESP_LOGW(TAG, "jpeg encode failed");
        goto out;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "cannot open %s", path);
        goto out;
    }
    const size_t written = fwrite(jpg, 1, jpg_len, f);
    fclose(f);
    if (written != jpg_len) {
        /* A short write is a full filesystem, and a truncated JPEG is worse
         * than none: the app would attach a picture that will not open. */
        ESP_LOGE(TAG, "short write to %s (%u of %u B) — removing",
                 path, (unsigned)written, (unsigned)jpg_len);
        remove(path);
        goto out;
    }

    ESP_LOGI(TAG, "%dx%d -> %s (%u B)", w, h, path, (unsigned)jpg_len);
    err = ESP_OK;

out:
    heap_caps_free(jpg);
    heap_caps_free(raw);
    return err;
}

/* How much of the log ring goes into a report. The ring itself is larger, but
 * a report is meant to be read and sent over BLE: the newest lines are the ones
 * that explain the fault, and log_capture_snapshot clips the oldest. */
#define REPORT_LOG_BYTES (64 * 1024)

esp_err_t report_log_to_file(void)
{
    char *buf = heap_caps_malloc(REPORT_LOG_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) {
        ESP_LOGE(TAG, "no PSRAM for a %u B log dump", (unsigned)REPORT_LOG_BYTES);
        return ESP_ERR_NO_MEM;
    }
    const size_t n = log_capture_snapshot(buf, REPORT_LOG_BYTES);

    esp_err_t err = ESP_FAIL;
    FILE *f = fopen(REPORT_LOG_PATH, "wb");
    if (!f) {
        ESP_LOGE(TAG, "cannot open %s", REPORT_LOG_PATH);
        goto done;
    }
    const size_t written = n ? fwrite(buf, 1, n, f) : 0;
    fclose(f);
    if (written != n) {
        ESP_LOGE(TAG, "short write to %s (%u of %u B)", REPORT_LOG_PATH,
                 (unsigned)written, (unsigned)n);
        goto done;
    }
    ESP_LOGI(TAG, "%u B of log -> %s", (unsigned)n, REPORT_LOG_PATH);
    err = ESP_OK;

done:
    heap_caps_free(buf);
    return err;
}
