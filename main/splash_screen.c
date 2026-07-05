#include "splash_screen.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strcasecmp */
#include <sys/stat.h>   /* stat() — LittleFS implements stat but NOT access() */

#include "bsp/esp-bsp.h"
#include "display_init.h"
#include "driver/jpeg_decode.h"
#include "driver/ppa.h"
#include "esp_cache.h"
#include "esp_private/esp_cache_private.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

#include "app_fs.h"
#include "dev_settings.h"

static const char *TAG = "splash";

#define SPLASH_PATH     "/vescfs/splash.gif"   /* POSIX path on the LittleFS */
#define SPLASH_LV_PATH  "S:" SPLASH_PATH       /* LVGL FS_POSIX drive letter 'S' */

/* New animated-splash type: a folder of JPEG frames named "<index>-<dur_ms>.jpg"
 * (e.g. "0-500.jpg"), pushed by the phone app from a GIF, each sized to the
 * user-facing landscape 800×480. Played frame-by-frame through the P4 hardware
 * JPEG decoder + PPA rotate, drawn STRAIGHT to the MIPI-DSI panel — bypassing
 * LVGL exactly like the AA video path. That's essential: the ~5 s dashboard
 * build holds the LVGL/BSP lock the whole time (super_vesc_ui_init on the main
 * task), so anything going through LVGL freezes on its first frame during boot.
 * A direct-to-panel task animates regardless. The GIF stays as the fallback
 * when this folder is absent or empty. */
#define SPLASH_DIR      "/vescfs/splash"
#define MAX_FRAMES      120
#define MIN_FRAME_MS    20
#define MAX_FRAME_MS    60000

/* Panel-native (portrait) and user-facing (landscape) dimensions. The panel
 * scans out 480×800; the app authors frames as 800×480 and PPA rotates 270°
 * into the panel buffer (same mapping display_video.c uses for AA video). */
#define PANEL_W   BSP_LCD_H_RES   /* 480 */
#define PANEL_H   BSP_LCD_V_RES   /* 800 */
#define LAND_W    800
#define LAND_H    480

#define ALIGN_UP(n, a)  (((n) + ((a) - 1)) & ~((a) - 1))

/* Bounded wait for the async LittleFS mount. A normal (already-formatted)
 * mount is fast; a first-ever boot formats (~1 min) — we never wait that long,
 * we just skip the splash (a fresh device has no splash file anyway). */
#define FS_WAIT_MS      800

/* Safety auto-stop: if the dashboard build never calls splash_screen_hide()
 * (hang, error path), the animation stops on its own. Generous enough to cover
 * the normal ~5 s dashboard build. */
#define SPLASH_MAX_MS   8000

/* ====================================================================== */
/* GIF mode (fallback) — LVGL top-layer overlay                           */
/* ====================================================================== */

static lv_obj_t   *s_overlay;
static lv_timer_t *s_safety_timer;

static void teardown_locked(void)
{
    if (s_safety_timer) {
        lv_timer_del(s_safety_timer);
        s_safety_timer = NULL;
    }
    if (s_overlay) {
        lv_obj_del(s_overlay);
        s_overlay = NULL;
    }
}

static void safety_timer_cb(lv_timer_t *t)
{
    (void)t;
    ESP_LOGI(TAG, "safety timeout — hiding gif splash");
    teardown_locked();  /* timer cb already runs under the LVGL lock */
}

static bool show_gif(void)
{
    struct stat st;
    if (stat(SPLASH_PATH, &st) != 0) {
        ESP_LOGI(TAG, "no %s — no splash", SPLASH_PATH);
        return false;
    }
    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGW(TAG, "display lock timeout — skipping splash");
        return false;
    }
    /* Full-screen black overlay on the top layer. The top layer sits above all
     * screens and is untouched by lv_scr_load(), so it stays visible across the
     * idle→dashboard screen swap that happens while the dashboard builds. */
    s_overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_overlay);
    lv_obj_set_size(s_overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *gif = lv_gif_create(s_overlay);
    lv_gif_set_src(gif, SPLASH_LV_PATH);
    lv_obj_center(gif);

    s_safety_timer = lv_timer_create(safety_timer_cb, SPLASH_MAX_MS, NULL);

    bsp_display_unlock();
    ESP_LOGI(TAG, "gif splash shown (%s)", SPLASH_PATH);
    return true;
}

