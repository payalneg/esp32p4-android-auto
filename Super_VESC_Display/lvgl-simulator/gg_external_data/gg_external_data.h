/*
 * Copyright 2024-2025 NXP
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
 * accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
 * terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef GG_EXTERNAL_DATA_H
#define GG_EXTERNAL_DATA_H

#include <stdint.h>
#include "lvgl.h"

typedef void (*gg_edata_task_cb_t)(void * param);

typedef struct _gg_edata_task_t
{
    uint64_t last_time;
    uint32_t period;
    gg_edata_task_cb_t cb;
    void * param;
} gg_edata_task_t;

enum gg_widget_type {
    GG_LABEL = 1,
    GG_CHART,
    GG_BAR,
    GG_METER,
    GG_ARC,
    GG_SLIDER,
    GG_SWITCH
};

/**
 * get time in us
 */
uint64_t gg_get_us_time();

/**
 * get time in ms
 */
uint64_t gg_get_ms_time();

/**
 * sleep in ns
 * @param ns number to sleep.
 */
void gg_nanosleep(int ns);

/**
 * init edata task link
 */
void gg_edata_task_init(void);

/**
 * clear edata task linked list, The list remain valid but become empty.
 */
void gg_edata_task_clear(lv_obj_t * act_scr);

/**
 * Create a new edata task
 * @param period call period in ms unit
 * @param cb a callback which will be called periodically.
 * @param param parameter for callback
 * @return pointer to the new task
 */
gg_edata_task_t * gg_edata_task_create(uint32_t period, gg_edata_task_cb_t cb, void * param);

/**
 * routine which will be executed in a new thread
 */
void *gg_edata_task_exec();

#endif
