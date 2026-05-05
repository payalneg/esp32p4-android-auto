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
#include <limits.h>
#include <errno.h>
#include <poll.h>
#include "lvgl.h"
#include "lv_drivers/wayland/wayland.h"
#include "lv_drivers/display/drm.h"
#include "lv_drivers/indev/evdev.h"
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"
#include <sys/time.h>

lv_ui guider_ui;

#if USE_WAYLAND
struct pollfd pfd;
uint32_t time_till_next;
int sleep_time;

static bool close_cb(lv_disp_t * disp)
{
}
#endif

static void hal_init(void);

int main(void)
{
    /* Initialize LVGL */
    lv_init();

    /* Initialize the HAL (display, input devices) for LVGL */
    hal_init();

    /* Create a GUI-Guider app */
    setup_ui(&guider_ui);
    events_init(&guider_ui);
    custom_init(&guider_ui);

    /* Handle LVGL tasks */
#if USE_WAYLAND
    pfd.fd = lv_wayland_get_fd();
    pfd.events = POLLIN;

    /* Currently, the wayland driver calls lv_timer_handler internaly */
    while(1) {
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        time_till_next = lv_wayland_timer_handler();
        /* Run until the last window closes */
        if (!lv_wayland_window_is_open(NULL)) {
            break;
        }
        /* Wait for something interesting to happen */
        if (time_till_next == LV_NO_TIMER_READY) {
            sleep_time = -1;
        } else if (time_till_next > INT_MAX) {
            sleep_time = INT_MAX;
        } else {
           sleep_time = time_till_next;
        }
        while ((poll(&pfd, 1, sleep_time) < 0) && (errno == EINTR));
    }
#else
    uint32_t idle_time;

    while(1) {
        /* Returns the time to the next timer execution */
        idle_time = lv_timer_handler();
	usleep(idle_time * 1000);
    }
#endif

    return 0;
}

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics library
 */
static void hal_init(void)
{
#if USE_WAYLAND
    lv_wayland_init();

    /* Create a display */
    lv_disp_t * disp = lv_wayland_create_window(LV_HOR_RES_MAX, LV_VER_RES_MAX, "GUI Guider", close_cb);
#elif USE_DRM
    /*DRM init*/
    static lv_color_t buf[2][LV_HOR_RES_MAX * LV_VER_RES_MAX];
    drm_init();

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf[0], buf[1], LV_HOR_RES_MAX * LV_VER_RES_MAX);

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = drm_flush;
    disp_drv.hor_res    = LV_HOR_RES_MAX;
    disp_drv.ver_res    = LV_VER_RES_MAX;
    lv_disp_drv_register(&disp_drv);

#if USE_EVDEV
    /* Linux input device init */
    evdev_init();

    /* Initialize and register a display input driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = evdev_read;
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv);

    LV_IMG_DECLARE(mouse_cursor_icon)
    lv_obj_t * cursor_obj = lv_img_create(lv_scr_act());
    lv_img_set_src(cursor_obj, &mouse_cursor_icon);
    lv_indev_set_cursor(mouse_indev, cursor_obj);
#endif
#else
    printf("please set a display backend, Wayland or DRM.\n");
    return 0;
#endif
}

/* Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR` */
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
