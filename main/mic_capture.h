#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

/* On-board microphone capture for the Android Auto mic channel.
 *
 * Hardware is board-specific and lives behind the BSP:
 *   - Waveshare 4.3: two MEMS mics on the ES7210 ADC inputs MIC1 and MIC3 (TDM read);
 *     we keep MIC1 only.
 *   - Guition JC4880: mic on the ES8311's own ADC (mono).
 * Output is always what the AA InputStreamChannel advertises: 16 kHz,
 * 16-bit signed little-endian, mono, delivered in fixed chunks from a
 * dedicated task. The callback runs on that task — it may block briefly
 * (it sends over TLS) but must not hold the chunk past the call.
 *
 * Compiled to no-op stubs (start → ESP_ERR_NOT_SUPPORTED) when
 * CONFIG_AA_MIC_ENABLE is off, so the caller doesn't need its own #if. */

#define MIC_CAPTURE_SAMPLE_RATE    16000
#define MIC_CAPTURE_CHUNK_MS       40
#define MIC_CAPTURE_CHUNK_SAMPLES  (MIC_CAPTURE_SAMPLE_RATE * MIC_CAPTURE_CHUNK_MS / 1000)

typedef void (*mic_pcm_cb_t)(const int16_t *pcm, size_t samples,
                             uint64_t timestamp_us, void *ctx);

/* Initialise the codec on first use, open the ADC and start streaming
 * chunks to cb. Idempotent while running. */
esp_err_t mic_capture_start(mic_pcm_cb_t cb, void *ctx);

/* Stop streaming and close the ADC (codec stays initialised for the next
 * start). Blocks until the capture task has exited — at most a few chunk
 * periods plus one TLS send. Safe to call when not running. */
void mic_capture_stop(void);

bool mic_capture_is_running(void);
