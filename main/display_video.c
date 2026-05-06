#include "display_video.h"

#include <string.h>

#include "bsp/esp-bsp.h"
#include "driver/ppa.h"
#include "esp_cache.h"
#include "esp_check.h"
#include "esp_imgfx_color_convert.h"
#include "esp_imgfx_types.h"
#include "esp_private/esp_cache_private.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "display_init.h"
#include "ui_mode.h"

static const char *TAG = "display_video";

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

/* Panel-native dimensions. BSP defines them as portrait (H=480 W=800 in
 * native coords). The OTA screen rotates LVGL output by 90° so the user
 * looks at landscape; for dummy-draw video the rotation has to be done by
 * PPA, since dummy_draw_blit bypasses the LVGL render pipeline. */
#define PANEL_NATIVE_W   BSP_LCD_H_RES   /* 480 */
#define PANEL_NATIVE_H   BSP_LCD_V_RES   /* 800 */

/* User-facing landscape resolution after rotation. */
#define USER_W           800
#define USER_H           480

static lv_display_t       *s_disp;
static esp_lcd_panel_handle_t s_panel;
static ppa_client_handle_t s_ppa;
static size_t              s_cache_line;
static void               *s_fb[3];
static int                 s_fb_count;
static int                 s_fb_idx;
static bool                s_adapter_paused;
static SemaphoreHandle_t   s_refresh_sem;
/* Intermediate landscape RGB565 buffer for the split PPA+CPU rotate
 * path. Allocated lazily in display_video_show_yuv420 once we know the
 * source dimensions. */
static uint16_t           *s_landscape_rgb;
static size_t              s_landscape_rgb_bytes;
static uint16_t            s_landscape_w;
static uint16_t            s_landscape_h;
/* SIMD-accelerated I420→RGB565 converter handle from esp_image_effects.
 * Allocated lazily once we know the source resolution; reused for every
 * frame. Not thread-safe — only the single decoder task touches it. */
static esp_imgfx_color_convert_handle_t s_imgfx;

static bool IRAM_ATTR refresh_done_cb(esp_lcd_panel_handle_t panel,
                                      esp_lcd_dpi_panel_event_data_t *edata,
                                      void *user_ctx)
{
    BaseType_t hp = pdFALSE;
    if (s_refresh_sem) xSemaphoreGiveFromISR(s_refresh_sem, &hp);
    return hp == pdTRUE;
}

esp_err_t display_video_init(void)
{
    if (s_panel) return ESP_OK;

    s_disp = (lv_display_t *)display_get();
    if (!s_disp) {
        ESP_LOGW(TAG, "no LVGL display — video will be silent");
        return ESP_ERR_INVALID_STATE;
    }

    s_panel = bsp_display_get_panel_handle();
    if (!s_panel) {
        ESP_LOGE(TAG, "bsp_display_get_panel_handle returned NULL");
        return ESP_FAIL;
    }

    esp_err_t err = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &s_cache_line);
    if (err != ESP_OK) s_cache_line = 64;

    /* Allocate two private RGB565 framebuffers in PSRAM, panel-native
     * 480×800 each. Bypassing the LCD's own DPI framebuffer pool while we
     * sort out why dummy_draw_blit-into-LCD-FB tiled the output 3× across
     * the panel — a private buffer + direct draw_bitmap takes the LVGL
     * adapter completely out of the loop. */
    size_t fb_bytes = ALIGN_UP(PANEL_NATIVE_W * PANEL_NATIVE_H * 2,
                               s_cache_line);
    s_fb_count = 2;
    for (int i = 0; i < s_fb_count; i++) {
        s_fb[i] = heap_caps_aligned_calloc(s_cache_line, 1, fb_bytes,
                                           MALLOC_CAP_SPIRAM);
        if (!s_fb[i]) {
            ESP_LOGE(TAG, "fb[%d] alloc %u failed", i, (unsigned)fb_bytes);
            return ESP_ERR_NO_MEM;
        }
    }
    ESP_LOGI(TAG, "allocated %d private framebuffers @%p,%p (%u bytes each)",
             s_fb_count, s_fb[0], s_fb[1], (unsigned)fb_bytes);

    ppa_client_config_t ppa_cfg = { .oper_type = PPA_OPERATION_SRM };
    err = ppa_register_client(&ppa_cfg, &s_ppa);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ppa_register_client: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "ready (panel %dx%d native, user %dx%d landscape)",
             PANEL_NATIVE_W, PANEL_NATIVE_H, USER_W, USER_H);
    return ESP_OK;
}

