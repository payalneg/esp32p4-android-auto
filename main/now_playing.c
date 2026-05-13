#include "now_playing.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lvgl.h"

static const char *TAG = "now_playing";

/* Subset font generated from Super_VESC_Display/import/font/montserratMedium.ttf
 * covering Latin + Latin-1 supp + Latin Ext-A (Polish) + Cyrillic — the
 * stock guider font lv_font_montserratMedium_12 is ASCII-only and would
 * render Russian/Polish AVRCP titles as boxes. See main/font_song_title_12.c. */
LV_FONT_DECLARE(font_song_title_12);

/* Max field sizes — AVRCP attribute responses are theoretically up to 65 KB
 * each but in practice phones send < 100 bytes per field. Anything longer
 * is truncated. Including the NUL these fit comfortably in BSS. */
#define NP_FIELD_MAX 192

static struct {
    SemaphoreHandle_t mtx;
    char  title[NP_FIELD_MAX];
    char  artist[NP_FIELD_MAX];
    char  album[NP_FIELD_MAX];
    bool  playing;
    lv_obj_t *lbl_title;
    lv_obj_t *lbl_artist;
    lv_obj_t *lbl_album;
    lv_obj_t *icn_play;
} s;

/* lv_async_call payload — heap-allocated so the caller can return before
 * LVGL gets around to draining the queue. The async callback frees it. */
typedef struct {
    lv_obj_t *obj;
    char      text[NP_FIELD_MAX];
    bool      is_icon;
    bool      playing;
} np_async_t;

static void async_apply(void *arg) {
    np_async_t *a = (np_async_t *)arg;
    if (!a->obj) goto done;
    if (a->is_icon) {
        /* LV_SYMBOL_PLAY / PAUSE — built-in glyphs in the lvgl symbol font. */
        lv_label_set_text(a->obj, a->playing ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
    } else {
        lv_label_set_text(a->obj, a->text);
    }
done:
    free(a);
}

static void push_label(lv_obj_t *obj, const char *text) {
    if (!obj) return;
    np_async_t *a = calloc(1, sizeof(*a));
    if (!a) return;
    a->obj = obj;
    strlcpy(a->text, text ? text : "", sizeof(a->text));
    lv_async_call(async_apply, a);
}

static void async_set_unicode_font(void *arg) {
    lv_obj_t *obj = (lv_obj_t *)arg;
    if (obj) {
        lv_obj_set_style_text_font(obj, &font_song_title_12,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void push_set_unicode_font(lv_obj_t *obj) {
    if (!obj) return;
    lv_async_call(async_set_unicode_font, obj);
}

static void push_icon(lv_obj_t *obj, bool playing) {
    if (!obj) return;
    np_async_t *a = calloc(1, sizeof(*a));
    if (!a) return;
    a->obj = obj;
    a->is_icon = true;
    a->playing = playing;
    lv_async_call(async_apply, a);
}

void now_playing_init(void) {
    if (s.mtx) return;
    s.mtx = xSemaphoreCreateMutex();
    s.title[0]  = '\0';
    s.artist[0] = '\0';
    s.album[0]  = '\0';
    s.playing   = false;
    ESP_LOGI(TAG, "ready");
}

void now_playing_attach_labels(lv_obj_t *title,
                               lv_obj_t *artist,
                               lv_obj_t *album,
                               lv_obj_t *play_icon) {
    if (!s.mtx) now_playing_init();
    xSemaphoreTake(s.mtx, portMAX_DELAY);
    s.lbl_title  = title;
    s.lbl_artist = artist;
    s.lbl_album  = album;
    s.icn_play   = play_icon;
    /* Snapshot under lock, push outside — push_label takes the LVGL async
     * queue, which is independent of our mutex. */
    char t[NP_FIELD_MAX], a[NP_FIELD_MAX], al[NP_FIELD_MAX];
    bool playing = s.playing;
    strlcpy(t,  s.title,  sizeof(t));
    strlcpy(a,  s.artist, sizeof(a));
    strlcpy(al, s.album,  sizeof(al));
    xSemaphoreGive(s.mtx);

    /* Override the guider-generated ASCII-only font with our Unicode subset
     * so Cyrillic / Polish glyphs render correctly. Icon label keeps its
     * original font — it only shows LV_SYMBOL_*, which is symbol-font glyphs. */
    push_set_unicode_font(title);
    push_set_unicode_font(artist);
    push_set_unicode_font(album);

    push_label(title,  t);
    push_label(artist, a);
    push_label(album,  al);
    push_icon (play_icon, playing);
    ESP_LOGI(TAG, "labels attached (title=%p artist=%p album=%p icon=%p)",
             title, artist, album, play_icon);
}

void now_playing_set_track(const char *title,
                           const char *artist,
                           const char *album) {
    if (!s.mtx) now_playing_init();
    xSemaphoreTake(s.mtx, portMAX_DELAY);
    strlcpy(s.title,  title  ? title  : "", sizeof(s.title));
    strlcpy(s.artist, artist ? artist : "", sizeof(s.artist));
    strlcpy(s.album,  album  ? album  : "", sizeof(s.album));
    lv_obj_t *lt = s.lbl_title, *la = s.lbl_artist, *lb = s.lbl_album;
    xSemaphoreGive(s.mtx);

    ESP_LOGI(TAG, "track: \"%s\" — %s [%s]",
             title  ? title  : "",
             artist ? artist : "",
             album  ? album  : "");

    push_label(lt, title);
    push_label(la, artist);
    push_label(lb, album);
}

void now_playing_set_state(bool playing) {
    if (!s.mtx) now_playing_init();
    xSemaphoreTake(s.mtx, portMAX_DELAY);
    s.playing = playing;
    lv_obj_t *icn = s.icn_play;
    xSemaphoreGive(s.mtx);

    ESP_LOGI(TAG, "state: %s", playing ? "playing" : "paused");
    push_icon(icn, playing);
}
