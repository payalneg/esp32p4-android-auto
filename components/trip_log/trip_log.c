/*
 * Trip time-series logger — raw circular log on the "triplog" flash partition
 * (no filesystem). Each 10 s a fixed 64-byte record is appended; records are
 * tagged with a trip id (rolled over on reset / battery swap) and a monotonic
 * seq. On NOR a programmed record is durable immediately (no fsync), so nothing
 * is lost on a power cut except an in-progress write (caught by CRC on the next
 * boot scan).
 *
 * Erase strategy: a flash erase stalls the LVGL render task (cache off for all
 * cores; the DSI DMA keeps the last frame from PSRAM so no blackout, but a
 * visible hitch). To keep erases off the ride, the writer pre-erases a runway
 * of sectors AHEAD on boot (BOOT_PREERASE_SECTORS ≈ 11 h of logging). Then,
 * before crossing into each fresh sector, it checks whether that sector is
 * already clean (the runway made it so) and erases only if it is not — so a
 * mid-ride erase happens at most once the runway is exhausted.
 *
 * All flash I/O runs on a dedicated low-priority writer task; trip_log_tick()
 * (called from the UI updater) only builds a record and queues it.
 */
#include "trip_log.h"

#include "vesc_trip_persist.h"
#include "vesc_battery_calc.h"
#include "vesc_can/crc.h"

#include "esp_log.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <string.h>
#include <stdlib.h>

static const char *TAG = "trip_log";

#define TRIPLOG_LABEL      "triplog"
#define REC_MAGIC          0x5452       /* "TR" */
#define REC_VER            1
#define REC_TYPE_SAMPLE    0
#define REC_TYPE_TRIP_START 1
#define REC_SIZE           64
#define SECTOR             4096
#define RECS_PER_SECTOR    (SECTOR / REC_SIZE)   /* 64 — records never straddle a sector */
#define SAMPLE_INTERVAL_US (10ULL * 1000 * 1000)
#define QUEUE_DEPTH        8
#define MAX_TRIPS          50          /* logical history window, enforced at boot only */
#define BOOT_PREERASE_SECTORS 64       /* runway erased ahead at boot (~11 h of log) */

/* Soft-delete: trip ids the user hid from the statistics UI. Kept in NVS (the
 * circular flash log has no per-record rewrite), filtered out of the reader,
 * and excluded from the MAX_TRIPS window so deleting frees up history slots. */
#define MAX_DELETED        64
#define NVS_NS             "trip_log"
#define NVS_KEY_DELETED    "deleted"

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  type;
    uint8_t  ver;
    uint32_t trip_id;
    uint32_t seq;          /* global monotonic; assigned by the writer */
    uint32_t t_s;          /* seconds into this trip */
    uint32_t distance_m;   /* trip total (resume seed) */
    float    ah;           /* trip total Ah (resume seed) */
    float    wh;           /* VESC watt-hours (cumulative) */
    uint32_t uptime_ms;    /* trip total uptime (resume seed) */
    int16_t  speed_dkmh;   /* km/h * 10 */
    int16_t  current_da;   /* input current A * 10 */
    uint16_t voltage_dv;   /* V * 10 */
    int16_t  temp_motor_dc;/* °C * 10 */
    int16_t  temp_fet_dc;  /* °C * 10 */
    uint8_t  batt_pct;
    uint8_t  fault;
    uint8_t  reserved[16];
    uint32_t crc;          /* crc32c over the first REC_SIZE-4 bytes */
} trip_rec_t;

_Static_assert(sizeof(trip_rec_t) == REC_SIZE, "trip_rec_t must be 64 bytes");

static const esp_partition_t *s_part;
static uint32_t s_total_recs;     /* ring capacity in records */
static uint32_t s_head;           /* next slot index to write */
static uint32_t s_seq;            /* next seq to assign */
static uint32_t s_trip_id;        /* current trip */
static uint32_t s_first_trip_id;  /* oldest exposed trip (boot-computed window) */
static uint32_t s_trip_t_s;       /* seconds into current trip */
static int64_t  s_last_sample_us;
static volatile bool s_pending_new;

