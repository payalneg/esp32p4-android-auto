/*
 * Copyright 2024-2025 NXP
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
 * accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
 * terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef FREEMASTER_CLIENT_H
#define FREEMASTER_CLIENT_H

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <jansson.h>
#include "connect_utils.h"
#include "lvgl.h"
#include "gg_external_data.h"
#include <pthread.h>

typedef struct  {
   char * varName;
   char * varType;
} fm_var;

typedef struct  {
   lv_obj_t *screen;
   lv_obj_t *parentObj;
   fm_var * varArray;
   void ** childObjArray;
   int arrayLen;
	int widget_type;
   char *apiName;
} readVariableParm;

/*
* FreeMaster JSONRPC response data example :
* {"jsonrpc": "2.0", "id": "1", "result": {"success": true, "xtra": {"retval": true}, "data": 14}}
*/

/*
   * Process cached widget data for FreeMaster integration.
*/
void process_cached_widget_data(void);

/*
   * for connect the websocket server.
*/
void connect_init();

/*
   * Disconnect the websocket server.
*/
void freemaster_disconnect();

/*
   * @param   {json_t*} params   eg: [ "counter", 1].
   * @param   {char} method_name  FreeMaster JSONRPC api name.
   * @returns {api_result* } the pointer of the type ap_result.
*/
json_t * callAPI(json_t* params, char* method_name);

void freeMasterParse(void *params);

/*
   * Show the freeMaster except info based on the LVGL label widget.
   * @param   {char*} params  For show the message box info.
*/
void prompt_display(char *message);

/*
   * @param   {char **} variable_name the binding variable name array.
   * @param   int arrary length of the binding variable name array.
   * @returns {char **} Array of data, In case of success,
   *          the result array will contain data property of type string representing the read formatted value.
*/
char** read_variable(fm_var *varArray, int arrayLen);
/*
   * @param   {char} variable_name   the binding variable name.
   * @param   {int} value Value to be written.
*/
void write_variable(char *varName, int value);

#endif
