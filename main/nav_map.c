#include "nav_map.h"

#include <math.h>

#include "esp_timer.h"
#include <string.h>

#include "nav_route.h"
#include "nav_tiles.h"

/* Ground that has no tile yet. Deliberately not black: a missing tile should
 * read as "not here yet", not as a hole in the screen. */
#define BG_COLOUR 0x4A69   /* muted slate, RGB565 */

#define MARKER_R      9
#define MARKER_RING   3
#define COL_RIDER     0x1B1B   /* blue */
#define COL_RING      0xFFFF

/* The panel the view is composed for. Only the debug helper needs it as a
 * constant; nav_map_render is told the size it is filling. */
#define NAV_VIEW_W 800
#define NAV_VIEW_H 480

static nav_map_view_t s_view;
static float s_speed_ms;

/* The rider's choice from the panel's zoom buttons, or 0 while they have not
 * touched them and the phone's zoom is used as it arrives. */
static uint8_t s_zoom_override;

/* What the last route draw cost, microseconds (navstat prints it). */
static uint32_t s_route_us;

/* Where the phone last said we are. The drawn view chases this. */
static int32_t s_target_lat_e7, s_target_lon_e7;
static bool    s_have_target;

/* How much of the remaining error to take out per frame, as a divisor: a
 * quarter converges within a couple of updates and never overshoots. A shift
 * rather than a multiply because the position is an integer, which is what
 * keeps eight steps a second from drifting. */
#define CATCH_UP_DIV 4

/* Fixed conversions, so the frame path has no named constants to divide by.
 *   e7 degrees -> radians : pi/180 * 1e-7
 *   metres -> e7 degrees  : 1e7 / 111320
 */
#define E7_TO_RAD   1.74532925e-9f
#define M_TO_E7     89.8320f
#define DEG_TO_RAD  0.0174532925f

/* A position that disagrees with the drawn one by more than this is not
 * drift, it is a different place: jump rather than glide. Two hundredths of a
 * degree is a couple of kilometres, well past a screen. */
#define JUMP_LAT_E7 200000
#define JUMP_LON_E7 300000

/* Integer division that rounds down rather than towards zero, so a view west
 * of the prime meridian picks the same tile as one east of it. */
static int64_t floor_div(int64_t a, int64_t b)
{
    return (a >= 0) ? a / b : -(((-a) + b - 1) / b);
}

/* The view centre in world pixels, which is the one place that needs more
 * precision than a float carries. */
static int64_t world_x_px(int32_t lon_e7, uint8_t zoom)
{
    /* Exact: (lon + 180) / 360 * 256 * 2^zoom, in integers all the way. */
    const int64_t n = (int64_t)NAV_TILE_PX << zoom;
    return ((int64_t)lon_e7 + 1800000000LL) * n / 3600000000LL;
}

static int64_t world_y_px(int32_t lat_e7, uint8_t zoom)
{
    /* The only logarithm left on the frame path: once, for the centre.
     *
     * Single precision carries about seven digits, and the result here runs
     * to sixty-seven million pixels at zoom 18 — so this can be up to three
     * pixels out (about a metre and a half on the ground). It is a systematic
     * offset, not jitter: the same position always gives the same answer, and
     * everything on screen — tiles, line, marker — shifts together, because
     * every other point is placed relative to this one. A double here would
     * be exact and would also put a software floating-point routine on the
     * frame path; the metre and a half is the better trade. */
    const float r = (float)lat_e7 * E7_TO_RAD;
    const float my = logf(tanf(r) + 1.0f / cosf(r));
    const float t = (1.0f - my * (float)M_1_PI) * 0.5f;
    return (int64_t)llroundf(t * (float)((int64_t)NAV_TILE_PX << zoom));
}

