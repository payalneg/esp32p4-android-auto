#include "h264dec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_crc.h"
#include "esp_clk_tree.h"
#include "esp_private/esp_clk.h"

#include "h264bsd_decoder.h"
#include "h264bsd_dpb.h"
#include "h264bsd_esp.h"
#include "h264bsd_esp_dirty.h"
#include "h264bsd_pie.h"
#include "h264bsd_storage.h"
#include "h264bsd_util.h"

/* tinyh264 exports this from h264bsd_decoder.c but never declared it. */
u32 h264bsdDecodeInternal(storage_t *pStorage, u8 *byteStrm, u32 len, u32 *readBytes);

static const char *TAG = "h264dec";

struct h264dec {
    storage_t     *st;
    const uint8_t *last_out;        /* data pointer of the previous output picture */
    uint32_t       content_id;
    bool           verify;
    h264dec_stats_t stats;
};

static void reset_pic_counters(void)
{
    g_h264bsd_skip_zero_mbs  = 0;
    g_h264bsd_skip_ref       = NULL;
    g_h264bsd_skip_ref_mixed = 0;
}

h264dec_t *h264dec_new(void)
{
    h264dec_t *dec = calloc(1, sizeof(*dec));
    if (!dec) return NULL;
    dec->st = h264bsdAlloc();
    if (!dec->st) {
        free(dec);
        return NULL;
    }
    /* noOutputReordering: baseline streams from gearhead have no B-frames
     * and we want each picture the moment it is decoded. */
    if (h264bsdInit(dec->st, 1) != HANTRO_OK) {
        ESP_LOGE(TAG, "h264bsdInit failed");
        h264bsdFree(dec->st);
        free(dec);
        return NULL;
    }
    reset_pic_counters();
    g_h264bsd_deblock_us = 0;
    h264bsdDirtyReset();
#ifdef H264BSD_ESP_PIE
    h264_pie_selfcheck();
#endif
    ESP_LOGI(TAG, "h264bsd decoder ready (fast path %s, stats %s)",
#ifdef H264BSD_ESP_FASTPATH
             "on",
#else
             "off",
#endif
#ifdef H264BSD_ESP_STATS
             "on"
#else
             "off"
#endif
             );
    return dec;
}

void h264dec_delete(h264dec_t *dec)
{
    if (!dec) return;
    h264bsdShutdown(dec->st);
    h264bsdFree(dec->st);
    free(dec);
}

