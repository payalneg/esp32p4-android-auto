#include "h264_pipe.h"

#include <stdlib.h>
#include <string.h>

#include "display_video.h"
#include "esp_h264_dec.h"
#include "esp_h264_dec_param.h"
#include "esp_h264_dec_sw.h"
#include "esp_h264_types.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ui_mode.h"

static const char *TAG = "h264_pipe";

#define H264_TASK_STACK_BYTES  (8 * 1024)
/* Below the touch poll task (10) so touch always preempts a long YUV pass,
 * but above the AAP recv-loop (5) so the decoder/display run between recv
 * iterations rather than being starved by them. */
#define H264_TASK_PRIORITY     8

/* Decoder queue depth. Latency = every frame the phone has in flight on our
 * side: this queue + the 2 display-stage slots + the one being presented.
 * gearhead does not drop stale frames (a 32-deep queue once measured as 3 s
 * of touch→video lag, see h264_pipe_init), so when the phone outruns the
 * decoder — any motion — the queue sits full and each slot is one frame of
 * lag. Two is enough to keep the decoder fed across the recv loop's TLS
 * decrypt of the next frame; anything more only adds lag. */
#define H264_QUEUE_DEPTH       2

/* In VESC mode (LVGL dashboard owns the panel) we still decode every frame —
 * H.264 is stateful and dropping P-frames here would leave the decoder unable
 * to recover until the next IDR (this phone only emits IDRs ~every 10 s and
 * doesn't honour VideoFocusIndication as a forced-keyframe trigger). What we
 * do is throttle the *ack* rate to 5 fps: with max_unacked=4 advertised in
 * handle_av_setup, gearhead naturally stops sending more than ~5 frames per
 * second, so the decoder runs at ~150 ms/s instead of ~900 ms/s, and the
 * phone never has TCP packets to drop. Display work is skipped inside
 * display_video_show_yuv420 (it returns early when ui_mode == VESC). */
#define VESC_ACK_INTERVAL_US   (1000 * 1000 / 5)

typedef struct {
    uint8_t            *buf;
    size_t              len;
    h264_pipe_ack_cb_t  ack_cb;
    void               *ack_ctx;
} pipe_item_t;

static esp_h264_dec_handle_t       s_dec;
static esp_h264_dec_param_handle_t s_dec_param;
static QueueHandle_t               s_queue;
static TaskHandle_t                s_task;

/* Stats — reported once per second from the decoder task. */
static uint32_t s_decoded_frames;
static uint32_t s_decode_errors;
static uint64_t s_decode_total_us;
/* Per-second high-water mark of queue depth, sampled both inside the
 * decoder pop loop (just after we dequeued — so this is the depth left
 * behind after take) and by push() before it enqueues. Lets us tell
 * "queue spikes to 32 once then drains" from "queue stays near 32". */
static uint32_t s_queue_hwm;
static uint32_t s_push_blocked;       /* number of push waits > 50 ms */
static uint64_t s_push_blocked_us;    /* sum of those waits */

static void log_stats_once_per_second(void)
{
    static int64_t window_start_us;
    int64_t now = esp_timer_get_time();
    if (window_start_us == 0) {
        window_start_us = now;
        return;
    }
    if (now - window_start_us < 1000000) return;

    if (s_decoded_frames > 0 || s_decode_errors > 0 || s_queue_hwm > 0) {
        /* 1 Hz decode-pipeline summary — handy when tuning the queue /
         * ACK pacing, otherwise just clutter once AA is up. */
        ESP_LOGD(TAG,
                 "decoded %u fr (avg %llu us/fr) | q hwm %u/4 | "
                 "push blocked %u times, %llu ms total | errors %u",
                 (unsigned)s_decoded_frames,
                 (unsigned long long)(s_decoded_frames
                     ? (s_decode_total_us / s_decoded_frames) : 0),
                 (unsigned)s_queue_hwm,
                 (unsigned)s_push_blocked,
                 (unsigned long long)(s_push_blocked_us / 1000),
                 (unsigned)s_decode_errors);
    }
    s_decoded_frames  = 0;
    s_decode_errors   = 0;
    s_decode_total_us = 0;
    s_queue_hwm       = 0;
    s_push_blocked    = 0;
    s_push_blocked_us = 0;
    window_start_us   = now;
}

/* Decode one AA message worth of NAL units. Every picture it yields is
 * handed to the display stage; returns the number of frames queued there
 * (0 = nothing to show, or LVGL owns the panel and the frame was dropped). */