void display_video_yield_panel(void)
{
    if (s_adapter_paused) {
        esp_lv_adapter_resume();
        s_adapter_paused = false;
        ESP_LOGI(TAG, "yielded panel to LVGL");
    }
}

/* When non-zero, skip PPA and instead fill s_fb[0] with a hard-coded
 * R/G/B horizontal-band pattern in panel-native (480 wide × 800 tall)
 * so we can isolate display-path bugs from PPA bugs. Verified the
 * panel pipeline with this — pattern shows clean R|G|B bands when held
 * landscape, so panel + draw_bitmap is fine. Disable now. */
#define DISPLAY_TEST_PATTERN 0

/* When non-zero, skip PPA entirely and CPU-convert the decoder's Y plane
 * into a grayscale RGB565 panel-native (480×800) buffer with a 90° CCW
 * rotation done in the same loop. Slow (~5-10 ms per frame on P4) but
 * proved the decoder Y plane is fine and the panel draw_bitmap path is
 * fine. */
#define DISPLAY_CPU_YUV_BYPASS 0

/* When non-zero, split the work: PPA does the heavy YUV→RGB565 colour
 * conversion (and any scaling) into a landscape-oriented intermediate
 * buffer, then a tight CPU loop transposes that 16-bpp buffer into the
 * panel-native 480×800 framebuffer. PPA combined-rotate-and-convert
 * tiled the output for unknown reasons; rotation alone on RGB565 is
 * just an index swap and stays cheap (~3 ms on P4). Disabled — turned
 * out PPA misreads our YUV420 input layout too, so we go full CPU. */
#define DISPLAY_PPA_THEN_CPU_ROTATE 0

/* Full CPU pipeline: per-pixel YUV→RGB565 (BT.601 limited range) with
 * the 90° transpose folded into the output index. esp_h264 produces I420
 * which is what we read directly. ~25-30 ms/frame on a single 360 MHz
 * RISC-V core; with CONFIG_ESP_H264_DUAL_TASK=y the decoder uses both
 * cores at ~50% each, leaving real headroom for this loop and for the
 * AAP / WiFi / BT loops too. */
#define DISPLAY_CPU_YUV_FULL 0

/* CPU does YUV→RGB565 into a landscape staging buffer, PPA does the
 * 90° rotation RGB565→RGB565 into the panel-native framebuffer.
 * Reasoning: PPA's `PPA_SRM_COLOR_MODE_YUV420` expects the JPEG-decoder
 * macroblock layout, NOT the planar I420 esp_h264 produces — so combined
 * YUV+rotate misreads the input and tiles the output. PPA on RGB565+
 * rotate, however, is well-supported (Waveshare reference examples use
 * exactly this path). Splitting saves the CPU transpose step (~5-10 ms)
 * vs the all-CPU path. */
#define DISPLAY_CPU_YUV_THEN_PPA_ROTATE 1

