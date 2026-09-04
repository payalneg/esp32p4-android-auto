#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* H.264 baseline decoder for the AA video pipeline — Hantro's h264bsd built
 * from source (components/h264dec/h264bsd) behind a small adapter. Same
 * stream contract as esp_h264's SW decoder: feed Annex-B byte-stream chunks
 * (one AA video message = one or more NAL units), get a picture back when a
 * frame completes. Output is I420 planar in the decoder's own DPB buffer and
 * stays valid until the next call that returns a picture. */

typedef struct h264dec h264dec_t;

typedef enum {
    H264DEC_OK = 0,     /* consumed headers / part of a picture, nothing to show */
    H264DEC_PIC,        /* *pic filled */
    H264DEC_ERROR,      /* stream error; decoder resyncs at the next IDR */
    H264DEC_NOMEM,
} h264dec_status_t;

typedef struct {
    const uint8_t *data;    /* Y (w*h), then Cb, then Cr (w*h/4 each); 16-byte aligned */
    uint32_t width, height;
    /* Bumps whenever the picture's pixels differ from the previous output
     * picture. Stays put when every macroblock was a P_Skip with a zero
     * motion vector out of the previous output — the same pixels again —
     * which is most frames of a static screen. The display stage keys its
     * staging cache on this. */
    uint32_t content_id;
    bool     unchanged;     /* content_id == previous picture's */
    /* Frame sequence number; pair with h264dec_changed_since(). 0 = n/a. */
    uint32_t version;
} h264dec_pic_t;

typedef struct {
    uint32_t pictures;          /* since the last take */
    uint32_t unchanged_pics;
    uint64_t decode_us;         /* everything but the deblocking pass */
    uint64_t deblock_us;        /* h264bsdFilterPicture */
    uint32_t skip_zero_mbs;     /* macroblocks through the P_Skip/zero-MV copy path */
    uint32_t total_mbs;
    uint32_t aliased_pics;      /* whole-picture skips served by a DPB buffer swap */
    uint32_t copied_pics;       /* ... or by one flat copy (more than one ref frame) */
    uint32_t mb_copied;         /* zero-MV skip MBs that had to be copied from the ref */
    uint32_t mb_nocopy;         /* ... that the destination buffer already held */
    /* macroblock-loop breakdown (per picture, µs) */
    uint32_t skip_mb_us, coded_mb_us, loop_us;
    uint32_t skip_mbs, coded_mbs;
} h264dec_stats_t;

h264dec_t *h264dec_new(void);
void       h264dec_delete(h264dec_t *dec);

/* Decode from buf; *consumed = bytes eaten (may be less than len when a
 * picture completed mid-buffer — call again with the rest). */
h264dec_status_t h264dec_decode(h264dec_t *dec, const uint8_t *buf, size_t len,
                                size_t *consumed, h264dec_pic_t *pic);

/* Which macroblocks differ between the picture with version `from` and the
 * one with version `to`: bit mbNum of `mask` (`words` u32s, raster order).
 * Returns false when the decoder no longer knows (then assume all). Lets a
 * consumer that still holds picture `from` update only what changed. */
bool h264dec_changed_since(h264dec_t *dec, uint32_t from, uint32_t to,
                           uint32_t *mask, size_t words);

/* Copy the counters since the last take, then reset them. */
void h264dec_stats_take(h264dec_t *dec, h264dec_stats_t *out);
