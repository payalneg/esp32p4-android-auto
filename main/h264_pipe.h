#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/* H.264 decode pipe for the AAP video channel.
 *
 * Async with a 32-slot queue. Recv-loop calls h264_pipe_push() which copies
 * the bytes in and returns; the decoder task pulls, decodes, presents on
 * display, and only then fires the ack callback.
 *
 * Ack-on-display matters: it makes gearhead respect our actual decode rate
 * via max_unacked. Acking on receive made it free-run at 22 fps, the
 * decoder couldn't keep up at 45 ms/frame, and the queue back-pressured
 * the recv-loop into TCP STALL after ~23 s. With ack-on-display the queue
 * stays light because the phone naturally paces itself.
 *
 * Queue=32 absorbs IDR-burst spikes (a single IDR can take 100+ ms to
 * decode while the phone bursts a few P-frames before settling); the deep
 * buffer means push never blocks and TCP keeps draining. Memory cost is
 * ~32 × 100 KiB peak ≈ 3 MB in PSRAM — fine. */

typedef esp_err_t (*h264_pipe_ack_cb_t)(void *ctx);

esp_err_t h264_pipe_init(void);

/* Turn on per-frame CRC logging in the decoder (own decoder only; no-op on
 * the prebuilt). Used by the phone-free `playclip` self-test. */
void h264_pipe_set_verify(bool on);

/* Enqueue `data..data+len`. Decoder task fires ack_cb(ack_ctx) once the
 * frame has been displayed. Blocks if the queue is somehow full (never
 * observed under normal operation; logs a warning if it ever happens). */
void h264_pipe_push(const uint8_t *data, size_t len,
                    h264_pipe_ack_cb_t ack_cb, void *ack_ctx);

/* Present only every Nth decoded picture (1 = all of them). Decoding is
 * unaffected — this halves the display stage's work at the cost of motion
 * smoothness. Defaults to CONFIG_AA_RENDER_EVERY. */
void    h264_pipe_set_render_every(uint8_t n);
uint8_t h264_pipe_get_render_every(void);
