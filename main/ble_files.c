/* BLE file manager — see ble_files.h.
 *
 * WIRE PROTOCOL (mirror lib/ble/file_ops.dart + file_manager.dart)
 *
 * Phone → P4 on FILE-CTRL (WRITE). First byte is the opcode; arguments follow.
 * Requests fit one ATT write (MTU ≤ 247 → ≤244 payload; FS paths are short):
 *   FOP_LIST     0x01  [TLV {1:path}]
 *   FOP_DOWNLOAD 0x02  [TLV {1:path}]
 *   FOP_UPLOAD   0x03  [u32 total_len][32B sha256][TLV {1:path}]
 *   FOP_UP_END   0x04  (finalize the in-flight upload)
 *   FOP_DELETE   0x05  [TLV {1:path}]
 *   FOP_RENAME   0x06  [TLV {1:src}{2:dst}]
 *   FOP_MKDIR    0x07  [TLV {1:path}]
 *   FOP_ABORT    0x08  (cancel current op)
 *
 * P4 → phone on FILE-CTRL (NOTIFY). Two shapes, told apart by the phone as
 * "len>=8 && byte0==PDU_TYPE_FILE(8)" → a chunk; otherwise a 5-byte status:
 *   status frame: [u8 status][u32 detail LE]
 *     FST_READY     0x10  detail = file size (download) / 0 (upload accepted)
 *     FST_PROGRESS  0x11  detail = bytes received (upload)
 *     FST_DONE      0x12  detail = 0
 *     FST_NOT_READY 0x1E  detail = app_fs_state()  (storage still mounting)
 *     FST_ERROR     0x1F  detail = FERR_* code
 *   bulk chunk: 8-byte chunk header [u8 PDU_TYPE_FILE][u8 flags][u16 seq]
 *               [u32 total_len] then body. Reassembled by the phone's chunk
 *               decoder. body[0] is a sub-tag:
 *     SUB_LIST     0x01  [sub][u8 flags(bit0=truncated)][u16 count]
 *                        count× [u16 name_len][name][u8 is_dir][u32 size][u32 mtime]
 *     SUB_DOWNLOAD 0x02  [sub][raw file bytes...]
 *
 * Listings and downloads ride out as PDU_TYPE_FILE chunks on the CTRL NOTIFY;
 * uploads stream raw bytes in on FILE-DATA exactly like ble_ota's DATA channel.
 * One operation in flight at a time. */

#include "ble_files.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bsp/esp-bsp.h"          /* BSP_SD_MOUNT_POINT, bsp_sdcard_mount */
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_littlefs.h"         /* esp_littlefs_info — /vescfs free space */
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "mbedtls/sha256.h"

#include "host/ble_att.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_mbuf.h"
#include "os/os_mbuf.h"

#include "app_fs.h"

static const char *TAG = "ble_files";

/* ---- wire constants (mirror Dart) ---- */
#define FOP_LIST       0x01
#define FOP_DOWNLOAD   0x02
#define FOP_UPLOAD     0x03
#define FOP_UP_END     0x04
#define FOP_DELETE     0x05
#define FOP_RENAME     0x06
#define FOP_MKDIR      0x07
#define FOP_ABORT      0x08

#define FST_READY      0x10
#define FST_PROGRESS   0x11
#define FST_DONE       0x12
#define FST_NOT_READY  0x1E
#define FST_ERROR      0x1F

#define FERR_NOT_READY 1
#define FERR_BADPATH   2
#define FERR_NOENT     3
#define FERR_NOTDIR    4
#define FERR_ISDIR     5
#define FERR_NOSPC     6
#define FERR_EXIST     7
#define FERR_IO        8
#define FERR_SHA       9
#define FERR_PROTO     10
#define FERR_ALLOC     11
#define FERR_BUSY      12
#define FERR_TOOBIG    13

#define PDU_TYPE_FILE  8
#define SUB_LIST       0x01
#define SUB_DOWNLOAD   0x02

#define CHUNK_HDR_LEN     8
#define CHUNK_FLAG_START  0x01
#define CHUNK_FLAG_END    0x02

