/*
 * Copyright 2024-2025 NXP
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
 * accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
 * terms, then you may not retain, install, activate or otherwise use the software.
 */

/*********************
 *      INCLUDES
 *********************/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "lv_ll.h"
#include "gg_external_data.h"
#include "freemaster_client.h"
#include <pthread.h>

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
/* Define a global variables */
lv_ll_t GG_EDATA_TASK_LN;

extern pthread_mutex_t gg_edata_ll_mutex;
extern pthread_cond_t gg_edata_ll_cond;
extern volatile int keep_running;

bool gg_edata_ll_changed = false;

uint64_t gg_get_us_time() {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        fprintf(stderr, "\nFailed to get time.");
        exit(EXIT_FAILURE);
    }

    return (uint64_t) (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

uint64_t gg_get_ms_time() {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        fprintf(stderr, "\nFailed to get time.");
        exit(EXIT_FAILURE);
    }

    return (uint64_t) (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void gg_nanosleep(int ns) {
    struct timespec remaining, request = { 0, ns };
    nanosleep(&request, &remaining);
}

void gg_edata_task_init(void) {
    _lv_ll_init(&GG_EDATA_TASK_LN, sizeof(gg_edata_task_t));
}

void gg_edata_task_clear(lv_obj_t * act_scr) {
#ifdef DEBUG
    fprintf(stdout, "External data: clear task list...\n");
#endif
    pthread_mutex_lock(&gg_edata_ll_mutex);
    //_lv_ll_clear(&GG_EDATA_TASK_LN);
    gg_edata_task_t * head;
    head = _lv_ll_get_head(&GG_EDATA_TASK_LN);
    while (head) {
        bool task_deleted = false;
        readVariableParm *param = head->param;
        if (param->screen == act_scr) {
            task_deleted = true;
            gg_edata_ll_changed = true;
            _lv_ll_remove(&GG_EDATA_TASK_LN, head);
        }
        if(task_deleted)
            head = _lv_ll_get_head(&GG_EDATA_TASK_LN);
        else
            head = _lv_ll_get_next(&GG_EDATA_TASK_LN, head);
    }
    pthread_mutex_unlock(&gg_edata_ll_mutex);
#ifdef DEBUG
    fprintf(stdout, "External data: clear task list [Done]\n");
#endif
}

gg_edata_task_t * gg_edata_task_create(uint32_t period, gg_edata_task_cb_t cb, void * param)
{
    pthread_mutex_lock(&gg_edata_ll_mutex);
    gg_edata_task_t * new_task = NULL;
    new_task = _lv_ll_ins_head(&GG_EDATA_TASK_LN);
    if(new_task != NULL) {
        new_task->last_time = 0;
        new_task->period = period;
        new_task->cb = cb;
        new_task->param = param;
        gg_edata_ll_changed = true;
        pthread_cond_signal(&gg_edata_ll_cond);
#ifdef DEBUG
        fprintf(stdout, "External data: create new task.\n");
#endif
    }
    pthread_mutex_unlock(&gg_edata_ll_mutex);
    return new_task;
}

void *gg_edata_task_exec()
{
    gg_edata_task_t * current = NULL;
    uint64_t last_cache_process = 0;
    
    while(keep_running) {
        pthread_mutex_lock(&gg_edata_ll_mutex);
        
        // Check if there are tasks that need to be executed
        bool has_ready_task = false;
        uint64_t currentTime = gg_get_ms_time();
        
        current = _lv_ll_get_head(&GG_EDATA_TASK_LN);
        while (current != NULL && keep_running) {
            if (current->last_time == 0 || 
                currentTime > (current->period + current->last_time)) {
                
                // Limit the number of concurrent executing tasks
                static int active_tasks = 0;
                if (active_tasks < 3) { // Maximum 3 concurrent tasks
                    active_tasks++;
                    current->cb(current->param);
                    current->last_time = currentTime;
                    active_tasks--;
                    has_ready_task = true;
                }
            }
            current = _lv_ll_get_next(&GG_EDATA_TASK_LN, current);
        }
        
        pthread_mutex_unlock(&gg_edata_ll_mutex);
        
        // Process cached data periodically (every 100ms)
        if (currentTime - last_cache_process > 100) {
            process_cached_widget_data();
            last_cache_process = currentTime;
        }
        
        // If no ready tasks, sleep for a longer time
        if (!has_ready_task) {
            gg_nanosleep(10000000); // 10ms
        } else {
            gg_nanosleep(1000000);  // 1ms
        }
    }
    
    return NULL;
}
#endif
