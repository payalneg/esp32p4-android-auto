#include "nav_map.h"
#include "nav_motion.h"
#include "nav_projection.h"

#include "lvgl.h"

#include <math.h>

#include "esp_timer.h"
#include <stdatomic.h>
#include <stdlib.h>
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

/* The panel the view is composed for. Asked of the live display rather than
 * fixed: the same navigator runs on the 800x480 head units and on the 480x480
 * dashboard board, and nav_map_view_tiles decides from this which tiles the
 * phone is asked for — wired to 800 it would have requested a screenful and a
 * half of them on the square panel. nav_map_render is told the size it fills,
 * so it never needed these. */
#define NAV_VIEW_W  ((int)lv_disp_get_hor_res(NULL))
#define NAV_VIEW_H  ((int)lv_disp_get_ver_res(NULL))

static nav_map_view_t s_view;
static float s_speed_ms;
static nav_motion_t s_motion;
static float s_camera_heading;
static bool s_camera_has_heading;
static float s_marker_heading;
static bool s_marker_has_heading;
static int s_heading_override = -1;

/* The rider's choice from the panel's zoom buttons, or 0 while they have not
 * touched them and the phone's zoom is used as it arrives. */
static uint8_t s_zoom_override;

/* What the last route draw cost, microseconds (navstat prints it). */
static uint32_t s_route_us;

/* Labels placed by the last render. Written by the BLE worker, copied by the
 * LVGL tick; a seqlock keeps the reader from seeing a half-updated list
 * without making either side take a mutex on the frame path. */
static nav_map_label_t s_labels[NAV_MAP_LABELS_MAX];
static atomic_uint s_label_seq;
static atomic_int  s_label_count;

/* Where the phone last said we are. The drawn view chases this. */
static int32_t s_target_lat_e7, s_target_lon_e7;

/* The rider dragged the map away from themselves. While this is set the view
 * stops following them: no easing towards their position, no dead reckoning,
 * and the marker is drawn wherever they actually are rather than in the
 * middle — which is the one assumption a centred view let us make. */
static bool s_panned;
static bool    s_have_target;

/* Last composed pose, not last GPS packet. Comparing here also catches zoom,
 * recenter and marker changes while browsing without redrawing a parked map. */
static nav_map_view_t s_drawn_view;
static float s_drawn_camera_heading, s_drawn_marker_heading;
static bool s_drawn_arrow, s_drawn_panned;
static int32_t s_drawn_target_lat, s_drawn_target_lon;

static bool marker_is_arrow(void)
{
    return s_marker_has_heading && s_view.heading_deg < 360 && s_speed_ms > 0.5f;
}

bool nav_map_pose_changed(void)
{
    const bool arrow = marker_is_arrow();
    return s_view.valid != s_drawn_view.valid ||
           s_view.lat_e7 != s_drawn_view.lat_e7 ||
           s_view.lon_e7 != s_drawn_view.lon_e7 ||
           s_view.zoom != s_drawn_view.zoom ||
           nav_map_camera_heading() != s_drawn_camera_heading ||
           arrow != s_drawn_arrow ||
           (arrow && s_marker_heading != s_drawn_marker_heading) ||
           s_panned != s_drawn_panned ||
           (s_panned && (s_target_lat_e7 != s_drawn_target_lat ||
                         s_target_lon_e7 != s_drawn_target_lon));
}

/* Time-based easing stays consistent when a slow frame changes the cadence. */

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
    return nav_world_y_px(lat_e7, zoom);
}

