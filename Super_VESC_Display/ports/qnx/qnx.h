/*
* Copyright 2023 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef _QNX_H
#define _QNX_H

#include <screen/screen.h>
#include <errno.h>
#include "lvgl.h"

void lv_qnx_init(void);
lv_disp_t *lv_qnx_create_window(lv_coord_t hor_res, lv_coord_t ver_res, const char *name);
int lv_qnx_destroy_window(lv_disp_t *disp);

#endif