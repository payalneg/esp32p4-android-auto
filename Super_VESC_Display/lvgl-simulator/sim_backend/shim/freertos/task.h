#pragma once
#include "freertos/FreeRTOS.h"
#include <sys/time.h>

static inline void vTaskDelay(TickType_t ticks) { usleep((useconds_t)ticks * 1000); }

static inline TickType_t xTaskGetTickCount(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (TickType_t)((uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