void nav_map_set_view(int32_t lat_e7, int32_t lon_e7, uint8_t zoom,
                     uint16_t heading_deg)
{
    s_target_lat_e7 = lat_e7;
    s_target_lon_e7 = lon_e7;
    s_have_target = true;
    s_view.zoom = s_zoom_override ? s_zoom_override : zoom;
    s_view.heading_deg = heading_deg;
    if (!s_view.valid) {
        /* Nothing drawn yet — start where we are told rather than easing in
         * from the middle of the ocean. */
        s_view.lat_e7 = lat_e7;
        s_view.lon_e7 = lon_e7;
        s_view.valid = true;
        return;
    }
    const int32_t dlat = lat_e7 > s_view.lat_e7 ? lat_e7 - s_view.lat_e7
                                                : s_view.lat_e7 - lat_e7;
    const int32_t dlon = lon_e7 > s_view.lon_e7 ? lon_e7 - s_view.lon_e7
                                                : s_view.lon_e7 - lon_e7;
    if (dlat > JUMP_LAT_E7 || dlon > JUMP_LON_E7) {
        s_view.lat_e7 = lat_e7;
        s_view.lon_e7 = lon_e7;
    }
}

bool nav_map_get_proj(nav_map_proj_t *out, int w, int h)
{
    if (!out || !s_view.valid) return false;
    /* Pixels per 1e-7 degree of longitude: 256 * 2^zoom / 3.6e9. Latitude is
     * the same times 1/cos(lat) — the Mercator stretch, taken at the centre
     * and constant to well under a pixel across one panel. */
    const float n = (float)((int64_t)NAV_TILE_PX << s_view.zoom);
    out->lat_e7 = s_view.lat_e7;
    out->lon_e7 = s_view.lon_e7;
    out->cx = w / 2;
    out->cy = h / 2;
    out->kx = n / 3.6e9f;
    out->ky = out->kx / cosf((float)s_view.lat_e7 * E7_TO_RAD);
    return true;
}

void nav_map_unproject(int x, int y, int w, int h,
                       int32_t *lat_e7, int32_t *lon_e7)
{
    if (!s_view.valid) {
        if (lat_e7) *lat_e7 = 0;
        if (lon_e7) *lon_e7 = 0;
        return;
    }
    /* The inverse of nav_map_project, and just as local: e7 degrees per
     * pixel, stretched by the latitude for the north-south axis. */
    const float per_px = 3.6e9f / (float)((int64_t)NAV_TILE_PX << s_view.zoom);
    if (lon_e7) {
        *lon_e7 = s_view.lon_e7 + (int32_t)lroundf((float)(x - w / 2) * per_px);
    }
    if (lat_e7) {
        const float stretch = cosf((float)s_view.lat_e7 * E7_TO_RAD);
        *lat_e7 = s_view.lat_e7 -
                  (int32_t)lroundf((float)(y - h / 2) * per_px * stretch);
    }
}

void nav_map_set_speed(uint16_t cm_per_s)
{
    s_speed_ms = (float)cm_per_s * 0.01f;
}

void nav_map_dead_reckon(uint32_t dt_ms)
{
    if (!s_view.valid) return;

    if (s_speed_ms > 0.1f && s_view.heading_deg <= 360) {
        const float metres = s_speed_ms * ((float)dt_ms * 0.001f);
        const float bearing = (float)s_view.heading_deg * DEG_TO_RAD;
        /* Flat-earth step: at a few metres a tick the error is far below a
         * pixel, and the easing below takes out whatever it gets wrong. */
        const float lat_rad = (float)s_view.lat_e7 * E7_TO_RAD;
        s_view.lat_e7 += (int32_t)lroundf(metres * cosf(bearing) * M_TO_E7);
        s_view.lon_e7 += (int32_t)lroundf(metres * sinf(bearing) * M_TO_E7 /
                                          cosf(lat_rad));
    }

    if (s_have_target) {
        s_view.lat_e7 += (s_target_lat_e7 - s_view.lat_e7) / CATCH_UP_DIV;
        s_view.lon_e7 += (s_target_lon_e7 - s_view.lon_e7) / CATCH_UP_DIV;
    }
}