/* ====================================================================== */
/* Frame-sequence mode — direct-to-panel JPEG playback                    */
/* ====================================================================== */

typedef struct {
    uint16_t idx;
    uint16_t dur;             /* ms, clamped to [MIN_FRAME_MS, MAX_FRAME_MS] */
    char     name[28];        /* "<idx>-<dur>.jpg" */
} frame_t;

static frame_t  *s_frames;
static int       s_count;
static size_t    s_max_fsize;

static jpeg_decoder_handle_t s_jpgd;
static ppa_client_handle_t   s_ppa;
static esp_lcd_panel_handle_t s_panel;
static size_t   s_cache_line = 64;

static uint8_t  *s_scratch;               /* DMA-aligned JPEG input copy */
static size_t    s_scratch_cap;
static uint8_t  *s_land;                  /* decoded landscape RGB565 (800×480) */
static size_t    s_land_bytes;
static uint8_t  *s_pfb[2];                /* panel-native RGB565 (480×800), double-buffered */
static size_t    s_pfb_bytes;

static TaskHandle_t      s_frame_task;
static SemaphoreHandle_t s_frame_done;
static bool              s_frame_active;
static bool              s_adapter_paused;
static int               s_target_loops = 1;   /* splash repeats (settings); 0 handled in show() */

/* "<digits>-<digits>.jpg" (or .jpeg), case-insensitive extension. */
static bool parse_frame_name(const char *name, int *idx, int *dur)
{
    const char *p = name;
    if (!isdigit((unsigned char)*p)) return false;
    long a = 0;
    while (isdigit((unsigned char)*p)) {
        a = a * 10 + (*p - '0');
        if (a > 100000) return false;
        p++;
    }
    if (*p != '-') return false;
    p++;
    if (!isdigit((unsigned char)*p)) return false;
    long b = 0;
    while (isdigit((unsigned char)*p)) {
        b = b * 10 + (*p - '0');
        if (b > 600000) return false;
        p++;
    }
    if (strcasecmp(p, ".jpg") != 0 && strcasecmp(p, ".jpeg") != 0) return false;
    *idx = (int)a;
    *dur = (int)b;
    return true;
}

static int cmp_frame(const void *a, const void *b)
{
    return (int)((const frame_t *)a)->idx - (int)((const frame_t *)b)->idx;
}

/* Join via a helper with a runtime size arg so -Werror=format-truncation
 * (which fires on compile-time-known sizes vs. an up-to-255-byte d_name)
 * stays quiet — snprintf truncates safely regardless (same as ble_files.c). */
static void join_path(char *out, size_t out_sz, const char *dir, const char *name)
{
    snprintf(out, out_sz, "%s/%s", dir, name);
}

/* Scan SPLASH_DIR into the sorted s_frames[] array. Returns the frame count
 * (0 → no frame splash; caller falls back to the GIF). */
static int enumerate_frames(void)
{
    DIR *d = opendir(SPLASH_DIR);
    if (!d) return 0;

    s_frames = calloc(MAX_FRAMES, sizeof(frame_t));
    if (!s_frames) { closedir(d); return 0; }

    int n = 0;
    s_max_fsize = 0;
    struct dirent *de;
    char full[160];
    while ((de = readdir(d)) != NULL && n < MAX_FRAMES) {
        int idx, dur;
        if (!parse_frame_name(de->d_name, &idx, &dur)) continue;
        if (dur < MIN_FRAME_MS) dur = MIN_FRAME_MS;
        if (dur > MAX_FRAME_MS) dur = MAX_FRAME_MS;
        join_path(full, sizeof full, SPLASH_DIR, de->d_name);
        struct stat st;
        if (stat(full, &st) != 0 || st.st_size == 0) continue;
        if ((size_t)st.st_size > s_max_fsize) s_max_fsize = (size_t)st.st_size;
        s_frames[n].idx = (uint16_t)idx;
        s_frames[n].dur = (uint16_t)dur;
        strlcpy(s_frames[n].name, de->d_name, sizeof s_frames[n].name);
        n++;
    }
    closedir(d);

    if (n == 0) { free(s_frames); s_frames = NULL; return 0; }
    qsort(s_frames, n, sizeof(frame_t), cmp_frame);
    s_count = n;
    return n;
}

