#pragma once

/* One JPEG-to-RGB565 entry point for both chips this firmware builds for.
 *
 * ESP32-P4 has a hardware JPEG codec (driver/jpeg_decode.h) — that is what the
 * boot splash and the album-art tile have always used. The ESP32-S3 has none,
 * so the same calls go through the espressif/esp_new_jpeg software decoder.
 * Callers get the decoded dimensions back rather than assuming them, because
 * the two backends pad differently: the P4 engine rounds its output up to a
 * multiple of 16 on both axes, esp_new_jpeg does not.
 *
 * Not a hot path: album art arrives at most once per track, the debug bridge
 * encodes on request. Anything per-frame stays on the native APIs.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bring the backend up. Idempotent; safe to call from any task. */
esp_err_t img_jpeg_init(void);

/* Allocate an output buffer with the alignment and memory caps the backend
 * needs (DMA-capable on the P4, 16-byte aligned everywhere). Free with
 * heap_caps_free(). */
void *img_jpeg_alloc_rgb565(size_t bytes);

/* Decode `src` (len bytes) into `dst` as RGB565 in LVGL's byte order.
 * On success writes the dimensions of the image actually laid into dst —
 * which may be padded wider/taller than the JPEG — to *out_w / *out_h.
 * Returns false if the backend is down, the image is unsupported, or the
 * result would not fit in dst_cap. */
bool img_jpeg_decode_rgb565(const uint8_t *src, size_t len,
                            uint8_t *dst, size_t dst_cap,
                            uint16_t *out_w, uint16_t *out_h);

/* Encode a w*h RGB565 image (LVGL byte order) as baseline JPEG at `quality`
 * (1-100). Writes into dst and reports the bitstream length in *out_len.
 * Used by the debug bridge's `screenshot` command; returns false if the
 * backend is unavailable, in which case the caller should fall back to
 * shipping raw RGB565. */
bool img_jpeg_encode_rgb565(const uint8_t *src, uint16_t w, uint16_t h,
                            int quality, uint8_t *dst, size_t dst_cap,
                            size_t *out_len);

/* Allocate an output buffer for img_jpeg_encode_rgb565. */
void *img_jpeg_alloc_encoded(size_t bytes);

#ifdef __cplusplus
}
#endif