uint32_t nav_map_last_route_us(void) { return s_route_us; }

void nav_map_set_zoom(uint8_t zoom)
{
    if (zoom < NAV_ZOOM_MIN) zoom = NAV_ZOOM_MIN;
    if (zoom > NAV_ZOOM_MAX) zoom = NAV_ZOOM_MAX;
    s_zoom_override = zoom;
    s_view.zoom = zoom;
}

void nav_map_get_view(nav_map_view_t *out)
{
    if (out) *out = s_view;
}

static void fill(uint16_t *dst, int w, int x0, int y0, int x1, int y1, uint16_t c)
{
    for (int y = y0; y < y1; y++) {
        uint16_t *row = dst + (size_t)y * w;
        for (int x = x0; x < x1; x++) row[x] = c;
    }
}

/* Which way the rider is pointing, as an arrow around the same spot the dot
 * marks. This is what a track-up map would have said, for nothing: turning
 * the whole map to the heading means warping all 384000 pixels between two
 * PSRAM buffers every frame, and the panel has no hardware for an arbitrary
 * angle (the PPA turns in 90-degree steps). Measured with `navwarp` on the
 * board: 38 ms at 30 degrees and 118 ms at 90, on top of 18-26 ms of
 * composing — against an LVGL flush that already costs 57 ms on the same
 * core. So the map stays north-up and the marker carries the heading.
 *
 * Filled by three edge tests per pixel over the arrow's own bounding box:
 * a few hundred pixels, unmeasurable next to the rest of the frame. */
static void draw_arrow(uint16_t *dst, int w, int h, int cx, int cy,
                       float heading_deg)
{
    const float rad = heading_deg * DEG_TO_RAD;
    /* Screen y grows downwards, so north (heading 0) is -y. */
    const float dx = sinf(rad), dy = -cosf(rad);
    const float px = -dy, py = dx;           /* to the rider's right */

    /* The body, then the same triangle grown about its own centre for the
     * casing. Growing it rather than nudging tip, tail and width by hand is
     * what makes the white border even — the first version did the latter and
     * the border vanished along two edges.
     *
     * Three vertices from one sine and one cosine, then the fill in plain
     * integers. That distinction is the whole cost: the first version ran the
     * three edge tests per pixel in double precision, and on a chip whose
     * hardware knows only single precision those two thousand pixels added
     * nearly THIRTY milliseconds to a frame that composes in twenty. */
    const float tipf = 15.0f, backf = 7.0f, halff = 9.0f;
    int vx[3], vy[3];
    vx[0] = (int)lroundf((float)cx + dx * tipf);
    vy[0] = (int)lroundf((float)cy + dy * tipf);
    vx[1] = (int)lroundf((float)cx - dx * backf + px * halff);
    vy[1] = (int)lroundf((float)cy - dy * backf + py * halff);
    vx[2] = (int)lroundf((float)cx - dx * backf - px * halff);
    vy[2] = (int)lroundf((float)cy - dy * backf - py * halff);
    const int gx = (vx[0] + vx[1] + vx[2]) / 3;
    const int gy = (vy[0] + vy[1] + vy[2]) / 3;

    for (int pass = 0; pass < 2; pass++) {
        /* 3/2 about the centre for the casing, then the body over it. */
        const int num = pass == 0 ? 3 : 1, den = pass == 0 ? 2 : 1;
        const uint16_t col = pass == 0 ? COL_RING : COL_RIDER;
        int ax[3], ay[3];
        for (int i = 0; i < 3; i++) {
            ax[i] = gx + (vx[i] - gx) * num / den;
            ay[i] = gy + (vy[i] - gy) * num / den;
        }

        int x0 = ax[0], x1 = ax[0], y0 = ay[0], y1 = ay[0];
        for (int i = 1; i < 3; i++) {
            if (ax[i] < x0) x0 = ax[i];
            if (ax[i] > x1) x1 = ax[i];
            if (ay[i] < y0) y0 = ay[i];
            if (ay[i] > y1) y1 = ay[i];
        }
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > w - 1) x1 = w - 1;
        if (y1 > h - 1) y1 = h - 1;

        /* Edge coefficients, so the per-pixel work is three multiply-adds. */
        int ex[3], ey[3], ec[3];
        for (int i = 0; i < 3; i++) {
            const int j = (i + 1) % 3;
            ex[i] = ay[i] - ay[j];
            ey[i] = ax[j] - ax[i];
            ec[i] = -(ex[i] * ax[i] + ey[i] * ay[i]);
        }
        for (int y = y0; y <= y1; y++) {
            uint16_t *row = dst + (size_t)y * w;
            for (int x = x0; x <= x1; x++) {
                const int e0 = ex[0] * x + ey[0] * y + ec[0];
                const int e1 = ex[1] * x + ey[1] * y + ec[1];
                const int e2 = ex[2] * x + ey[2] * y + ec[2];
                if ((e0 >= 0 && e1 >= 0 && e2 >= 0) ||
                    (e0 <= 0 && e1 <= 0 && e2 <= 0)) {
                    row[x] = col;
                }
            }
        }
    }
}

