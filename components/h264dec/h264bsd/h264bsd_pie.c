#include "h264bsd_pie.h"

#ifdef H264BSD_ESP_PIE

#include <stdio.h>
#include <string.h>

#include "esp_attr.h"
#include "esp_log.h"
#include "esp_random.h"

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
    if (!fails) ESP_LOGW(TAG, "selfcheck hhalf+vhalf+midhalf: PASS (2000 cases x widths x offsets)");
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
void h264_ref_midhalf(const u8 *ref, u32 width, u8 *mb, u32 pw, u32 ph)
{ (void)ref; (void)width; (void)mb; (void)pw; (void)ph; }
#endif