static QueueHandle_t s_queue;

static uint32_t s_deleted[MAX_DELETED];   /* hidden trip ids */
static int      s_deleted_count;

static bool is_deleted(uint32_t id)
{
    for (int i = 0; i < s_deleted_count; i++) if (s_deleted[i] == id) return true;
    return false;
}

static void load_deleted(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return;   /* none yet → empty */
    size_t sz = sizeof s_deleted;
    if (nvs_get_blob(h, NVS_KEY_DELETED, s_deleted, &sz) == ESP_OK) {
        s_deleted_count = (int)(sz / sizeof(uint32_t));
        if (s_deleted_count > MAX_DELETED) s_deleted_count = MAX_DELETED;
    }
    nvs_close(h);
}

static void save_deleted(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_blob(h, NVS_KEY_DELETED, s_deleted, (size_t)s_deleted_count * sizeof(uint32_t));
    nvs_commit(h);
    nvs_close(h);
}

static uint32_t slot_off(uint32_t slot)
{
    uint32_t sec = slot / RECS_PER_SECTOR;
    uint32_t in  = slot % RECS_PER_SECTOR;
    return sec * SECTOR + in * REC_SIZE;
}

static bool rec_valid(const trip_rec_t *r)
{
    if (r->magic != REC_MAGIC) return false;
    return crc32c((const uint8_t *)r, REC_SIZE - 4) == r->crc;
}

/* ---- boot scan: find head / seq / current trip / last totals ---- */
static void boot_scan(void)
{
    uint32_t sectors = s_part->size / SECTOR;
    s_total_recs = sectors * RECS_PER_SECTOR;

    /* Coarse pass: read the first record of each sector, find the sector whose
     * (valid) first record has the highest seq — the newest region. */
    trip_rec_t r;
    uint32_t best_seq = 0;
    int      best_sec = -1;
    for (uint32_t sec = 0; sec < sectors; sec++) {
        if (esp_partition_read(s_part, sec * SECTOR, &r, REC_SIZE) != ESP_OK) continue;
        if (rec_valid(&r) && (best_sec < 0 || r.seq >= best_seq)) {
            best_seq = r.seq;
            best_sec = (int)sec;
        }
    }

    if (best_sec < 0) {
        /* empty / unformatted log */
        s_head = 0; s_seq = 1; s_trip_id = 1; s_first_trip_id = 1; s_trip_t_s = 0;
        ESP_LOGI(TAG, "empty log: starting trip 1");
        return;
    }

    /* Fine pass: within the newest sector, find the last valid record. The head
     * is the slot after it (or the first slot of the next sector if full). */
    uint32_t last_slot = best_sec * RECS_PER_SECTOR;
    trip_rec_t last;
    bool got = false;
    for (uint32_t i = 0; i < RECS_PER_SECTOR; i++) {
        uint32_t slot = best_sec * RECS_PER_SECTOR + i;
        if (esp_partition_read(s_part, slot_off(slot), &r, REC_SIZE) != ESP_OK) continue;
        if (rec_valid(&r) && (!got || r.seq >= last.seq)) { last = r; last_slot = slot; got = true; }
    }

    s_seq     = last.seq + 1;
    s_trip_id = last.trip_id;
    s_trip_t_s = last.t_s;
    s_head    = (last_slot + 1) % s_total_recs;

    /* 50-trip window — checked once here, at boot only (no runtime pruning). */
    s_first_trip_id = (s_trip_id > MAX_TRIPS) ? (s_trip_id - MAX_TRIPS + 1) : 1;

    /* Resume the dashboard running totals from the last record. */
    trip_persist_seed_totals(last.distance_m, last.ah, last.uptime_ms);

    ESP_LOGI(TAG, "resumed: trip=%u (window %u..%u) seq=%u head=%u dist=%um ah=%.2f",
             (unsigned)s_trip_id, (unsigned)s_first_trip_id, (unsigned)s_trip_id,
             (unsigned)s_seq, (unsigned)s_head,
             (unsigned)last.distance_m, last.ah);
}