void nav_map_set_view(int32_t lat_e7, int32_t lon_e7, uint8_t zoom,
                     uint16_t heading_deg)
{
    nav_motion_observe(&s_motion, lat_e7, lon_e7,
                       &s_target_lat_e7, &s_target_lon_e7);
    s_have_target = true;
    s_view.zoom = s_zoom_override ? s_zoom_override : zoom;
    /* The cache protects the blurred layers relative to this. */
    nav_tiles_set_detail_zoom(s_view.zoom);
    s_view.heading_deg = heading_deg;
    /* Where the rider is still matters — the marker and the route need it —
     * but it no longer decides what is in the middle of the screen. */
    if (s_panned && s_view.valid) return;
    if (!s_view.valid) {
        /* Nothing drawn yet — start where we are told rather than easing in
         * from the middle of the ocean. */
        s_view.lat_e7 = lat_e7;
        s_view.lon_e7 = lon_e7;
        s_view.valid = true;
        return;
    }
    const int64_t dlat = llabs((int64_t)lat_e7 - s_view.lat_e7);
    const int64_t dlon = llabs((int64_t)lon_e7 - s_view.lon_e7);
    if (dlat > JUMP_LAT_E7 || dlon > JUMP_LON_E7) {
        s_view.lat_e7 = lat_e7;
        s_view.lon_e7 = lon_e7;
    }
}

void nav_map_set_heading_override(int degrees)
{
    if (degrees < -1 || degrees >= 360) degrees = -1;
    __atomic_store_n(&s_heading_override, degrees, __ATOMIC_RELAXED);
}

int nav_map_heading_override(void)
{
    return __atomic_load_n(&s_heading_override, __ATOMIC_RELAXED);
}

float nav_map_camera_heading(void)
{
    const int override = __atomic_load_n(&s_heading_override, __ATOMIC_RELAXED);
    return override >= 0 ? (float)override : s_camera_heading;
}

static void camera_axes(float *c, float *s)
{
    const float angle = nav_map_camera_heading() * DEG_TO_RAD;
    *c = cosf(angle);
    *s = sinf(angle);
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
    camera_axes(&out->cos_heading, &out->sin_heading);
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
    float c, s;
    camera_axes(&c, &s);
    const float dx = (float)(x - w / 2), dy = (float)(y - h / 2);
    if (lon_e7) {
        *lon_e7 = s_view.lon_e7 + (int32_t)lroundf((c * dx - s * dy) * per_px);
    }
    if (lat_e7) {
        const float stretch = cosf((float)s_view.lat_e7 * E7_TO_RAD);
        *lat_e7 = s_view.lat_e7 -
                  (int32_t)lroundf((s * dx + c * dy) * per_px * stretch);
    }
}

void nav_map_set_speed(uint16_t cm_per_s)
{
    if (cm_per_s == 0 && s_speed_ms > 0 && s_motion.valid) {
        s_target_lat_e7 = s_motion.lat;
        s_target_lon_e7 = s_motion.lon;
    }
    s_speed_ms = (float)cm_per_s * 0.01f;
}

void nav_map_dead_reckon(uint32_t dt_ms)
{
    if (!s_view.valid) return;
    /* The marker needs its own continuous bearing, including in north-up
     * and while browsing the map. Always take the short arc through north. */
    if (s_view.heading_deg < 360 && s_speed_ms > 0.5f) {
        const float target = (float)s_view.heading_deg;
        if (!s_marker_has_heading) {
            s_marker_heading = target;
            s_marker_has_heading = true;
        } else {
            const float weight = 1.0f - expf(-(float)dt_ms / 250.0f);
            s_marker_heading += remainderf(target - s_marker_heading, 360.0f) * weight;
            s_marker_heading = fmodf(s_marker_heading + 360.0f, 360.0f);
        }
    }
    if (s_panned) return;

    if (s_speed_ms >= 1.5f && s_view.heading_deg < 360) {
        const float target = (float)s_view.heading_deg;
        if (!s_camera_has_heading) {
            s_camera_heading = target;
            s_camera_has_heading = true;
        } else {
            float delta = remainderf(target - s_camera_heading, 360.0f);
            const float weight = (float)dt_ms / (210.0f + (float)dt_ms);
            s_camera_heading = fabsf(delta) < 1.0f
                ? target : s_camera_heading + delta * weight;
            if (s_camera_heading < 0.0f) s_camera_heading += 360.0f;
            if (s_camera_heading >= 360.0f) s_camera_heading -= 360.0f;
        }
    }

    if (s_have_target) {
        nav_motion_step(&s_motion, &s_view.lat_e7, &s_view.lon_e7,
                        &s_target_lat_e7, &s_target_lon_e7,
                        s_speed_ms, s_view.heading_deg, dt_ms);
    }
}