/* The rider, drawn where the view puts them. A ring so the dot reads against
 * both the pale roads and the dark parks. */
static void draw_marker(uint16_t *dst, int w, int h, int cx, int cy)
{
    const int r = MARKER_R;
    for (int dy = -r; dy <= r; dy++) {
        const int y = cy + dy;
        if (y < 0 || y >= h) continue;
        uint16_t *row = dst + (size_t)y * w;
        for (int dx = -r; dx <= r; dx++) {
            const int x = cx + dx;
            if (x < 0 || x >= w) continue;
            const int d2 = dx * dx + dy * dy;
            if (d2 > r * r) continue;
            row[x] = (d2 > (r - MARKER_RING) * (r - MARKER_RING))
                         ? COL_RING : COL_RIDER;
        }
    }
}

bool nav_map_view_tiles(uint8_t *z, int64_t *x0, int64_t *x1,
                        int64_t *y0, int64_t *y1)
{
    if (!s_view.valid) return false;
    const int64_t left = world_x_px(s_view.lon_e7, s_view.zoom) - NAV_VIEW_W / 2;
    const int64_t top  = world_y_px(s_view.lat_e7, s_view.zoom) - NAV_VIEW_H / 2;
    if (z)  *z  = s_view.zoom;
    if (x0) *x0 = floor_div(left, NAV_TILE_PX);
    if (x1) *x1 = floor_div(left + NAV_VIEW_W - 1, NAV_TILE_PX);
    if (y0) *y0 = floor_div(top, NAV_TILE_PX);
    if (y1) *y1 = floor_div(top + NAV_VIEW_H - 1, NAV_TILE_PX);
    return true;
}

/* Blit one zoom level into the view.
 *
 * `shift` is how many levels below the detail zoom this one sits, so each of
 * its pixels covers 1<<shift on screen. Walks the tiles that intersect the
 * viewport and expands each one into place; a tile that has not arrived is
 * simply skipped, leaving whatever a coarser level already put there.
 *
 * Per tile rather than per screen pixel, and each source row is written once
 * and then copied to the rest of its band. The first version of this walked
 * every destination pixel computing its source — correct, and 155 ms for two
 * coarse layers, which is most of a frame. This is a few milliseconds.
 *
 * Returns how many of the tiles it wanted were actually there. */
typedef struct { int x0, y0, x1, y1; } rect_t;   /* x1/y1 exclusive */