/* ---- writer task (all flash I/O) ---- */
static void erase_sector(uint32_t sec)
{
    esp_err_t e = esp_partition_erase_range(s_part, (size_t)sec * SECTOR, SECTOR);
    if (e != ESP_OK) ESP_LOGE(TAG, "erase sec %u: %s", (unsigned)sec, esp_err_to_name(e));
}

/* A sector is "clean" (ready to write) when its first slot is in the erased
 * (all-0xFF) state — i.e. the magic reads back as 0xFFFF. The boot runway
 * pre-erase leaves sectors in this state, so a mid-ride crossing usually finds
 * the sector already clean and skips the (display-hitching) erase. */
static bool sector_is_clean(uint32_t sec)
{
    uint16_t magic = 0;
    if (esp_partition_read(s_part, (size_t)sec * SECTOR, &magic, sizeof magic) != ESP_OK) {
        return false;   /* read failed → be safe, erase */
    }
    return magic == 0xFFFF;
}

/* Erase a runway of sectors ahead of the head once, on boot, so the upcoming
 * ride writes into already-clean flash without a mid-ride erase. The head's
 * current sector is left alone (it may hold the resumed trip's recent records);
 * the next sector onward is fair game (old data there is evicted ring-style). */
static void preerase_runway(void)
{
    uint32_t sectors  = s_total_recs / RECS_PER_SECTOR;
    uint32_t head_sec = s_head / RECS_PER_SECTOR;
    bool partial      = (s_head % RECS_PER_SECTOR) != 0;
    uint32_t first    = partial ? (head_sec + 1) : head_sec;   /* first fresh sector */
    uint32_t k        = BOOT_PREERASE_SECTORS;
    if (k > sectors) k = sectors;

    for (uint32_t i = 0; i < k; i++) {
        uint32_t sec = (first + i) % sectors;
        if (!sector_is_clean(sec)) erase_sector(sec);
        /* Yield so the LVGL render task draws a frame between erases — turns one
         * long boot freeze into brief, interleaved hitches. */
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    ESP_LOGI(TAG, "pre-erased %u-sector runway from sector %u", (unsigned)k, (unsigned)first);
}

static void writer_task(void *arg)
{
    (void)arg;
    preerase_runway();   /* prepare the runway before the first sample */

    trip_rec_t rec;
    for (;;) {
        if (xQueueReceive(s_queue, &rec, portMAX_DELAY) != pdTRUE) continue;

        /* Entering a fresh sector → check it's clean, erase only if it isn't.
         * The runway pre-erase makes this a no-op for the whole ride; an erase
         * here only happens once the runway is exhausted (~11 h). */
        if (s_head % RECS_PER_SECTOR == 0) {
            uint32_t sec = s_head / RECS_PER_SECTOR;
            if (!sector_is_clean(sec)) erase_sector(sec);
        }

        rec.seq = s_seq;
        rec.crc = crc32c((const uint8_t *)&rec, REC_SIZE - 4);
        if (esp_partition_write(s_part, slot_off(s_head), &rec, REC_SIZE) == ESP_OK) {
            s_seq++;
            s_head = (s_head + 1) % s_total_recs;
        } else {
            ESP_LOGE(TAG, "write slot %u failed", (unsigned)s_head);
        }
    }
}

/* ---- public ---- */
void trip_log_init(void)
{
    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                      ESP_PARTITION_SUBTYPE_ANY, TRIPLOG_LABEL);
    if (!s_part) {
        ESP_LOGE(TAG, "triplog partition not found — trip logging disabled");
        return;
    }
    boot_scan();
    load_deleted();
    trip_persist_set_reset_cb(trip_log_new_trip);

    s_queue = xQueueCreate(QUEUE_DEPTH, sizeof(trip_rec_t));
    if (!s_queue || xTaskCreate(writer_task, "trip_wr", 4096, NULL, 2, NULL) != pdPASS) {
        ESP_LOGE(TAG, "writer task/queue init failed — trip logging disabled");
        s_part = NULL;
    }
}

