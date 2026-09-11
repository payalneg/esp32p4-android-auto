/* Composing the map the phone's tiles describe.
 *
 * The head unit holds the tiles (nav_tiles.c) and the phone says where the
 * rider is; this turns the two into an 800x480 picture. Nothing here talks to
 * the radio or to LVGL — it writes pixels into a buffer the caller owns. */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Where to look, as the phone last said. */
typedef struct {
    double   lat, lon;      /* the rider */
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

void nav_map_set_view(double lat, double lon, uint8_t zoom, uint16_t heading_deg);

/* Rider speed, in centimetres per second, for the dead-reckoning between
 * position updates. */
void nav_map_set_speed(uint16_t cm_per_s);

/* Advance the view by `dt_ms` of travel at the last known speed and heading.
 * Called between the phone's updates — twice a second is a visible step, and
 * the head unit can redraw far more often than that from what it holds. */
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
void nav_map_unproject(int x, int y, int w, int h, double *lat, double *lon);

/* Slippy helpers, exposed for the debug command and the tests. */
double nav_map_world_x(double lon, uint8_t zoom);
double nav_map_world_y(double lat, uint8_t zoom);

#ifdef __cplusplus
}
#endif
