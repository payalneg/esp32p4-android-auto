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
#include "vesc_can/crc.h"

#include "esp_log.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <string.h>

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
    rec.batt_pct      = (uint8_t)(rt->battery_level * 100.0f);
    rec.fault         = rt->fault_code;
    /* seq + crc are filled by the writer task */

    /* Non-blocking: at one record / 10 s the writer keeps up; drop if somehow full. */
    if (xQueueSend(s_queue, &rec, 0) != pdTRUE) {
        ESP_LOGW(TAG, "queue full — sample dropped");
    }
}

uint32_t trip_log_first_trip_id(void)   { return s_first_trip_id; }
uint32_t trip_log_current_trip_id(void) { return s_trip_id; }
