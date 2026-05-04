#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/* H.264 decode pipe for the AAP video channel.
 *
 * Async with a single-slot queue. Recv-loop calls h264_pipe_push() with NAL
 * bytes; the call returns immediately after copying into the queue. A decoder
 * task picks the slot up, decodes, presents on the display, and only then
 * calls the ack callback. Combined with phone-side max_unacked=1 this gives
 * us:
 *
 *   - recv-loop always reads TCP — no FRAMER_WRITER_STALL on the phone;
 *   - phone never has more than one in-flight frame, so the queue stays at
 *     depth ≤ 1 and we never need a ring buffer / drop-recovery;
 *   - ack reflects "frame is on screen", not "frame was received". */

typedef esp_err_t (*h264_pipe_ack_cb_t)(void *ctx);

esp_err_t h264_pipe_init(void);

/* Copy `data..data+len` into the decode queue and arrange for `ack_cb(ack_ctx)`
 * to be called from the decoder task right after the frame has been displayed.
 * Non-blocking — if the queue is already full (shouldn't happen under
 * max_unacked=1), the new frame is dropped and the ack callback is NOT
 * invoked, so the phone keeps waiting on its in-flight frame instead of
 * piling up another one. */
void h264_pipe_push(const uint8_t *data, size_t len,
                    h264_pipe_ack_cb_t ack_cb, void *ack_ctx);