esp_err_t display_video_show_yuv420(const uint8_t *yuv,
                                    uint16_t src_w, uint16_t src_h)
{
    if (!s_panel || !s_ppa || !s_disp) return ESP_ERR_INVALID_STATE;
    if (!yuv || src_w == 0 || src_h == 0) return ESP_ERR_INVALID_ARG;

    /* VESC dashboard owns the panel via LVGL — drop the frame and resume
     * the LVGL worker if we'd previously paused it. The H.264 decoder keeps
     * running (cheap to drop, expensive to tear down/restart), so frames
     * are silently discarded until the user toggles back to AA. */
    if (ui_mode_get() == UI_MODE_VESC) {
        if (s_adapter_paused) {
            esp_lv_adapter_resume();
            s_adapter_paused = false;
            ESP_LOGI(TAG, "VESC mode — resumed LVGL worker, frames dropped");
        }
        /* AA path naturally yields inside PPA + DPI flush; the drop path
         * doesn't, and h264_pipe keeps slamming us with frames as fast as
         * the decoder produces them. Without yielding here, IDLE0 starves
         * and TWDT fires (observed). One tick is enough — at 100 Hz tick
         * rate that's 10 ms, well under one source frame interval. */
        vTaskDelay(1);
        return ESP_OK;
    }

    /* On first frame (or after returning from VESC): pause the LVGL worker
     * entirely so it stops fighting us for the panel.
     *
     * NOTE: do NOT register our own on_refresh_done here. The LVGL adapter
     * (lvgl_bridge_v8.c) already registered its on_refresh_done +
     * on_color_trans_done at init, and esp_lcd_dpi_panel_register_event_callbacks
     * REPLACES the registration — taking those over leaves the adapter's
     * flush waiting forever for completion events on the next yield → resume.
     * Production path doesn't wait on s_refresh_sem anyway (see comment at the
     * draw_bitmap call below); DISPLAY_TEST_PATTERN's wait will just time out
     * harmlessly. */
    if (!s_adapter_paused) {
        if (!s_refresh_sem) {
            s_refresh_sem = xSemaphoreCreateBinary();
        }
        if (esp_lv_adapter_pause(2000) == ESP_OK) {
            s_adapter_paused = true;
            ESP_LOGI(TAG, "paused LVGL worker; panel ours");
        } else {
            ESP_LOGW(TAG, "esp_lv_adapter_pause timed out");
        }
    }

    int idx = s_fb_idx;
    s_fb_idx = (s_fb_idx + 1) % s_fb_count;

#if DISPLAY_TEST_PATTERN
    /* Fill s_fb[idx] with three horizontal RGB565 bands (R/G/B), top-down,
     * in panel-native 480 wide × 800 tall layout. If the panel shows three
     * clean horizontal bands when held in landscape orientation (after our
     * adapter rotation), the buffer-stride model is right. If we still
     * see tiling, the panel hardware is wired up differently than we
     * assume. */
    uint16_t *fb = (uint16_t *)s_fb[idx];
    const int W = PANEL_NATIVE_W;
    const int H = PANEL_NATIVE_H;
    const int third = H / 3;
    static const uint16_t COLORS[3] = {
        0xF800,  /* red   (5 bits R, 6 G, 5 B) */
        0x07E0,  /* green */
        0x001F,  /* blue  */
    };
    for (int y = 0; y < H; y++) {
        int band = (y < third) ? 0 : (y < 2 * third) ? 1 : 2;
        uint16_t c = COLORS[band];
        for (int x = 0; x < W; x++) {
            fb[y * W + x] = c;
        }
    }
    /* Cache flush so the DPI DMA sees the fresh bytes. */
    esp_cache_msync(fb, ALIGN_UP(W * H * 2, s_cache_line),
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0, W, H, fb);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "panel_draw_bitmap (test pattern): %s",
                 esp_err_to_name(err));
        return err;
    }
    if (s_refresh_sem) {
        xSemaphoreTake(s_refresh_sem, 0);
        xSemaphoreTake(s_refresh_sem, pdMS_TO_TICKS(100));
    }
    return ESP_OK;