static bool ensure_scratch(size_t need)
{
    if (need <= s_scratch_cap) return true;
    if (s_scratch) heap_caps_free(s_scratch);
    size_t alloc = ALIGN_UP(need, 1024);
    /* JPEG hardware reads via DMA — input must be cache-aligned PSRAM. */
    s_scratch = heap_caps_aligned_calloc(64, 1, alloc,
                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (!s_scratch) { s_scratch_cap = 0; return false; }
    s_scratch_cap = alloc;
    return true;
}

/* Read frame i's JPEG file fully into the scratch buffer. */
static bool read_frame(int i, size_t *out_len)
{
    if (!ensure_scratch(s_max_fsize)) return false;
    char full[180];
    join_path(full, sizeof full, SPLASH_DIR, s_frames[i].name);
    FILE *f = fopen(full, "rb");
    if (!f) return false;
    size_t n = fread(s_scratch, 1, s_scratch_cap, f);
    fclose(f);
    if (n == 0) return false;
    *out_len = n;
    return true;
}

static bool decode_into_land(size_t len)
{
    jpeg_decode_cfg_t cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        /* BGR matches what the ST7701 / panel pipeline expects on this board —
         * RGB order would surface as a red/blue swap (same as music_info_view.c
         * and the AA video path). */
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
    };
    uint32_t out_size = 0;
    esp_err_t r = jpeg_decoder_process(s_jpgd, &cfg, s_scratch, len,
                                       s_land, s_land_bytes, &out_size);
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "frame jpeg decode failed: %s", esp_err_to_name(r));
        return false;
    }
    return true;
}

/* PPA: landscape s_land (800×480) → RGB565 + 270° rotate → panel-native
 * s_pfb[idx] (480×800). Mirrors the RGB565 rotate path in display_video.c. */
static bool ppa_rotate_to(int idx)
{
    esp_cache_msync(s_land, s_land_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    ppa_srm_oper_config_t op = {
        .in = {
            .buffer  = s_land,
            .pic_w   = LAND_W,
            .pic_h   = LAND_H,
            .block_w = LAND_W,
            .block_h = LAND_H,
            .srm_cm  = PPA_SRM_COLOR_MODE_RGB565,
        },
        .out = {
            .buffer      = s_pfb[idx],
            .buffer_size = s_pfb_bytes,
            .pic_w       = PANEL_W,
            .pic_h       = PANEL_H,
            .srm_cm      = PPA_SRM_COLOR_MODE_RGB565,
        },
        /* Flipped mounting rotates the other way (see display_init.c). */
        .rotation_angle = display_flip_active() ? PPA_SRM_ROTATION_ANGLE_90
                                                : PPA_SRM_ROTATION_ANGLE_270,
        .scale_x = 1.0f,
        .scale_y = 1.0f,
        .mode    = PPA_TRANS_MODE_BLOCKING,
    };
    esp_err_t e = ppa_do_scale_rotate_mirror(s_ppa, &op);
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "ppa rotate: %s", esp_err_to_name(e));
        return false;
    }
    esp_cache_msync(s_pfb[idx], s_pfb_bytes,
                    ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    return true;
}

/* Free everything the frame mode allocated and hand the panel back to LVGL.
 * Called from the fail path in show_frames (no task) and from
 * splash_screen_hide() after the task has stopped. NULL-safe / idempotent. */
static void frame_free_resources(void)
{
    if (s_adapter_paused) {
        esp_lv_adapter_resume();   /* LVGL repaints the live screen over our last frame */
        s_adapter_paused = false;
    }
    if (s_land)   { heap_caps_free(s_land);   s_land = NULL; }
    if (s_pfb[0]) { heap_caps_free(s_pfb[0]); s_pfb[0] = NULL; }
    if (s_pfb[1]) { heap_caps_free(s_pfb[1]); s_pfb[1] = NULL; }
    if (s_scratch){ heap_caps_free(s_scratch); s_scratch = NULL; s_scratch_cap = 0; }
    if (s_ppa)    { ppa_unregister_client(s_ppa); s_ppa = NULL; }
    if (s_jpgd)   { jpeg_del_decoder_engine(s_jpgd); s_jpgd = NULL; }
    if (s_frames) { free(s_frames); s_frames = NULL; }
    s_count = 0;
    s_panel = NULL;
}

/* Hold a frame for its full duration so the animation actually plays. We do
 * NOT cut a frame short on a stop request — hide() is honoured at the next loop
 * boundary instead (see frame_task), so the user always sees a complete loop.
 * The only early-out is the hard SPLASH_MAX_MS cap, so a long GIF can't block
 * boot forever. */
