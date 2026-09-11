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

void nav_map_set_view(double lat, double lon, uint8_t zoom, uint16_t heading_deg);
void nav_map_get_view(nav_map_view_t *out);

/* Compose the current view into `dst` (w x h, RGB565). Tiles that have not
 * arrived leave their patch in the background colour, so a half-filled map
 * draws rather than waiting. Returns how many of the tiles it wanted were
 * actually there. */
int nav_map_render(uint16_t *dst, int w, int h, int *out_wanted);

/* Slippy helpers, exposed for the debug command and the tests. */
double nav_map_world_x(double lon, uint8_t zoom);
double nav_map_world_y(double lat, uint8_t zoom);

#ifdef __cplusplus
}
#endif
