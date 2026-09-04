#include "h264bsd_pie.h"
#include "h264bsd_reconstruct.h"

#ifdef H264BSD_ESP_PIE

#include <stdio.h>
#include <string.h>

#include "esp_attr.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *TAG = "h264pie";

/* ---------- scalar reference (exact h264bsd formula) ---------- */
void h264_ref_hhalf_row(const u8 *s, u8 *dst, u32 w)
{
    for (u32 i = 0; i < w; i++) {
        i32 v = (i32)s[i] - 5 * s[i + 1] + 20 * s[i + 2] + 20 * s[i + 3]
              - 5 * s[i + 4] + s[i + 5];
        v = (v + 16) >> 5;
        dst[i] = (u8)(v < 0 ? 0 : (v > 255 ? 255 : v));
    }
}

void h264_ref_vhalf_row(const u8 *s, u32 stride, u8 *dst, u32 w)
{
    for (u32 i = 0; i < w; i++) {
        i32 v = (i32)s[i] - 5 * s[stride + i] + 20 * s[2 * stride + i]
              + 20 * s[3 * stride + i] - 5 * s[4 * stride + i] + s[5 * stride + i];
        v = (v + 16) >> 5;
        dst[i] = (u8)(v < 0 ? 0 : (v > 255 ? 255 : v));
    }
}

void h264_ref_midhalf(const u8 *ref, u32 width, u8 *mb, u32 pw, u32 ph)
{
    static i32 t[21 * 16];
    for (u32 r = 0; r < ph + 5; r++) {
        const u8 *s = ref + r * width;
        for (u32 i = 0; i < pw; i++)
            t[r * 16 + i] = (i32)s[i] - 5 * s[i+1] + 20 * s[i+2]
                          + 20 * s[i+3] - 5 * s[i+4] + s[i+5];
    }
    for (u32 y = 0; y < ph; y++) {
        for (u32 i = 0; i < pw; i++) {
            i32 v = t[(y)*16+i] - 5*t[(y+1)*16+i] + 20*t[(y+2)*16+i]
                  + 20*t[(y+3)*16+i] - 5*t[(y+4)*16+i] + t[(y+5)*16+i];
            v = (v + 512) >> 10;
            mb[y*16+i] = (u8)(v < 0 ? 0 : (v > 255 ? 255 : v));
        }
    }
}

void h264_ref_hquarter_row(const u8 *s, u8 *dst, u32 w, u32 off)
{
    u8 half[24];
    h264_ref_hhalf_row(s, half, w);
    for (u32 i = 0; i < w; i++) dst[i] = (u8)((half[i] + s[i + 2 + off] + 1) >> 1);
}

void h264_ref_vquarter_row(const u8 *s, u32 stride, u8 *dst, u32 w, u32 off)
{
    u8 half[24];
    h264_ref_vhalf_row(s, stride, half, w);
    const u8 *n = s + (2 + off) * stride;
    for (u32 i = 0; i < w; i++) dst[i] = (u8)((half[i] + n[i] + 1) >> 1);
}

/* ---------- PIE kernel ----------
 *
 * 8 outputs per pass, all in 16-bit lanes:
 *   q0 = src[i .. i+15]        (unaligned load, realigned via SAR + src.q)
 *   q1 = src[i+16 .. i+31]
 *   for tap k in 0..5:
 *     SAR = k bytes; q2 = (q0:q1) >> 8k   → src[i+k .. i+k+15]
 *     q3 = vext.u8(q2)                    → low 8 bytes as 16-bit lanes
 *     qacc += q3 * coeff[k]               (vmulas.s16.qacc, coeff broadcast)
 *   q4 = srcmb.s16.qacc >> 5 (rounded)    → 8 s16 results
 *   clamp with vmax/vmin, pack low bytes.
 *
 * Coefficients {1,-5,20,20,-5,1} live in a 16-byte table broadcast per tap
 * with vldbc.16. The final pack: vunzip.8 of (result, result) keeps the low
 * byte of each 16-bit lane in the low 8 bytes. */