static int blit_level(uint16_t *dst, int w, int h, int64_t sl, int64_t st,
                      uint8_t zoom, int shift, const rect_t *clip,
                      int *out_wanted)
{
    const int scale = 1 << shift;
    const int64_t span = (int64_t)1 << zoom;
    /* Visible range in this level's own world pixels, narrowed to the clip so
     * a gap in the detail layer only costs the pixels it actually covers. */
    const int cx0 = clip ? clip->x0 : 0;
    const int cy0 = clip ? clip->y0 : 0;
    const int cx1 = clip ? clip->x1 : w;
    const int cy1 = clip ? clip->y1 : h;
    if (cx1 <= cx0 || cy1 <= cy0) return 0;
    const int64_t lx0 = (sl + cx0) >> shift, ly0 = (st + cy0) >> shift;
    const int64_t lx1 = (sl + cx1 - 1) >> shift, ly1 = (st + cy1 - 1) >> shift;

    int have = 0, wanted = 0;
    for (int64_t ty = ly0 >> 8; ty <= (ly1 >> 8); ty++) {
        if (ty < 0 || ty >= span) continue;
        for (int64_t tx = lx0 >> 8; tx <= (lx1 >> 8); tx++) {
            int64_t wx = tx % span;
            if (wx < 0) wx += span;
            wanted++;
            const uint16_t *px = nav_tiles_get(zoom, (uint32_t)wx, (uint32_t)ty);
            if (!px) continue;
            have++;

            /* This tile's pixels, clipped to what is on screen. */
            int64_t a0 = tx * NAV_TILE_PX, a1 = a0 + NAV_TILE_PX - 1;
            if (a0 < lx0) a0 = lx0;
            if (a1 > lx1) a1 = lx1;
            int64_t b0 = ty * NAV_TILE_PX, b1 = b0 + NAV_TILE_PX - 1;
            if (b0 < ly0) b0 = ly0;
            if (b1 > ly1) b1 = ly1;

            for (int64_t ly = b0; ly <= b1; ly++) {
                const uint16_t *srow =
                    px + (size_t)(ly - ty * NAV_TILE_PX) * NAV_TILE_PX;
                const int band_y = (int)((ly << shift) - st);
                int y = band_y < cy0 ? cy0 : band_y;
                if (y >= cy1) break;

                uint16_t *drow = dst + (size_t)y * w;

                /* The detail level maps one to one, so a row is a memcpy.
                 * Writing it pixel by pixel like the scaled levels cost
                 * 60 ms of the frame. */
                if (scale == 1) {
                    const int x0 = (int)(a0 - sl);
                    memcpy(drow + x0, srow + (a0 - tx * NAV_TILE_PX),
                           (size_t)(a1 - a0 + 1) * 2);
                    continue;
                }

                /* Write the first row of the band... */
                int run_x0 = cx1, run_x1 = cx0;
                for (int64_t lx = a0; lx <= a1; lx++) {
                    const uint16_t v = srow[lx - tx * NAV_TILE_PX];
                    const int bx = (int)((lx << shift) - sl);
                    int x = bx < cx0 ? cx0 : bx;
                    const int xe = (bx + scale < cx1) ? bx + scale : cx1;
                    if (x < run_x0) run_x0 = x;
                    if (xe > run_x1) run_x1 = xe;
                    for (; x < xe; x++) drow[x] = v;
                }
                /* ...and copy it down the rest. */
                if (run_x1 > run_x0) {
                    const int ye = (band_y + scale < cy1) ? band_y + scale : cy1;
                    for (int yy = y + 1; yy < ye; yy++) {
                        memcpy(dst + (size_t)yy * w + run_x0, drow + run_x0,
                               (size_t)(run_x1 - run_x0) * 2);
                    }
                }
            }
        }
    }
    if (out_wanted) *out_wanted = wanted;
    return have;
}