/* Pixels dragged on the panel, into a move of the view centre.
 *
 * The same local projection unproject uses, run the other way: e7 degrees per
 * pixel at this zoom, stretched by the latitude going north-south. Dragging
 * the map right means looking further west, hence the signs. */
void nav_map_pan(int dx_px, int dy_px)
{
    if (!s_view.valid) return;
    const float per_px = 3.6e9f / (float)((int64_t)NAV_TILE_PX << s_view.zoom);
    const float stretch = cosf((float)s_view.lat_e7 * E7_TO_RAD);
    float c, s;
    camera_axes(&c, &s);
    s_view.lon_e7 -= (int32_t)lroundf((c * dx_px - s * dy_px) * per_px);
    s_view.lat_e7 += (int32_t)lroundf((s * dx_px + c * dy_px) * per_px * stretch);
    /* Past the poles there is no map and the projection stops meaning
     * anything; the date line is fine and wraps on its own. */
    if (s_view.lat_e7 >  850000000) s_view.lat_e7 =  850000000;
    if (s_view.lat_e7 < -850000000) s_view.lat_e7 = -850000000;
    s_panned = true;
}

void nav_map_pan_reset(void)
{
    if (!s_panned) return;
    s_panned = false;
    /* Straight back to the rider rather than easing across a city. */
    if (s_have_target) {
        s_view.lat_e7 = s_target_lat_e7;
        s_view.lon_e7 = s_target_lon_e7;
    }
}

bool nav_map_is_panned(void) { return s_panned; }

void nav_map_get_centre(int32_t *lat_e7, int32_t *lon_e7)
{
    if (lat_e7) *lat_e7 = s_view.lat_e7;
    if (lon_e7) *lon_e7 = s_view.lon_e7;
}

uint32_t nav_map_last_route_us(void) { return s_route_us; }

