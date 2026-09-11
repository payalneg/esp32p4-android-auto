/* The route the phone is guiding along, and the turn it is guiding towards.
 *
 * The phone owns the routing; the head unit only needs the shape of the line
 * to draw and a few numbers to show. Both arrive once per route (the line) or
 * whenever they change (the numbers), so neither costs anything to keep. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Points in a route line we will hold. A city crossing simplified for
 * drawing is a few hundred; 2000 x 8 bytes is 16 KB of PSRAM. */
#define NAV_ROUTE_MAX_POINTS 2000

/* Must mirror ManeuverType in flutter-application/lib/nav/maneuvers.dart. */
typedef enum {
    NAV_TURN_STRAIGHT = 0,
    NAV_TURN_SLIGHT_LEFT,
    NAV_TURN_SLIGHT_RIGHT,
    NAV_TURN_LEFT,
    NAV_TURN_RIGHT,
    NAV_TURN_SHARP_LEFT,
    NAV_TURN_SHARP_RIGHT,
    NAV_TURN_UTURN,
    NAV_TURN_ARRIVE,
    NAV_TURN_COUNT,
} nav_turn_t;

/* What to show about the next turn and the rest of the ride. */
typedef struct {
    nav_turn_t turn;
    uint16_t   dist_m;        /* to the turn */
    uint32_t   remaining_m;   /* to the destination */
    uint16_t   remaining_s;
    bool       off_route;
    bool       valid;
} nav_guide_t;

/* Replace the line. Points are lat/lon in 1e7 fixed point, as they arrive on
 * the wire. Worker task only. */
bool nav_route_set(const uint8_t *points, size_t bytes);
void nav_route_clear(void);
size_t nav_route_count(void);

void nav_route_set_guide(const nav_guide_t *g);
void nav_route_get_guide(nav_guide_t *out);

/* Draw the line into a composed view. `zoom` and the screen origin in world
 * pixels are what nav_map used to place the tiles, so the line lands exactly
 * on the roads underneath. */
void nav_route_draw(uint16_t *dst, int w, int h, int64_t sl, int64_t st,
                    uint8_t zoom);

#ifdef __cplusplus
}
#endif