/* The other direction: tiles one level SHARPER than the view, halved. Right
 * after a zoom-out every tile we hold is like this, and the phone needs a
 * second or two to send the new level — sampling every other pixel keeps the
 * ground on screen instead of showing the rider an empty slate. */
static void blit_level_half(uint16_t *dst, int w, int h, int64_t sl, int64_t st,
                            uint8_t zoom, const rect_t *clip)
{
    const int64_t span = (int64_t)1 << zoom;
    const int cx0 = clip ? clip->x0 : 0;
    const int cy0 = clip ? clip->y0 : 0;
    const int cx1 = clip ? clip->x1 : w;
    const int cy1 = clip ? clip->y1 : h;
    if (cx1 <= cx0 || cy1 <= cy0) return;

    /* Source world pixels are twice ours. */
    const int64_t sx_lo = 2 * (sl + cx0), sx_hi = 2 * (sl + cx1) - 1;
    const int64_t sy_lo = 2 * (st + cy0), sy_hi = 2 * (st + cy1) - 1;

    for (int64_t ty = sy_lo >> 8; ty <= (sy_hi >> 8); ty++) {
        if (ty < 0 || ty >= span) continue;
        for (int64_t tx = sx_lo >> 8; tx <= (sx_hi >> 8); tx++) {
            int64_t wx = tx % span;
            if (wx < 0) wx += span;
            const uint16_t *px = nav_tiles_get(zoom, (uint32_t)wx, (uint32_t)ty);
            if (!px) continue;

            /* Destination pixels whose source falls inside this tile. */
            const int64_t a0 = tx * NAV_TILE_PX, a1 = a0 + NAV_TILE_PX - 1;
            const int64_t b0 = ty * NAV_TILE_PX, b1 = b0 + NAV_TILE_PX - 1;
            int x_from = (int)((a0 + 1) / 2 - sl), x_to = (int)(a1 / 2 - sl);
            int y_from = (int)((b0 + 1) / 2 - st), y_to = (int)(b1 / 2 - st);
            if (x_from < cx0) x_from = cx0;
            if (y_from < cy0) y_from = cy0;
            if (x_to > cx1 - 1) x_to = cx1 - 1;
            if (y_to > cy1 - 1) y_to = cy1 - 1;

            for (int y = y_from; y <= y_to; y++) {
                const uint16_t *srow =
                    px + (size_t)(2 * (st + y) - b0) * NAV_TILE_PX;
                uint16_t *drow = dst + (size_t)y * w;
                for (int x = x_from; x <= x_to; x++) {
                    drow[x] = srow[2 * (sl + x) - a0];
                }
            }
        }
    }
}