void trip_log_new_trip(void)
{
    s_pending_new = true;   /* handled in trip_log_tick (single context) */
}

void trip_log_tick(const vesc_setup_values_t *rt)
{
    if (!rt || !s_part || !s_queue) return;

    if (s_pending_new) {
        s_pending_new = false;
        s_trip_id++;
        s_trip_t_s = 0;
        s_last_sample_us = 0;   /* sample the new trip promptly */
        ESP_LOGI(TAG, "new trip %u", (unsigned)s_trip_id);
    }

    int64_t now = esp_timer_get_time();
    if (s_last_sample_us != 0 && (now - s_last_sample_us) < (int64_t)SAMPLE_INTERVAL_US) {
        return;
    }
    bool first = (s_last_sample_us == 0);
    s_last_sample_us = now;
    if (!first) s_trip_t_s += 10;

    trip_rec_t rec;
    memset(&rec, 0, sizeof rec);
    rec.magic         = REC_MAGIC;
    rec.type          = first ? REC_TYPE_TRIP_START : REC_TYPE_SAMPLE;
    rec.ver           = REC_VER;
    rec.trip_id       = s_trip_id;
    rec.t_s           = s_trip_t_s;
    rec.distance_m    = (uint32_t)(trip_persist_get_trip_km() * 1000.0f);
    rec.ah            = trip_persist_get_amp_hours();
    rec.wh            = rt->watt_hours;
    rec.uptime_ms     = trip_persist_get_uptime_ms();
    rec.speed_dkmh    = (int16_t)(rt->speed * 3.6f * 10.0f);
    rec.current_da    = (int16_t)(rt->current_in * 10.0f);
    rec.voltage_dv    = (uint16_t)(rt->v_in * 10.0f);
    rec.temp_motor_dc = (int16_t)(rt->temp_motor * 10.0f);
    rec.temp_fet_dc   = (int16_t)(rt->temp_mos * 10.0f);
    /* Log the on-screen battery % (Smart vs Direct per setting), not the raw
     * controller level — so the statistics chart matches the dashboard. */
    rec.batt_pct      = (uint8_t)(battery_calc_display_percentage(
                                      rt->battery_level, rt->amp_hours, rt->amp_hours_charged) + 0.5f);
    rec.fault         = rt->fault_code;
    /* seq + crc are filled by the writer task */

    /* Non-blocking: at one record / 10 s the writer keeps up; drop if somehow full. */
    if (xQueueSend(s_queue, &rec, 0) != pdTRUE) {
        ESP_LOGW(TAG, "queue full — sample dropped");
    }
}

uint32_t trip_log_first_trip_id(void)   { return s_first_trip_id; }
uint32_t trip_log_current_trip_id(void) { return s_trip_id; }

bool trip_log_is_trip_deleted(uint32_t trip_id) { return is_deleted(trip_id); }

void trip_log_delete_trip(uint32_t trip_id)
{
    if (trip_id == 0 || trip_id == s_trip_id) return;   /* never hide the live trip */
    if (is_deleted(trip_id)) return;
    if (s_deleted_count >= MAX_DELETED) {
        /* Set full — evict the smallest (oldest) id; it has almost certainly
         * aged out of the physical ring already, so nothing reappears. */
        int mn = 0;
        for (int i = 1; i < s_deleted_count; i++)
            if (s_deleted[i] < s_deleted[mn]) mn = i;
        s_deleted[mn] = s_deleted[s_deleted_count - 1];
        s_deleted_count--;
    }
    s_deleted[s_deleted_count++] = trip_id;
    save_deleted();
    ESP_LOGI(TAG, "trip %u hidden (%d total)", (unsigned)trip_id, s_deleted_count);
}

