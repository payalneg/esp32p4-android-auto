/* ESP32-P4 PIE (128-bit SIMD) kernels for the H.264 luma interpolation hot
 * path. Everything here is gated by CONFIG_H264DEC_PIE (default OFF) and has
 * a scalar C reference for the on-device self-check (h264_pie_selfcheck):
 * PIE output must be byte-identical to the reference on random + edge inputs
 * before any kernel is wired into the decoder. */
#ifndef H264BSD_PIE_H
#define H264BSD_PIE_H

#include "basetype.h"

/* One row of horizontal half-pel 6-tap: for i in [0,w),
 *   dst[i] = clip0_255( (s[i] -5 s[i+1] +20 s[i+2] +20 s[i+3] -5 s[i+4] + s[i+5] + 16) >> 5 )
 * src must have w+5 readable bytes. w is a multiple of 4 (4/8/16). */
void h264_pie_hhalf_row(const u8 *src, u8 *dst, u32 w);
void h264_ref_hhalf_row(const u8 *src, u8 *dst, u32 w);   /* scalar reference */

/* Runs the PIE kernels against their references on many random cases and
 * edge values; logs a one-line pass/fail (+ first mismatch). Returns 0 on
 * full pass. Safe to call once at boot. */
int h264_pie_selfcheck(void);

#endif