/* PIE GPR operands are restricted to x8..x15 (s0,s1,a0-a5) — a6/a7/t0/t1 are
 * rejected by the assembler — so every pointer the asm touches is pinned to an
 * "a" register with a register variable. Constants live in one table walked
 * with addi (vldbc's immediate step is encoded oddly on xesppie; avoid it). */
/* Walked sequentially by the kernel: bias pair, taps, clamp bounds.
 * srcmb only truncates (verified: it gave (sum)>>5, not (sum+16)>>5), so the
 * +16 rounding bias is accumulated explicitly as 1*16. */
static const int16_t s_tbl[12] __attribute__((aligned(16))) = {
    1, 16,                  /* bias: 1 * 16 -> qacc */
    1, -5, 20, 20, -5, 1,   /* 6-tap coefficients   */
    0, 255,                 /* clamp bounds         */
    0, 0
};

static void pie_hhalf8(const u8 *srcp, u8 *dstp)
{
    u8 out[16] __attribute__((aligned(16)));
    register const u8    *src  asm("a0") = srcp;
    register u8          *o    asm("a1") = out;
    register const int16_t *tbl asm("a2") = s_tbl;
    register unsigned     sh   asm("a3") = 5;

    asm volatile(
        /* q0 = unaligned 16-byte window at src (realigned via SAR) */
        "esp.ld.128.usar.ip q5, %[src], 16   \n"
        "esp.ld.128.usar.ip q6, %[src], 0    \n"
        "esp.src.q.qup q0, q5, q6            \n"
        "esp.zero.qacc                       \n"
        /* qacc = 16 (rounding bias) */
        "esp.vldbc.16.ip q6, %[tbl], 0       \n"
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vldbc.16.ip q7, %[tbl], 0       \n"
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vmulas.s16.qacc q6, q7          \n"

#define TAP(k) \
        "esp.vldbc.16.ip q6, %[tbl], 0       \n" \
        "addi %[tbl], %[tbl], 2              \n" \
        "li a4, " #k "                       \n" \
        "esp.movx.w.sar.bytes a4             \n" \
        "esp.src.q q3, q0, q0                \n" \
        "esp.vext.u8 q2, q3, q3              \n" \
        "esp.vmulas.s16.qacc q2, q6          \n"
        TAP(0) TAP(1) TAP(2) TAP(3) TAP(4) TAP(5)
#undef TAP

        /* clamp bounds follow the taps in the table */
        "esp.vldbc.16.ip q4, %[tbl], 0       \n"   /* q4 = 0   */
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vldbc.16.ip q5, %[tbl], 0       \n"   /* q5 = 255 */
        /* (sum + 16) >> 5 into 16-bit lanes (bias already in qacc) */
        "esp.srcmb.s16.qacc q2, %[sh], 1     \n"
        /* clamp to [0,255] and pack the low byte of each lane */
        "esp.vmax.s16 q2, q2, q4             \n"
        "esp.vmin.s16 q2, q2, q5             \n"
        "esp.vunzip.8 q2, q3                 \n"
        "esp.vst.128.ip q2, %[out], 0        \n"
        : [src] "+r" (src), [tbl] "+r" (tbl)
        : [out] "r" (o), [sh] "r" (sh)
        : "a4", "memory");

    memcpy(dstp, out, 8);
}

/* Vertical 6-tap: the taps are whole rows, so this is pure lane-wise work —
 * no byte shifts, each row realigned by its own ld.128.usar. 8 columns/pass. */
