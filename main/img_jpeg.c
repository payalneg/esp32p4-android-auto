#include "img_jpeg.h"

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"

/* Both halves of this file need a JPEG encoder as well as a decoder; the P4
 * has both in one peripheral, esp_new_jpeg ships them as two APIs. The
 * SOC_JPEG_DECODE_SUPPORTED switch below therefore selects the whole
 * backend, not just the decoder. */

static const char *TAG = "img_jpeg";

#if SOC_JPEG_DECODE_SUPPORTED
/* ---------------------------------------------------------------------------
 * ESP32-P4: hardware JPEG engine.
 * ------------------------------------------------------------------------ */
#include "driver/jpeg_decode.h"

/* The engine rounds its output up to whole 16x16 blocks. */
#define ALIGN16(v)  (((v) + 15u) & ~15u)

static jpeg_decoder_handle_t s_engine;
static uint8_t              *s_scratch;      /* DMA-aligned copy of the input */
static size_t                s_scratch_cap;

esp_err_t img_jpeg_init(void)
{
    if (s_engine) return ESP_OK;
    jpeg_decode_engine_cfg_t cfg = {
        .intr_priority = 0,
        .timeout_ms    = 200,
    };
    esp_err_t r = jpeg_new_decoder_engine(&cfg, &s_engine);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "jpeg engine init failed: %s", esp_err_to_name(r));
    }
    return r;
}

void *img_jpeg_alloc_rgb565(size_t bytes)
{
    /* The engine writes its output over DMA. */
    return heap_caps_aligned_calloc(64, 1, bytes,
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
}

static bool ensure_scratch(size_t need)
{
    if (need <= s_scratch_cap) return true;
    if (s_scratch) heap_caps_free(s_scratch);
    /* JPEG hardware reads via DMA — input must be cache-aligned. PSRAM is
     * fine, the engine internally bursts through cache. */
    size_t alloc = (need + 1023u) & ~1023u;
    s_scratch = heap_caps_aligned_calloc(64, 1, alloc,
                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    s_scratch_cap = s_scratch ? alloc : 0;
    return s_scratch != NULL;
}

bool img_jpeg_decode_rgb565(const uint8_t *src, size_t len,
                            uint8_t *dst, size_t dst_cap,
                            uint16_t *out_w, uint16_t *out_h)
{
    if (!s_engine || !src || !dst || len == 0) return false;
    if (!ensure_scratch(len)) return false;
    memcpy(s_scratch, src, len);

    jpeg_decode_picture_info_t info = {0};
    if (jpeg_decoder_get_info(s_scratch, len, &info) != ESP_OK) return false;
    uint32_t w = ALIGN16(info.width);
    uint32_t h = ALIGN16(info.height);
    if ((size_t)w * h * 2u > dst_cap) {
        ESP_LOGW(TAG, "%ux%u (padded %ux%u) needs %u B > %u B buffer",
                 (unsigned)info.width, (unsigned)info.height,
                 (unsigned)w, (unsigned)h,
                 (unsigned)(w * h * 2u), (unsigned)dst_cap);
        return false;
    }

    jpeg_decode_cfg_t cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        /* BGR matches what the ST7701 / LVGL RGB565 pipeline expects on
         * these boards — RGB order would surface as a red/blue swap. */
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
    };
    uint32_t out_size = 0;
    esp_err_t r = jpeg_decoder_process(s_engine, &cfg, s_scratch, len,
                                       dst, dst_cap, &out_size);
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "decode failed: %s", esp_err_to_name(r));
        return false;
    }
    if (out_w) *out_w = (uint16_t)w;
    if (out_h) *out_h = (uint16_t)h;
    return true;
}

/* ---- encoder (hardware) ---- */
#include "driver/jpeg_encode.h"

static jpeg_encoder_handle_t s_encoder;

void *img_jpeg_alloc_encoded(size_t bytes)
{
    jpeg_encode_memory_alloc_cfg_t mcfg = {
        .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER,
    };
    size_t got = 0;
    return jpeg_alloc_encoder_mem(bytes, &mcfg, &got);
}

bool img_jpeg_encode_rgb565(const uint8_t *src, uint16_t w, uint16_t h,
                            int quality, uint8_t *dst, size_t dst_cap,
                            size_t *out_len)
{
    if (!src || !dst) return false;
    if (!s_encoder) {
        jpeg_encode_engine_cfg_t cfg = { .intr_priority = 0, .timeout_ms = 1000 };
        if (jpeg_new_encoder_engine(&cfg, &s_encoder) != ESP_OK) {
            s_encoder = NULL;
            return false;
        }
    }
    jpeg_encode_cfg_t ecfg = {
        .width         = w,
        .height        = h,
        .src_type      = JPEG_ENCODE_IN_FORMAT_RGB565,
        .sub_sample    = JPEG_DOWN_SAMPLING_YUV420,
        .image_quality = (uint32_t)quality,
    };
    uint32_t got = 0;
    esp_err_t e = jpeg_encoder_process(s_encoder, &ecfg, src,
                                       (uint32_t)w * h * 2u,
                                       dst, dst_cap, &got);
    if (e != ESP_OK || got == 0) {
        ESP_LOGW(TAG, "encode failed: %s", esp_err_to_name(e));
        return false;
    }
    if (out_len) *out_len = got;
    return true;
}

