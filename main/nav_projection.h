#pragma once
#include <stdint.h>
#include <math.h>

/* Just the absolute centre needs double precision. Single precision at
 * 67 million world pixels quantizes a moving centre into 2–4 pixel steps.
 * Per-point projection stays local/single precision in nav_map.h. */
static inline int64_t nav_world_y_px(int32_t lat_e7, uint8_t zoom)
{
    const double r = (double)lat_e7 * (3.14159265358979323846 / 1800000000.0);
    const double my = log(tan(r) + 1.0 / cos(r));
    return (int64_t)llround((1.0 - my / 3.14159265358979323846) * 0.5 *
                          (double)((int64_t)256 << zoom));
}