static void pie_vhalf8(const u8 *srcp, u32 stridev, u8 *dstp)
{
    u8 out[16] __attribute__((aligned(16)));
    register const u8    *row  asm("a0") = srcp;
    register u8          *o    asm("a1") = out;
    register const int16_t *tbl asm("a2") = s_tbl;
    register unsigned     sh   asm("a3") = 5;
    register unsigned     st   asm("a4") = stridev;
    register const u8    *scr  asm("a5");
    (void)scr;

    asm volatile(
        "esp.zero.qacc                       \n"
        /* qacc = 16 (rounding bias; srcmb truncates) */
        "esp.vldbc.16.ip q6, %[tbl], 0       \n"
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vldbc.16.ip q7, %[tbl], 0       \n"
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vmulas.s16.qacc q6, q7          \n"

#define VTAP() \
        "esp.vldbc.16.ip q6, %[tbl], 0       \n" \
        "addi %[tbl], %[tbl], 2              \n" \
        "mv a5, %[row]                       \n" \
        "esp.ld.128.usar.ip q4, a5, 16       \n" \
        "esp.ld.128.usar.ip q5, a5, 0        \n" \
        "esp.src.q.qup q3, q4, q5            \n" \
        "esp.vext.u8 q2, q3, q3              \n" \
        "esp.vmulas.s16.qacc q2, q6          \n" \
        "add %[row], %[row], %[st]           \n"
        VTAP() VTAP() VTAP() VTAP() VTAP() VTAP()
#undef VTAP

        /* clamp bounds follow the taps in the table */
        "esp.vldbc.16.ip q4, %[tbl], 0       \n"
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vldbc.16.ip q5, %[tbl], 0       \n"
        "esp.srcmb.s16.qacc q2, %[sh], 1     \n"
        "esp.vmax.s16 q2, q2, q4             \n"
        "esp.vmin.s16 q2, q2, q5             \n"
        "esp.vunzip.8 q2, q3                 \n"
        "esp.vst.128.ip q2, %[out], 0        \n"
        : [row] "+r" (row), [tbl] "+r" (tbl)
        : [out] "r" (o), [sh] "r" (sh), [st] "r" (st)
        : "a5", "memory");

    memcpy(dstp, out, 8);
}

void h264_pie_vhalf_row(const u8 *src, u32 stride, u8 *dst, u32 w)
{
    u32 i = 0;
    for (; i + 8 <= w; i += 8) pie_vhalf8(src + i, stride, dst + i);
    if (i < w) h264_ref_vhalf_row(src + i, stride, dst + i, w - i);
}

/* mid pass 1: u8 row -> 8 raw 6-tap sums as s16 (no bias, no shift, no clamp) */
/* avg: rounding term 1, then multiplier 1 for each operand */
static const int16_t s_tbl_avg[4] __attribute__((aligned(16))) = { 1, 1, 0, 0 };
static const int16_t s_tbl_m1[8] __attribute__((aligned(16))) = { 1, -5, 20, 20, -5, 1, 0, 0 };
/* mid pass 2: bias 1*512, taps, clamp bounds */
static const int16_t s_tbl_m2[12] __attribute__((aligned(16))) = { 1, 512, 1, -5, 20, 20, -5, 1, 0, 255, 0, 0 };

static void pie_mid_h8(const u8 *srcp, int16_t *dstp)
{
    register const u8      *src asm("a0") = srcp;
    register int16_t       *o   asm("a1") = dstp;
    register const int16_t *tbl asm("a2") = s_tbl_m1;
    register unsigned       sh  asm("a3") = 0;
    asm volatile(
        "esp.ld.128.usar.ip q5, %[src], 16   \n"
        "esp.ld.128.usar.ip q6, %[src], 0    \n"
        "esp.src.q.qup q0, q5, q6            \n"
        "esp.zero.qacc                       \n"
#define MTAP(k) \
        "esp.vldbc.16.ip q6, %[tbl], 0       \n" \
        "addi %[tbl], %[tbl], 2              \n" \
        "li a4, " #k "                       \n" \
        "esp.movx.w.sar.bytes a4             \n" \
        "esp.src.q q3, q0, q0                \n" \
        "esp.vext.u8 q2, q3, q3              \n" \
        "esp.vmulas.s16.qacc q2, q6          \n"
        MTAP(0) MTAP(1) MTAP(2) MTAP(3) MTAP(4) MTAP(5)
#undef MTAP
        "esp.srcmb.s16.qacc q2, %[sh], 1     \n"
        "esp.vst.128.ip q2, %[out], 0        \n"
        : [src] "+r" (src), [tbl] "+r" (tbl)
        : [out] "r" (o), [sh] "r" (sh)
        : "a4", "memory");
}