#elif DISPLAY_CPU_YUV_FULL
    /* Full I420 → RGB565 colour conversion + 90° transpose, one pass.
     * Same dest-to-src mapping as the grayscale bypass: sx = dy,
     * sy = (W-1) - dx. esp_h264's I420 layout is Y plane (src_w*src_h)
     * then U plane (src_w/2 * src_h/2) then V plane (same).
     *
     * BT.601 limited-range integer math, fixed-point with 16-bit
     * coefficients (lifted from the standard reference equations):
     *   C = Y - 16; D = U - 128; E = V - 128
     *   R = clip((298*C + 409*E + 128) >> 8)
     *   G = clip((298*C - 100*D - 208*E + 128) >> 8)
     *   B = clip((298*C + 516*D + 128) >> 8)
     */
    {
        uint16_t *fb = (uint16_t *)s_fb[idx];
        const int W = PANEL_NATIVE_W;
        const int H = PANEL_NATIVE_H;
        const uint8_t *Yp = yuv;
        const uint8_t *Up = yuv + (size_t)src_w * src_h;
        const uint8_t *Vp = Up   + ((size_t)src_w * src_h) / 4;
        const int uv_w = src_w / 2;
        for (int dy = 0; dy < H; dy++) {
            int sx = dy;
            if (sx >= src_w) {
                memset(&fb[dy * W], 0, W * 2);
                continue;
            }
            for (int dx = 0; dx < W; dx++) {
                int sy = (W - 1) - dx;
                if (sy >= src_h) {
                    fb[dy * W + dx] = 0;
                    continue;
                }
                int Y = Yp[sy * src_w + sx];
                int U = Up[(sy / 2) * uv_w + (sx / 2)];
                int V = Vp[(sy / 2) * uv_w + (sx / 2)];
                int C = Y - 16;
                int D = U - 128;
                int E = V - 128;
                int R = (298 * C + 409 * E + 128) >> 8;
                int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
                int B = (298 * C + 516 * D + 128) >> 8;
                if (R < 0)   R = 0;   else if (R > 255) R = 255;
                if (G < 0)   G = 0;   else if (G > 255) G = 255;
                if (B < 0)   B = 0;   else if (B > 255) B = 255;
                fb[dy * W + dx] = ((R & 0xF8) << 8) |
                                  ((G & 0xFC) << 3) |
                                  ((B & 0xF8) >> 3);
            }
        }
        esp_cache_msync(fb, ALIGN_UP(W * H * 2, s_cache_line),
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0, W, H, fb);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "panel_draw_bitmap (full cpu): %s",
                     esp_err_to_name(err));
        }
        return err;
    }
