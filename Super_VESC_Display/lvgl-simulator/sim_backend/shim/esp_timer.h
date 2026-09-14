#pragma once
/* Host shim: microsecond clock. */
#include <stdint.h>
#include <sys/time.h>

static inline int64_t esp_timer_get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}