/* mid pass 2: 6 s16 rows (aligned stride) -> 8 clipped u8 */
static void pie_mid_v8(const int16_t *srcp, u32 stridev, u8 *dstp)
{
    u8 out[16] __attribute__((aligned(16)));
    register const int16_t *row asm("a0") = srcp;
    register u8            *o   asm("a1") = out;
    register const int16_t *tbl asm("a2") = s_tbl_m2;
    register unsigned       sh  asm("a3") = 10;
    register unsigned       st  asm("a4") = stridev * 2;   /* bytes */
    asm volatile(
        "esp.zero.qacc                       \n"
        "esp.vldbc.16.ip q6, %[tbl], 0       \n"
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vldbc.16.ip q7, %[tbl], 0       \n"
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vmulas.s16.qacc q6, q7          \n"   /* + 512 */
#define MVTAP() \
        "esp.vldbc.16.ip q6, %[tbl], 0       \n" \
        "addi %[tbl], %[tbl], 2              \n" \
        "mv a5, %[row]                       \n" \
        "esp.vld.128.ip q2, a5, 0            \n" \
        "esp.vmulas.s16.qacc q2, q6          \n" \
        "add %[row], %[row], %[st]           \n"
        MVTAP() MVTAP() MVTAP() MVTAP() MVTAP() MVTAP()
#undef MVTAP
        "esp.vldbc.16.ip q4, %[tbl], 0       \n"
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vldbc.16.ip q5, %[tbl], 0       \n"
        "esp.srcmb.s16.qacc q2, %[sh], 1     \n"
        "esp.vmax.s16 q2, q2, q4             \n"
        "esp.vmin.s16 q2, q2, q5             \n"
        "esp.vunzip.8 q2, q3                 \n"
        "esp.vst.128.ip q2, %[out], 0        \n"
        : [row] "+r" (row), [tbl] "+r" (tbl)
        : [out] "r" (o), [sh] "r" (sh), [st] "r" (st)
        : "a5", "memory");
    memcpy(dstp, out, 8);
}

/* (a + b + 1) >> 1 over 8 bytes, both operands unaligned. */
static void pie_avg8(const u8 *ap, const u8 *bp, u8 *dstp)
{
    u8 out[16] __attribute__((aligned(16)));
    register const u8      *a   asm("a0") = ap;
    register const u8      *b   asm("a1") = bp;
    register u8            *o   asm("a2") = out;
    register const int16_t *tbl asm("a3") = s_tbl_avg;
    register unsigned       sh  asm("a4") = 1;
    asm volatile(
        "esp.ld.128.usar.ip q4, %[a], 16     \n"
        "esp.ld.128.usar.ip q5, %[a], 0      \n"
        "esp.src.q.qup q0, q4, q5            \n"
        "esp.ld.128.usar.ip q4, %[b], 16     \n"
        "esp.ld.128.usar.ip q5, %[b], 0      \n"
        "esp.src.q.qup q1, q4, q5            \n"
        "esp.zero.qacc                       \n"
        /* qacc = 1 (the rounding term), then + a*1 + b*1 */
        "esp.vldbc.16.ip q6, %[tbl], 0       \n"
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vldbc.16.ip q7, %[tbl], 0       \n"
        "addi %[tbl], %[tbl], 2              \n"
        "esp.vmulas.s16.qacc q6, q7          \n"
        "esp.vext.u8 q2, q0, q0              \n"
        "esp.vmulas.s16.qacc q2, q7          \n"
        "esp.vext.u8 q2, q1, q1              \n"
        "esp.vmulas.s16.qacc q2, q7          \n"
        "esp.srcmb.s16.qacc q2, %[sh], 1     \n"
        "esp.vunzip.8 q2, q3                 \n"
        "esp.vst.128.ip q2, %[out], 0        \n"
        : [a] "+r" (a), [b] "+r" (b), [tbl] "+r" (tbl)
        : [out] "r" (o), [sh] "r" (sh)
        : "memory");
    memcpy(dstp, out, 8);
}

