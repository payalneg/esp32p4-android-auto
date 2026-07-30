/*
    Copyright 2026 Adapted to ESP-IDF for ESP32-P4 (GPL-3.0).

    LISP code transfer over CAN — see vesc_lisp_code.h.

    Wire format (matches VESC Tool's CodeLoader — the same layout the Flutter
    app's vesc_code_loader.dart uses over the NUS bridge):

      On flash the VESC stores  [u32 size][u16 crc16][packed]  where
      packed = [u16 flags=0][code bytes][NUL][i16 num_imports=0] and the
      size field counts packed MINUS the 2 flag bytes. WRITE offsets are
      absolute into that layout (header included), but READ is served from
      flash_helper_code_data() which points PAST the 8 header bytes — so a
      read at offset 0 returns the raw code text directly. The asymmetry is
      the firmware's, not ours.

      Upload:
        blob = [u32 packed_size-2][u16 crc16(packed)][packed]
        1) COMM_LISP_SET_RUNNING [u8 0]                      -> ack
        2) COMM_LISP_ERASE_CODE  [i32 blob_size + slack]     -> ack
        3) COMM_LISP_WRITE_CODE  [u32 offset][chunk...]      -> ack (echoed off)
           repeated until the whole blob is written
        4) COMM_LISP_SET_RUNNING [u8 1]   (if run_after)     -> ack
      The u32/u16 header MUST be present: on start the VESC checks the
      stored crc against the size field and refuses to load the script on
      mismatch — a raw (headerless) upload flashes fine but never runs.

      Read:
        request: COMM_LISP_READ_CODE [i32 len][i32 offset]
        reply:   [u8 cmd][i32 total_len][i32 offset][raw code bytes]
        total_len is the stored size field (code + NUL + 2 import-count
        bytes) — read until we have that many bytes, then cut the string
        at the first NUL to drop the import-count tail.
*/

#include "vesc_can/vesc_lisp_code.h"

#include "vesc_can/buffer.h"
#include "vesc_can/comm_can.h"
#include "vesc_can/vesc_datatypes.h"
#include "vesc_can/vesc_rt_data.h"
#include "vesc_can/vesc_lisp_poll.h"
#include "vesc_can/vesc_lisp_panel.h"
#include "vesc_can/vesc_io_data.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "vesc_can/crc.h"

#include <stdlib.h>
#include <string.h>

static const char *TAG = "vesc_lisp_code";

#define LISP_CHUNK      256           /* payload bytes per WRITE/READ packet */
#define LISP_MAX        (120 * 1024)  /* VESC's LISP flash area (STM32F405)  */
#define BLOB_HDR        6             /* u32 size + u16 crc before packed    */
#define STEP_TIMEOUT_MS  1500
/* Erasing the LISP flash sector(s) blocks the VESC comm thread for up to a
 * few seconds; the ack only comes when it's done. A short timeout would
 * re-send ERASE on top of the still-running erase. Same 8 s VESC Tool uses. */
#define ERASE_TIMEOUT_MS 8000
#define STEP_RETRIES     3

enum { OP_NONE = 0, OP_UPLOAD, OP_READ };

static uint8_t        s_target  = 10;
static volatile bool  s_busy    = false;
static int            s_op      = OP_NONE;
static TaskHandle_t   s_worker  = NULL;
static SemaphoreHandle_t s_start_sem = NULL;
static SemaphoreHandle_t s_ack_sem   = NULL;

/* ack hand-off (written by process_response, read by worker after sem take) */
static volatile uint8_t  s_ack_cmd;
static volatile bool     s_ack_ok;
static volatile uint32_t s_ack_off;
static volatile uint32_t s_ack_len;
static volatile uint32_t s_read_total;   /* total stored code length (READ) */

/* operation params / buffers */
static uint8_t  *s_buf;        /* packed buffer (upload) / read-assembly buf */
static uint32_t  s_buf_cap;    /* allocated capacity                         */
static uint32_t  s_total;      /* upload: packed size; read: bytes expected  */
static bool      s_run_after;
static vlc_progress_cb_t    s_progress;
static vlc_upload_done_cb_t s_upload_done;
static vlc_read_done_cb_t   s_read_done;
static void      *s_user;