#elif DISPLAY_CPU_YUV_THEN_PPA_ROTATE
    /* Step 1: SIMD-accelerated I420 → RGB565 in landscape via esp_imgfx
     * (uses ESP32-P4 PIE assembly path — ~2-4× faster than the naive
     * BT.601 C loop in DISPLAY_CPU_YUV_FULL).
     * Step 2: PPA does RGB565 → RGB565 with 90° rotation into the
     * panel framebuffer. PPA on RGB+rotate is well-tested (Waveshare
     * reference pipelines use exactly this); we keep the conversion on
     * the CPU because PPA's YUV420 path expects the JPEG-decoder
     * macroblock layout, not planar I420. */
    {
        /* Lazy-allocate the landscape RGB staging buffer + the imgfx
         * converter, both keyed on src dimensions. esp_imgfx I420 path
         * needs 16-pixel alignment of the source — 800×480 is fine. */
        size_t need = ALIGN_UP((size_t)src_w * src_h * 2, s_cache_line);
        if (s_landscape_rgb == NULL || need > s_landscape_rgb_bytes ||
            src_w != s_landscape_w || src_h != s_landscape_h) {
            free(s_landscape_rgb);
            s_landscape_rgb = heap_caps_aligned_calloc(s_cache_line, 1,
                                                       need,
                                                       MALLOC_CAP_SPIRAM);
            if (!s_landscape_rgb) {
                ESP_LOGE(TAG, "landscape_rgb alloc %u failed", (unsigned)need);
                return ESP_ERR_NO_MEM;
            }
            s_landscape_rgb_bytes = need;
            s_landscape_w = src_w;
            s_landscape_h = src_h;
            /* Reset converter so it picks up new resolution. */
            if (s_imgfx) {
                esp_imgfx_color_convert_close(s_imgfx);
                s_imgfx = NULL;
            }
        }
        if (!s_imgfx) {
            esp_imgfx_color_convert_cfg_t cfg = {
                .in_res         = { .width = src_w, .height = src_h },
                .in_pixel_fmt   = ESP_IMGFX_PIXEL_FMT_I420,
                .out_pixel_fmt  = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
                .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601,
            };
            esp_imgfx_err_t e = esp_imgfx_color_convert_open(&cfg, &s_imgfx);
            if (e != ESP_IMGFX_ERR_OK) {
                ESP_LOGE(TAG, "esp_imgfx_color_convert_open: %d", (int)e);
                return ESP_FAIL;
            }
        }

        esp_imgfx_data_t in  = {
            .data     = (uint8_t *)yuv,
            .data_len = (uint32_t)((size_t)src_w * src_h * 3 / 2),
        };
        esp_imgfx_data_t out = {
            .data     = (uint8_t *)s_landscape_rgb,
            .data_len = (uint32_t)((size_t)src_w * src_h * 2),
        };
        esp_imgfx_err_t e = esp_imgfx_color_convert_process(s_imgfx, &in, &out);
        if (e != ESP_IMGFX_ERR_OK) {
            ESP_LOGW(TAG, "imgfx_convert: %d", (int)e);
            return ESP_FAIL;
        }
        /* PPA reads via DMA bypassing the cache; flush CPU writes first. */
        esp_cache_msync(s_landscape_rgb, s_landscape_rgb_bytes,
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M);

        size_t out_buf_size = ALIGN_UP(PANEL_NATIVE_W * PANEL_NATIVE_H * 2,
                                       s_cache_line);
        ppa_srm_oper_config_t op = {
            .in = {
                .buffer  = s_landscape_rgb,
                .pic_w   = src_w,
                .pic_h   = src_h,
                .block_w = src_w,
                .block_h = src_h,
                .srm_cm  = PPA_SRM_COLOR_MODE_RGB565,
            },
            .out = {
                .buffer      = s_fb[idx],
                .buffer_size = out_buf_size,
                .pic_w       = PANEL_NATIVE_W,
                .pic_h       = PANEL_NATIVE_H,
                .srm_cm      = PPA_SRM_COLOR_MODE_RGB565,
            },
            .rotation_angle = PPA_SRM_ROTATION_ANGLE_270,
            .scale_x = 1.0f,
            .scale_y = 1.0f,
            .mode    = PPA_TRANS_MODE_BLOCKING,
        };
        esp_err_t err = ppa_do_scale_rotate_mirror(s_ppa, &op);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "ppa rotate: %s", esp_err_to_name(err));
            return err;
        }
        esp_cache_msync(s_fb[idx], out_buf_size,
                        ESP_CACHE_MSYNC_FLAG_DIR_M2C |
                        ESP_CACHE_MSYNC_FLAG_INVALIDATE);
        err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0,
                                        PANEL_NATIVE_W, PANEL_NATIVE_H,
                                        s_fb[idx]);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "panel_draw_bitmap (cpu+ppa): %s",
                     esp_err_to_name(err));
        }
        return err;
    }
#elif DISPLAY_CPU_YUV_BYPASS
    /* CPU path: take source Y plane (validated correct via dump) and
     * transpose-with-vertical-flip into panel-native 480×800 RGB565
     * grayscale.
     *
     * Empirically (test_pattern verified the panel side):
     *   user-landscape horizontal axis (0..USER_W) = panel row dy
     *   user-landscape vertical axis   (0..USER_H) = (W-1) - panel col dx
     * So src(sx, sy) → dst(dy=sx, dx=H-1-sy), or inverted:
     *   sx = dy, sy = (PANEL_NATIVE_W - 1) - dx
     * (the simple `sy = dx` we tried first showed the picture flipped
     * vertically). */
    {
        uint16_t *fb = (uint16_t *)s_fb[idx];
        const int W = PANEL_NATIVE_W;
        const int H = PANEL_NATIVE_H;
        for (int dy = 0; dy < H; dy++) {
            for (int dx = 0; dx < W; dx++) {
                int sx = dy;
                int sy = (W - 1) - dx;
                uint8_t Y = (sx < src_w && sy < src_h)
                                ? yuv[sy * src_w + sx]
                                : 0;
                uint16_t r5 = Y >> 3;
                uint16_t g6 = Y >> 2;
                uint16_t b5 = Y >> 3;
                fb[dy * W + dx] = (r5 << 11) | (g6 << 5) | b5;
            }
        }
        esp_cache_msync(fb, ALIGN_UP(W * H * 2, s_cache_line),
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0, W, H, fb);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "panel_draw_bitmap (cpu): %s",
                     esp_err_to_name(err));
        }
        return err;
    }