static int decode_and_show(const uint8_t *data, size_t len)
{
    static bool seen_resolution;
    int queued = 0;

    esp_h264_dec_in_frame_t  in  = {
        .raw_data = { (uint8_t *)data, (uint32_t)len },
    };
    esp_h264_dec_out_frame_t out = { 0 };

    while (in.raw_data.len > 0) {
        int64_t t0 = esp_timer_get_time();
        esp_h264_err_t e = esp_h264_dec_process(s_dec, &in, &out);
        int64_t dt = esp_timer_get_time() - t0;

        if (e != ESP_H264_ERR_OK) {
            s_decode_errors++;
            ESP_LOGW(TAG, "dec_process err=%d at %u/%u bytes",
                     (int)e,
                     (unsigned)in.consume,
                     (unsigned)in.raw_data.len);
            break;
        }

        if (out.out_size > 0) {
            s_decoded_frames++;
            s_decode_total_us += (uint64_t)dt;
            esp_h264_resolution_t res = {0};
            bool have_res = (s_dec_param &&
                esp_h264_dec_get_resolution(s_dec_param, &res)
                    == ESP_H264_ERR_OK);
            if (!seen_resolution && have_res) {
                ESP_LOGI(TAG, "first frame %ux%u, %u bytes I420",
                         res.width, res.height, (unsigned)out.out_size);
                seen_resolution = true;
            }
            if (have_res) {
                /* Shuffles out.outbuf into a staging slot right here (the
                 * next decode may overwrite it), then the core-0 display
                 * task does PPA + HUD + panel while we decode the next one. */
                if (display_video_submit_yuv420(out.outbuf, res.width, res.height,
                                                NULL, NULL) == ESP_OK) {
                    queued++;
                }
            }
        }

        if (in.consume == 0) break;
        in.raw_data.buffer += in.consume;
        in.raw_data.len    -= in.consume;
        in.consume = 0;
    }
    return queued;
}

/* Ack trampoline for frames that went through the display stage: fires on
 * the display task after the message's last frame has been presented, so
 * the phone is still paced by what is actually on the panel. */
typedef struct {
    h264_pipe_ack_cb_t cb;
    void              *ctx;
} deferred_ack_t;

/* Small ring so a fence can carry its own ack pointer without heap traffic;
 * a few more slots than the display queue can hold. */
static deferred_ack_t s_acks[8];
static unsigned       s_ack_next;

static void deferred_ack_cb(void *arg)
{
    deferred_ack_t *a = (deferred_ack_t *)arg;
    esp_err_t e = a->cb(a->ctx);
    if (e != ESP_OK) ESP_LOGW(TAG, "ack cb returned %s", esp_err_to_name(e));
}

/* With the display stage on core 0 this task no longer blocks on the PPA
 * every frame, so under a steady stream it is runnable back-to-back at prio
 * 8 and IDLE1 never gets the CPU — the task watchdog then complains about
 * IDLE1 every 5 s (seen in logs/20260902-213235.log). Once IDLE1 has made no
 * progress for a second, sleep one tick so it can feed the watchdog (and free
 * any task deleted on this core). Costs ≤10 ms per second of saturation. */
static void core1_idle_guard(void)
{
#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
    static uint32_t last_idle;
    static int64_t  last_change_us;
    uint32_t idle = (uint32_t)ulTaskGetIdleRunTimeCounterForCore(1);
    int64_t  now  = esp_timer_get_time();
    if (idle != last_idle || last_change_us == 0) {
        last_idle = idle;
        last_change_us = now;
        return;
    }
    if (now - last_change_us > 1000000) {
        vTaskDelay(1);
        last_change_us = esp_timer_get_time();
    }
#endif
}

static void decoder_task(void *arg)
{
    (void)arg;
    pipe_item_t it;
    int64_t last_ack_us = 0;
    while (true) {
        if (xQueueReceive(s_queue, &it, portMAX_DELAY) != pdTRUE) continue;
        core1_idle_guard();
        /* Depth right after dequeue: items still waiting behind this one.
         * A persistently full queue means push is paying the warn cost on
         * every frame; a one-off spike means we just had a burst. */
        uint32_t depth = (uint32_t)uxQueueMessagesWaiting(s_queue);
        if (depth > s_queue_hwm) s_queue_hwm = depth;
        int queued = decode_and_show(it.buf, it.len);
        free(it.buf);
        if (it.ack_cb && queued > 0) {
            /* Frames are on their way to the panel via the display task:
             * ack from there, after the last one is presented (a fence is
             * processed in order behind the frames). The ring is deeper
             * than the display queue, so the slot can't be reused while
             * its fence is still pending. */
            deferred_ack_t *a = &s_acks[s_ack_next++ % 8];
            a->cb  = it.ack_cb;
            a->ctx = it.ack_ctx;
            display_video_fence(deferred_ack_cb, a);
        } else if (it.ack_cb) {
            /* Nothing reached the display (SPS/PPS-only message, or the
             * VESC dashboard owns the panel and the frame was dropped). */
            /* VESC dashboard active → pace acks to 5 fps so phone backs
             * off via max_unacked. See VESC_ACK_INTERVAL_US comment above. */
            if (ui_mode_get() == UI_MODE_VESC) {
                int64_t now    = esp_timer_get_time();
                int64_t target = last_ack_us + VESC_ACK_INTERVAL_US;
                if (now < target) {
                    int wait_ms = (int)((target - now + 999) / 1000);
                    vTaskDelay(pdMS_TO_TICKS(wait_ms));
                }
            }
            last_ack_us = esp_timer_get_time();
            esp_err_t e = it.ack_cb(it.ack_ctx);
            if (e != ESP_OK) {
                ESP_LOGW(TAG, "ack cb returned %s", esp_err_to_name(e));
            }
        }
        log_stats_once_per_second();
    }
}