static bool past_cap(TickType_t start)
{
    return (xTaskGetTickCount() - start) > pdMS_TO_TICKS(SPLASH_MAX_MS);
}

static void frame_sleep(uint32_t ms, TickType_t start)
{
    uint32_t waited = 0;
    while (waited < ms) {
        if (past_cap(start)) return;
        uint32_t slice = (ms - waited > 50) ? 50 : (ms - waited);
        vTaskDelay(pdMS_TO_TICKS(slice));
        waited += slice;
    }
}

static void frame_task(void *arg)
{
    (void)arg;
    int idx = 0;
    TickType_t start = xTaskGetTickCount();

    /* Play the animation s_target_loops times, then stop. Even on a fast boot
     * (dashboard built in ~75 ms) hide() waits for these loops to finish, so the
     * whole animation is shown the configured number of times rather than a
     * single frame. The SPLASH_MAX_MS cap bounds the total so a long GIF can
     * never hold boot hostage. */
    for (int loop = 0; loop < s_target_loops; loop++) {
        for (int i = 0; i < s_count; i++) {
            if (past_cap(start)) goto done;
            size_t len = 0;
            if (read_frame(i, &len) && decode_into_land(len) && ppa_rotate_to(idx)) {
                esp_lcd_panel_draw_bitmap(s_panel, 0, 0, PANEL_W, PANEL_H, s_pfb[idx]);
                idx ^= 1;   /* double-buffer: next frame draws the other buffer */
            }
            frame_sleep(s_frames[i].dur, start);
        }
    }
done:
    /* Stop touching the panel and signal hide(); resources + adapter resume are
     * freed by hide() (always called from main at boot). */
    xSemaphoreGive(s_frame_done);
    vTaskDelete(NULL);
}

/* Try to start the direct-to-panel frame splash. Returns false (leaving nothing
 * running, everything freed) if there are no frames or any setup step fails —
 * the caller then falls back to the GIF. */
static bool show_frames(void)
{
    if (enumerate_frames() <= 0) return false;

    if (esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &s_cache_line) != ESP_OK) {
        s_cache_line = 64;
    }

    jpeg_decode_engine_cfg_t ecfg = { .intr_priority = 0, .timeout_ms = 200 };
    if (jpeg_new_decoder_engine(&ecfg, &s_jpgd) != ESP_OK) {
        ESP_LOGW(TAG, "jpeg engine init failed");
        goto fail;
    }

    /* Validate the first frame is the expected 800×480 (the app's contract). */
    size_t len = 0;
    if (!read_frame(0, &len)) goto fail;
    jpeg_decode_picture_info_t info = {0};
    if (jpeg_decoder_get_info(s_scratch, len, &info) != ESP_OK) {
        ESP_LOGW(TAG, "cannot read frame 0 dimensions");
        goto fail;
    }
    if (info.width != LAND_W || info.height != LAND_H) {
        ESP_LOGW(TAG, "frame is %ux%u, expected %dx%d — using gif fallback",
                 info.width, info.height, LAND_W, LAND_H);
        goto fail;
    }

    s_land_bytes = ALIGN_UP((size_t)LAND_W * LAND_H * 2, s_cache_line);
    s_pfb_bytes  = ALIGN_UP((size_t)PANEL_W * PANEL_H * 2, s_cache_line);
    /* Uninitialised on purpose: every frame fully overwrites these via DMA
     * (jpeg → s_land, PPA → s_pfb), and the CPU never writes them, so there are
     * no dirty cache lines to worry about. */
    s_land   = heap_caps_aligned_alloc(s_cache_line, s_land_bytes, MALLOC_CAP_SPIRAM);
    s_pfb[0] = heap_caps_aligned_alloc(s_cache_line, s_pfb_bytes, MALLOC_CAP_SPIRAM);
    s_pfb[1] = heap_caps_aligned_alloc(s_cache_line, s_pfb_bytes, MALLOC_CAP_SPIRAM);
    if (!s_land || !s_pfb[0] || !s_pfb[1]) {
        ESP_LOGW(TAG, "no PSRAM for splash framebuffers");
        goto fail;
    }

    ppa_client_config_t ppa_cfg = { .oper_type = PPA_OPERATION_SRM };
    if (ppa_register_client(&ppa_cfg, &s_ppa) != ESP_OK) {
        ESP_LOGW(TAG, "ppa_register_client failed");
        goto fail;
    }

    s_panel = bsp_display_get_panel_handle();
    if (!s_panel) { ESP_LOGW(TAG, "no panel handle"); goto fail; }

    /* Take the panel away from LVGL for the whole splash: its flush worker
     * would otherwise paint the idle/ota screens over our frames. The dashboard
     * build itself doesn't need the worker (it only creates widgets under the
     * lock); resume() in hide() repaints the finished dashboard. */
    if (esp_lv_adapter_pause(2000) == ESP_OK) {
        s_adapter_paused = true;
    } else {
        ESP_LOGW(TAG, "esp_lv_adapter_pause timed out — splash may flicker");
    }

    s_frame_done = xSemaphoreCreateBinary();
    if (!s_frame_done) goto fail;
    if (xTaskCreatePinnedToCore(frame_task, "splash_anim", 5120, NULL, 3,
                                &s_frame_task, 0) != pdPASS) {
        vSemaphoreDelete(s_frame_done);
        s_frame_done = NULL;
        goto fail;
    }

    s_frame_active = true;
    ESP_LOGI(TAG, "frame splash shown (%d frames, %dx%d)", s_count, LAND_W, LAND_H);
    return true;

