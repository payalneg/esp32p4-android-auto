#include "h264bsd_esp_dirty.h"

#include <string.h>

/* Per-version written-set history for the DISPLAY incremental path
 * (h264bsdDirtyChangedSince). This part is version-based and robust to DPB
 * buffer moves — versions are monotonic and each records exactly which
 * macroblocks that frame wrote.
 *
 * The per-macroblock COPY-ELISION that used to live here (skip copying a
 * P_Skip MB when the destination buffer was assumed to already hold it) was
 * REMOVED 2026-09-04: its "physical buffer B holds frame version V" model is
 * broken by the whole-picture DPB alias, which swaps data pointers between
 * slots, so on complex/mixed frames it elided copies that were actually
 * needed and corrupted the picture (CRC A/B: fast-path ON diverged from plain
 * h264bsd on frames 3-45; with elision off it is bit-exact). h264bsdDirtyNeed-
 * Copy now always copies. The static-frame win comes from the whole-picture
 * alias + the display incremental, both proven bit-exact. */

#define DIRTY_MAX_MBS     2400
#define DIRTY_WORDS       (DIRTY_MAX_MBS / 32)
#define DIRTY_HISTORY     8

static u32 s_written[DIRTY_HISTORY][DIRTY_WORDS];
static u32 s_cur[DIRTY_WORDS];
static u32 s_seq = 1;                 /* next frame's version */
static u32 s_hist_valid_from = 1;     /* versions below this have no written-set */
static u32 s_words;
static u32 s_cur_all;

u32 g_h264bsd_dirty_copied_mbs;
u32 g_h264bsd_dirty_skipped_mbs;

void h264bsdDirtyReset(void)
{
    /* Versions stay monotonic — consumers hold old numbers. Only the history
     * behind us becomes unknown. */
    memset(s_written, 0, sizeof(s_written));
    s_hist_valid_from = s_seq;
}

void h264bsdDirtyBeginPicture(const u8 *dst, const u8 *ref, u32 picSizeInMbs)
{
    (void)dst; (void)ref;
    s_words = (picSizeInMbs + 31) / 32;
    if (s_words > DIRTY_WORDS) s_words = DIRTY_WORDS;
    memset(s_cur, 0, s_words * sizeof(u32));
    s_cur_all = 0;
}

u32 h264bsdDirtyNeedCopy(u32 mbNum, const u8 *ref)
{
    (void)mbNum; (void)ref;
    g_h264bsd_dirty_copied_mbs++;
    return 1;                          /* always copy: correctness over the elision */
}

void h264bsdDirtyMarkWritten(u32 mbNum)
{
    if (mbNum < DIRTY_MAX_MBS) s_cur[mbNum >> 5] |= 1u << (mbNum & 31);
}

void h264bsdDirtyMarkAllWritten(void)
{
    s_cur_all = 1;
}

u32 h264bsdDirtyEndPicture(const u8 *dst)
{
    (void)dst;
    u32 v = s_seq++;
    u32 *w = s_written[v % DIRTY_HISTORY];
    if (s_cur_all) memset(w, 0xFF, sizeof(s_written[0]));
    else           memcpy(w, s_cur, sizeof(s_written[0]));
    return v;
}

u32 h264bsdDirtyChangedSince(u32 from, u32 to, u32 *mask, u32 words)
{
    if (from == 0 || to <= from || to >= s_seq) return 0;
    if (to - from >= DIRTY_HISTORY) return 0;
    if (from + 1 < s_hist_valid_from) return 0;   /* range crosses a reset */
    if (words > DIRTY_WORDS) words = DIRTY_WORDS;
    memset(mask, 0, words * sizeof(u32));
    for (u32 v = from + 1; v <= to; v++) {
        const u32 *w = s_written[v % DIRTY_HISTORY];
        for (u32 i = 0; i < words; i++) mask[i] |= w[i];
    }
    return 1;
}

u32 h264bsdDirtyMbTouched(u32 mbNum, u32 widthMbs)
{
    if (s_cur_all) return 1;
#define WRITTEN(n) ((s_cur[(n) >> 5] >> ((n) & 31)) & 1u)
    if (WRITTEN(mbNum)) return 1;
    if ((mbNum % widthMbs) && WRITTEN(mbNum - 1)) return 1;
    if (mbNum >= widthMbs && WRITTEN(mbNum - widthMbs)) return 1;
#undef WRITTEN
    return 0;
}