void h264_pie_hquarter_row(const u8 *src, u8 *dst, u32 w, u32 off)
{
    u8 half[24] __attribute__((aligned(16)));
    h264_pie_hhalf_row(src, half, w);
    u32 i = 0;
    for (; i + 8 <= w; i += 8) pie_avg8(half + i, src + i + 2 + off, dst + i);
    for (; i < w; i++) dst[i] = (u8)((half[i] + src[i + 2 + off] + 1) >> 1);
}

void h264_pie_vquarter_row(const u8 *src, u32 stride, u8 *dst, u32 w, u32 off)
{
    u8 half[24] __attribute__((aligned(16)));
    h264_pie_vhalf_row(src, stride, half, w);
    const u8 *n = src + (2 + off) * stride;
    u32 i = 0;
    for (; i + 8 <= w; i += 8) pie_avg8(half + i, n + i, dst + i);
    for (; i < w; i++) dst[i] = (u8)((half[i] + n[i] + 1) >> 1);
}

void h264_pie_horverquarter(const u8 *ref, u32 width, u8 *mb, u32 pw, u32 ph, u32 hv)
{
    const u32 hOff = hv & 1u, vOff = (hv >> 1) & 1u;
    u8 tH[24] __attribute__((aligned(16)));
    u8 tV[24] __attribute__((aligned(16)));
    for (u32 y = 0; y < ph; y++) {
        h264_pie_hhalf_row(ref + (y + 2 + vOff) * width, tH, pw);
        h264_pie_vhalf_row(ref + y * width + 2 + hOff, width, tV, pw);
        u8 *d = mb + y * 16;
        u32 i = 0;
        for (; i + 8 <= pw; i += 8) pie_avg8(tH + i, tV + i, d + i);
        for (; i < pw; i++) d[i] = (u8)((tH[i] + tV[i] + 1) >> 1);
    }
}

int h264_pie_midhalf(const u8 *ref, u32 width, u8 *mb, u32 pw, u32 ph)
{
    if (pw < 8) return 1;                       /* 4-wide: let C handle it */
    static int16_t tmp[21 * 16] __attribute__((aligned(16)));
    const u32 ts = 16;                          /* s16 stride: 32 B, aligned */
    for (u32 r = 0; r < ph + 5; r++)
        for (u32 i = 0; i + 8 <= pw; i += 8)
            pie_mid_h8(ref + r * width + i, tmp + r * ts + i);
    for (u32 y = 0; y < ph; y++)
        for (u32 i = 0; i + 8 <= pw; i += 8)
            pie_mid_v8(tmp + y * ts + i, ts, mb + y * 16 + i);
    return 0;
}

void h264_pie_hhalf_row(const u8 *src, u8 *dst, u32 w)
{
    u32 i = 0;
    for (; i + 8 <= w; i += 8) pie_hhalf8(src + i, dst + i);
    if (i < w) h264_ref_hhalf_row(src + i, dst + i, w - i);   /* w==4 tail */
}