#elif DISPLAY_PPA_THEN_CPU_ROTATE
    /* Split path: PPA does YUV→RGB565 (no rotation, no scaling that
     * would require it). Result is landscape src_w × src_h RGB565. We
     * then transpose into the panel-native 480×800 framebuffer with a
     * cheap CPU loop. PPA's combined rotate-and-convert was tiling the
     * output for reasons we couldn't pin down with the obvious config
     * options; this split keeps the heavy colour math hardware-fast
     * while the trivial index swap runs on the CPU. */
    {
        /* Lazy-allocate the landscape staging buffer matching src_w×src_h. */
        size_t need = ALIGN_UP((size_t)src_w * src_h * 2, s_cache_line);
        if (s_landscape_rgb == NULL || need > s_landscape_rgb_bytes ||
            src_w != s_landscape_w || src_h != s_landscape_h) {
            free(s_landscape_rgb);
            s_landscape_rgb = heap_caps_aligned_calloc(s_cache_line, 1,
                                                       need,
                                                       MALLOC_CAP_SPIRAM);
            if (!s_landscape_rgb) {
                ESP_LOGE(TAG, "landscape_rgb alloc %u failed", (unsigned)need);
                return ESP_ERR_NO_MEM;
            }
            s_landscape_rgb_bytes = need;
            s_landscape_w = src_w;
            s_landscape_h = src_h;
        }

        ppa_srm_oper_config_t op = {
            .in = {
                .buffer = yuv,
                .pic_w = src_w,
                .pic_h = src_h,
                .block_w = src_w,
                .block_h = src_h,
                .srm_cm = PPA_SRM_COLOR_MODE_YUV420,
                .yuv_range = COLOR_RANGE_LIMIT,
                .yuv_std   = COLOR_CONV_STD_RGB_YUV_BT601,
            },
            .out = {
                .buffer = s_landscape_rgb,
                .buffer_size = s_landscape_rgb_bytes,
                .pic_w = src_w,
                .pic_h = src_h,
                .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
            },
            .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
            .scale_x = 1.0f,
            .scale_y = 1.0f,
            .mode = PPA_TRANS_MODE_BLOCKING,
        };
        esp_err_t err = ppa_do_scale_rotate_mirror(s_ppa, &op);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "ppa convert: %s", esp_err_to_name(err));
            return err;
        }
        esp_cache_msync(s_landscape_rgb, s_landscape_rgb_bytes,
                        ESP_CACHE_MSYNC_FLAG_DIR_M2C |
                        ESP_CACHE_MSYNC_FLAG_INVALIDATE);

        /* CPU transpose: same mapping that worked in the YUV-bypass
         * path (sx=dy, sy=W-1-dx) but now reading 16-bpp pixels. */
        uint16_t *dst = (uint16_t *)s_fb[idx];
        const int W = PANEL_NATIVE_W;
        const int H = PANEL_NATIVE_H;
        for (int dy = 0; dy < H; dy++) {
            int sx = dy;
            if (sx >= src_w) {
                /* Past source width — leave row black. */
                memset(&dst[dy * W], 0, W * 2);
                continue;
            }
            for (int dx = 0; dx < W; dx++) {
                int sy = (W - 1) - dx;
                dst[dy * W + dx] = (sy < src_h)
                    ? s_landscape_rgb[sy * src_w + sx]
                    : 0;
            }
        }
        esp_cache_msync(dst, ALIGN_UP(W * H * 2, s_cache_line),
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0, W, H, dst);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "panel_draw_bitmap (split): %s",
                     esp_err_to_name(err));
        }
        return err;
    }