h264dec_status_t h264dec_decode(h264dec_t *dec, const uint8_t *buf, size_t len,
                                size_t *consumed, h264dec_pic_t *pic)
{
    size_t off = 0;
    *consumed = 0;
    if (!dec || !buf || len == 0) return H264DEC_OK;

    int64_t t0 = esp_timer_get_time();
    uint64_t deblock0 = g_h264bsd_deblock_us;

    /* Same loop as tinyh264's h264bsdDecode: keep feeding NAL units until a
     * picture completes, an error stops us, or the buffer runs dry. */
    u32 ret = H264BSD_RDY;
    int hdrs_retry = 0;
    while (off < len) {
        u32 readBytes = 0;
        ret = h264bsdDecodeInternal(dec->st, (u8 *)buf + off, (u32)(len - off), &readBytes);
        off += readBytes;
        if (ret == H264BSD_PIC_RDY || ret == H264BSD_ERROR ||
            ret == H264BSD_PARAM_SET_ERROR || ret == H264BSD_MEMALLOC_ERROR) {
            break;
        }
        if (ret == H264BSD_HDRS_RDY) {
            /* Parameter sets activated (phase 1) on this slice's NAL; the
             * decoder reports 0 bytes read and expects the SAME NAL again to
             * finish activation (phase 2) and decode it — that is how
             * tinyh264's own loop works. Once. */
            if (hdrs_retry++ == 0) continue;
            break;
        }
        if (readBytes == 0) break;      /* nothing more it can take from this buffer */
    }
    *consumed = off;

    uint64_t deblock_dt = g_h264bsd_deblock_us - deblock0;
    dec->stats.decode_us  += (uint64_t)(esp_timer_get_time() - t0) - deblock_dt;
    dec->stats.deblock_us += deblock_dt;

    switch (ret) {
    case H264BSD_PIC_RDY: {
        dpbOutPicture_t *out = h264bsdDpbOutputPicture(dec->st->dpb);
        if (!out) return H264DEC_OK;
        storage_t *st = dec->st;
        u32 mbs = st->picSizeInMbs;
        bool unchanged = (g_h264bsd_skip_zero_mbs == mbs) && mbs > 0 &&
                         !g_h264bsd_skip_ref_mixed &&
                         g_h264bsd_skip_ref == dec->last_out;
        if (!unchanged) dec->content_id++;

        dec->stats.pictures++;
        dec->stats.total_mbs     += mbs;
        dec->stats.skip_zero_mbs += g_h264bsd_skip_zero_mbs;
        if (unchanged) dec->stats.unchanged_pics++;
        dec->stats.aliased_pics += g_h264bsd_pic_aliased;
        dec->stats.copied_pics  += g_h264bsd_pic_copied;
        g_h264bsd_pic_aliased = g_h264bsd_pic_copied = 0;
        dec->stats.mb_copied  += g_h264bsd_dirty_copied_mbs;
        dec->stats.mb_nocopy  += g_h264bsd_dirty_skipped_mbs;
        g_h264bsd_dirty_copied_mbs = g_h264bsd_dirty_skipped_mbs = 0;
#ifdef H264BSD_ESP_STATS
        {
            uint32_t mhz = (uint32_t)(esp_clk_cpu_freq() / 1000000);
            if (!mhz) mhz = 360;
            dec->stats.skip_mb_us  += (uint32_t)(g_h264bsd_cyc_skip_mb  / mhz);
            dec->stats.coded_mb_us += (uint32_t)(g_h264bsd_cyc_coded_mb / mhz);
            dec->stats.loop_us     += (uint32_t)(g_h264bsd_cyc_loop     / mhz);
            dec->stats.skip_mbs    += g_h264bsd_n_skip_mb;
            dec->stats.coded_mbs   += g_h264bsd_n_coded_mb;
            g_h264bsd_cyc_skip_mb = g_h264bsd_cyc_coded_mb = g_h264bsd_cyc_loop = 0;
            g_h264bsd_n_skip_mb = g_h264bsd_n_coded_mb = 0;
            dec->stats.parse_us    += (uint32_t)(g_h264bsd_cyc_parse    / mhz);
            dec->stats.residual_us += (uint32_t)(g_h264bsd_cyc_residual / mhz);
            dec->stats.mc_us       += (uint32_t)(g_h264bsd_cyc_mc       / mhz);
            dec->stats.write_us    += (uint32_t)(g_h264bsd_cyc_write    / mhz);
            dec->stats.intra_us    += (uint32_t)(g_h264bsd_cyc_intra    / mhz);
            g_h264bsd_cyc_parse = g_h264bsd_cyc_residual = g_h264bsd_cyc_mc = 0;
            g_h264bsd_cyc_write = g_h264bsd_cyc_intra = 0;
            dec->stats.inter_mbs += g_h264bsd_n_inter_mb;
            g_h264bsd_n_inter_mb = 0;
            dec->stats.mvpred_us   += (uint32_t)(g_h264bsd_cyc_mvpred   / mhz);
            dec->stats.skipcopy_us += (uint32_t)(g_h264bsd_cyc_skipcopy / mhz);
            g_h264bsd_cyc_mvpred = g_h264bsd_cyc_skipcopy = 0;
            for (int i = 0; i < 16; i++) {
                dec->stats.frac_us[i] += (uint32_t)(g_h264bsd_cyc_frac[i] / mhz);
                dec->stats.frac_n[i]  += g_h264bsd_n_frac[i];
                g_h264bsd_cyc_frac[i] = 0;
                g_h264bsd_n_frac[i]   = 0;
            }
        }
#endif
        reset_pic_counters();

        pic->data       = out->data;
        pic->width      = st->activeSps->picWidthInMbs * 16;
        pic->height     = st->activeSps->picHeightInMbs * 16;
        pic->content_id = dec->content_id;
        pic->unchanged  = unchanged;
        pic->version    = g_h264bsd_pic_version;
        pic->crc        = 0;
        if (dec->verify) {
            u32 bytes = st->picSizeInMbs * 384;   /* Y + Cb + Cr, 384 B/MB */
            pic->crc = esp_rom_crc32_le(0, out->data, bytes);
            ESP_LOGI(TAG, "verify: pic v%u crc %08x%s", (unsigned)pic->version,
                     (unsigned)pic->crc, unchanged ? " (unchanged)" : "");
        }
        dec->last_out   = out->data;
        return H264DEC_PIC;
    }
    case H264BSD_MEMALLOC_ERROR:
        ESP_LOGE(TAG, "out of memory");
        return H264DEC_NOMEM;
    case H264BSD_ERROR:
    case H264BSD_PARAM_SET_ERROR: {
        /* Lint-era code, no error reporting: show what it choked on. */
        char hex[3 * 24 + 1] = { 0 };
        size_t n = len < 24 ? len : 24;
        for (size_t i = 0; i < n; i++) sprintf(hex + 3 * i, "%02x ", buf[i]);
        ESP_LOGW(TAG, "ret %u at %u/%u: %s", (unsigned)ret, (unsigned)off,
                 (unsigned)len, hex);
        reset_pic_counters();
        h264bsdDirtyReset();            /* pixels may be anything now: copy all */
        return H264DEC_ERROR;
    }
    default:
        return H264DEC_OK;
    }
}

bool h264dec_changed_since(h264dec_t *dec, uint32_t from, uint32_t to,
                           uint32_t *mask, size_t words)
{
    (void)dec;
    return h264bsdDirtyChangedSince(from, to, (u32 *)mask, (u32)words) != 0;
}

void h264dec_set_verify(h264dec_t *dec, bool on)
{
    if (dec) dec->verify = on;
}

void h264dec_stats_take(h264dec_t *dec, h264dec_stats_t *out)
{
    if (!dec || !out) return;
    *out = dec->stats;
    memset(&dec->stats, 0, sizeof(dec->stats));
}
