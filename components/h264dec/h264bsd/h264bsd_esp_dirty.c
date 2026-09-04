#include "h264bsd_esp_dirty.h"

#include <string.h>

/* Two things live here, both keyed on a monotonic frame version:
 *
 * 1. A per-version written-set history, used by the DISPLAY incremental path
 *    (h264bsdDirtyChangedSince) to re-shuffle only what moved.
 * 2. The per-macroblock COPY ELISION for zero-MV P_Skip macroblocks — the
 *    single biggest cost in this decoder (~1130 of 1500 MBs on a moving
 *    800x480 frame, 22.9 us each, over half the decode time), because each
 *    one is 32 short strided reads and writes across PSRAM.
 *
 * The elision asks: does the buffer we are decoding into ALREADY hold, at
 * this macroblock, the very bytes we are about to copy out of the reference?
 * With one reference frame the DPB alternates between two buffers, so the
 * destination holds frame N-2 while the reference is N-1 — and they differ
 * only where N-1 actually changed something.
 *
 * Getting that right needs three things the first attempt (removed 2026-09-04
 * for corrupting frames 3-45) got wrong:
 *
 * - Content is tracked per PHYSICAL buffer (data pointer) with an explicit
 *   table, and the whole-picture alias — which swaps storage between DPB
 *   slots — invalidates that table instead of silently invalidating the
 *   model (h264bsdDirtyBuffersMoved).
 * - A version's written set is DILATED by one macroblock in each direction
 *   before it is stored, because the deblocking filter reaches across edges:
 *   filtering macroblock m rewrites pixels of m, of m-1 and of m-width. Those
 *   neighbours are not "written" by the decoder but their pixels do change,
 *   and treating them as unchanged is exactly how the old code lost frames.
 * - A version only participates if its reference was the immediately previous
 *   version, since "written" means "differs from my reference" and the chain
 *   arithmetic assumes that means "differs from my predecessor".
 *
 * Everything the tracker is unsure about — unknown buffer, history gap,
 * concealment, a mixed reference — degrades to "copy it", so a wrong guess
 * can only cost time, never pixels. */

#define DIRTY_MAX_MBS     2400
#define DIRTY_WORDS       (DIRTY_MAX_MBS / 32)
#define DIRTY_HISTORY     8
#define DIRTY_SLOTS       24

static u32 s_written[DIRTY_HISTORY][DIRTY_WORDS];
static u8  s_ref_was_prev[DIRTY_HISTORY];
static u32 s_cur[DIRTY_WORDS];
static u32 s_seq = 1;                 /* next frame's version */
static u32 s_hist_valid_from = 1;     /* versions below this have no written-set */
static u32 s_words;
static u32 s_mbs;
static u32 s_width_mbs;
static u32 s_cur_all;

/* Which version's pixels each physical buffer currently holds. 0 = unknown. */
static const u8 *s_slot_ptr[DIRTY_SLOTS];
static u32       s_slot_ver[DIRTY_SLOTS];

/* State for the picture being decoded. */
static const u8 *s_ref_ptr;
static u32       s_ref_ver;
static u32       s_elide;                  /* the model holds for this picture */
static u32       s_agree[DIRTY_WORDS];     /* dst already equals ref here */

u32 g_h264bsd_dirty_copied_mbs;
u32 g_h264bsd_dirty_skipped_mbs;

static u32 SlotGet(const u8 *ptr)
{
    if (!ptr) return 0;
    for (u32 i = 0; i < DIRTY_SLOTS; i++)
        if (s_slot_ptr[i] == ptr) return s_slot_ver[i];
    return 0;
}

static void SlotSet(const u8 *ptr, u32 ver)
{
    if (!ptr) return;
    for (u32 i = 0; i < DIRTY_SLOTS; i++)
        if (s_slot_ptr[i] == ptr) { s_slot_ver[i] = ver; return; }
    for (u32 i = 0; i < DIRTY_SLOTS; i++)
        if (!s_slot_ptr[i]) { s_slot_ptr[i] = ptr; s_slot_ver[i] = ver; return; }
    /* More buffers than slots: forget the table rather than guess. */
    memset(s_slot_ptr, 0, sizeof(s_slot_ptr));
    memset(s_slot_ver, 0, sizeof(s_slot_ver));
    s_slot_ptr[0] = ptr; s_slot_ver[0] = ver;
}

void h264bsdDirtyBuffersMoved(void)
{
    memset(s_slot_ptr, 0, sizeof(s_slot_ptr));
    memset(s_slot_ver, 0, sizeof(s_slot_ver));
    s_elide = 0;
}

