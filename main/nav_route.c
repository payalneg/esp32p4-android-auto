#include "nav_route.h"

#include <math.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "nav_map.h"
#include "nav_tiles.h"

static const char *TAG = "nav_route";

/* Blue with a white casing, like every map app — the casing is what makes the
 * line readable over both pale streets and dark parks. */
#define ROUTE_COLOUR  0x1B1B   /* RGB565 blue */
#define CASING_COLOUR 0xFFFF
#define ROUTE_HALF    3        /* half width, pixels; casing adds one each side */

static int32_t *s_pts;         /* lat/lon pairs, 1e7 fixed point */
static int32_t *s_px;          /* screen pixels, recomputed per frame */
static uint8_t *s_on;          /* near the screen this frame */
static size_t   s_count;
static nav_guide_t s_guide;

bool nav_route_set(const uint8_t *points, size_t bytes)
{
    const size_t n = bytes / 8;
    if (n == 0 || n > NAV_ROUTE_MAX_POINTS) {
        ESP_LOGW(TAG, "route of %u points rejected", (unsigned)n);
        return false;
    }
    if (!s_pts) {
        s_pts = heap_caps_malloc(NAV_ROUTE_MAX_POINTS * 8,
                                 MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_pts) return false;
    }
    memcpy(s_pts, points, n * 8);
    s_count = n;
    ESP_LOGI(TAG, "route of %u points", (unsigned)n);
    return true;
}

void nav_route_clear(void)
{
    s_count = 0;
    s_guide.valid = false;
}

size_t nav_route_count(void) { return s_count; }

void nav_route_set_guide(const nav_guide_t *g)
{
    if (g) s_guide = *g;
}

void nav_route_get_guide(nav_guide_t *out)
{
    if (out) *out = s_guide;
}

/* A square stamp, for the joins between spans. */
static inline void stamp(uint16_t *dst, int w, int h, int cx, int cy,
                         int half, uint16_t c)
{
    for (int yy = cy - half; yy <= cy + half; yy++) {
        if (yy < 0 || yy >= h) continue;
        uint16_t *row = dst + (size_t)yy * w;
        for (int xx = cx - half; xx <= cx + half; xx++) {
            if (xx < 0 || xx >= w) continue;
            row[xx] = c;
        }
    }
}

/* One span of the line: a bar swept along it, one bar per step across the
 * span's long axis, with a square at each end so the joins are not notched.
 * Stamping a full square at every step instead wrote each pixel seven times
 * over and cost 15-25 ms of the frame on a long route. */
static void draw_span(uint16_t *dst, int w, int h,
                      int x0, int y0, int x1, int y1, int half, uint16_t c)
{
    /* Clip to the screen first. A route holds whole straight stretches as one
     * segment, so an end can land tens of thousands of pixels away — stepping
     * that pixel by pixel and clipping inside the loop cost far more than
     * drawing everything visible. Liang-Barsky against the panel grown by the
     * line's half width. */
    const float fdx = (float)(x1 - x0), fdy = (float)(y1 - y0);
    const float pq[4][2] = {
        { -fdx, (float)(x0 + half) },          /* left   */
        {  fdx, (float)(w + half - x0) },      /* right  */
        { -fdy, (float)(y0 + half) },          /* top    */
        {  fdy, (float)(h + half - y0) },      /* bottom */
    };
    float t0 = 0.0f, t1 = 1.0f;
    for (int k = 0; k < 4; k++) {
        if (pq[k][0] == 0.0f) {
            if (pq[k][1] < 0.0f) return;       /* parallel and outside */
            continue;
        }
        const float r = pq[k][1] / pq[k][0];
        if (pq[k][0] < 0.0f) { if (r > t1) return; if (r > t0) t0 = r; }
        else                 { if (r < t0) return; if (r < t1) t1 = r; }
    }
    const int ax = x0 + (int)(t0 * fdx), ay = y0 + (int)(t0 * fdy);
    const int bx = x0 + (int)(t1 * fdx), by = y0 + (int)(t1 * fdy);

    const int dx = abs(bx - ax), dy = abs(by - ay);
    const int steps = (dx > dy ? dx : dy);
    if (steps == 0) {
        stamp(dst, w, h, ax, ay, half, c);
        return;
    }
    if (dx >= dy) {
        /* Mostly horizontal: one pass per row of the thickness, stepping x
         * inside it. A vertical bar per column is the obvious way and the
         * slow one — PSRAM charges a cache-line fetch for every 2-byte write
         * that misses, and a bar of seven pixels misses seven times. Sweeping
         * along x keeps consecutive writes in the same line. */
        for (int dyo = -half; dyo <= half; dyo++) {
            for (int i = 0; i <= steps; i++) {
                const int cx = ax + (bx - ax) * i / steps;
                const int cy = ay + (by - ay) * i / steps + dyo;
                if (cx < 0 || cx >= w || cy < 0 || cy >= h) continue;
                dst[(size_t)cy * w + cx] = c;
            }
        }
    } else {
        /* Mostly vertical: a horizontal run per row, which is also one
         * contiguous write. */
        for (int i = 0; i <= steps; i++) {
            const int cy = ay + (by - ay) * i / steps;
            if (cy < 0 || cy >= h) continue;
            const int cx = ax + (bx - ax) * i / steps;
            int x0c = cx - half, x1c = cx + half;
            if (x0c < 0) x0c = 0;
            if (x1c >= w) x1c = w - 1;
            uint16_t *row = dst + (size_t)cy * w;
            for (int xx = x0c; xx <= x1c; xx++) row[xx] = c;
        }
    }
    stamp(dst, w, h, ax, ay, half, c);
    stamp(dst, w, h, bx, by, half, c);
}