/* ---------- self-check ---------- */
int h264_pie_selfcheck(void)
{
    static u8 src[6 * 19 + 32];
    u8 ref[16], pie[16];
    int fails = 0;

    for (int t = 0; t < 2000 && fails == 0; t++) {
        if (t == 0)      memset(src, 0x00, sizeof(src));
        else if (t == 1) memset(src, 0xFF, sizeof(src));
        else if (t == 2) { for (u32 i = 0; i < sizeof(src); i++) src[i] = (i & 1) ? 0xFF : 0x00; }
        else             esp_fill_random(src, sizeof(src));

        for (u32 w = 4; w <= 16; w += 4) {
            for (u32 off = 0; off < 4; off++) {          /* unaligned starts */
                memset(ref, 0xA5, sizeof(ref)); memset(pie, 0xA5, sizeof(pie));
                h264_ref_hhalf_row(src + off, ref, w);
                h264_pie_hhalf_row(src + off, pie, w);
                if (memcmp(ref, pie, w) == 0) {
                    /* vertical: 6 rows of `w` from the same buffer, stride w+3
                     * (deliberately not a multiple of 16 → unaligned rows) */
                    u32 stride = w + 3;
                    memset(ref, 0xA5, sizeof(ref)); memset(pie, 0xA5, sizeof(pie));
                    h264_ref_vhalf_row(src + off, stride, ref, w);
                    h264_pie_vhalf_row(src + off, stride, pie, w);
                    if (memcmp(ref, pie, w) != 0) {
                        fails++;
                        int i = 0; while (ref[i] == pie[i]) i++;
                        ESP_LOGE(TAG, "vhalf MISMATCH t=%d w=%u off=%u i=%d ref=%02x pie=%02x",
                                 t, (unsigned)w, (unsigned)off, i, ref[i], pie[i]);
                        break;
                    }
                }
                if (memcmp(ref, pie, w) != 0) {
                    fails++;
                    int i = 0; while (ref[i] == pie[i]) i++;
                    ESP_LOGE(TAG, "hhalf MISMATCH t=%d w=%u off=%u i=%d ref=%02x pie=%02x "
                                  "(in: %02x %02x %02x %02x %02x %02x)",
                             t, (unsigned)w, (unsigned)off, i, ref[i], pie[i],
                             src[off+i], src[off+i+1], src[off+i+2],
                             src[off+i+3], src[off+i+4], src[off+i+5]);
                    break;
                }
            }
            if (fails) break;
        }
    }
    /* quarter positions */
    if (!fails) {
        static u8 qs[64];
        u8 qr[24], qp[24];
        for (int t = 0; t < 500 && !fails; t++) {
            esp_fill_random(qs, sizeof(qs));
            for (u32 w = 8; w <= 16; w += 8)
                for (u32 off = 0; off < 2 && !fails; off++) {
                    memset(qr, 0xA5, sizeof(qr)); memset(qp, 0xA5, sizeof(qp));
                    h264_ref_hquarter_row(qs, qr, w, off);
                    h264_pie_hquarter_row(qs, qp, w, off);
                    if (memcmp(qr, qp, w)) { fails++; ESP_LOGE(TAG, "hquarter MISMATCH w=%u off=%u", (unsigned)w, (unsigned)off); break; }
                    memset(qr, 0xA5, sizeof(qr)); memset(qp, 0xA5, sizeof(qp));
                    h264_ref_vquarter_row(qs, 19, qr, w, off);
                    h264_pie_vquarter_row(qs, 19, qp, w, off);
                    if (memcmp(qr, qp, w)) { fails++; ESP_LOGE(TAG, "vquarter MISMATCH w=%u off=%u", (unsigned)w, (unsigned)off); break; }
                }
        }
    }

    /* Diagonal quarters e/g/p/r against the stock C. This one matters: the
     * PIE version COMPOSES the horizontal and vertical half-pel results, so
     * the row/column the two passes start from is a derivation of mine, not
     * something the arithmetic can confirm. The end-to-end clip only ever
     * exercises three of the four offsets — 'r' never appears in it — so the
     * oracle here is h264bsd's own interpolator on an interior block. */
    if (!fails) {
        static u8 plane[48 * 48];
        static u8 rmb[16 * 16], pmb[16 * 16];
        const u32 W = 48;
        for (int t = 0; t < 200 && !fails; t++) {
            esp_fill_random(plane, sizeof(plane));
            for (u32 hv = 0; hv < 4 && !fails; hv++) {
                for (u32 pw = 8; pw <= 16 && !fails; pw += 8) {
                    /* interior: x0,y0 leave room for the 5-tap halo */
                    const i32 x0 = 4, y0 = 4;
                    memset(rmb, 0xA5, sizeof(rmb));
                    memset(pmb, 0xA5, sizeof(pmb));
                    h264bsdInterpolateHorVerQuarter(plane, rmb, x0, y0, W, W,
                                                    pw, pw, hv);
                    h264_pie_horverquarter(plane + (u32)y0 * W + (u32)x0, W,
                                           pmb, pw, pw, hv);
                    for (u32 y = 0; y < pw && !fails; y++) {
                        if (memcmp(rmb + y * 16, pmb + y * 16, pw) == 0) continue;
                        u32 i = 0; while (rmb[y * 16 + i] == pmb[y * 16 + i]) i++;
                        fails++;
                        ESP_LOGE(TAG, "horverquarter MISMATCH hv=%u pw=%u "
                                      "row=%u col=%u ref=%02x pie=%02x",
                                 (unsigned)hv, (unsigned)pw, (unsigned)y,
                                 (unsigned)i, rmb[y * 16 + i], pmb[y * 16 + i]);
                    }
                }
            }
        }
    }

    /* mid ('j'): 16x16 and 8x8 blocks over a random reference plane */
    if (!fails) {
        static u8 plane[32 * 32];
        static u8 rmb[16 * 16], pmb[16 * 16];
        for (int t = 0; t < 200 && !fails; t++) {
            esp_fill_random(plane, sizeof(plane));
            for (u32 pw = 8; pw <= 16; pw += 8) {
                memset(rmb, 0xA5, sizeof(rmb)); memset(pmb, 0xA5, sizeof(pmb));
                h264_ref_midhalf(plane, 32, rmb, pw, pw);
                if (h264_pie_midhalf(plane, 32, pmb, pw, pw) == 0 &&
                    memcmp(rmb, pmb, sizeof(rmb)) != 0) {
                    fails++;
                    int i = 0; while (rmb[i] == pmb[i]) i++;
                    ESP_LOGE(TAG, "midhalf MISMATCH t=%d pw=%u i=%d ref=%02x pie=%02x",
                             t, (unsigned)pw, i, rmb[i], pmb[i]);
                    break;
                }
            }
        }
    }
    /* Micro-benchmark: is PIE actually faster than plain C here? Both run the
     * same work; the C side is the naive reference (h264bsd's own inner loop
     * is hand-tuned and thus somewhat faster than this, so treat the ratio as
     * an upper bound on the win). */
    if (!fails) {
        static u8 bsrc[64], bdst[32];
        esp_fill_random(bsrc, sizeof(bsrc));
        const int N = 20000;
        int64_t t0 = esp_timer_get_time();
        for (int i = 0; i < N; i++) h264_ref_hhalf_row(bsrc, bdst, 16);
        int64_t t1 = esp_timer_get_time();
        for (int i = 0; i < N; i++) h264_pie_hhalf_row(bsrc, bdst, 16);
        int64_t t2 = esp_timer_get_time();
        for (int i = 0; i < N; i++) h264_ref_vhalf_row(bsrc, 19, bdst, 16);
        int64_t t3 = esp_timer_get_time();
        for (int i = 0; i < N; i++) h264_pie_vhalf_row(bsrc, 19, bdst, 16);
        int64_t t4 = esp_timer_get_time();
        ESP_LOGW(TAG, "bench 16px row x%d: hhalf C %lld us / PIE %lld us | vhalf C %lld us / PIE %lld us",
                 N, (long long)(t1-t0), (long long)(t2-t1),
                 (long long)(t3-t2), (long long)(t4-t3));

        /* The same kernels again, but reading a reference picture the way the
         * decoder really does: out of PSRAM, a different macroblock every
         * call, so no row is ever in cache. If the numbers collapse together
         * here, the interpolation is memory-bound and no amount of SIMD in
         * the arithmetic will move it. */
        const u32 W = 800, H = 480;
        u8 *big = heap_caps_malloc((size_t)W * H, MALLOC_CAP_SPIRAM);
        if (big) {
            esp_fill_random(big, 4096);
            for (u32 y = 1; y < H; y++) memcpy(big + y * W, big, W > 4096 ? 4096 : W);
            const int M = 20000;
            /* walk macroblock positions so consecutive calls are far apart */
            #define POS(i) (big + ((u32)(((i) * 37u) % (H - 24)) + 2) * W \
                                + ((u32)(((i) * 53u) % (W - 32)) + 2))
            int64_t p0 = esp_timer_get_time();
            for (int i = 0; i < M; i++) h264_ref_hhalf_row(POS(i), bdst, 16);
            int64_t p1 = esp_timer_get_time();
            for (int i = 0; i < M; i++) h264_pie_hhalf_row(POS(i), bdst, 16);
            int64_t p2 = esp_timer_get_time();
            for (int i = 0; i < M; i++) h264_ref_vhalf_row(POS(i), W, bdst, 16);
            int64_t p3 = esp_timer_get_time();
            for (int i = 0; i < M; i++) h264_pie_vhalf_row(POS(i), W, bdst, 16);
            int64_t p4 = esp_timer_get_time();
            #undef POS
            ESP_LOGW(TAG, "bench PSRAM scattered x%d: hhalf C %lld us / PIE %lld us | vhalf C %lld us / PIE %lld us",
                     M, (long long)(p1-p0), (long long)(p2-p1),
                     (long long)(p3-p2), (long long)(p4-p3));
            heap_caps_free(big);
        }
    }
    if (!fails) ESP_LOGW(TAG, "selfcheck half + quarter + horverquarter(e/g/p/r): PASS");
    else        ESP_LOGE(TAG, "selfcheck: FAIL");
    return fails;
}