/* ============================ Reader API ============================ */

/* Walk every valid record in the ring, sector by sector (a 4 KB read is far
 * cheaper than 64 separate 64-byte reads), invoking cb for each. Read-only;
 * concurrent appends/erases by the writer are tolerated (the flash driver
 * serialises, and a torn/erased record fails its CRC and is skipped). */
typedef void (*rec_cb_t)(const trip_rec_t *r, void *u);

static void scan_records(rec_cb_t cb, void *u)
{
    if (!s_part || s_total_recs == 0) return;
    uint8_t *buf = malloc(SECTOR);
    if (!buf) { ESP_LOGW(TAG, "scan: no mem"); return; }

    uint32_t sectors = s_total_recs / RECS_PER_SECTOR;
    for (uint32_t sec = 0; sec < sectors; sec++) {
        if (esp_partition_read(s_part, (size_t)sec * SECTOR, buf, SECTOR) != ESP_OK) continue;
        for (uint32_t i = 0; i < RECS_PER_SECTOR; i++) {
            const trip_rec_t *r = (const trip_rec_t *)(buf + i * REC_SIZE);
            if (rec_valid(r)) cb(r, u);
        }
    }
    free(buf);
}

/* ---- list trips ---- */
typedef struct {
    bool     seen;
    uint32_t max_t_s;
    uint32_t max_dist;
    uint32_t count;
    float    max_ah;
    float    min_wh, max_wh;
    int32_t  max_speed;     /* dkmh */
    uint16_t min_volt;      /* dv, 0 = unset */
} trip_acc_t;

typedef struct { trip_acc_t *acc; uint32_t first, current; } list_ctx_t;

static void list_cb(const trip_rec_t *r, void *u)
{
    list_ctx_t *c = (list_ctx_t *)u;
    if (r->trip_id < c->first || r->trip_id > c->current) return;   /* outside the window */
    if (is_deleted(r->trip_id)) return;                             /* hidden by the user */
    trip_acc_t *a = &c->acc[r->trip_id - c->first];
    if (!a->seen) { a->seen = true; a->min_wh = a->max_wh = r->wh; a->max_speed = r->speed_dkmh; }
    if (r->t_s        > a->max_t_s)  a->max_t_s  = r->t_s;
    if (r->distance_m > a->max_dist) a->max_dist = r->distance_m;
    if (r->ah         > a->max_ah)   a->max_ah   = r->ah;
    if (r->wh < a->min_wh) a->min_wh = r->wh;
    if (r->wh > a->max_wh) a->max_wh = r->wh;
    if (r->speed_dkmh > a->max_speed) a->max_speed = r->speed_dkmh;
    if (r->voltage_dv > 0 && (a->min_volt == 0 || r->voltage_dv < a->min_volt)) a->min_volt = r->voltage_dv;
    a->count++;
}