#define MAX_PATH          384
#define UPLOAD_BEGIN_LEN  (1 + 4 + 32)        /* op + u32 len + sha256 */
#define FILE_UP_MAX       (8u * 1024 * 1024)  /* PSRAM staging ceiling */
#define UP_PROGRESS_STEP  (256 * 1024)
#define LIST_BODY_MAX     (64 * 1024)
#define UP_FLASH_CHUNK    4096

/* ---- module state ---- */
typedef enum { FS_IDLE, FS_BUSY, FS_UP_RECV, FS_UP_FINAL } files_state_t;

typedef enum {
    EV_LIST, EV_DOWNLOAD, EV_UP_BEGIN, EV_UP_END,
    EV_DELETE, EV_RENAME, EV_MKDIR, EV_PROGRESS, EV_BUSY, EV_ABORT
} ev_kind_t;

typedef struct {
    ev_kind_t kind;
    char      path[MAX_PATH];
    char      path2[MAX_PATH];   /* rename dst */
    uint32_t  total_len;         /* upload */
    uint32_t  detail;            /* progress bytes */
    uint8_t   sha[32];           /* upload */
} files_evt_t;

static QueueHandle_t s_q;
static TaskHandle_t  s_task;

static uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_ctrl_handle;

static volatile files_state_t s_state = FS_IDLE;
static volatile bool s_abort;          /* breaks the worker's send/write loops */
static uint16_t s_tx_seq;              /* per-MESSAGE chunk seq (constant across
                                          a message's chunks, ++ between messages
                                          — matches the Dart ChunkedDecoder) */

/* Upload staging (written by host task in data_write, read by worker). */
static uint8_t  *s_up_stage;
static uint32_t  s_up_total, s_up_recv, s_up_next_prog;
static volatile bool s_up_overflow;
static uint8_t   s_up_sha_expect[32];
static mbedtls_sha256_context s_up_sha;
static bool      s_up_sha_active;
static char      s_up_path[MAX_PATH];

/* ---- little-endian helpers ---- */
static inline uint32_t rd_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline void wr_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

/* ---- path helpers (the security boundary — keep tight) ---- */

static bool is_under(const char *path, const char *root) {
    size_t n = strlen(root);
    return strncmp(path, root, n) == 0 && (path[n] == '\0' || path[n] == '/');
}
static bool is_under_vescfs(const char *path) { return is_under(path, app_fs_base()); }
static bool is_under_sdcard(const char *path) { return is_under(path, BSP_SD_MOUNT_POINT); }

/* Adapted verbatim from feat/offline-navigation:files_http.c, widened to two
 * roots. Rejects anything not under /vescfs or /sdcard, and any "." / ".."
 * path segment. Trims trailing slashes. Mutates path in place. */
static bool path_safe(char *path) {
    if (!path || !*path) return false;
    bool u1 = is_under_vescfs(path);
    bool u2 = is_under_sdcard(path);
    if (!u1 && !u2) return false;
    const char *p = path;
    while ((p = strchr(p, '/')) != NULL) {
        p++;
        if (p[0] == '.' && (p[1] == '\0' || p[1] == '/')) return false;
        if (p[0] == '.' && p[1] == '.' &&
            (p[2] == '\0' || p[2] == '/')) return false;
        if (p[0] == '\0') break;
    }
    size_t rlen = strlen(u1 ? app_fs_base() : BSP_SD_MOUNT_POINT);
    size_t l = strlen(path);
    while (l > rlen && path[l - 1] == '/') path[--l] = '\0';
    return true;
}

/* Join via a helper with a runtime size arg so the compiler's
 * -Werror=format-truncation analysis (which fires on compile-time-known sizes)
 * stays quiet — snprintf truncates safely regardless. */
static void join_path(char *out, size_t out_sz, const char *dir, const char *name) {
    snprintf(out, out_sz, "%s/%s", dir, name);
}

static bool sd_try_mount(void) {
    esp_err_t e = bsp_sdcard_mount();
    bool ok = (e == ESP_OK || e == ESP_ERR_INVALID_STATE);
    if (!ok) ESP_LOGI(TAG, "no SD card (mount rc=0x%x) — /sdcard hidden", e);
    return ok;
}