#else  /* DISPLAY_TEST_PATTERN */

    /* PPA: YUV420 source → RGB565 in **panel-native** portrait layout
     * (480×800). The LVGL adapter's dummy_draw_blit goes straight to
     * esp_lcd_panel_draw_bitmap with no rotation applied (we read the
     * adapter source: display_lcd_blit_area calls draw_bitmap directly),
     * so PPA has to do the rotation that turns the landscape source into
     * the panel's portrait orientation.
     *
     * Order in ppa_srm.c is scale then rotate, and for 90/270 rotations
     * new_block_w = scale_y*in.block_h and new_block_h = scale_x*in.block_w.
     * Solving for new_block_w=480, new_block_h=800 with whatever (src_w,
     * src_h) the phone really sends gives:
     *   scale_y = 480 / src_h
     *   scale_x = 800 / src_w
     */
    static bool s_logged_src;
    if (!s_logged_src) {
        ESP_LOGI(TAG, "first frame to PPA: src %ux%u → panel %dx%d",
                 (unsigned)src_w, (unsigned)src_h,
                 PANEL_NATIVE_W, PANEL_NATIVE_H);
        s_logged_src = true;
    }

    size_t out_buf_size = ALIGN_UP(PANEL_NATIVE_W * PANEL_NATIVE_H * 2,
                                   s_cache_line);
    ppa_srm_oper_config_t op = {
        .in = {
            .buffer = yuv,
            .pic_w = src_w,
            .pic_h = src_h,
            .block_w = src_w,
            .block_h = src_h,
            .block_offset_x = 0,
            .block_offset_y = 0,
            .srm_cm = PPA_SRM_COLOR_MODE_YUV420,
            .yuv_range = COLOR_RANGE_LIMIT,
            .yuv_std   = COLOR_CONV_STD_RGB_YUV_BT601,
        },
        .out = {
            .buffer = s_fb[idx],
            .buffer_size = out_buf_size,
            .pic_w = PANEL_NATIVE_W,
            .pic_h = PANEL_NATIVE_H,
            .block_offset_x = 0,
            .block_offset_y = 0,
            .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
        },
        .rotation_angle = PPA_SRM_ROTATION_ANGLE_270,
        .scale_x = (float)PANEL_NATIVE_H / (float)src_w,
        .scale_y = (float)PANEL_NATIVE_W / (float)src_h,
        .mirror_x = false,
        .mirror_y = false,
        .rgb_swap = 0,
        .byte_swap = 0,
        .mode = PPA_TRANS_MODE_BLOCKING,
    };

    esp_err_t err = ppa_do_scale_rotate_mirror(s_ppa, &op);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ppa: %s (src %ux%u)", esp_err_to_name(err),
                 (unsigned)src_w, (unsigned)src_h);
        return err;
    }

    /* PPA writes via DMA, bypassing the cache. Make sure the CPU's view
     * of this buffer matches what PPA just wrote before the DPI driver
     * (also DMA) reads from it. Without this, M2C-stale lines were a
     * plausible cause of the 3-copy tiling we kept seeing. */
    esp_cache_msync(s_fb[idx], out_buf_size,
                    ESP_CACHE_MSYNC_FLAG_DIR_M2C |
                    ESP_CACHE_MSYNC_FLAG_INVALIDATE);

    /* Direct panel draw — LVGL worker is paused, panel is ours. Don't
     * wait for the refresh callback: at 30 fps source the 33 ms DPI
     * cycle plus our 100 ms wait cap throughput at ~10 fps, which makes
     * the AAP ring overflow ("ring full, dropped N bytes" — observed)
     * and the next decode lands on a corrupted bitstream. The DPI driver
     * already back-pressures inside esp_lcd_panel_draw_bitmap when its
     * previous transaction hasn't drained, so any necessary serialisation
     * happens there with no per-frame extra wait. With multiple SPIRAM
     * framebuffers the alternating write/scan-out is naturally
     * pipelined. */
    err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0,
                                    PANEL_NATIVE_W, PANEL_NATIVE_H,
                                    s_fb[idx]);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "panel_draw_bitmap: %s", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
#endif  /* DISPLAY_TEST_PATTERN */
}
