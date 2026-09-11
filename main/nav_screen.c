#include "nav_screen.h"

#include <stdatomic.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_cache.h"
#include "esp_private/esp_cache_private.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "fonts/aabridge_fonts.h"

static const char *TAG = "nav_screen";

/* How often the LVGL side looks for a committed frame. Frames arrive around
 * once a second; a short tick only costs a flag read and keeps the "waiting"
 * text honest. */
#define TICK_PERIOD_MS 100

/* Ticks the just-retired front buffer stays off limits after a swap. LVGL
 * renders the image into the panel framebuffer asynchronously (and twice, once
 * per framebuffer in DOUBLE_DIRECT), so handing it straight back to the BLE
 * worker's DMA would tear the picture being drawn. 300 ms at one frame per
 * second costs nothing. */
#define SWAP_COOLDOWN_TICKS 3

static lv_obj_t *s_screen;
static lv_obj_t *s_img;
static lv_obj_t *s_status_box;
static lv_obj_t *s_status_lbl;
static lv_timer_t *s_tick;

static uint16_t *s_fb[2];
static size_t    s_fb_bytes;
static int       s_front;          /* index of the buffer lv_img points at */
static lv_img_dsc_t s_dsc;

/* Written by the BLE worker, read by the LVGL tick (and the other way round
 * for the cooldown) — one flag each way, so plain atomics are enough. */
static atomic_bool s_pending;
static atomic_int  s_cooldown;

static atomic_bool s_active;
static atomic_bool s_phone;
static atomic_bool s_streaming;

static atomic_uint s_frames;
static _Atomic int64_t s_last_frame_us;

/* Text currently on the placeholder, so a tick that changes nothing does not
 * touch LVGL at all. */
static const char *s_status_text;