void nav_route_draw(uint16_t *dst, int w, int h)
{
    if (!dst || s_count < 2 || !s_pts) return;
    if (!s_px) {
        s_px = heap_caps_malloc(NAV_ROUTE_MAX_POINTS * sizeof(int32_t) * 2,
                                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        s_on = heap_caps_malloc(NAV_ROUTE_MAX_POINTS,
                                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_px || !s_on) return;
    }

    /* Which points are anywhere near the screen, in latitude and longitude —
     * plain integer comparisons on the wire's own units. This is what keeps a
     * long route cheap: it used to project every point of a four-kilometre
     * line, at a logarithm and a tangent each, and that alone was 40 ms of
     * the frame. */
    nav_map_proj_t proj;
    if (!nav_map_get_proj(&proj, w, h)) return;
    const int margin_px = (w > h ? w : h);
    int32_t lat_lo, lat_hi, lon_lo, lon_hi;
    nav_map_unproject(-margin_px, h + margin_px, w, h, &lat_lo, &lon_lo);
    nav_map_unproject(w + margin_px, -margin_px, w, h, &lat_hi, &lon_hi);
    if (lat_lo > lat_hi) { const int32_t t = lat_lo; lat_lo = lat_hi; lat_hi = t; }
    if (lon_lo > lon_hi) { const int32_t t = lon_lo; lon_lo = lon_hi; lon_hi = t; }

    for (size_t i = 0; i < s_count; i++) {
        const int32_t la = s_pts[i * 2], lo = s_pts[i * 2 + 1];
        s_on[i] = (la >= lat_lo && la <= lat_hi && lo >= lon_lo && lo <= lon_hi);
    }

    /* Place what the spans below need — a point is wanted if it or either
     * neighbour is near the screen — with two multiplies each, against the
     * projection built once for the view's centre. */
    int32_t *px = s_px;
    bool any = false;
    for (size_t i = 0; i < s_count; i++) {
        const bool wanted = s_on[i] ||
                            (i > 0 && s_on[i - 1]) ||
                            (i + 1 < s_count && s_on[i + 1]);
        if (!wanted) continue;
        int x, y;
        nav_map_project(&proj, s_pts[i * 2], s_pts[i * 2 + 1], &x, &y);
        px[i * 2]     = x;
        px[i * 2 + 1] = y;
        any = true;
    }
    if (!any) return;

    /* Casing first over the whole visible line, then the line itself, so the
     * two never interleave at the joins. */
    for (int pass = 0; pass < 2; pass++) {
        const int half = pass == 0 ? ROUTE_HALF + 1 : ROUTE_HALF;
        const uint16_t c = pass == 0 ? CASING_COLOUR : ROUTE_COLOUR;
        for (size_t i = 0; i + 1 < s_count; i++) {
            if (!s_on[i] && !s_on[i + 1]) continue;
            draw_span(dst, w, h, px[i * 2], px[i * 2 + 1],
                      px[(i + 1) * 2], px[(i + 1) * 2 + 1], half, c);
        }
    }
}