#else /* !SOC_JPEG_DECODE_SUPPORTED */
/* ---------------------------------------------------------------------------
 * ESP32-S3: espressif/esp_new_jpeg, software.
 *
 * The handle carries per-image state (the parsed header), so it is opened and
 * closed per decode instead of being kept alive. At the rate art arrives that
 * costs nothing and keeps the decoder from holding scratch between tracks.
 * ------------------------------------------------------------------------ */
#include "esp_jpeg_common.h"
#include "esp_jpeg_dec.h"

esp_err_t img_jpeg_init(void)
{
    return ESP_OK;   /* nothing to bring up */
}

void *img_jpeg_alloc_rgb565(size_t bytes)
{
    /* esp_new_jpeg requires a 16-byte-aligned output buffer. */
    return heap_caps_aligned_calloc(16, 1, bytes,
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

bool img_jpeg_decode_rgb565(const uint8_t *src, size_t len,
                            uint8_t *dst, size_t dst_cap,
                            uint16_t *out_w, uint16_t *out_h)
{
    if (!src || !dst || len == 0) return false;

    jpeg_dec_config_t cfg = DEFAULT_JPEG_DEC_CONFIG();
    /* LVGL stores RGB565 little-endian when LV_COLOR_16_SWAP is 0. */
    cfg.output_type = JPEG_PIXEL_FORMAT_RGB565_LE;

    jpeg_dec_handle_t dec = NULL;
    if (jpeg_dec_open(&cfg, &dec) != JPEG_ERR_OK || !dec) {
        ESP_LOGW(TAG, "decoder open failed");
        return false;
    }

    bool ok = false;
    jpeg_dec_io_t io = {
        .inbuf     = (uint8_t *)src,
        .inbuf_len = (int)len,
        .outbuf    = dst,
    };
    jpeg_dec_header_info_t hdr = {0};

    if (jpeg_dec_parse_header(dec, &io, &hdr) != JPEG_ERR_OK) {
        ESP_LOGW(TAG, "bad JPEG header");
        goto out;
    }
    int need = 0;
    if (jpeg_dec_get_outbuf_len(dec, &need) != JPEG_ERR_OK || need <= 0) {
        goto out;
    }
    if ((size_t)need > dst_cap) {
        ESP_LOGW(TAG, "%ux%u needs %d B > %u B buffer",
                 (unsigned)hdr.width, (unsigned)hdr.height,
                 need, (unsigned)dst_cap);
        goto out;
    }
    if (jpeg_dec_process(dec, &io) != JPEG_ERR_OK) {
        ESP_LOGW(TAG, "decode failed");
        goto out;
    }
    if (out_w) *out_w = hdr.width;
    if (out_h) *out_h = hdr.height;
    ok = true;

out:
    jpeg_dec_close(dec);
    return ok;
}

/* ---- encoder (software) ---- */
#include "esp_jpeg_enc.h"

void *img_jpeg_alloc_encoded(size_t bytes)
{
    return heap_caps_aligned_calloc(16, 1, bytes,
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

bool img_jpeg_encode_rgb565(const uint8_t *src, uint16_t w, uint16_t h,
                            int quality, uint8_t *dst, size_t dst_cap,
                            size_t *out_len)
{
    if (!src || !dst) return false;

    jpeg_enc_config_t cfg = DEFAULT_JPEG_ENC_CONFIG();
    cfg.width       = w;
    cfg.height      = h;
    cfg.src_type    = JPEG_PIXEL_FORMAT_RGB565_LE;
    cfg.subsampling = JPEG_SUBSAMPLE_420;
    cfg.quality     = (uint8_t)quality;

    jpeg_enc_handle_t enc = NULL;
    if (jpeg_enc_open(&cfg, &enc) != JPEG_ERR_OK || !enc) {
        ESP_LOGW(TAG, "encoder open failed");
        return false;
    }
    int got = 0;
    jpeg_error_t e = jpeg_enc_process(enc, src, (int)((size_t)w * h * 2u),
                                      dst, (int)dst_cap, &got);
    jpeg_enc_close(enc);
    if (e != JPEG_ERR_OK || got <= 0) {
        ESP_LOGW(TAG, "encode failed (%d)", (int)e);
        return false;
    }
    if (out_len) *out_len = (size_t)got;
    return true;
}

#endif /* SOC_JPEG_DECODE_SUPPORTED */