static void set_status(const char *text)
{
    if (text == s_status_text) return;
    s_status_text = text;
    if (!s_status_box) return;
    if (text == NULL) {
        lv_obj_add_flag(s_status_box, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_label_set_text(s_status_lbl, text);
    lv_obj_clear_flag(s_status_box, LV_OBJ_FLAG_HIDDEN);
}

static void show_committed_frame(void)
{
    s_front ^= 1;
    s_dsc.header.always_zero = 0;
    s_dsc.header.w  = NAV_SCREEN_W;
    s_dsc.header.h  = NAV_SCREEN_H;
    s_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_dsc.data      = (const uint8_t *)s_fb[s_front];
    s_dsc.data_size = s_fb_bytes;
    /* Same descriptor pointer every frame — drop LVGL's cached decode of it
     * or the previous picture stays on screen (music_info_view.c hit this). */
    lv_img_cache_invalidate_src(&s_dsc);
    lv_img_set_src(s_img, &s_dsc);
    lv_obj_invalidate(s_img);
    atomic_store(&s_frames, atomic_load(&s_frames) + 1);
    atomic_store(&s_last_frame_us, esp_timer_get_time());
}

static void tick_cb(lv_timer_t *t)
{
    (void)t;
    /* DOUBLE_DIRECT redraws only dirty regions, and an invalidate on a screen
     * that is not live bleeds onto the one that is. Do nothing at all unless
     * the rider is looking at us. */
    if (lv_scr_act() != s_screen) return;

    int cd = atomic_load(&s_cooldown);
    if (cd > 0) atomic_store(&s_cooldown, cd - 1);

    if (atomic_exchange(&s_pending, false)) {
        show_committed_frame();
        atomic_store(&s_cooldown, SWAP_COOLDOWN_TICKS);
    }

    /* A caption only when there is nothing to look at, or when the phone
     * said it stopped. Quiet is not a fault: the app skips frames whose
     * pixels did not change, so a parked bike sends nothing for minutes and
     * the last picture is still the right one. */
    const bool have_frame = atomic_load(&s_frames) > 0;
    if (!atomic_load(&s_phone)) {
        set_status("Phone not connected");
    } else if (!have_frame) {
        set_status("Waiting for the navigator app...");
    } else if (!atomic_load(&s_streaming)) {
        set_status("Navigator app stopped streaming");
    } else {
        set_status(NULL);
    }
}

esp_err_t nav_screen_init(void)
{
    if (s_screen) return ESP_OK;

    size_t line = 64;
    if (esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &line) != ESP_OK || line == 0) {
        line = 64;
    }
    s_fb_bytes = ((size_t)NAV_SCREEN_W * NAV_SCREEN_H * 2 + line - 1) & ~(line - 1);
    for (int i = 0; i < 2; i++) {
        /* The JPEG decoder and the PPA both write here over DMA. */
        s_fb[i] = heap_caps_aligned_calloc(line, 1, s_fb_bytes,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
        if (!s_fb[i]) {
            ESP_LOGE(TAG, "no PSRAM for a %u-byte framebuffer", (unsigned)s_fb_bytes);
            for (int j = 0; j < 2; j++) {
                if (s_fb[j]) { heap_caps_free(s_fb[j]); s_fb[j] = NULL; }
            }
            return ESP_ERR_NO_MEM;
        }
    }

    if (bsp_display_lock(1000) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl lock timeout");
        return ESP_FAIL;
    }

    s_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x101418), 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    s_img = lv_img_create(s_screen);
    lv_obj_set_pos(s_img, 0, 0);
    lv_obj_set_size(s_img, NAV_SCREEN_W, NAV_SCREEN_H);

    /* Placeholder card, centred, drawn over whatever the last picture was. */
    s_status_box = lv_obj_create(s_screen);
    lv_obj_set_size(s_status_box, 560, 96);
    lv_obj_align(s_status_box, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_status_box, lv_color_hex(0x1c2530), 0);
    lv_obj_set_style_bg_opa(s_status_box, LV_OPA_80, 0);
    lv_obj_set_style_border_width(s_status_box, 0, 0);
    lv_obj_set_style_radius(s_status_box, 12, 0);
    lv_obj_clear_flag(s_status_box, LV_OBJ_FLAG_SCROLLABLE);

    s_status_lbl = lv_label_create(s_status_box);
    lv_obj_center(s_status_lbl);
    lv_obj_set_style_text_color(s_status_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_status_lbl, &aabridge_font_32, 0);
    lv_label_set_text(s_status_lbl, "Phone not connected");
    s_status_text = "Phone not connected";

    s_tick = lv_timer_create(tick_cb, TICK_PERIOD_MS, NULL);

    bsp_display_unlock();
    ESP_LOGI(TAG, "navigator screen ready (2 x %u-byte framebuffers)",
             (unsigned)s_fb_bytes);
    return ESP_OK;
}

lv_obj_t *nav_screen_get(void) { return s_screen; }

uint16_t *nav_screen_back_buffer(void)
{
    if (!s_fb[0] || !s_fb[1]) return NULL;
    if (atomic_load(&s_pending)) return NULL;
    if (atomic_load(&s_cooldown) > 0) return NULL;
    return s_fb[s_front ^ 1];
}

size_t nav_screen_back_buffer_bytes(void) { return s_fb_bytes; }

void nav_screen_commit(void) { atomic_store(&s_pending, true); }

void nav_screen_set_active(bool active) { atomic_store(&s_active, active); }
bool nav_screen_active(void) { return atomic_load(&s_active); }

void nav_screen_set_phone(bool connected)
{
    atomic_store(&s_phone, connected);
    if (!connected) atomic_store(&s_streaming, false);
}

void nav_screen_set_streaming(bool streaming)
{
    atomic_store(&s_streaming, streaming);
}

uint32_t nav_screen_frames_shown(void) { return atomic_load(&s_frames); }
int64_t  nav_screen_last_frame_us(void) { return atomic_load(&s_last_frame_us); }