static uint32_t errno_to_ferr(void) {
    switch (errno) {
        case ENOENT:  return FERR_NOENT;
        case ENOTDIR: return FERR_NOTDIR;
        case EISDIR:  return FERR_ISDIR;
        case ENOSPC:  return FERR_NOSPC;
        case EEXIST:  return FERR_EXIST;
        default:      return FERR_IO;
    }
}

/* Free bytes on the target FS; 0 = unknown (skip precheck, rely on write-time
 * ENOSPC). Only LittleFS (/vescfs) exposes a cheap query. */
static uint64_t fs_free_bytes(const char *path) {
    if (is_under_vescfs(path)) {
        size_t total = 0, used = 0;
        if (esp_littlefs_info("storage", &total, &used) == ESP_OK && total >= used)
            return (uint64_t)(total - used);
    }
    return 0;
}

/* ---- BLE notify helpers (worker task only) ---- */

static void notify_status(uint8_t status, uint32_t detail) {
    if (s_conn == BLE_HS_CONN_HANDLE_NONE || s_ctrl_handle == 0) return;
    uint8_t f[5] = {
        status, (uint8_t)detail, (uint8_t)(detail >> 8),
        (uint8_t)(detail >> 16), (uint8_t)(detail >> 24),
    };
    /* Retry on TX-buffer exhaustion (ENOMEM). The terminal DONE/ERROR MUST
     * reach the phone even right after a big chunk burst has congested the
     * link — otherwise the app's listDir/download future just times out and
     * surfaces a useless "unknown error" instead of the real status. ~4 s. */
    for (int attempt = 0; attempt < 400; attempt++) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(f, sizeof(f));
        if (om && ble_gatts_notify_custom(s_conn, s_ctrl_handle, om) == 0) return;
        if (s_abort) return;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGW(TAG, "notify status=0x%02x gave up", status);
}

static uint16_t max_payload(void) {
    uint16_t mtu = (s_conn != BLE_HS_CONN_HANDLE_NONE) ? ble_att_mtu(s_conn) : 23;
    if (mtu < 23) mtu = 23;
    int pl = (int)mtu - 3 - CHUNK_HDR_LEN;
    if (pl < 16)  pl = 16;
    if (pl > 244) pl = 244;
    return (uint16_t)pl;
}

/* Send one PDU_TYPE_FILE chunk, backing off on host TX-buffer exhaustion
 * (ENOMEM). Returns 0 / -2 aborted / -3 gave up. The mbuf is consumed by
 * ble_gatts_notify_custom in all cases. */