/* ---- low-level send helpers ---- */

static void send_set_running(bool run)
{
    uint8_t b[2];
    b[0] = COMM_LISP_SET_RUNNING;
    b[1] = run ? 1 : 0;
    comm_can_send_buffer(s_target, b, 2, 0);
}

static void drain_ack(void) { xSemaphoreTake(s_ack_sem, 0); }

/* Wait for an ack with command `cmd`. Returns true on match within timeout. */
static bool wait_ack(uint8_t cmd, uint32_t timeout_ms)
{
    if (xSemaphoreTake(s_ack_sem, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        return false;
    }
    return s_ack_cmd == cmd;
}

static bool step_erase(uint32_t size)
{
    for (int retry = 0; retry < STEP_RETRIES; retry++) {
        uint8_t b[5];
        int32_t ind = 0;
        b[ind++] = COMM_LISP_ERASE_CODE;
        buffer_append_int32(b, (int32_t)size, &ind);
        drain_ack();
        comm_can_send_buffer(s_target, b, ind, 0);
        if (wait_ack(COMM_LISP_ERASE_CODE, ERASE_TIMEOUT_MS) && s_ack_ok) {
            return true;
        }
        ESP_LOGW(TAG, "erase ack timeout/nak (retry %d)", retry + 1);
    }
    return false;
}

static bool step_write(uint32_t offset, const uint8_t *data, uint32_t n)
{
    for (int retry = 0; retry < STEP_RETRIES; retry++) {
        uint8_t b[5 + LISP_CHUNK];
        int32_t ind = 0;
        b[ind++] = COMM_LISP_WRITE_CODE;
        buffer_append_uint32(b, offset, &ind);
        memcpy(b + ind, data, n);
        ind += (int32_t)n;
        drain_ack();
        comm_can_send_buffer(s_target, b, ind, 0);
        if (wait_ack(COMM_LISP_WRITE_CODE, STEP_TIMEOUT_MS) && s_ack_ok &&
            s_ack_off == offset) {
            return true;
        }
        ESP_LOGW(TAG, "write ack mismatch off=%u got=%u ok=%d (retry %d)",
                 (unsigned)offset, (unsigned)s_ack_off, (int)s_ack_ok,
                 retry + 1);
    }
    return false;
}

/* Read `n` bytes at `offset`; process_response copies them into s_buf. */
static bool step_read(uint32_t offset, uint32_t n)
{
    for (int retry = 0; retry < STEP_RETRIES; retry++) {
        uint8_t b[9];
        int32_t ind = 0;
        b[ind++] = COMM_LISP_READ_CODE;
        buffer_append_int32(b, (int32_t)n, &ind);
        buffer_append_int32(b, (int32_t)offset, &ind);
        drain_ack();
        comm_can_send_buffer(s_target, b, ind, 0);
        bool got = wait_ack(COMM_LISP_READ_CODE, STEP_TIMEOUT_MS);
        if (got && s_ack_off == offset) {
            return true;
        }
        ESP_LOGW(TAG, "read %s want_off=%u got_off=%u (retry %d)",
                 got ? "off-mismatch" : "timeout",
                 (unsigned)offset, (unsigned)s_ack_off, retry + 1);
    }
    return false;
}

/* ---- operations ---- */

/* The comm_can reassembler keys its RX buffers by our own controller id, so it
 * can only hold ONE multi-frame transfer at a time. The periodic RT/LISP/IO
 * pollers each produce multi-frame replies that would collide with the LISP
 * code transfer's multi-frame replies (CRC mismatch -> dropped -> timeout).
 * Pause them for the duration of an upload/read so the bus is ours. */
static void pause_pollers(void)
{
    vesc_rt_data_stop();
    vesc_lisp_poll_stop();
    vesc_io_data_set_active(false);
    /* Includes the dashboard poll, which runs regardless of the drawer. */
    vesc_lisp_panel_polls_pause(true);
}

static void resume_pollers(void)
{
    vesc_rt_data_start();
    vesc_lisp_poll_start();
    vesc_lisp_panel_polls_pause(false);
    /* io_data is driven by the realtime screen's own active flag; leave off. */
}

static void do_upload(void)
{
    vlc_result_t res = VLC_OK;
    pause_pollers();

    send_set_running(false);
    drain_ack();
    wait_ack(COMM_LISP_SET_RUNNING, 500);  /* best-effort stop */

    /* +100 slack like VESC Tool — erase rounds up to whole flash sectors. */
    if (!step_erase(s_total + 100)) { res = VLC_ERR_TIMEOUT; goto done; }

    for (uint32_t off = 0; off < s_total; ) {
        uint32_t n = s_total - off;
        if (n > LISP_CHUNK) n = LISP_CHUNK;
        if (!step_write(off, s_buf + off, n)) { res = VLC_ERR_TIMEOUT; goto done; }
        off += n;
        if (s_progress) s_progress(s_user, off, s_total);
    }

    if (s_run_after) {
        send_set_running(true);
        drain_ack();
        wait_ack(COMM_LISP_SET_RUNNING, 500);  /* best-effort start */
    }

done:
    resume_pollers();
    if (s_upload_done) s_upload_done(s_user, res);
    free(s_buf);
    s_buf = NULL;
}

static void do_read(void)
{
    vlc_result_t res = VLC_OK;
    const char *code = NULL;
    uint32_t code_str_len = 0;
    pause_pollers();

    /* First read: the reply header carries the total stored code length. The
     * code is stored raw (no packed header), so it starts at s_buf[0]. */
    s_read_total = 0;
    if (!step_read(0, LISP_CHUNK)) { res = VLC_ERR_TIMEOUT; goto done; }

    uint32_t total = s_read_total;
    if (total == 0) { res = VLC_ERR_EMPTY; goto done; }
    if (total > s_buf_cap - 1) total = s_buf_cap - 1;   /* room for NUL */

    uint32_t have = s_ack_len;          /* bytes already in s_buf from first read */
    while (have < total) {
        uint32_t n = total - have;
        if (n > LISP_CHUNK) n = LISP_CHUNK;
        if (!step_read(have, n)) { res = VLC_ERR_TIMEOUT; goto done; }
        if (s_ack_len == 0) break;      /* guard against a stuck stream */
        have += s_ack_len;
        if (s_progress) s_progress(s_user, have, total);
    }

    /* The stored total counts code + NUL + 2 import-count bytes; cut the
     * string at the first NUL so callers get just the code text. */
    s_buf[have] = '\0';
    code = (const char *)s_buf;
    code_str_len = (uint32_t)strnlen(code, have);

done:
    resume_pollers();
    if (s_read_done) s_read_done(s_user, res, res == VLC_OK ? code : NULL, code_str_len);
    free(s_buf);
    s_buf = NULL;
}

static void worker_task(void *arg)
{
    (void)arg;
    for (;;) {
        xSemaphoreTake(s_start_sem, portMAX_DELAY);
        if (s_op == OP_UPLOAD) {
            do_upload();
        } else if (s_op == OP_READ) {
            do_read();
        }
        s_op   = OP_NONE;
        s_busy = false;
    }
}

/* ---- public API ---- */

void vesc_lisp_code_init(uint8_t target_vesc_id)
{
    s_target = target_vesc_id;
    if (!s_ack_sem)   s_ack_sem   = xSemaphoreCreateBinary();
    if (!s_start_sem) s_start_sem = xSemaphoreCreateBinary();
    if (!s_worker) {
        xTaskCreatePinnedToCore(worker_task, "vesc_lisp_code", 4096, NULL, 4,
                                &s_worker, 0);
    }
    ESP_LOGI(TAG, "init target=%u", target_vesc_id);
}

void vesc_lisp_code_set_target(uint8_t target_vesc_id) { s_target = target_vesc_id; }

bool vesc_lisp_code_busy(void) { return s_busy; }

bool vesc_lisp_code_upload(const char *code, uint32_t len, bool run_after,
                           vlc_progress_cb_t progress, vlc_upload_done_cb_t done,
                           void *user)
{
    if (!code || !s_start_sem) return false;
    if (s_busy) return false;

    /* Build the full upload blob VESC Tool would send:
     *   [u32 packed-2][u16 crc16(packed)][u16 flags=0][code][NUL][i16 0] */
    uint32_t packed_len = 2 + len + 1 + 2;   /* flags + code + NUL + imports */
    uint32_t blob_len   = BLOB_HDR + packed_len;
    if (blob_len > LISP_MAX) return false;

    uint8_t *buf = heap_caps_malloc(blob_len,
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) return false;

    uint8_t *packed = buf + BLOB_HDR;
    packed[0] = 0;                            /* u16 flags = 0 */
    packed[1] = 0;
    memcpy(packed + 2, code, len);
    packed[2 + len] = '\0';
    packed[3 + len] = 0;                      /* i16 num_imports = 0 */
    packed[4 + len] = 0;

    int32_t ind = 0;
    buffer_append_uint32(buf, packed_len - 2, &ind);
    buffer_append_uint16(buf, crc16(packed, packed_len), &ind);

    s_buf         = buf;
    s_buf_cap     = blob_len;
    s_total       = blob_len;
    s_run_after   = run_after;
    s_progress    = progress;
    s_upload_done = done;
    s_read_done   = NULL;
    s_user        = user;
    s_op          = OP_UPLOAD;
    s_busy        = true;
    xSemaphoreGive(s_start_sem);
    return true;
}

bool vesc_lisp_code_read(vlc_progress_cb_t progress, vlc_read_done_cb_t done,
                         void *user)
{
    if (!s_start_sem) return false;
    if (s_busy) return false;

    uint8_t *buf = heap_caps_calloc(1, LISP_MAX,
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) return false;

    s_buf         = buf;
    s_buf_cap     = LISP_MAX;
    s_total       = 0;
    s_progress    = progress;
    s_upload_done = NULL;
    s_read_done   = done;
    s_user        = user;
    s_op          = OP_READ;
    s_busy        = true;
    xSemaphoreGive(s_start_sem);
    return true;
}

void vesc_lisp_code_set_running(bool run)
{
    /* Direct fire-and-forget. Avoid stepping on an in-flight upload/read. */
    if (s_busy) return;
    send_set_running(run);
}

void vesc_lisp_code_process_response(const uint8_t *data, unsigned int len)
{
    if (!s_busy || len < 1 || !s_ack_sem) return;

    uint8_t cmd = data[0];
    int32_t ind = 1;

    switch (cmd) {
    case COMM_LISP_ERASE_CODE:
    case COMM_LISP_SET_RUNNING:
        /* reply: [cmd][u8 ok] */
        s_ack_ok  = (ind < (int)len) ? (data[ind] != 0) : true;
        s_ack_cmd = cmd;
        s_ack_off = 0;
        s_ack_len = 0;
        xSemaphoreGive(s_ack_sem);
        break;

    case COMM_LISP_WRITE_CODE:
        /* reply: [cmd][u8 ok][u32 offset] — the ok byte comes FIRST (same
         * as the app's parseLispWrite); skipping it shifts the offset read
         * by 8 bits (a successful off=0 ack parses as 0x01000000). */
        s_ack_ok = (ind < (int)len) ? (data[ind++] != 0) : false;
        if (ind + 4 <= (int)len) s_ack_off = buffer_get_uint32(data, &ind);
        s_ack_cmd = cmd;
        s_ack_len = 0;
        xSemaphoreGive(s_ack_sem);
        break;

    case COMM_LISP_READ_CODE: {
        /* reply: [cmd][int32 total_len][int32 offset][raw code bytes] */
        if (ind + 8 > (int)len) return;
        uint32_t total = buffer_get_uint32(data, &ind);
        uint32_t off   = buffer_get_uint32(data, &ind);
        uint32_t dn    = (uint32_t)len - (uint32_t)ind;
        if (s_buf && off + dn <= s_buf_cap) {
            memcpy(s_buf + off, data + ind, dn);
        } else {
            dn = 0;
        }
        s_read_total = total;
        s_ack_cmd = cmd;
        s_ack_off = off;
        s_ack_len = dn;
        xSemaphoreGive(s_ack_sem);
        break;
    }

    default:
        break;
    }
}