#else  /* !H264BSD_ESP_PIE */
int  h264_pie_selfcheck(void) { return 0; }
void h264_pie_hhalf_row(const u8 *src, u8 *dst, u32 w) { (void)src; (void)dst; (void)w; }
void h264_ref_hhalf_row(const u8 *src, u8 *dst, u32 w) { (void)src; (void)dst; (void)w; }
void h264_pie_vhalf_row(const u8 *src, u32 stride, u8 *dst, u32 w)
{ (void)src; (void)stride; (void)dst; (void)w; }
void h264_ref_vhalf_row(const u8 *src, u32 stride, u8 *dst, u32 w)
{ (void)src; (void)stride; (void)dst; (void)w; }
int  h264_pie_midhalf(const u8 *ref, u32 width, u8 *mb, u32 pw, u32 ph)
{ (void)ref; (void)width; (void)mb; (void)pw; (void)ph; return 1; }
void h264_pie_hquarter_row(const u8 *s, u8 *d, u32 w, u32 o) { (void)s;(void)d;(void)w;(void)o; }
void h264_pie_horverquarter(const u8 *r, u32 wd, u8 *m, u32 pw, u32 ph, u32 hv)
{ (void)r;(void)wd;(void)m;(void)pw;(void)ph;(void)hv; }
void h264_ref_hquarter_row(const u8 *s, u8 *d, u32 w, u32 o) { (void)s;(void)d;(void)w;(void)o; }
void h264_pie_vquarter_row(const u8 *s, u32 st, u8 *d, u32 w, u32 o) { (void)s;(void)st;(void)d;(void)w;(void)o; }
void h264_ref_vquarter_row(const u8 *s, u32 st, u8 *d, u32 w, u32 o) { (void)s;(void)st;(void)d;(void)w;(void)o; }
void h264_ref_midhalf(const u8 *ref, u32 width, u8 *mb, u32 pw, u32 ph)
{ (void)ref; (void)width; (void)mb; (void)pw; (void)ph; }
#endif