int nav_map_render(uint16_t *dst, int w, int h, int *out_wanted)
{
    if (out_wanted) *out_wanted = 0;
    if (!dst || w <= 0 || h <= 0) return 0;
    if (!s_view.valid) {
        fill(dst, w, 0, 0, w, h, BG_COLOUR);
        return 0;
    }

    /* World pixel of the top-left corner of the screen, at the detail zoom.
     * Integers: the longitude exactly, the latitude through the frame's one
     * logarithm (see world_y_px). */
    const int64_t sl = world_x_px(s_view.lon_e7, s_view.zoom) - w / 2;
    const int64_t st = world_y_px(s_view.lat_e7, s_view.zoom) - h / 2;

    /* The detail tiles first, then the blurred layers only where they are
     * missing. Painting every layer across the whole screen wrote each pixel
     * three times over — 63 ms a frame on this panel, most of it PSRAM
     * bandwidth. Filling only the gaps costs nothing once the map has caught
     * up, and no more than before while it has not. */
    rect_t gaps[24];
    int ngaps = 0;
    int wanted = 0, have = 0;

    const int64_t span = (int64_t)1 << s_view.zoom;
    const int64_t tx0 = sl / NAV_TILE_PX - (sl % NAV_TILE_PX < 0 ? 1 : 0);
    const int64_t ty0 = st / NAV_TILE_PX - (st % NAV_TILE_PX < 0 ? 1 : 0);
    const int64_t tx1 = (sl + w - 1) / NAV_TILE_PX;
    const int64_t ty1 = (st + h - 1) / NAV_TILE_PX;

    for (int64_t ty = ty0; ty <= ty1; ty++) {
        for (int64_t tx = tx0; tx <= tx1; tx++) {
            rect_t r;
            r.x0 = (int)(tx * NAV_TILE_PX - sl);
            r.y0 = (int)(ty * NAV_TILE_PX - st);
            r.x1 = r.x0 + NAV_TILE_PX;
            r.y1 = r.y0 + NAV_TILE_PX;
            if (r.x0 < 0) r.x0 = 0;
            if (r.y0 < 0) r.y0 = 0;
            if (r.x1 > w) r.x1 = w;
            if (r.y1 > h) r.y1 = h;
            if (r.x1 <= r.x0 || r.y1 <= r.y0) continue;

            const uint16_t *px = NULL;
            if (ty >= 0 && ty < span) {
                int64_t wx = tx % span;
                if (wx < 0) wx += span;
                wanted++;
                px = nav_tiles_get(s_view.zoom, (uint32_t)wx, (uint32_t)ty);
            }
            if (px) {
                have++;
                const int sx = r.x0 - (int)(tx * NAV_TILE_PX - sl);
                const int sy = r.y0 - (int)(ty * NAV_TILE_PX - st);
                for (int y = r.y0; y < r.y1; y++) {
                    memcpy(dst + (size_t)y * w + r.x0,
                           px + (size_t)(sy + y - r.y0) * NAV_TILE_PX + sx,
                           (size_t)(r.x1 - r.x0) * 2);
                }
            } else if (ngaps < (int)(sizeof gaps / sizeof gaps[0])) {
                gaps[ngaps++] = r;
            }
        }
    }

    for (int i = 0; i < ngaps; i++) {
        const rect_t r = gaps[i];
        fill(dst, w, r.x0, r.y0, r.x1, r.y1, BG_COLOUR);
        if (s_view.zoom >= NAV_WIDE_DZ) {
            blit_level(dst, w, h, sl, st,
                       (uint8_t)(s_view.zoom - NAV_WIDE_DZ), NAV_WIDE_DZ,
                       &r, NULL);
        }
        if (s_view.zoom >= NAV_COARSE_DZ) {
            blit_level(dst, w, h, sl, st,
                       (uint8_t)(s_view.zoom - NAV_COARSE_DZ), NAV_COARSE_DZ,
                       &r, NULL);
        }
        /* The two levels either side of this one, sharpest last. Normally we
         * hold neither and both lookups miss cheaply; right after the rider
         * has pressed a zoom button, one of them is the whole map. */
        if (s_view.zoom >= 1) {
            blit_level(dst, w, h, sl, st, (uint8_t)(s_view.zoom - 1), 1,
                       &r, NULL);
        }
        blit_level_half(dst, w, h, sl, st, (uint8_t)(s_view.zoom + 1), &r);
    }

    /* The line goes over the map and under the rider. Timed on its own: it
     * is the one part of the frame whose cost depends on the route rather
     * than on the panel, so it is the first suspect when a frame gets slow. */
    const int64_t t0 = esp_timer_get_time();
    nav_route_draw(dst, w, h);
    s_route_us = (uint32_t)(esp_timer_get_time() - t0);
    /* An arrow while there is a heading to show, a plain dot when standing
     * still — a parked bike pointing somewhere definite is a lie. */
    if (s_view.heading_deg <= 360 && s_speed_ms > 0.5) {
        draw_arrow(dst, w, h, w / 2, h / 2, (float)s_view.heading_deg);
    } else {
        draw_marker(dst, w, h, w / 2, h / 2);
    }
    if (out_wanted) *out_wanted = wanted;
    return have;
}
