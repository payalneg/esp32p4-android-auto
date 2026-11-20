#include <assert.h>
#include <stdio.h>
#include "../../main/nav_motion.h"
#include "../../main/nav_projection.h"

#ifdef __TINYC__
/* Windows' old msvcrt exports the double C89 functions only. Round the result
 * back to float for the production single-precision motion arithmetic. */
float sinf(float x) { return (float)sin(x); }
float cosf(float x) { return (float)cos(x); }
float tanf(float x) { return (float)tan(x); }
float logf(float x) { return (float)log(x); }
float expf(float x) { return (float)exp(x); }
#endif

static int64_t legacy_y(int32_t lat, int zoom)
{
    float r = (float)lat * 1.74532925e-9f;
    float my = logf(tanf(r) + 1.0f / cosf(r));
    return (int64_t)llroundf((1.0f - my * 0.3183098861837907f) * 0.5f *
                           (float)((int64_t)256 << zoom));
}

static void ride(bool legacy)
{
    nav_motion_t m = {0};
    const int32_t start = 500619000;
    int32_t lat = start, lon = 199368000, target_lat = lat, target_lon = lon;
    int bad_steps = 0, min_step = 10000;
    for (int t = 0; t < 20000; t += 20) {
        /* A real GPS at 1 Hz; the phone repeats it every 500 ms. */
        if (t % 500 == 0) {
            int32_t fix = start + (int32_t)lround((t / 1000) * 5.0 * 89.832);
            nav_motion_observe(&m, fix, lon, &target_lat, &target_lon);
            if (legacy) target_lat = fix; /* previous unconditional reset */
        }
        int32_t before = lat;
        nav_motion_step(&m, &lat, &lon, &target_lat, &target_lon, 5, 0, 20);
        if (t > 1000) {
            int delta = lat - before;
            if (delta <= 0) bad_steps++;
            if (delta < min_step) min_step = delta;
        }
    }
    printf("GPS 1Hz / VIEW 2Hz %s: backward/stopped steps=%d minimum=%d e7\n",
           legacy ? "before" : "after", bad_steps, min_step);
    assert(legacy ? bad_steps > 0 : bad_steps == 0);
}

static void straight_ride(float speed, uint16_t heading, int frame_ms, int fix_ms)
{
    nav_motion_t m = {0};
    const int32_t start_lat = 500619000, start_lon = 199368000;
    int32_t lat = start_lat, lon = start_lon, target_lat = lat, target_lon = lon;
    const double rad = heading * 0.017453292519943295;
    int next_view = 0;
    for (int t = 0; t < 20000; t += frame_ms) {
        if (t >= next_view) {
            const double fix_second = (t / fix_ms) * fix_ms * .001;
            const int32_t fix_lat = start_lat + (int32_t)lround(fix_second * speed * cos(rad) * 89.832);
            const int32_t fix_lon = start_lon + (int32_t)lround(fix_second * speed * sin(rad) * 89.832 / cos(start_lat * 1.74532925e-9));
            nav_motion_observe(&m, fix_lat, fix_lon, &target_lat, &target_lon);
            next_view += 500;
        }
        int32_t old_lat = lat, old_lon = lon;
        nav_motion_step(&m, &lat, &lon, &target_lat, &target_lon, speed, heading, frame_ms);
        const double forward = (lat-old_lat)*cos(rad) + (lon-old_lon)*sin(rad);
        assert(forward > 0);
    }
    double metres = (lat-start_lat) * cos(rad) / 89.832 +
                    (lon-start_lon) * sin(rad) * cos(start_lat * 1.74532925e-9) / 89.832;
    assert(fabs(metres-speed*20) < speed*.25 + .1);
}

int main(void)
{
    ride(true); ride(false);
    for (int h = 0; h < 360; h += 90) {
        for (int fix_ms = 200; fix_ms <= 1000; fix_ms += fix_ms == 200 ? 300 : 500) {
            straight_ride(1.4f, h, 60, fix_ms);
            straight_ride(5.0f, h, 100, fix_ms);
            straight_ride(15.0f, h, 160, fix_ms);
        }
    }
    int repeats[2] = {0}, longest[2] = {0};
    int64_t previous[2] = {0};
    for (int t = 0; t < 20000; t += 60) {
        int32_t lat = 500619000 + (int32_t)lround(t * .001 * 6.0 * 89.832);
        int64_t y[2] = {legacy_y(lat, 18), nav_world_y_px(lat, 18)};
        for (int i = 0; i < 2; i++) {
            repeats[i] = t && y[i] == previous[i] ? repeats[i] + 1 : 0;
            if (repeats[i] > longest[i]) longest[i] = repeats[i];
            previous[i] = y[i];
        }
    }
    printf("6m/s z18 centre unchanged: before=%dms after=%dms\n", longest[0]*60, longest[1]*60);
    assert(longest[1] <= 1 && longest[0] > longest[1]);
    /* GPS stops, but packets keep arriving with the same old speed. */
    nav_motion_t m = {0}; int32_t lat = 0, lon = 0, target_lat = 0, target_lon = 0;
    nav_motion_observe(&m, 0, 0, &target_lat, &target_lon);
    for (int t = 0; t < 10000; t += 50) {
        nav_motion_observe(&m, 0, 0, &target_lat, &target_lon);
        nav_motion_step(&m, &lat, &lon, &target_lat, &target_lon, 10, 0, 50);
    }
    assert(target_lat >= 1346 && target_lat <= 1348); /* bounded at 15m */
    assert(m.age_ms == 1500);
    puts("PASS: continuous movement, projection, bounded GPS gap");
    return 0;
}