fail:
    frame_free_resources();   /* synchronous — no frame task running yet */
    return false;
}

/* ====================================================================== */
/* Public API                                                             */
/* ====================================================================== */

void splash_screen_show(void)
{
    if (s_frame_active || s_overlay) return;  /* already showing */

    app_fs_ensure();
    for (int waited = 0; !app_fs_ready() && waited < FS_WAIT_MS; waited += 50) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (!app_fs_ready()) {
        ESP_LOGI(TAG, "storage not ready — no splash");
        return;
    }

    /* Boot-splash repeats setting: 0 disables the splash entirely. */
    uint8_t loops = settings_get_splash_loops();
    if (loops == 0) {
        ESP_LOGI(TAG, "splash disabled (loops=0)");
        return;
    }
    s_target_loops = loops;

    /* New animated frame splash takes precedence; the GIF is the fallback. */
    if (show_frames()) return;
    show_gif();
}

void splash_screen_hide(void)
{
    /* Frame mode: the direct-to-panel task plays the configured number of loops
     * then gives s_frame_done and exits on its own. Wait for it (bounded by the
     * SPLASH_MAX_MS cap inside the task + margin), then free resources and hand
     * the panel back to LVGL. This runs at boot, long before any phone connects,
     * so blocking here for the splash duration is fine. */
    if (s_frame_active) {
        if (s_frame_done) {
            if (xSemaphoreTake(s_frame_done, pdMS_TO_TICKS(SPLASH_MAX_MS + 3000)) != pdTRUE) {
                ESP_LOGW(TAG, "frame splash wait timeout");
            }
            vSemaphoreDelete(s_frame_done);
            s_frame_done = NULL;
        }
        s_frame_task = NULL;
        frame_free_resources();
        s_frame_active = false;
        ESP_LOGI(TAG, "frame splash hidden");
        return;
    }

    /* GIF mode. */
    if (!s_overlay && !s_safety_timer) return;

    /* Tear down deterministically — do NOT bail on a single short lock
     * timeout. The prio-6 LVGL worker outranks the prio-1 main task, so the
     * instant ui_mode_init() releases the lock the worker grabs it to paint
     * the freshly-loaded ~1700-widget dashboard, and that first full 800×480
     * render holds the lock well past 1 s. A short attempt here would time out
     * and leave teardown to the safety timer — but that timer is an LVGL timer,
     * and auto-reconnect pauses the LVGL worker on the first AA video frame
     * before it ever fires (paused worker → lv_timer_handler stops → timer
     * never runs). The black top-layer overlay would then be stranded over
     * every LVGL screen forever. This runs at boot, long before any phone
     * connects, so it's safe to wait as long as it takes to grab the lock. */
    bool locked = false;
    for (int attempt = 0; attempt < 10 && !locked; attempt++) {
        locked = (bsp_display_lock(1000) == ESP_OK);
    }
    if (!locked) {
        ESP_LOGW(TAG, "display lock unavailable — splash left to safety timer");
        return;  /* safety timer still pending */
    }
    teardown_locked();
    bsp_display_unlock();
    ESP_LOGI(TAG, "splash hidden");
}
