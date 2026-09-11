#include "nav_map.h"

#include <math.h>
#include <string.h>

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
static double s_speed_ms;

double nav_map_world_x(double lon, uint8_t zoom)
{
    return (lon + 180.0) / 360.0 * (double)NAV_TILE_PX * (double)(1u << zoom);
}

double nav_map_world_y(double lat, uint8_t zoom)
{
    const double r = lat * M_PI / 180.0;
    const double s = log(tan(r) + 1.0 / cos(r));
    return (1.0 - s / M_PI) / 2.0 * (double)NAV_TILE_PX * (double)(1u << zoom);
}

void nav_map_set_view(double lat, double lon, uint8_t zoom, uint16_t heading_deg)
{
    s_view.lat = lat;
    s_view.lon = lon;
    s_view.zoom = zoom;
    s_view.heading_deg = heading_deg;
    s_view.valid = true;
}

void nav_map_set_speed(uint16_t cm_per_s)
{
    s_speed_ms = cm_per_s / 100.0;
}

void nav_map_dead_reckon(uint32_t dt_ms)
{
    if (!s_view.valid || s_speed_ms <= 0.1) return;
    if (s_view.heading_deg > 360) return;      /* unknown heading */
    const double metres = s_speed_ms * (dt_ms / 1000.0);
    const double bearing = s_view.heading_deg * M_PI / 180.0;
    /* Flat-earth step: at a few metres a tick the error is far below a pixel,
     * and the next position from the phone corrects it anyway. */
    const double dlat = metres * cos(bearing) / 111320.0;
    const double dlon = metres * sin(bearing) /
                        (111320.0 * cos(s_view.lat * M_PI / 180.0));
    s_view.lat += dlat;
    s_view.lon += dlon;
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
    const double cx = nav_map_world_x(s_view.lon, s_view.zoom);
    const double cy = nav_map_world_y(s_view.lat, s_view.zoom);
    const double left = cx - NAV_VIEW_W / 2.0;
    const double top  = cy - NAV_VIEW_H / 2.0;
    if (z)  *z  = s_view.zoom;
    if (x0) *x0 = (int64_t)floor(left / NAV_TILE_PX);
    if (x1) *x1 = (int64_t)floor((left + NAV_VIEW_W - 1) / NAV_TILE_PX);
    if (y0) *y0 = (int64_t)floor(top / NAV_TILE_PX);
    if (y1) *y1 = (int64_t)floor((top + NAV_VIEW_H - 1) / NAV_TILE_PX);
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

int nav_map_render(uint16_t *dst, int w, int h, int *out_wanted)
{
    if (out_wanted) *out_wanted = 0;
    if (!dst || w <= 0 || h <= 0) return 0;
    if (!s_view.valid) {
        fill(dst, w, 0, 0, w, h, BG_COLOUR);
        return 0;
    }

    /* World pixel of the top-left corner of the screen, at the detail zoom. */
    const double cx = nav_map_world_x(s_view.lon, s_view.zoom);
    const double cy = nav_map_world_y(s_view.lat, s_view.zoom);
    const int64_t sl = (int64_t)floor(cx - w / 2.0);
    const int64_t st = (int64_t)floor(cy - h / 2.0);

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
    }

    draw_marker(dst, w, h, w / 2, h / 2);
    if (out_wanted) *out_wanted = wanted;
    return have;
}
