#include "h264bsd_esp_dirty.h"

#include <string.h>

#define DIRTY_MAX_MBS     2400          /* up to 1024x600 */
#define DIRTY_WORDS       (DIRTY_MAX_MBS / 32)
#define DIRTY_BUFFERS     4             /* physical picture buffers tracked */
#define DIRTY_HISTORY     8             /* frames of written-sets kept */

typedef struct {
    const u8 *data;
    u32       version;                  /* frame seq of the content, 0 = unknown */
} buf_ver_t;

static buf_ver_t s_bufs[DIRTY_BUFFERS];
static u32       s_written[DIRTY_HISTORY][DIRTY_WORDS];  /* by version % HISTORY */
static u32       s_need[DIRTY_WORDS];
static u32       s_cur[DIRTY_WORDS];
static u32       s_seq = 1;             /* next frame's version */
static u32       s_hist_valid_from = 1; /* versions below this have no written-set */
static u32       s_words;
static u32       s_need_all;            /* copy everything this picture */
static u32       s_cur_all;             /* everything was written this picture */
static const u8 *s_ref;                 /* reference the copy set was computed against */

u32 g_h264bsd_dirty_copied_mbs;
u32 g_h264bsd_dirty_skipped_mbs;

static buf_ver_t *find_buf(const u8 *data, u32 create)
{
    buf_ver_t *free_slot = NULL;
    for (int i = 0; i < DIRTY_BUFFERS; i++) {
        if (s_bufs[i].data == data) return &s_bufs[i];
        if (!s_bufs[i].data && !free_slot) free_slot = &s_bufs[i];
    }
    if (!create) return NULL;
    if (!free_slot) {                   /* evict the oldest */
        free_slot = &s_bufs[0];
        for (int i = 1; i < DIRTY_BUFFERS; i++) {
            if (s_bufs[i].version < free_slot->version) free_slot = &s_bufs[i];
        }
    }
    free_slot->data    = data;
    free_slot->version = 0;
    return free_slot;
}

void h264bsdDirtyReset(void)
{
    /* Versions stay monotonic: consumers (staging slots, framebuffers) hold
     * old version numbers and must never see them reused with a different
     * meaning. Only the history behind us becomes unknown. */
    memset(s_bufs, 0, sizeof(s_bufs));
    memset(s_written, 0, sizeof(s_written));
    s_hist_valid_from = s_seq;
    s_need_all = 1;
}

void h264bsdDirtyBeginPicture(const u8 *dst, const u8 *ref, u32 picSizeInMbs)
{
    s_words = (picSizeInMbs + 31) / 32;
    if (s_words > DIRTY_WORDS) s_words = DIRTY_WORDS;
    memset(s_cur, 0, s_words * sizeof(u32));
    s_cur_all  = 0;
    s_need_all = 1;
    s_ref      = ref;

    buf_ver_t *d = find_buf(dst, 1);
    buf_ver_t *r = ref ? find_buf(ref, 0) : NULL;
    if (!r || !r->version || !d->version || d->version > r->version) {
        return;                         /* unknown history: copy everything */
    }
    /* dst holds frame d->version, ref holds r->version: union the written
     * sets of every frame in (d->version, r->version]. */
    u32 gap = r->version - d->version;
    if (gap == 0 || gap >= DIRTY_HISTORY) return;
    memset(s_need, 0, s_words * sizeof(u32));
    for (u32 v = d->version + 1; v <= r->version; v++) {
        const u32 *w = s_written[v % DIRTY_HISTORY];
        for (u32 i = 0; i < s_words; i++) s_need[i] |= w[i];
    }
    s_need_all = 0;
}

u32 h264bsdDirtyNeedCopy(u32 mbNum, const u8 *ref)
{
    if (s_need_all || ref != s_ref) { g_h264bsd_dirty_copied_mbs++; return 1; }
    u32 need = (s_need[mbNum >> 5] >> (mbNum & 31)) & 1u;
    if (need) g_h264bsd_dirty_copied_mbs++; else g_h264bsd_dirty_skipped_mbs++;
    return need;
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
    buf_ver_t *d = find_buf(dst, 1);
    u32 v = s_seq++;
    u32 *w = s_written[v % DIRTY_HISTORY];
    if (s_cur_all) {
        memset(w, 0xFF, sizeof(s_written[0]));
    } else {
        memcpy(w, s_cur, sizeof(s_written[0]));
    }
    d->version = v;
    return v;
}

u32 h264bsdDirtyChangedSince(u32 from, u32 to, u32 *mask, u32 words)
{
    if (from == 0 || to <= from || to >= s_seq) return 0;
    if (to - from >= DIRTY_HISTORY) return 0;
    if (from + 1 < s_hist_valid_from) return 0;   /* history crosses a reset */
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