static int notify_chunk(uint8_t flags, uint16_t seq, uint32_t total_len,
                        const uint8_t *body, uint16_t blen) {
    if (s_conn == BLE_HS_CONN_HANDLE_NONE || s_ctrl_handle == 0) return -1;
    uint8_t hdr[CHUNK_HDR_LEN];
    hdr[0] = PDU_TYPE_FILE;
    hdr[1] = flags;
    hdr[2] = (uint8_t)seq;
    hdr[3] = (uint8_t)(seq >> 8);
    wr_u32(&hdr[4], total_len);
    /* Ride out TX-buffer exhaustion (BLE_HS_ENOMEM) rather than giving up: on a
     * slow link (e.g. 48 ms connection interval) a big listing/download is
     * hundreds of chunks and the NimBLE host mbuf pool fills faster than it
     * drains. Giving up here aborted the whole transfer (the "notify_chunk gave
     * up" warning → app saw an empty/`unknown error` listing or IO-error
     * download). The host DOES drain given time, so wait it out — bounded only
     * as a safety net (~20 s/chunk; only a genuinely stuck chunk reaches it),
     * and break promptly on disconnect/abort. */
    for (int attempt = 0; attempt < 2000; attempt++) {
        if (s_abort) return -2;
        struct os_mbuf *om = ble_hs_mbuf_from_flat(hdr, CHUNK_HDR_LEN);
        if (om && blen) {
            if (os_mbuf_append(om, body, blen) != 0) {
                os_mbuf_free_chain(om);   /* notify not called → we own it */
                om = NULL;
            }
        }
        if (om) {
            int rc = ble_gatts_notify_custom(s_conn, s_ctrl_handle, om);
            if (rc == 0) return 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGW(TAG, "notify_chunk gave up");
    return -3;
}

/* Chunk a fully-in-memory buffer (used for directory listings). */
static int send_chunks(const uint8_t *buf, uint32_t len) {
    uint16_t maxpl = max_payload();
    uint16_t seq = s_tx_seq++;     /* one seq for the whole message */
    uint32_t off = 0;
    if (len == 0) return notify_chunk(CHUNK_FLAG_START | CHUNK_FLAG_END, seq, 0, NULL, 0);
    while (off < len) {
        if (s_abort) return -2;
        uint16_t n = ((len - off) < maxpl) ? (uint16_t)(len - off) : maxpl;
        uint8_t flags = 0;
        if (off == 0)        flags |= CHUNK_FLAG_START;
        if (off + n >= len)  flags |= CHUNK_FLAG_END;
        int rc = notify_chunk(flags, seq, len, buf + off, n);
        if (rc != 0) return rc;
        off += n;
    }
    return 0;
}

/* Stream a file's bytes as a SUB_DOWNLOAD message without buffering the whole
 * file: total body = sub byte + file size. */
static int send_file(FILE *f, uint32_t fsize) {
    uint16_t maxpl = max_payload();
    uint8_t *pl = malloc(maxpl);
    if (!pl) return -1;
    uint32_t total = fsize + 1;     /* +1 for the sub byte */
    uint16_t seq = s_tx_seq++;      /* one seq for the whole message */
    uint32_t done = 0;
    bool first = true;
    int ret = 0;
    while (done < total) {
        if (s_abort) { ret = -2; break; }
        uint16_t n = 0;
        if (first) { pl[0] = SUB_DOWNLOAD; n = 1; }
        uint16_t want = maxpl - n;
        if (want > 0 && fsize > 0) {
            size_t got = fread(pl + n, 1, want, f);
            n += (uint16_t)got;
        }
        if (!first && n == 0) { ret = -4; break; }  /* short read vs stat size */
        uint8_t flags = 0;
        if (first) flags |= CHUNK_FLAG_START;
        done += n;
        if (done >= total) flags |= CHUNK_FLAG_END;
        int rc = notify_chunk(flags, seq, total, pl, n);
        if (rc != 0) { ret = rc; break; }
        first = false;
    }
    free(pl);
    return ret;
}

/* ---- worker handlers (the only place FS I/O and notifies happen) ---- */

static void stage_free(void) {
    if (s_up_stage) { heap_caps_free(s_up_stage); s_up_stage = NULL; }
    if (s_up_sha_active) { mbedtls_sha256_free(&s_up_sha); s_up_sha_active = false; }
    s_up_total = s_up_recv = s_up_next_prog = 0;
    s_up_overflow = false;
    s_up_path[0] = '\0';
}

/* Build + send the synthetic "/" listing: the mounted drives as directories. */
static void send_root_list(void) {
    uint8_t body[80];
    uint32_t off = 4;
    uint16_t count = 0;
    static const char *const names[2] = { "vescfs", "sdcard" };
    bool present[2] = { true, sd_try_mount() };
    for (int i = 0; i < 2; i++) {
        if (!present[i]) continue;
        size_t nl = strlen(names[i]);
        body[off++] = (uint8_t)nl; body[off++] = (uint8_t)(nl >> 8);
        memcpy(body + off, names[i], nl); off += nl;
        body[off++] = 1;                       /* is_dir */
        wr_u32(body + off, 0); off += 4;        /* size */
        wr_u32(body + off, 0); off += 4;        /* mtime */
        count++;
    }
    body[0] = SUB_LIST; body[1] = 0;
    body[2] = (uint8_t)count; body[3] = (uint8_t)(count >> 8);
    send_chunks(body, off);
}

static void do_list(const char *path) {
    s_abort = false;
    if (strcmp(path, "/") == 0) {
        send_root_list();
        notify_status(FST_DONE, 0);
        return;
    }
    char p[MAX_PATH];
    strlcpy(p, path, sizeof p);
    if (!path_safe(p)) { notify_status(FST_ERROR, FERR_BADPATH); return; }
    if (is_under_vescfs(p) && !app_fs_ready()) {
        notify_status(FST_NOT_READY, (uint32_t)app_fs_state());
        return;
    }
    if (is_under_sdcard(p)) sd_try_mount();

    DIR *d = opendir(p);
    if (!d) { notify_status(FST_ERROR, errno_to_ferr()); return; }

    uint8_t *body = malloc(LIST_BODY_MAX);
    if (!body) { closedir(d); notify_status(FST_ERROR, FERR_ALLOC); return; }
    uint32_t off = 4;       /* reserve [sub][flags][u16 count] */
    uint16_t count = 0;
    uint8_t  truncated = 0;
    struct dirent *de;
    char full[MAX_PATH];
    while ((de = readdir(d)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        size_t nl = strlen(de->d_name);
        if (nl > 255) nl = 255;
        uint32_t need = 2 + nl + 1 + 4 + 4;
        if (off + need > LIST_BODY_MAX || count == 0xFFFF) { truncated = 1; break; }
        join_path(full, sizeof full, p, de->d_name);
        struct stat st;
        uint8_t is_dir = 0; uint32_t sz = 0, mt = 0;
        if (stat(full, &st) == 0) {
            is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
            sz = (uint32_t)st.st_size;
            mt = (uint32_t)st.st_mtime;
        }
        body[off++] = (uint8_t)nl; body[off++] = (uint8_t)(nl >> 8);
        memcpy(body + off, de->d_name, nl); off += nl;
        body[off++] = is_dir;
        wr_u32(body + off, sz); off += 4;
        wr_u32(body + off, mt); off += 4;
        count++;
        if (s_abort) break;
    }
    closedir(d);
    body[0] = SUB_LIST; body[1] = truncated;
    body[2] = (uint8_t)count; body[3] = (uint8_t)(count >> 8);
    int rc = send_chunks(body, off);
    free(body);
    if (rc == -2) return;                 /* aborted — caller already torn down */
    if (rc != 0) { notify_status(FST_ERROR, FERR_IO); return; }
    notify_status(FST_DONE, 0);
}

static void do_download(const char *path) {
    s_abort = false;
    char p[MAX_PATH];
    strlcpy(p, path, sizeof p);
    if (!path_safe(p)) { notify_status(FST_ERROR, FERR_BADPATH); return; }
    if (is_under_vescfs(p) && !app_fs_ready()) {
        notify_status(FST_NOT_READY, (uint32_t)app_fs_state());
        return;
    }
    if (is_under_sdcard(p)) sd_try_mount();

    struct stat st;
    if (stat(p, &st) != 0) { notify_status(FST_ERROR, errno_to_ferr()); return; }
    if (S_ISDIR(st.st_mode)) { notify_status(FST_ERROR, FERR_ISDIR); return; }
    FILE *f = fopen(p, "rb");
    if (!f) { notify_status(FST_ERROR, errno_to_ferr()); return; }

    notify_status(FST_READY, (uint32_t)st.st_size);
    int rc = send_file(f, (uint32_t)st.st_size);
    fclose(f);
    if (rc == 0)        notify_status(FST_DONE, 0);
    else if (rc != -2)  notify_status(FST_ERROR, FERR_IO);  /* -2 = aborted, no notify */
}

static void do_delete(const char *path) {
    char p[MAX_PATH];
    strlcpy(p, path, sizeof p);
    if (!path_safe(p)) { notify_status(FST_ERROR, FERR_BADPATH); return; }
    if (strcmp(p, app_fs_base()) == 0 || strcmp(p, BSP_SD_MOUNT_POINT) == 0) {
        notify_status(FST_ERROR, FERR_BADPATH);   /* never delete a drive root */
        return;
    }
    if (is_under_vescfs(p) && !app_fs_ready()) {
        notify_status(FST_NOT_READY, (uint32_t)app_fs_state());
        return;
    }
    if (is_under_sdcard(p)) sd_try_mount();
    struct stat st;
    if (stat(p, &st) != 0) { notify_status(FST_ERROR, errno_to_ferr()); return; }
    int rc = S_ISDIR(st.st_mode) ? rmdir(p) : unlink(p);
    if (rc != 0) { notify_status(FST_ERROR, errno_to_ferr()); return; }
    notify_status(FST_DONE, 0);
}

static void do_rename(const char *src, const char *dst) {
    char s[MAX_PATH], d[MAX_PATH];
    strlcpy(s, src, sizeof s);
    strlcpy(d, dst, sizeof d);
    if (!path_safe(s) || !path_safe(d)) { notify_status(FST_ERROR, FERR_BADPATH); return; }
    if ((is_under_vescfs(s) || is_under_vescfs(d)) && !app_fs_ready()) {
        notify_status(FST_NOT_READY, (uint32_t)app_fs_state());
        return;
    }
    if (is_under_sdcard(s) || is_under_sdcard(d)) sd_try_mount();
    /* rename() can't cross mounts (EXDEV) → FERR_IO; the app falls back to
     * download+upload+delete. */
    if (rename(s, d) != 0) { notify_status(FST_ERROR, errno_to_ferr()); return; }
    notify_status(FST_DONE, 0);
}

static void do_mkdir(const char *path) {
    char p[MAX_PATH];
    strlcpy(p, path, sizeof p);
    if (!path_safe(p)) { notify_status(FST_ERROR, FERR_BADPATH); return; }
    if (is_under_vescfs(p) && !app_fs_ready()) {
        notify_status(FST_NOT_READY, (uint32_t)app_fs_state());
        return;
    }
    if (is_under_sdcard(p)) sd_try_mount();
    if (mkdir(p, 0775) != 0) { notify_status(FST_ERROR, errno_to_ferr()); return; }
    notify_status(FST_DONE, 0);
}

static void do_upload_begin(const files_evt_t *ev) {
    stage_free();
    s_state = FS_BUSY;

    char p[MAX_PATH];
    strlcpy(p, ev->path, sizeof p);
    if (!path_safe(p)) { notify_status(FST_ERROR, FERR_BADPATH); s_state = FS_IDLE; return; }
    if (is_under_vescfs(p) && !app_fs_ready()) {
        notify_status(FST_NOT_READY, (uint32_t)app_fs_state()); s_state = FS_IDLE; return;
    }
    if (is_under_sdcard(p)) sd_try_mount();
    if (ev->total_len == 0) { notify_status(FST_ERROR, FERR_PROTO); s_state = FS_IDLE; return; }
    if (ev->total_len > FILE_UP_MAX) { notify_status(FST_ERROR, FERR_TOOBIG); s_state = FS_IDLE; return; }

    struct stat st;
    if (stat(p, &st) == 0 && S_ISDIR(st.st_mode)) {
        notify_status(FST_ERROR, FERR_ISDIR); s_state = FS_IDLE; return;  /* don't clobber a dir */
    }
    uint64_t freeb = fs_free_bytes(p);
    if (freeb && ev->total_len > freeb) {
        notify_status(FST_ERROR, FERR_TOOBIG); s_state = FS_IDLE; return;
    }

    s_up_stage = heap_caps_malloc(ev->total_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_up_stage) {
        ESP_LOGE(TAG, "PSRAM staging %u failed", (unsigned)ev->total_len);
        notify_status(FST_ERROR, FERR_ALLOC); s_state = FS_IDLE; return;
    }
    mbedtls_sha256_init(&s_up_sha);
    mbedtls_sha256_starts(&s_up_sha, 0);
    s_up_sha_active = true;
    memcpy(s_up_sha_expect, ev->sha, 32);
    s_up_total = ev->total_len;
    s_up_recv = 0;
    s_up_next_prog = UP_PROGRESS_STEP;
    s_up_overflow = false;
    strlcpy(s_up_path, p, sizeof s_up_path);

    s_state = FS_UP_RECV;
    ESP_LOGI(TAG, "upload begin: %u bytes -> %s", (unsigned)s_up_total, s_up_path);
    notify_status(FST_READY, 0);   /* app starts streaming FILE-DATA now */
}

static void do_upload_end(void) {
    if (s_state != FS_UP_RECV) { notify_status(FST_ERROR, FERR_PROTO); return; }
    s_state = FS_UP_FINAL;

    if (s_up_overflow || s_up_recv != s_up_total) {
        ESP_LOGE(TAG, "upload end recv=%u want=%u ovf=%d",
                 (unsigned)s_up_recv, (unsigned)s_up_total, (int)s_up_overflow);
        notify_status(FST_ERROR, FERR_PROTO);
        stage_free(); s_state = FS_IDLE; return;
    }
    unsigned char digest[32];
    mbedtls_sha256_finish(&s_up_sha, digest);
    mbedtls_sha256_free(&s_up_sha);
    s_up_sha_active = false;
    if (memcmp(digest, s_up_sha_expect, 32) != 0) {
        notify_status(FST_ERROR, FERR_SHA);
        stage_free(); s_state = FS_IDLE; return;
    }

    FILE *f = fopen(s_up_path, "wb");
    if (!f) {
        notify_status(FST_ERROR, errno_to_ferr());
        stage_free(); s_state = FS_IDLE; return;
    }
    uint32_t off = 0;
    int err = 0;
    while (off < s_up_total) {
        if (s_abort) { err = FERR_IO; break; }
        uint32_t chunk = (s_up_total - off) < UP_FLASH_CHUNK
                         ? (s_up_total - off) : UP_FLASH_CHUNK;
        size_t w = fwrite(s_up_stage + off, 1, chunk, f);
        if (w != chunk) { err = (errno == ENOSPC) ? FERR_NOSPC : FERR_IO; break; }
        off += chunk;
    }
    fclose(f);
    if (err) {
        unlink(s_up_path);   /* don't leave a half-written file behind */
        notify_status(FST_ERROR, (uint32_t)err);
        stage_free(); s_state = FS_IDLE; return;
    }
    ESP_LOGI(TAG, "upload done: %s (%u bytes)", s_up_path, (unsigned)s_up_total);
    stage_free();
    s_state = FS_IDLE;
    notify_status(FST_DONE, 0);
}

static void worker(void *arg) {
    (void)arg;
    files_evt_t ev;
    for (;;) {
        if (xQueueReceive(s_q, &ev, portMAX_DELAY) != pdTRUE) continue;
        switch (ev.kind) {
            case EV_LIST:     do_list(ev.path);            s_state = FS_IDLE; break;
            case EV_DOWNLOAD: do_download(ev.path);        s_state = FS_IDLE; break;
            case EV_DELETE:   do_delete(ev.path);          s_state = FS_IDLE; break;
            case EV_RENAME:   do_rename(ev.path, ev.path2); s_state = FS_IDLE; break;
            case EV_MKDIR:    do_mkdir(ev.path);           s_state = FS_IDLE; break;
            case EV_UP_BEGIN: do_upload_begin(&ev);        break;  /* sets state */
            case EV_UP_END:   do_upload_end();             break;  /* sets state */
            case EV_PROGRESS: notify_status(FST_PROGRESS, ev.detail); break;
            case EV_BUSY:     notify_status(FST_ERROR, FERR_BUSY);    break;
            case EV_ABORT:
                ESP_LOGW(TAG, "abort");
                stage_free();
                s_abort = false;
                s_state = FS_IDLE;
                break;
        }
    }
}

/* ---- TLV request parsing (host task) ---- */

static bool tlv_get(const uint8_t *b, uint16_t len, uint8_t want_tag,
                    char *out, size_t out_sz) {
    uint16_t off = 0;
    out[0] = '\0';
    while (off + 5 <= len) {
        uint8_t  tag = b[off];
        uint32_t l = rd_u32(b + off + 1);
        off += 5;
        if (l > (uint32_t)(len - off)) break;
        if (tag == want_tag) {
            size_t n = (l < out_sz - 1) ? l : out_sz - 1;
            memcpy(out, b + off, n);
            out[n] = '\0';
            return n > 0;
        }
        off += l;
    }
    return false;
}

/* ---- public API ---- */

void ble_files_init(void) {
    if (s_q) return;
    app_fs_ensure();   /* kick the async /vescfs mount so it's ready when used */
    s_q = xQueueCreate(8, sizeof(files_evt_t));
    xTaskCreatePinnedToCore(worker, "ble_files", 8192, NULL, 5, &s_task, 0);
    ESP_LOGI(TAG, "ble_files ready");
}

void ble_files_set_link(uint16_t conn, uint16_t ctrl_val_handle) {
    s_conn = conn;
    s_ctrl_handle = ctrl_val_handle;
    s_abort = false;
}

void ble_files_on_disconnect(void) {
    s_conn = BLE_HS_CONN_HANDLE_NONE;
    s_abort = true;   /* break any in-flight send/write loop */
    if (s_q) {
        files_evt_t e = { .kind = EV_ABORT };
        xQueueSend(s_q, &e, 0);
    }
}

/* ---- routed from notif_bridge access_cb (NimBLE host task) ---- */

void ble_files_ctrl_write(const uint8_t *data, uint16_t len) {
    if (len < 1 || !s_q) return;
    uint8_t op = data[0];

    if (op == FOP_ABORT) {
        s_abort = true;
        files_evt_t e = { .kind = EV_ABORT };
        xQueueSend(s_q, &e, 0);
        return;
    }
    if (op == FOP_UP_END) {
        files_evt_t e = { .kind = EV_UP_END };
        xQueueSend(s_q, &e, portMAX_DELAY);
        return;
    }
    if (s_state != FS_IDLE) {            /* one op at a time */
        files_evt_t e = { .kind = EV_BUSY };
        xQueueSend(s_q, &e, 0);
        return;
    }

    files_evt_t e;
    memset(&e, 0, sizeof e);
    bool ok = true;
    switch (op) {
        case FOP_LIST:
            e.kind = EV_LIST;
            tlv_get(data + 1, len - 1, 1, e.path, sizeof e.path);
            break;
        case FOP_DOWNLOAD:
            e.kind = EV_DOWNLOAD;
            tlv_get(data + 1, len - 1, 1, e.path, sizeof e.path);
            break;
        case FOP_DELETE:
            e.kind = EV_DELETE;
            tlv_get(data + 1, len - 1, 1, e.path, sizeof e.path);
            break;
        case FOP_MKDIR:
            e.kind = EV_MKDIR;
            tlv_get(data + 1, len - 1, 1, e.path, sizeof e.path);
            break;
        case FOP_RENAME:
            e.kind = EV_RENAME;
            tlv_get(data + 1, len - 1, 1, e.path, sizeof e.path);
            tlv_get(data + 1, len - 1, 2, e.path2, sizeof e.path2);
            break;
        case FOP_UPLOAD:
            e.kind = EV_UP_BEGIN;
            if (len >= UPLOAD_BEGIN_LEN) {
                e.total_len = rd_u32(data + 1);
                memcpy(e.sha, data + 5, 32);
                tlv_get(data + UPLOAD_BEGIN_LEN, len - UPLOAD_BEGIN_LEN,
                        1, e.path, sizeof e.path);
            }   /* else total_len stays 0 → worker rejects with FERR_PROTO */
            break;
        default:
            ok = false;
            break;
    }
    if (!ok) return;
    s_state = FS_BUSY;   /* claim the slot synchronously (host task) */
    xQueueSend(s_q, &e, portMAX_DELAY);
}

void ble_files_data_write(const uint8_t *data, uint16_t len) {
    if (s_state != FS_UP_RECV || !len || s_up_overflow) return;
    if (s_up_recv + len > s_up_total) {
        ESP_LOGE(TAG, "upload overflow recv=%u len=%u total=%u",
                 (unsigned)s_up_recv, (unsigned)len, (unsigned)s_up_total);
        /* Flag it but leave s_state == FS_UP_RECV so do_upload_end's
         * FS_UP_RECV gate still passes and it cleans up (frees staging,
         * resets to IDLE) instead of leaking + wedging in BUSY. Further
         * data writes are dropped by the s_up_overflow guard above. */
        s_up_overflow = true;
        files_evt_t e = { .kind = EV_UP_END };
        if (s_q) xQueueSend(s_q, &e, 0);
        return;
    }
    memcpy(s_up_stage + s_up_recv, data, len);
    mbedtls_sha256_update(&s_up_sha, data, len);
    s_up_recv += len;
    if (s_up_recv >= s_up_next_prog) {
        files_evt_t e = { .kind = EV_PROGRESS, .detail = s_up_recv };
        if (s_q) xQueueSend(s_q, &e, 0);   /* best-effort */
        s_up_next_prog += UP_PROGRESS_STEP;
    }
}
