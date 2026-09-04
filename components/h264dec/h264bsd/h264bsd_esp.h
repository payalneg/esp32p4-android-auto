/* ESP32-P4 additions to Hantro's h264bsd — the hooks h264dec.c reads.
 * Kept in one header so the diff against upstream stays greppable
 * (every change is under H264BSD_ESP_FASTPATH / H264BSD_ESP_STATS). */
#ifndef H264BSD_ESP_H
#define H264BSD_ESP_H

#include "basetype.h"
#include "h264bsd_image.h"

/* Per-picture counters, reset by the adapter after it takes a picture.
 * skipZeroMvMbs: macroblocks that went through the P_Skip/zero-MV copy path
 * (see h264bsdInterPrediction); skipRef / skipRefMixed: the reference picture
 * those copies came from, and whether more than one was seen. When
 * skipZeroMvMbs == picSizeInMbs and the single reference is the previous
 * output picture, the new picture is pixel-identical to the last one. */
extern u32 g_h264bsd_skip_zero_mbs;
extern u8 *g_h264bsd_skip_ref;
extern u32 g_h264bsd_skip_ref_mixed;
/* Set by the fast path for the macroblock being decoded, cleared by
 * h264bsdDecodeMacroblock before dispatch — tells it nothing was written. */
extern u32 g_h264bsd_mb_fastpath;
/* Time spent in h264bsdFilterPicture (deblocking), accumulated. */
extern u64 g_h264bsd_deblock_us;

/* Cycle accumulators for the macroblock loop (H264BSD_ESP_STATS). Read with
 * the CPU cycle counter; the adapter converts to microseconds. */
extern u64 g_h264bsd_cyc_skip_mb;   /* skipped MBs: SetMbParams..DecodeMacroblock */
extern u64 g_h264bsd_cyc_coded_mb;  /* coded MBs: parse + DecodeMacroblock */
extern u64 g_h264bsd_cyc_loop;      /* MoreRbspData + NextMbAddress per MB */
extern u32 g_h264bsd_n_skip_mb, g_h264bsd_n_coded_mb;
/* Coded-MB stage split (cycles): CAVLC parse, dequant+IDCT, motion
 * compensation (h264bsdPredictSamples), residual add + picture write, intra. */
extern u64 g_h264bsd_cyc_parse, g_h264bsd_cyc_residual, g_h264bsd_cyc_mc,
           g_h264bsd_cyc_write, g_h264bsd_cyc_intra;
#ifdef H264BSD_ESP_STATS
#include "esp_cpu.h"
#define H264BSD_CYC() ((u32)esp_cpu_get_cycle_count())
#else
#define H264BSD_CYC() 0u
#endif

/* 16x16 luma + two 8x8 chroma blocks straight from the co-located position
 * in ref into the current image (whose luma/cb/cr pointers already sit on
 * this macroblock). */
void h264bsdCopyMbFromRef(image_t *currImage, const image_t *refImage,
                          u32 colPx, u32 rowPx);

/* Pictures that were entirely one skip run (bit-exact copies of their
 * reference) and were produced by swapping DPB buffers instead of copying. */
extern u32 g_h264bsd_pic_aliased;
extern u32 g_h264bsd_pic_copied;
/* Version (frame sequence, see h264bsd_esp_dirty.h) of the last picture. */
extern u32 g_h264bsd_pic_version;

#endif