int trip_log_list_trips(trip_summary_t *out, int max)
{
    if (!out || max <= 0 || !s_part) return 0;
    uint32_t current = s_trip_id;
    if (current == 0) return 0;
    /* Look back MAX_TRIPS *visible* trips: extend the id range by the number of
     * hidden trips so deleting frees up history slots (older trips surface,
     * as far back as the ring still physically holds them). list_cb skips the
     * hidden ids, so they never occupy an output slot. */
    uint32_t span  = MAX_TRIPS + (uint32_t)s_deleted_count;
    uint32_t first = (current > span) ? (current - span + 1) : 1;
    uint32_t window = current - first + 1;

    trip_acc_t *acc = calloc(window, sizeof(trip_acc_t));
    if (!acc) return 0;
    list_ctx_t ctx = { acc, first, current };
    scan_records(list_cb, &ctx);

    int n = 0;
    for (int64_t id = (int64_t)current; id >= (int64_t)first && n < max; id--) {
        trip_acc_t *a = &acc[id - first];
        if (!a->seen) continue;
        trip_summary_t *s = &out[n++];
        s->trip_id        = (uint32_t)id;
        s->duration_s     = a->max_t_s;
        s->distance_m     = a->max_dist;
        s->sample_count   = a->count;
        s->ah             = a->max_ah;
        s->wh             = a->max_wh - a->min_wh;
        s->max_speed_dkmh = (uint16_t)(a->max_speed > 0 ? a->max_speed : 0);
        s->min_voltage_dv = a->min_volt;
        /* Overall average over the whole trip incl. stops: distance / duration.
         * dkmh = (m / s) * 3.6 * 10 = distance_m * 36 / duration_s. */
        s->avg_speed_dkmh = (a->max_t_s > 0)
                          ? (uint16_t)(((uint64_t)a->max_dist * 36u) / a->max_t_s) : 0;
        s->is_current     = ((uint32_t)id == s_trip_id);
    }
    free(acc);
    return n;
}

/* ---- per-trip time-series, bucket-averaged to <= max points ---- */
typedef struct { uint32_t trip_id, max_t_s, count; } span_ctx_t;

static void span_cb(const trip_rec_t *r, void *u)
{
    span_ctx_t *c = (span_ctx_t *)u;
    if (r->trip_id != c->trip_id) return;
    if (r->t_s > c->max_t_s) c->max_t_s = r->t_s;
    c->count++;
}

typedef struct { int64_t speed, power, volt, tmot, tfet, batt; uint32_t n; } bucket_t;
typedef struct { uint32_t trip_id, span; int nb; bucket_t *b; } bk_ctx_t;

static void bucket_cb(const trip_rec_t *r, void *u)
{
    bk_ctx_t *c = (bk_ctx_t *)u;
    if (r->trip_id != c->trip_id) return;
    int idx = (c->span > 0) ? (int)(((uint64_t)r->t_s * c->nb) / (c->span + 1)) : 0;
    if (idx < 0) idx = 0;
    if (idx >= c->nb) idx = c->nb - 1;
    bucket_t *b = &c->b[idx];
    b->speed += r->speed_dkmh;
    b->power += ((int64_t)r->voltage_dv * r->current_da) / 100;   /* (V·10)·(A·10)/100 = V·A */
    b->volt  += r->voltage_dv;
    b->tmot  += r->temp_motor_dc;
    b->tfet  += r->temp_fet_dc;
    b->batt  += r->batt_pct;
    b->n++;
}

int trip_log_read_series(uint32_t trip_id, trip_sample_t *out, int max)
{
    if (!out || max <= 0 || !s_part) return 0;

    span_ctx_t span = { trip_id, 0, 0 };
    scan_records(span_cb, &span);
    if (span.count == 0) return 0;

    bucket_t *b = calloc(max, sizeof(bucket_t));
    if (!b) return 0;
    bk_ctx_t bc = { trip_id, span.max_t_s, max, b };
    scan_records(bucket_cb, &bc);

    int n = 0;
    for (int i = 0; i < max; i++) {
        if (b[i].n == 0) continue;          /* gap bucket — skip rather than interpolate */
        int64_t cnt = (int64_t)b[i].n;
        trip_sample_t *o = &out[n++];
        o->t_s           = (span.max_t_s > 0) ? (uint32_t)(((uint64_t)i * (span.max_t_s + 1)) / max) : 0;
        o->speed_dkmh    = (int16_t)(b[i].speed / cnt);
        o->power_w       = (int16_t)(b[i].power / cnt);
        o->voltage_dv    = (uint16_t)(b[i].volt / cnt);
        o->temp_motor_dc = (int16_t)(b[i].tmot / cnt);
        o->temp_fet_dc   = (int16_t)(b[i].tfet / cnt);
        o->batt_pct      = (uint8_t)(b[i].batt / cnt);
    }
    free(b);
    return n;
}
