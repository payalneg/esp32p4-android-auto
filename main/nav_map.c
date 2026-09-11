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

int nav_map_render(uint16_t *dst, int w, int h, int *out_wanted)
{
    if (out_wanted) *out_wanted = 0;
    if (!dst || w <= 0 || h <= 0) return 0;
    if (!s_view.valid) {
        fill(dst, w, 0, 0, w, h, BG_COLOUR);
        return 0;
    }

    /* World pixel of the top-left corner of the screen. */
    const double cx = nav_map_world_x(s_view.lon, s_view.zoom);
    const double cy = nav_map_world_y(s_view.lat, s_view.zoom);
    const double left = cx - w / 2.0;
    const double top  = cy - h / 2.0;

    const int64_t tile_x0 = (int64_t)floor(left / NAV_TILE_PX);
    const int64_t tile_y0 = (int64_t)floor(top / NAV_TILE_PX);
    const int64_t tile_x1 = (int64_t)floor((left + w - 1) / NAV_TILE_PX);
    const int64_t tile_y1 = (int64_t)floor((top + h - 1) / NAV_TILE_PX);
    const int64_t span = (int64_t)1 << s_view.zoom;

    int have = 0, wanted = 0;
    for (int64_t ty = tile_y0; ty <= tile_y1; ty++) {
        for (int64_t tx = tile_x0; tx <= tile_x1; tx++) {
            /* Where this tile lands on screen. */
            const int dx0 = (int)lround(tx * NAV_TILE_PX - left);
            const int dy0 = (int)lround(ty * NAV_TILE_PX - top);
            int sx0 = 0, sy0 = 0;
            int x0 = dx0, y0 = dy0;
            if (x0 < 0) { sx0 = -x0; x0 = 0; }
            if (y0 < 0) { sy0 = -y0; y0 = 0; }
            int x1 = dx0 + NAV_TILE_PX; if (x1 > w) x1 = w;
            int y1 = dy0 + NAV_TILE_PX; if (y1 > h) y1 = h;
            if (x1 <= x0 || y1 <= y0) continue;

            /* Off the top or bottom of the world, or wrapped round it. */
            const uint16_t *px = NULL;
            if (ty >= 0 && ty < span) {
                int64_t wx = tx % span;
                if (wx < 0) wx += span;
                wanted++;
                px = nav_tiles_get(s_view.zoom, (uint32_t)wx, (uint32_t)ty);
            }
            if (!px) {
                fill(dst, w, x0, y0, x1, y1, BG_COLOUR);
                continue;
            }
            have++;
            const int cols = x1 - x0;
            for (int y = y0; y < y1; y++) {
                memcpy(dst + (size_t)y * w + x0,
                       px + (size_t)(sy0 + (y - y0)) * NAV_TILE_PX + sx0,
                       (size_t)cols * 2);
            }
        }
    }

    draw_marker(dst, w, h, w / 2, h / 2);
    if (out_wanted) *out_wanted = wanted;
    return have;
}
