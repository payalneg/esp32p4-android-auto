/*
 * SPDX-License-Identifier: MIT
 * Copyright 2023 NXP
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "lvgl.h"

#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"

#include "qnx.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hal_init(void);
static void * tick_thread(void * data);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

lv_ui guider_ui;



int main(int argc, char ** argv)
{
    (void) argc;    /*Unused*/
    (void) argv;    /*Unused*/

    /*Initialize LittlevGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LittlevGL*/
    hal_init();

    /*Create a GUI-Guider app */
    setup_ui(&guider_ui);
    events_init(&guider_ui);
    custom_init(&guider_ui);

    while(1) {
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        lv_timer_handler();
#if LV_USE_VIDEO
        video_play(&guider_ui);
#endif
        usleep(5*1000);
    }

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
/**
 * Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
 */
static void hal_init(void)
{
    lv_qnx_init();
    /* Create a display */
    lv_disp_t * disp = lv_qnx_create_window(LV_HOR_RES_MAX, LV_VER_RES_MAX, "GUI Guider");
    LV_ASSERT(disp);
    /* Tick init.
     * You have to call 'lv_tick_inc()' in periodically to inform LittelvGL about how much time were elapsed
     */
    pthread_t tid;
    int err = pthread_create(&tid, NULL, &tick_thread, NULL);
    if (err != 0) {
        printf("\ncan't create thread :[%s]", strerror(err));
        exit(1);
    }
}

/**
 * A task to measure the elapsed time for LittlevGL
 * @param data unused
 * @return never return
 */
static void * tick_thread(void * data)
{
    (void)data;

    while(1) {
        usleep(5*1000); /*Sleep for 5 millisecond*/
        lv_tick_inc(5); /*Tell LittelvGL that 5 milliseconds were elapsed*/
    }
    return data;
}