esp_err_t h264_pipe_init(void)
{
    if (s_dec) return ESP_OK;

    esp_h264_dec_cfg_sw_t cfg = {
        .pic_type = ESP_H264_RAW_FMT_I420,
    };
    esp_h264_err_t e = esp_h264_dec_sw_new(&cfg, &s_dec);
    if (e != ESP_H264_ERR_OK) {
        ESP_LOGE(TAG, "esp_h264_dec_sw_new: %d", (int)e);
        return ESP_FAIL;
    }
    e = esp_h264_dec_open(s_dec);
    if (e != ESP_H264_ERR_OK) {
        ESP_LOGE(TAG, "esp_h264_dec_open: %d", (int)e);
        esp_h264_dec_del(s_dec);
        s_dec = NULL;
        return ESP_FAIL;
    }
    if (esp_h264_dec_sw_get_param_hd(s_dec, &s_dec_param) != ESP_H264_ERR_OK) {
        s_dec_param = NULL;
    }

    /* 4 slots. Profiling (logs/20260511-143558.log) showed gearhead ignores
     * max_unacked=1 and streams at source rate (~30 fps), while our
     * decoder+display+ack pipeline tops out near ~10 fps. With queue=32 the
     * queue stayed pegged at 32 → 32×~100ms = ~3s of touch→video latency.
     * With queue=4 the recv-loop back-pressures TCP within ~400ms of decode
     * lag; phone-side TCP buffer fills and gearhead slows to our actual
     * rate via flow control instead of via the protocol's ack window. No
     * frames are dropped (push still blocks on full, just for less time
     * and with much less accumulated lag), so H.264 decoder state stays
     * valid across IDR boundaries. */
    s_queue = xQueueCreate(H264_QUEUE_DEPTH, sizeof(pipe_item_t));
    if (!s_queue) {
        ESP_LOGE(TAG, "xQueueCreate failed");
        esp_h264_dec_close(s_dec);
        esp_h264_dec_del(s_dec);
        s_dec = NULL;
        return ESP_ERR_NO_MEM;
    }

    /* Pin to core 1 — same core the openh264 dual-task helper runs on
     * (CONFIG_ESP_H264_DUAL_TASK_CORE=1, prio 17). Keeps both decode threads
     * on one core so the LVGL adapter (pinned to core 0 in ota_screen.c) can
     * render the dashboard without being preempted by decode work. */
    BaseType_t ok = xTaskCreatePinnedToCore(decoder_task, "h264_dec",
                                            H264_TASK_STACK_BYTES, NULL,
                                            H264_TASK_PRIORITY, &s_task, 1);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "decoder ready (async, queue=32, ack-on-display)");
    return ESP_OK;
}

void h264_pipe_push(const uint8_t *data, size_t len,
                    h264_pipe_ack_cb_t ack_cb, void *ack_ctx)
{
    if (!s_queue || !data || len == 0) return;

    pipe_item_t it = {
        .buf     = malloc(len),
        .len     = len,
        .ack_cb  = ack_cb,
        .ack_ctx = ack_ctx,
    };
    if (!it.buf) {
        ESP_LOGE(TAG, "push malloc %u failed", (unsigned)len);
        return;
    }
    memcpy(it.buf, data, len);

    /* Depth right before enqueue: shows what the producer sees. Combined
     * with the post-dequeue hwm we can tell whether queue is filling up
     * faster than it drains or sitting steady. */
    uint32_t pre_depth = (uint32_t)uxQueueMessagesWaiting(s_queue);
    if (pre_depth > s_queue_hwm) s_queue_hwm = pre_depth;

    /* Block on full queue. Should not happen with depth=32 + max_unacked
     * gating — accumulate wait time so the per-second stats can summarise. */
    int64_t t0 = esp_timer_get_time();
    if (xQueueSend(s_queue, &it, portMAX_DELAY) != pdTRUE) {
        free(it.buf);
        return;
    }
    int64_t dt_us = esp_timer_get_time() - t0;
    if (dt_us > 50 * 1000) {
        s_push_blocked++;
        s_push_blocked_us += (uint64_t)dt_us;
    }
}
