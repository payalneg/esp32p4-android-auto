#pragma once
/* Host shim: the handful of FreeRTOS names the shared UI sources touch. They
 * only ever use them for delays and tick arithmetic. */
#include <stdint.h>
#include <unistd.h>

typedef uint32_t TickType_t;
typedef void *   TaskHandle_t;
typedef void *   SemaphoreHandle_t;

#define portTICK_PERIOD_MS  1u
#define configTICK_RATE_HZ  1000u
#define pdMS_TO_TICKS(ms)   ((TickType_t)(ms))
#define pdTRUE              1
#define pdFALSE             0
#define portMAX_DELAY       0xFFFFFFFFu

/* Critical sections guard snapshots the UI reads from another task on the
 * device. The simulator is single-threaded for this purpose, so they are
 * no-ops rather than a real lock. */
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(mux)  do { (void)(mux); } while (0)
#define portEXIT_CRITICAL(mux)   do { (void)(mux); } while (0)
