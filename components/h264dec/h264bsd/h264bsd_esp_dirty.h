/* ESP32-P4: per-buffer dirty tracking for skipped macroblocks.
 *
 * A P_Skip macroblock with a zero motion vector is the co-located block of
 * the reference picture. The stock decoder copies it into the current
 * picture — 1500 copies of 384 bytes on a static 800x480 screen, ~10 ms of
 * PSRAM traffic per frame, for pixels the destination buffer very often
 * already holds: with one reference frame the DPB alternates between two
 * buffers, so the buffer being decoded into contains frame N-2, and frame N-1
 * differs from it only where frame N-1 coded something. Copy exactly those.
 *
 * Tracked per PHYSICAL buffer (data pointer), not per DPB slot, because the
 * whole-picture skip path swaps buffer storage between slots. Each buffer
 * remembers the frame sequence number of its content; each frame remembers
 * the set of macroblocks it wrote. Copy set for a skipped MB = union of the
 * written sets of all frames newer than the destination's content, up to the
 * reference. Anything the tracker is unsure about (unknown buffer, gap too
 * old, concealment, error) degrades to "copy everything", so a wrong guess
 * can only cost time, never pixels. */
#ifndef H264BSD_ESP_DIRTY_H
#define H264BSD_ESP_DIRTY_H

#include "basetype.h"

/* Forget everything (new sequence, DPB reset, error). */
void h264bsdDirtyReset(void);

/* A picture starts decoding into `dst` (its data pointer), predicting from
 * `ref`. Computes the copy set. picSizeInMbs for sizing. */
void h264bsdDirtyBeginPicture(const u8 *dst, const u8 *ref, u32 picSizeInMbs);

/* Does skipped MB `mbNum` need its pixels copied from `ref`? Always yes when
 * `ref` is not the reference the picture was begun with. */
u32 h264bsdDirtyNeedCopy(u32 mbNum, const u8 *ref);

/* MB `mbNum` was (or will be) written by the decoder this picture. */
void h264bsdDirtyMarkWritten(u32 mbNum);

/* Every MB of the current picture is to be treated as written
 * (concealment, or the tracker lost the thread). */
void h264bsdDirtyMarkAllWritten(void);

/* Deblocking gate: does MB `mbNum` have any edge that can carry a non-zero
 * boundary strength? True when it, its left or its top neighbour was written
 * this picture; a zero-MV skip surrounded by zero-MV skips has bS == 0 on
 * every edge and its filter pass is a no-op. */
u32 h264bsdDirtyMbTouched(u32 mbNum, u32 widthMbs);

/* The picture in `dst` is complete; returns its version (frame sequence). */
u32 h264bsdDirtyEndPicture(const u8 *dst);

/* Union of the written sets of every version in (from, to] into `mask`
 * (`words` u32s). Returns 0 when that range is not fully known (from == 0,
 * too old, or in the future) — the caller then treats everything as changed.
 * Used by the display stage to re-shuffle only the macroblocks that differ
 * between the frame its staging buffer holds and the new one. */
u32 h264bsdDirtyChangedSince(u32 from, u32 to, u32 *mask, u32 words);

/* Counters for the stats line. */
extern u32 g_h264bsd_dirty_copied_mbs;
extern u32 g_h264bsd_dirty_skipped_mbs;

#endif