void h264bsdDirtyReset(void)
{
    /* Versions stay monotonic — consumers hold old numbers. Only the history
     * behind us becomes unknown. */
    memset(s_written, 0, sizeof(s_written));
    memset(s_ref_was_prev, 0, sizeof(s_ref_was_prev));
    s_hist_valid_from = s_seq;
    h264bsdDirtyBuffersMoved();
}

void h264bsdDirtyBeginPicture(const u8 *dst, const u8 *ref, u32 picSizeInMbs,
                              u32 picWidthInMbs)
{
    s_mbs       = picSizeInMbs;
    s_width_mbs = picWidthInMbs;
    s_words = (picSizeInMbs + 31) / 32;
    if (s_words > DIRTY_WORDS) s_words = DIRTY_WORDS;
    memset(s_cur, 0, s_words * sizeof(u32));
    s_cur_all = 0;

    s_ref_ptr = ref;
    s_ref_ver = SlotGet(ref);
    s_elide   = 0;

    u32 dstVer = SlotGet(dst);
    /* Only chain versions whose own reference was their predecessor, and only
     * when this picture predicts from the frame right before it. */
    if (!dstVer || !s_ref_ver || s_ref_ver + 1 != s_seq) return;

    u32 lo = dstVer < s_ref_ver ? dstVer : s_ref_ver;
    u32 hi = dstVer < s_ref_ver ? s_ref_ver : dstVer;

    if (lo == hi) {                       /* dst already IS the reference */
        memset(s_agree, 0xFF, s_words * sizeof(u32));
        s_elide = 1;
        return;
    }
    if (hi - lo >= DIRTY_HISTORY) return;
    if (lo + 1 < s_hist_valid_from) return;

    memset(s_agree, 0, s_words * sizeof(u32));
    for (u32 v = lo + 1; v <= hi; v++) {
        if (!s_ref_was_prev[v % DIRTY_HISTORY]) return;
        const u32 *w = s_written[v % DIRTY_HISTORY];
        for (u32 i = 0; i < s_words; i++) s_agree[i] |= w[i];
    }
    for (u32 i = 0; i < s_words; i++) s_agree[i] = ~s_agree[i];
    s_elide = 1;
}

u32 h264bsdDirtyNeedCopy(u32 mbNum, const u8 *ref)
{
    if (s_elide && ref == s_ref_ptr && mbNum < DIRTY_MAX_MBS &&
        ((s_agree[mbNum >> 5] >> (mbNum & 31)) & 1u))
    {
        g_h264bsd_dirty_skipped_mbs++;
        return 0;
    }
    g_h264bsd_dirty_copied_mbs++;
    return 1;
}

void h264bsdDirtyMarkWritten(u32 mbNum)
{
    if (mbNum < DIRTY_MAX_MBS) s_cur[mbNum >> 5] |= 1u << (mbNum & 31);
}

void h264bsdDirtyMarkAllWritten(void)
{
    s_cur_all = 1;
}

/* Grow the set by one macroblock in each direction: the deblocking filter
 * rewrites pixels on both sides of every edge it touches, so a neighbour of a
 * written macroblock has changed pixels even though the decoder never wrote
 * it. Conservative — a too-large set only costs copies. */
static void DilateWritten(u32 *out, const u32 *in)
{
    memset(out, 0, s_words * sizeof(u32));
    u32 w = s_width_mbs ? s_width_mbs : 1;
    for (u32 m = 0; m < s_mbs; m++) {
        if (!((in[m >> 5] >> (m & 31)) & 1u)) continue;
        u32 col = m % w;
#define SET(n) do { u32 n_ = (n); if (n_ < s_mbs) out[n_ >> 5] |= 1u << (n_ & 31); } while (0)
        SET(m);
        if (col)         SET(m - 1);
        if (col + 1 < w) SET(m + 1);
        if (m >= w)      SET(m - w);
        SET(m + w);
#undef SET
    }
}

u32 h264bsdDirtyEndPicture(const u8 *dst)
{
    u32 v = s_seq++;
    u32 *w = s_written[v % DIRTY_HISTORY];
    if (s_cur_all) memset(w, 0xFF, sizeof(s_written[0]));
    else           DilateWritten(w, s_cur);
    /* This picture's own reference was the previous version exactly when the
     * begin-of-picture bookkeeping said so. */
    s_ref_was_prev[v % DIRTY_HISTORY] = (u8)(s_ref_ver + 1 == v);
    SlotSet(dst, v);
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
