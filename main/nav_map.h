/* Composing the map the phone's tiles describe.
 *
 * The head unit holds the tiles (nav_tiles.c) and the phone says where the
 * rider is; this turns the two into an 800x480 picture. Nothing here talks to
 * the radio or to LVGL — it writes pixels into a buffer the caller owns. */
#pragma once

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Where to look, as the phone last said.
 *
 * Position in tenths of a micro-degree — the unit the wire already uses, and
 * about eleven millimetres. Integers on purpose: this is stepped eight times
 * a second by dead reckoning, and it is the accumulation that decides whether
 * the marker glides or grinds. There is also no double-precision hardware on
 * this chip, so a double here is a software routine on the frame path; the
 * projection below is arranged so single precision is enough everywhere it is
 * used. */
typedef struct {
    int32_t  lat_e7, lon_e7;   /* the rider */
    uint8_t  zoom;          /* slippy zoom of the tiles to compose from */
    uint16_t heading_deg;   /* 0..359, 0 = north; 0xFFFF when unknown */
    bool     valid;
} nav_map_view_t;

/* The fallback layers, in levels below the detail zoom.
 *
 * Two of them, because one is not enough. Three levels down, a tile covers
 * sixty-four detail tiles — enough for the blur to bridge the seconds while
 * sharp tiles arrive alongside. Six levels down, a single tile is twenty
 * kilometres across, so four of them blanket a whole city: jump the view to
 * somewhere nothing is cached and there is still a map, coarse as it is.
 *
 * Drawn coarsest first, each overwriting the last where it has tiles, then
 * the detail layer on top. */
#define NAV_COARSE_DZ  3
#define NAV_WIDE_DZ    6

/* What the rider's zoom buttons may reach. Below 14 a town is a smudge at
 * this tile size, above 18 the tiles arrive slower than the ground moves. */
#define NAV_ZOOM_MIN 14
#define NAV_ZOOM_MAX 18

/* Where the phone says the rider is. The view does not jump there: it eases
 * towards it over the following frames, so a position that disagrees with
 * where dead reckoning had got to does not yank the map. */
void nav_map_set_view(int32_t lat_e7, int32_t lon_e7, uint8_t zoom,
                     uint16_t heading_deg);

/* Rider speed, in centimetres per second, for the dead-reckoning between
 * position updates. */
void nav_map_set_speed(uint16_t cm_per_s);

/* How to turn a position near the view into a pixel on it.
 *
 * Mercator is a logarithm and a tangent, and doing it per point cost forty
 * milliseconds a frame. It only has to be done once, for the view's own
 * centre: within a screen of it the projection is linear to well under a
 * pixel, so every other point is two multiplies away. Differences are taken
 * in integers, which is what keeps single precision honest — the absolute
 * coordinates are in the hundreds of millions and a float cannot hold those
 * to the pixel, but a difference of a screen's width it holds exactly. */
typedef struct {
    int32_t lat_e7, lon_e7;   /* the view centre this was built for */
    int     cx, cy;           /* where that centre sits on the panel */
    float   kx, ky;           /* pixels per 1e-7 degree, east and north */
} nav_map_proj_t;

/* Build the projection for the current view on a w x h panel. Returns false
 * when no view has been set. */
bool nav_map_get_proj(nav_map_proj_t *out, int w, int h);

/* Place a position on the panel. Off-screen results are normal and wanted —
 * the caller clips. */
static inline void nav_map_project(const nav_map_proj_t *p,
                                   int32_t lat_e7, int32_t lon_e7,
                                   int *x, int *y)
{
    const float dx = (float)((int64_t)lon_e7 - (int64_t)p->lon_e7);
    const float dy = (float)((int64_t)lat_e7 - (int64_t)p->lat_e7);
    *x = p->cx + (int)lroundf(dx * p->kx);
    *y = p->cy - (int)lroundf(dy * p->ky);   /* north is up */
}

/* The rider's own zoom, from the buttons on the panel. Once set it wins over
 * the zoom the phone sends: the rider is looking at this screen, and the
 * phone is told to send tiles for the level they chose.
 *
 * The picture does not go bare in the meantime. Until tiles for the new level
 * arrive the composer falls back to what it already holds — a level out gets
 * doubled, a level in gets halved — so a zoom is a coarser or softer map for a
 * second or two rather than an empty one. */
void nav_map_set_zoom(uint8_t zoom);

/* What drawing the route cost in the last frame, microseconds. */
uint32_t nav_map_last_route_us(void);

/* Carry the view forward by `dt_ms` of travel at the last known speed and
 * heading, then ease it towards the last position the phone reported.
 *
 * Both halves matter. Dead reckoning alone drifts and then snaps back on
 * every update — which is exactly what a jumping marker looks like. Easing
 * alone lags a whole update behind. Together the map glides and the error
 * bleeds away without a visible step. */
void nav_map_dead_reckon(uint32_t dt_ms);
void nav_map_get_view(nav_map_view_t *out);

/* Compose the current view into `dst` (w x h, RGB565). Tiles that have not
 * arrived leave their patch in the background colour, so a half-filled map
 * draws rather than waiting. Returns how many of the tiles it wanted were
 * actually there. */
int nav_map_render(uint16_t *dst, int w, int h, int *out_wanted);

/* The tile rectangle the current view covers, for the debug command.
 * Returns false when no view has been set yet. */
bool nav_map_view_tiles(uint8_t *z, int64_t *x0, int64_t *x1,
                        int64_t *y0, int64_t *y1);

/* Where a point on the panel is on the ground — the inverse of what
 * nav_map_render does, for picking a destination by tapping the map. */
void nav_map_unproject(int x, int y, int w, int h,
                       int32_t *lat_e7, int32_t *lon_e7);

#ifdef __cplusplus
}
#endif
