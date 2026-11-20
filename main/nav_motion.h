#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

/* Raw observations are distinct from the moving prediction. VIEW keepalives
 * often repeat one 1 Hz GPS observation at 2 Hz. They must not rewind it. */
typedef struct {
    bool valid;
    int32_t lat, lon;
    uint32_t age_ms;
    float lat_fraction, lon_fraction;
} nav_motion_t;

static inline bool nav_motion_observe(nav_motion_t *m, int32_t lat, int32_t lon,
                                      int32_t *target_lat, int32_t *target_lon)
{
    if (m->valid && m->lat == lat && m->lon == lon) return false;
    m->valid = true;
    m->lat = *target_lat = lat;
    m->lon = *target_lon = lon;
    m->age_ms = 0;
    return true;
}

static inline void nav_motion_step(nav_motion_t *m, int32_t *lat, int32_t *lon,
                                   int32_t *target_lat, int32_t *target_lon,
                                   float speed, uint16_t heading, uint32_t dt_ms)
{
    /* Repeated packets do not extend the prediction of a missing GPS fix. */
    uint32_t travel_ms = m->age_ms < 1500 ? 1500 - m->age_ms : 0;
    if (travel_ms > dt_ms) travel_ms = dt_ms;
    m->age_ms = m->age_ms < 1500 ? m->age_ms + travel_ms : 1500;
    if (speed > 0.1f && heading < 360 && travel_ms) {
        const float bearing = (float)heading * 0.0174532925f;
        const float metres_e7 = speed * ((float)travel_ms * 0.001f) * 89.8320f;
        m->lat_fraction += metres_e7 * cosf(bearing);
        m->lon_fraction += metres_e7 * sinf(bearing) / cosf((float)*lat * 1.74532925e-9f);
        const int32_t dlat = (int32_t)lroundf(m->lat_fraction);
        const int32_t dlon = (int32_t)lroundf(m->lon_fraction);
        m->lat_fraction -= dlat;
        m->lon_fraction -= dlon;
        *lat += dlat; *lon += dlon;
        *target_lat += dlat; *target_lon += dlon;
    }
    const float weight = 1.0f - expf(-(float)dt_ms / 350.0f);
    *lat += (int32_t)lroundf(((int64_t)*target_lat - *lat) * weight);
    *lon += (int32_t)lroundf(((int64_t)*target_lon - *lon) * weight);
}
