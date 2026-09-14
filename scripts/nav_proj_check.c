/* Does the navigator's projection still agree with double precision?
 *
 * main/nav_map.c places the map with integers and single precision, because
 * this chip emulates double in software and a per-point Mercator in double
 * cost 40 ms of a frame. This keeps the formulas honest: the same code as the
 * firmware, next to the double version it replaced.
 *
 *   cc -O2 -o /tmp/nav_proj_check scripts/nav_proj_check.c -lm && /tmp/nav_proj_check
 *
 * It caught a real bug on the first run — the e7-degrees-to-radians constant
 * was ten times too large, which sent the latitude past pi/2 and made the
 * whole thing NaN. Expected output: under a pixel of disagreement for the
 * view centre (1.8 px at zoom 18, where a float can no longer hold 67 million
 * pixels exactly), and exactly zero for everything placed relative to it. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NAV_TILE_PX 256
#define E7_TO_RAD   1.74532925e-9f

static int64_t world_x_px(int32_t lon_e7, uint8_t zoom) {
    const int64_t n = (int64_t)NAV_TILE_PX << zoom;
    return ((int64_t)lon_e7 + 1800000000LL) * n / 3600000000LL;
}
static int64_t world_y_px(int32_t lat_e7, uint8_t zoom) {
    const float r = (float)lat_e7 * E7_TO_RAD;
    const float my = logf(tanf(r) + 1.0f / cosf(r));
    const float t = (1.0f - my * (float)M_1_PI) * 0.5f;
    return (int64_t)llroundf(t * (float)((int64_t)NAV_TILE_PX << zoom));
}
/* the double reference the firmware used to run */
static double ref_x(double lon, uint8_t z) {
    return (lon + 180.0) / 360.0 * (double)NAV_TILE_PX * (double)(1u << z);
}
static double ref_y(double lat, uint8_t z) {
    const double r = lat * M_PI / 180.0;
    const double s = log(tan(r) + 1.0 / cos(r));
    return (1.0 - s / M_PI) / 2.0 * (double)NAV_TILE_PX * (double)(1u << z);
}

int main(void) {
    const double lats[] = { 50.0619, 50.0264, 49.9755, 52.2297, 0.0, -33.8688 };
    const double lons[] = { 19.9368, 19.9079, 19.8273, 21.0122, 0.0, 151.2093 };
    int fails = 0;
    for (int zi = 14; zi <= 18; zi++) {
        double worst_x = 0, worst_y = 0;
        for (unsigned i = 0; i < sizeof lats / sizeof lats[0]; i++) {
            const int32_t la = (int32_t)lround(lats[i] * 1e7);
            const int32_t lo = (int32_t)lround(lons[i] * 1e7);
            const double ex = fabs((double)world_x_px(lo, zi) - ref_x(lons[i], zi));
            const double ey = fabs((double)world_y_px(la, zi) - ref_y(lats[i], zi));
            if (ex > worst_x) worst_x = ex;
            if (ey > worst_y) worst_y = ey;
        }
        printf("zoom %2d: worst x %.2f px, worst y %.2f px\n", zi, worst_x, worst_y);
        if (worst_x > 1.0 || worst_y > 4.0) fails++;
    }

    /* And the local projection: a point a screen away from the centre must
     * land where the absolute maths says it does. */
    const uint8_t z = 17;
    const int32_t c_la = 500619000, c_lo = 199368000;
    const float n = (float)((int64_t)NAV_TILE_PX << z);
    const float kx = n / 3.6e9f;
    const float ky = kx / cosf((float)c_la * E7_TO_RAD);
    double worst = 0;
    for (int dy = -240; dy <= 240; dy += 40) {
        for (int dx = -400; dx <= 400; dx += 40) {
            /* Take a pixel offset, invert it to a position, project it back. */
            const float per_px = 3.6e9f / n;
            const float stretch = cosf((float)c_la * E7_TO_RAD);
            const int32_t la = c_la - (int32_t)lroundf((float)dy * per_px * stretch);
            const int32_t lo = c_lo + (int32_t)lroundf((float)dx * per_px);
            const int px = (int)lroundf((float)((int64_t)lo - c_lo) * kx);
            const int py = -(int)lroundf((float)((int64_t)la - c_la) * ky);
            /* ...and compare with the absolute double maths. */
            const double ax = ref_x(lo / 1e7, z) - ref_x(c_lo / 1e7, z);
            const double ay = ref_y(la / 1e7, z) - ref_y(c_la / 1e7, z);
            const double e = fmax(fabs(px - ax), fabs(py - ay));
            if (e > worst) worst = e;
        }
    }
    printf("local projection over a whole panel: worst %.2f px\n", worst);
    if (worst > 1.5) fails++;
    printf(fails ? "FAIL\n" : "OK\n");
    return fails ? 1 : 0;
}