void nav_map_set_zoom(uint8_t zoom)
{
    if (zoom < NAV_ZOOM_MIN) zoom = NAV_ZOOM_MIN;
    if (zoom > NAV_ZOOM_MAX) zoom = NAV_ZOOM_MAX;
    s_zoom_override = zoom;
    s_view.zoom = zoom;
    nav_tiles_set_detail_zoom(zoom);
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

/* Draw the rider's arrow relative to the camera heading. */
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

static void rotated_extent(int w, int h, float c, float s, int *rx, int *ry)
{
    const float hw = (float)((w + 1) / 2), hh = (float)((h + 1) / 2);
    *rx = (int)ceilf(fabsf(c) * hw + fabsf(s) * hh) + 1;
    *ry = (int)ceilf(fabsf(s) * hw + fabsf(c) * hh) + 1;
}

bool nav_map_view_tiles(uint8_t *z, int64_t *x0, int64_t *x1,
                        int64_t *y0, int64_t *y1)
{
    if (!s_view.valid) return false;
    const int64_t cx = world_x_px(s_view.lon_e7, s_view.zoom);
    const int64_t cy = world_y_px(s_view.lat_e7, s_view.zoom);
    int rx = NAV_VIEW_W / 2, ry = NAV_VIEW_H / 2;
    if (nav_map_camera_heading() != 0.0f) {
        float c, s;
        camera_axes(&c, &s);
        rotated_extent(NAV_VIEW_W, NAV_VIEW_H, c, s, &rx, &ry);
    }
    if (z)  *z  = s_view.zoom;
    if (x0) *x0 = floor_div(cx - rx, NAV_TILE_PX);
    if (x1) *x1 = floor_div(cx + rx - 1, NAV_TILE_PX);
    if (y0) *y0 = floor_div(cy - ry, NAV_TILE_PX);
    if (y1) *y1 = floor_div(cy + ry - 1, NAV_TILE_PX);
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

/* Resolve tile lookups outside the pixel loop. A 128 px cell fits in one
 * tile even at the next finer zoom; all the other levels cover whole cells. */
#define CELL_BITS 7
#define CELL_PX (1 << CELL_BITS)
#define CELL_MASK (CELL_PX - 1)
#define Q_BITS 16
#define Q_ONE (1 << Q_BITS)
#define ROTATE_BLOCK 8

typedef struct {
    const uint16_t *origin;
    uint8_t shift; /* source offset = (cell offset * 2) >> shift */
} sample_cell_t;

static const uint16_t *tile_at(int z, int64_t x, int64_t y)
{
    const int64_t size = (int64_t)1 << z;
    if (y < 0 || y >= size) return NULL;
    return nav_tiles_get((uint8_t)z, (uint32_t)(x & (size - 1)), (uint32_t)y);
}

static sample_cell_t sample_cell(int z, int64_t wx, int64_t wy, const uint16_t *detail)
{
    if (detail) {
        return (sample_cell_t){detail + (wy & 255) * NAV_TILE_PX + (wx & 255), 1};
    }
    const uint16_t *px = tile_at(z + 1, wx >> CELL_BITS, wy >> CELL_BITS);
    if (px) return (sample_cell_t){px, 0};

    static const int fallback[] = {1, NAV_COARSE_DZ, NAV_WIDE_DZ};
    for (unsigned i = 0; i < sizeof fallback / sizeof fallback[0]; i++) {
        const int dz = fallback[i];
        if (dz > z) continue;
        const int64_t x = wx >> dz, y = wy >> dz;
        px = tile_at(z - dz, x >> 8, y >> 8);
        if (px) {
            return (sample_cell_t){px + (y & 255) * NAV_TILE_PX + (x & 255), dz + 1};
        }
    }
    return (sample_cell_t){0};
}

static inline __attribute__((always_inline))
uint16_t sample_pixel(sample_cell_t cell, int32_t xq, int32_t yq)
{
    const int sx = ((xq >> (Q_BITS - 1)) & (CELL_PX * 2 - 1)) >> cell.shift;
    const int sy = ((yq >> (Q_BITS - 1)) & (CELL_PX * 2 - 1)) >> cell.shift;
    return cell.origin[sy * NAV_TILE_PX + sx];
}

/* Keep the pixel loop optimized even in the usual -Og firmware build. */
static int __attribute__((optimize("O2")))
render_rotated(uint16_t *dst, int w, int h, int64_t wx, int64_t wy,
               float angle, int *out_wanted)
{
    const float c = cosf(angle), s = sinf(angle);
    int rx, ry;
    rotated_extent(w, h, c, s, &rx, &ry);
    /* Align to detail tiles so each detail lookup can populate four cells. */
    const int64_t x0 = floor_div(wx - rx, NAV_TILE_PX) * NAV_TILE_PX;
    const int64_t y0 = floor_div(wy - ry, NAV_TILE_PX) * NAV_TILE_PX;
    const int cols = 2 * (int)((wx + rx - x0) / NAV_TILE_PX + 1);
    const int rows = 2 * (int)((wy + ry - y0) / NAV_TILE_PX + 1);
    sample_cell_t cells[rows][cols];
    const int z = s_view.zoom;
    int have = 0, wanted = 0;
    for (int y = 0; y < rows; y += 2) {
        for (int x = 0; x < cols; x += 2) {
            const int64_t tx = x0 + x * CELL_PX, ty = y0 + y * CELL_PX;
            const uint16_t *detail = tile_at(z, tx >> 8, ty >> 8);
            wanted++;
            have += detail != NULL;
            cells[y][x] = sample_cell(z, tx, ty, detail);
            cells[y][x + 1] = sample_cell(z, tx + CELL_PX, ty, detail);
            cells[y + 1][x] = sample_cell(z, tx, ty + CELL_PX, detail);
            cells[y + 1][x + 1] = sample_cell(z, tx + CELL_PX, ty + CELL_PX, detail);
        }
    }

    const int32_t cq = (int32_t)lroundf(c * Q_ONE), sq = (int32_t)lroundf(s * Q_ONE);
    const int32_t xq0 = (int32_t)(wx - x0) * Q_ONE - (w / 2) * cq + (h / 2) * sq + Q_ONE / 2;
    const int32_t yq0 = (int32_t)(wy - y0) * Q_ONE - (w / 2) * sq - (h / 2) * cq + Q_ONE / 2;

    /* Walk small blocks to reuse PSRAM cache lines even near a quarter turn.
     * Most blocks lie in one source cell. Crossing blocks resolve the cell
     * per pixel, without looking up the tile cache again. */
    for (int by = 0; by < h; by += ROTATE_BLOCK) {
        const int bh = h - by < ROTATE_BLOCK ? h - by : ROTATE_BLOCK;
        for (int bx = 0; bx < w; bx += ROTATE_BLOCK) {
            const int bw = w - bx < ROTATE_BLOCK ? w - bx : ROTATE_BLOCK;
            int32_t xq = xq0 + bx * cq - by * sq;
            int32_t yq = yq0 + bx * sq + by * cq;
            const int ax = (bw - 1) * cq, ay = (bw - 1) * sq;
            const int bxq = -(bh - 1) * sq, byq = (bh - 1) * cq;
            const int mx = xq + (ax < 0 ? ax : 0) + (bxq < 0 ? bxq : 0);
            const int my = yq + (ay < 0 ? ay : 0) + (byq < 0 ? byq : 0);
            const int cell_x = mx >> (Q_BITS + CELL_BITS);
            const int cell_y = my >> (Q_BITS + CELL_BITS);
            const bool single = ((mx + abs(ax) + abs(bxq)) >> (Q_BITS + CELL_BITS)) == cell_x &&
                                ((my + abs(ay) + abs(byq)) >> (Q_BITS + CELL_BITS)) == cell_y;
            const sample_cell_t cell = cells[cell_y][cell_x];
            for (int y = 0; y < bh; y++, xq -= sq, yq += cq) {
                uint16_t *row = dst + (size_t)(by + y) * w + bx;
                int32_t xp = xq, yp = yq;
                if (single && cell.origin) {
                    for (int x = 0; x < bw; x++, xp += cq, yp += sq) {
                        row[x] = sample_pixel(cell, xp, yp);
                    }
                } else if (single) {
                    for (int x = 0; x < bw; x++) row[x] = BG_COLOUR;
                } else {
                    for (int x = 0; x < bw; x++, xp += cq, yp += sq) {
                        const sample_cell_t next = cells[yp >> (Q_BITS + CELL_BITS)]
                                                        [xp >> (Q_BITS + CELL_BITS)];
                        row[x] = next.origin ? sample_pixel(next, xp, yp) : BG_COLOUR;
                    }
                }
            }
        }
    }
    if (out_wanted) *out_wanted = wanted;
    return have;
}

static int render_north_up(uint16_t *dst, int w, int h, int64_t sl, int64_t st,
                           int *out_wanted)
{
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
    if (out_wanted) *out_wanted = wanted;
    return have;
}

/* ---- labels ---- */

static void labels_publish(const nav_map_label_t *in, int n)
{
    const uint32_t seq = atomic_load_explicit(&s_label_seq, memory_order_relaxed);
    atomic_store_explicit(&s_label_seq, seq + 1, memory_order_relaxed);
    atomic_thread_fence(memory_order_release);
    if (in && n > 0) {
        if (n > NAV_MAP_LABELS_MAX) n = NAV_MAP_LABELS_MAX;
        memcpy(s_labels, in, (size_t)n * sizeof(s_labels[0]));
    } else {
        n = 0;
    }
    atomic_store_explicit(&s_label_count, n, memory_order_relaxed);
    atomic_store_explicit(&s_label_seq, seq + 2, memory_order_relaxed);
}

int nav_map_copy_labels(nav_map_label_t *out, int max)
{
    if (!out || max <= 0) return 0;
    for (int attempt = 0; attempt < 8; attempt++) {
        const uint32_t before =
            atomic_load_explicit(&s_label_seq, memory_order_acquire);
        if (before & 1u) continue;
        int n = atomic_load_explicit(&s_label_count, memory_order_relaxed);
        if (n > max) n = max;
        if (n > 0) memcpy(out, s_labels, (size_t)n * sizeof(out[0]));
        atomic_thread_fence(memory_order_acquire);
        const uint32_t after =
            atomic_load_explicit(&s_label_seq, memory_order_acquire);
        if (before == after) return n;
    }
    return 0;
}

#define LABEL_CAND_MAX 64
typedef struct {
    nav_map_label_t lbl;
    bool            detail;   /* came from the zoom being drawn */
    int64_t         dist2;    /* from the middle, for tie-breaking */
} label_cand_t;

static void label_add(label_cand_t *cand, int *n, int max,
                      const nav_vector_label_t *src, bool detail,
                      int64_t dx, int64_t dy, float c, float s, int w, int h)
{
    if (!src->name[0] || *n >= max) return;

    const float sx = (float)(w / 2) + (float)dx * c + (float)dy * s;
    const float sy = (float)(h / 2) - (float)dx * s + (float)dy * c;
    /* A label whose anchor is just off screen is not useful, and one twenty
     * kilometres away is a bug in the tile range. */
    if (sx < -64.0f || sx > (float)w + 64.0f ||
        sy < -64.0f || sy > (float)h + 64.0f) {
        return;
    }

    label_cand_t next;
    next.lbl.kind = src->kind;
    next.lbl.rank = src->rank;
    next.lbl.x = (int16_t)lroundf(sx);
    next.lbl.y = (int16_t)lroundf(sy);
    memcpy(next.lbl.name, src->name, sizeof(next.lbl.name));
    next.lbl.name[sizeof(next.lbl.name) - 1] = '\0';
    next.detail = detail;
    next.dist2 = dx * dx + dy * dy;

    /* Same name from a detail tile and a fallback tile: the detail one wins
     * even if the fallback has a lower rank, because its anchor is the one
     * that actually sits on the road. Same detail level: the better rank
     * wins. Either way there is one label per name, not two on top of each
     * other. */
    for (int i = 0; i < *n; i++) {
        if (strcmp(cand[i].lbl.name, src->name) != 0) continue;
        if (cand[i].detail && !detail) return;
        if (!cand[i].detail && detail) {
            cand[i] = next;
            return;
        }
        if (cand[i].lbl.rank <= src->rank) return;
        cand[i] = next;
        return;
    }
    cand[(*n)++] = next;
}

static int label_cmp(const void *a, const void *b)
{
    const label_cand_t *x = a, *y = b;
    if (x->lbl.rank != y->lbl.rank) return (int)x->lbl.rank - (int)y->lbl.rank;
    if (x->detail != y->detail) return x->detail ? -1 : 1;
    return x->dist2 < y->dist2 ? -1 : (x->dist2 > y->dist2 ? 1 : 0);
}

/* Walk the tiles the view covers and keep the most useful names. This runs
 * after the ground has been drawn, so a tile the compositor just fetched has
 * its label array ready in the same slot. */
static void collect_labels(int w, int h, int64_t wx, int64_t wy,
                           float c, float s)
{
    label_cand_t cand[LABEL_CAND_MAX];
    int ncand = 0;

    int rx, ry;
    rotated_extent(w, h, c, s, &rx, &ry);
    const uint8_t z = s_view.zoom;
    const int64_t span = (int64_t)1 << z;

    /* Detail first. */
    const int64_t tx0 = floor_div(wx - rx, NAV_TILE_PX);
    const int64_t tx1 = floor_div(wx + rx - 1, NAV_TILE_PX);
    const int64_t ty0 = floor_div(wy - ry, NAV_TILE_PX);
    const int64_t ty1 = floor_div(wy + ry - 1, NAV_TILE_PX);
    for (int64_t ty = ty0; ty <= ty1; ty++) {
        if (ty < 0 || ty >= span) continue;
        for (int64_t tx = tx0; tx <= tx1; tx++) {
            int64_t mx = tx % span;
            if (mx < 0) mx += span;
            nav_vector_label_t tmp[NAV_TILE_LABELS];
            const uint8_t n = nav_tiles_get_labels(
                z, (uint32_t)mx, (uint32_t)ty, tmp, NAV_TILE_LABELS);
            for (uint8_t i = 0; i < n; i++) {
                const int64_t dx = tx * NAV_TILE_PX + tmp[i].x - wx;
                const int64_t dy = ty * NAV_TILE_PX + tmp[i].y - wy;
                label_add(cand, &ncand, LABEL_CAND_MAX, &tmp[i], true,
                          dx, dy, c, s, w, h);
            }
        }
    }

    /* Then the blurred layers: enough names to keep a half-filled map from
     * looking anonymous, with detail winning any duplicate. */
    static const int k_dz[] = { NAV_COARSE_DZ, NAV_WIDE_DZ };
    for (unsigned k = 0; k < sizeof k_dz / sizeof k_dz[0]; k++) {
        const int dz = k_dz[k];
        if (dz > z) continue;
        const uint8_t lz = (uint8_t)(z - dz);
        const int64_t lspan = (int64_t)1 << lz;
        const int scale = 1 << dz;
        const int64_t lwx = wx >> dz, lwy = wy >> dz;
        const int lrx = (rx + scale - 1) >> dz;
        const int lry = (ry + scale - 1) >> dz;
        const int64_t lx0 = floor_div(lwx - lrx, NAV_TILE_PX);
        const int64_t lx1 = floor_div(lwx + lrx - 1, NAV_TILE_PX);
        const int64_t ly0 = floor_div(lwy - lry, NAV_TILE_PX);
        const int64_t ly1 = floor_div(lwy + lry - 1, NAV_TILE_PX);
        for (int64_t ty = ly0; ty <= ly1; ty++) {
            if (ty < 0 || ty >= lspan) continue;
            for (int64_t tx = lx0; tx <= lx1; tx++) {
                int64_t mx = tx % lspan;
                if (mx < 0) mx += lspan;
                nav_vector_label_t tmp[NAV_TILE_LABELS];
                const uint8_t n = nav_tiles_get_labels(
                    lz, (uint32_t)mx, (uint32_t)ty, tmp, NAV_TILE_LABELS);
                for (uint8_t i = 0; i < n; i++) {
                    const int64_t dx =
                        (int64_t)(tx * NAV_TILE_PX + tmp[i].x) * scale - wx;
                    const int64_t dy =
                        (int64_t)(ty * NAV_TILE_PX + tmp[i].y) * scale - wy;
                    label_add(cand, &ncand, LABEL_CAND_MAX, &tmp[i], false,
                              dx, dy, c, s, w, h);
                }
            }
        }
    }

    qsort(cand, (size_t)ncand, sizeof(cand[0]), label_cmp);

    /* Place greedily. A label that would sit on top of a better one is
     * dropped; a map with six readable names is better than one with twenty
     * none of which can be read. */
    nav_map_label_t out[NAV_MAP_LABELS_MAX];
    int placed = 0;
    int rect[NAV_MAP_LABELS_MAX][4];
    for (int i = 0; i < ncand && placed < NAV_MAP_LABELS_MAX; i++) {
        const int tw = (int)strlen(cand[i].lbl.name) * 12 + 12;
        const int th = 30;
        if (tw >= w - 8) continue;
        int cx = cand[i].lbl.x;
        int cy = cand[i].lbl.y;
        if (cx < tw / 2 + 4) cx = tw / 2 + 4;
        if (cx > w - tw / 2 - 4) cx = w - tw / 2 - 4;
        if (cy < th / 2 + 4) cy = th / 2 + 4;
        if (cy > h - th / 2 - 4) cy = h - th / 2 - 4;

        const int x0 = cx - tw / 2, x1 = cx + tw / 2;
        const int y0 = cy - th / 2, y1 = cy + th / 2;
        bool clash = false;
        for (int j = 0; j < placed; j++) {
            if (x0 - 2 < rect[j][2] && x1 + 2 > rect[j][0] &&
                y0 - 2 < rect[j][3] && y1 + 2 > rect[j][1]) {
                clash = true;
                break;
            }
        }
        if (clash) continue;
        out[placed] = cand[i].lbl;
        out[placed].x = (int16_t)cx;
        out[placed].y = (int16_t)cy;
        rect[placed][0] = x0;
        rect[placed][1] = y0;
        rect[placed][2] = x1;
        rect[placed][3] = y1;
        placed++;
    }
    labels_publish(out, placed);
}

int nav_map_render(uint16_t *dst, int w, int h, int *out_wanted)
{
    if (out_wanted) *out_wanted = 0;
    if (!dst || w <= 0 || h <= 0) return 0;
    if (!s_view.valid) {
        fill(dst, w, 0, 0, w, h, BG_COLOUR);
        labels_publish(NULL, 0);
        return 0;
    }

    /* Save the pose BEFORE composing. A new phone fix can arrive during the
     * render; retaining that newer pose here would wrongly mark it drawn. */
    const nav_map_view_t drawn_view = s_view;
    const float drawn_marker_heading = s_marker_heading;
    const bool drawn_arrow = marker_is_arrow(), drawn_panned = s_panned;
    const int32_t drawn_target_lat = s_target_lat_e7;
    const int32_t drawn_target_lon = s_target_lon_e7;
    const int64_t wx = world_x_px(s_view.lon_e7, s_view.zoom);
    const int64_t wy = world_y_px(s_view.lat_e7, s_view.zoom);
    const float heading = nav_map_camera_heading();
    const int have = heading == 0.0f
        ? render_north_up(dst, w, h, wx - w / 2, wy - h / 2, out_wanted)
        : render_rotated(dst, w, h, wx, wy, heading * DEG_TO_RAD, out_wanted);

    /* The line goes over the map and under the rider. Timed on its own: it
     * is the one part of the frame whose cost depends on the route rather
     * than on the panel, so it is the first suspect when a frame gets slow. */
    const int64_t t0 = esp_timer_get_time();
    nav_route_draw(dst, w, h);
    s_route_us = (uint32_t)(esp_timer_get_time() - t0);
    /* An arrow while there is a heading to show, a plain dot when standing
     * still — a parked bike pointing somewhere definite is a lie. */
    /* Centred on the rider, the marker belongs in the middle by construction.
     * Dragged away from them it does not, so place it the same way the route
     * line is placed — and let it go off-screen, which is the honest answer
     * when the rider is no longer in view. */
    int mx = w / 2, my = h / 2;
    if (s_panned) {
        nav_map_proj_t proj;
        if (s_have_target && nav_map_get_proj(&proj, w, h)) {
            nav_map_project(&proj, s_target_lat_e7, s_target_lon_e7, &mx, &my);
        } else {
            mx = my = -10000;   /* nothing to draw */
        }
    }
    if (marker_is_arrow()) {
        draw_arrow(dst, w, h, mx, my, s_marker_heading - heading);
    } else {
        draw_marker(dst, w, h, mx, my);
    }

    /* Names go on a layer of their own so they stay upright; this only
     * works out where each one belongs and publishes the list. */
    float hc, hs;
    camera_axes(&hc, &hs);
    collect_labels(w, h, wx, wy, hc, hs);
    s_drawn_view = drawn_view;
    s_drawn_camera_heading = heading;
    s_drawn_marker_heading = drawn_marker_heading;
    s_drawn_arrow = drawn_arrow;
    s_drawn_panned = drawn_panned;
    s_drawn_target_lat = drawn_target_lat;
    s_drawn_target_lon = drawn_target_lon;
    return have;
}
