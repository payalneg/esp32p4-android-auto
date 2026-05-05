/*
 * Copyright 2024-2025 NXP
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
 * accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
 * terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <time.h>
#include <errno.h>
#include <string.h>
#include "freemaster_client.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "external_data_init.h"
#define CALL_RETRY_COUNT 5
#define DEFAULT_STRING "NULL"

CURL *ws_connect = NULL;
lv_obj_t *gg_prompt = NULL;
extern pthread_mutex_t lvgl_mutex;
extern pthread_mutex_t jsonrpc_mutex;
static uint32_t retry_count = 0;


// Add connection status check and rebuild mechanism
static bool is_connection_valid(CURL *conn) {
    if (conn == NULL) return false;
    
    // Can add more connection health checks
    return true;
}

static CURL* ensure_connection() {
    pthread_mutex_lock(&jsonrpc_mutex);
    
    if (!is_connection_valid(ws_connect)) {
        if (ws_connect != NULL) {
            curl_easy_cleanup(ws_connect);
            ws_connect = NULL;
        }
        
        // Retry connection, maximum 3 times
        for (int i = 0; i < 3 && ws_connect == NULL; i++) {
            ws_connect = websocket_connect(freemaster_server);
            if (ws_connect == NULL) {
                gg_nanosleep(100000000); // 100ms
            }
        }
    }
    
    CURL *result = ws_connect;
    pthread_mutex_unlock(&jsonrpc_mutex);
    return result;
}


typedef struct {
    readVariableParm *param;
    char **data;
    uint64_t timestamp;
    bool valid;
} cached_widget_data_t;

#define MAX_CACHED_DATA 32
static cached_widget_data_t data_cache[MAX_CACHED_DATA];
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static int cache_index = 0;

// Release the memory occupied by the string array
void free_string_array(char **array, int count)
{
    for (int i = 0; i < count; i++)
    {
        free(array[i]);
    }
    free(array);
}

static void update_widget_data(readVariableParm *user_parm, char **dataArray)
{
    switch (user_parm->widget_type) {
        case GG_LABEL:
            lv_label_set_text(user_parm->parentObj, dataArray[0]);
            break;
        case GG_CHART:
            {
                lv_chart_series_t *childObj = NULL;
                for (int i = 0; i < user_parm->arrayLen; i++) {
                    childObj = (user_parm->childObjArray)[i];
                    lv_chart_set_next_value(user_parm->parentObj, childObj, atoi(dataArray[i]));
                }
                lv_chart_refresh(user_parm->parentObj);
            }
            break;
        case GG_BAR:
            lv_bar_set_value(user_parm->parentObj, atoi(dataArray[0]), LV_ANIM_OFF);
            break;
        case GG_METER:
            {
                lv_meter_indicator_t *childObj = NULL;
                for (int i = 0; i < user_parm->arrayLen; i++) {
                    childObj = (user_parm->childObjArray)[i];
                    lv_meter_set_indicator_value(user_parm->parentObj, childObj, atoi(dataArray[i]));
                }
            }
            break;
        case GG_ARC:
            lv_arc_set_value(user_parm->parentObj, atoi(dataArray[0]));
            break;
        case GG_SLIDER:
            lv_slider_set_value(user_parm->parentObj, atoi(dataArray[0]), LV_ANIM_OFF);
            break;
        case GG_SWITCH:
            {
                int value = atoi(dataArray[0]);
                if (value == 0 && lv_obj_has_state(user_parm->parentObj, LV_STATE_CHECKED)) {
                    lv_obj_clear_state(user_parm->parentObj, LV_STATE_CHECKED);
                } else if (value == 1 && !lv_obj_has_state(user_parm->parentObj, LV_STATE_CHECKED)) {
                    lv_obj_add_state(user_parm->parentObj, LV_STATE_CHECKED);
                }
            }
            break;
        default:
            break;
    }
}

static void cache_widget_data(readVariableParm *user_parm, char **dataArray)
{
    pthread_mutex_lock(&cache_mutex);
    
    // Check if cache for this widget already exists
    int target_index = -1;
    for (int i = 0; i < MAX_CACHED_DATA; i++) {
        if (data_cache[i].valid && data_cache[i].param == user_parm) {
            target_index = i;
            break;
        }
    }
    
    // If not found, use a new cache slot
    if (target_index == -1) {
        for (int i = 0; i < MAX_CACHED_DATA; i++) {
            if (!data_cache[i].valid) {
                target_index = i;
                break;
            }
        }
    }
    
    // If still not found, use circular index
    if (target_index == -1) {
        target_index = cache_index;
        cache_index = (cache_index + 1) % MAX_CACHED_DATA;
        
        // Clean up old data
        if (data_cache[target_index].valid) {
            free_string_array(data_cache[target_index].data, 
                            data_cache[target_index].param->arrayLen);
        }
    }
    
    // Copy data to cache
    char **cached_data = malloc(user_parm->arrayLen * sizeof(char*));
    if (cached_data) {
        for (int i = 0; i < user_parm->arrayLen; i++) {
            cached_data[i] = strdup(dataArray[i]);
        }
        
        data_cache[target_index].param = user_parm;
        data_cache[target_index].data = cached_data;
        data_cache[target_index].timestamp = gg_get_ms_time();
        data_cache[target_index].valid = true;
    }
    
    pthread_mutex_unlock(&cache_mutex);
}

// Function to process cached data, called in LVGL main loop
void process_cached_widget_data(void)
{
    if (pthread_mutex_trylock(&lvgl_mutex) != 0) {
        return; // LVGL is busy, try again later
    }
    
    pthread_mutex_lock(&cache_mutex);
    
    uint64_t current_time = gg_get_ms_time();
    
    for (int i = 0; i < MAX_CACHED_DATA; i++) {
        if (data_cache[i].valid) {
            // Check if data is expired (over 5 seconds)
            if (current_time - data_cache[i].timestamp > 5000) {
                free_string_array(data_cache[i].data, data_cache[i].param->arrayLen);
                data_cache[i].valid = false;
                continue;
            }
            
            // Update UI
            update_widget_data(data_cache[i].param, data_cache[i].data);
            
            // Clean up processed data
            free_string_array(data_cache[i].data, data_cache[i].param->arrayLen);
            data_cache[i].valid = false;
        }
    }
    
    pthread_mutex_unlock(&cache_mutex);
    pthread_mutex_unlock(&lvgl_mutex);
}

void connect_init()
{
    ws_connect = websocket_connect(freemaster_server);
    if (ws_connect == NULL)
    {
        prompt_display("websocket connect failed.");
        return;
    }
    return;
}

void freemaster_disconnect()
{
    if (ws_connect != NULL)
    {
        websocket_close(ws_connect);
        ws_connect = NULL;
    }
    return;
}

bool equal_to_double_max(double a)
{
    /* set the threshold of double type */
    const double epsilon = 1e-9;
    return fabs(fabs(a) - DBL_MAX) / DBL_MAX < epsilon;
}


void  freeMasterParse(void *param)
{
    if (strcmp(((readVariableParm *)param)->apiName, "ReadVariable") != 0) {
        return;
    }
    readVariableParm *user_parm = param;
    char **dataArray = read_variable(user_parm->varArray, user_parm->arrayLen);
    if (dataArray == NULL) {
        fprintf(stderr, "Failed to read variables, skipping UI update\n");
        return;
    }
    bool all_default = true;
    for (int i = 0; i < user_parm->arrayLen; i++) {
        if (strcmp(dataArray[i], DEFAULT_STRING) != 0) {
            all_default = false;
            break;
        }
    }
    if (all_default) {
        fprintf(stderr, "All variables returned default values, skipping UI update\n");
        free_string_array(dataArray, user_parm->arrayLen);
        return;
    }

    int err = pthread_mutex_trylock(&lvgl_mutex);

    if(err == 0)
    {
        update_widget_data(user_parm, dataArray);
        pthread_mutex_unlock(&lvgl_mutex);
    } else if (err == EBUSY) {
        cache_widget_data(user_parm, dataArray);
    } else {
        fprintf(stderr, "Failed to acquire LVGL mutex, data discarded\n");
    }
    free_string_array(dataArray, user_parm->arrayLen);
}

void prompt_display(char *message)
{
    if (gg_prompt == NULL || !lv_obj_is_valid(gg_prompt))
    {
        gg_prompt = lv_label_create(lv_layer_top()); /* create the prompt label on the top layer */
        lv_label_set_text(gg_prompt, message);
        lv_obj_set_pos(gg_prompt, 0, 0);                                                 /* set the prompt label position */
        lv_obj_set_size(gg_prompt, lv_disp_get_hor_res(NULL), 30);                       /* set the prompt label size */
        lv_label_set_long_mode(gg_prompt, LV_LABEL_LONG_SCROLL);                         /* set the label text long mode */
        lv_color_t red = lv_color_hex(0xff0027); 
        lv_obj_set_style_border_width(gg_prompt, 1, LV_PART_MAIN|LV_STATE_DEFAULT);      /* define the font color as yellow */
        lv_obj_set_style_text_color(gg_prompt, red, LV_PART_MAIN | LV_STATE_DEFAULT);    /* set the label font color */
        lv_obj_set_style_radius(gg_prompt, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    }
}

json_t *callAPI(json_t* params, char *method_name)
{
    static uint32_t id = 1;
    json_error_t error;
    json_t *json_obj = NULL;
    json_t *request_data = NULL;
    char *param_str = NULL;
    char *origin_response = NULL;
    
    // Ensure connection is available
    CURL *current_connection = ensure_connection();
    if (current_connection == NULL) {
        fprintf(stderr, "Failed to establish WebSocket connection\n");
        return NULL;
    }

    request_data = json_object();
    if (request_data == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&jsonrpc_mutex);
    
    // Build request
    json_object_set_new(request_data, "jsonrpc", json_string("2.0"));
    json_object_set_new(request_data, "id", json_integer(id));
    json_object_set_new(request_data, "method", json_string(method_name));
    json_object_set_new(request_data, "params", params);

    id++;
    if (id == UINT_MAX) {
        id = 1;
    }
    
    pthread_mutex_unlock(&jsonrpc_mutex);

    // Convert to string
    param_str = json_dumps(request_data, JSON_COMPACT);
    if (param_str == NULL) {
        goto cleanup;
    }

    // Send request with retry mechanism
    int api_retry = 0;
    while (api_retry < 2) { // Maximum 2 retries
        origin_response = websocket_request(current_connection, param_str);
        
        if (origin_response != NULL) {
            break; // Successfully got response
        }
        
        // First failure, try to reconnect
        if (api_retry == 0) {
            fprintf(stderr, "API call failed, attempting to reconnect...\n");
            pthread_mutex_lock(&jsonrpc_mutex);
            if (ws_connect != NULL) {
                curl_easy_cleanup(ws_connect);
                ws_connect = NULL;
            }
            pthread_mutex_unlock(&jsonrpc_mutex);
            
            current_connection = ensure_connection();
            if (current_connection == NULL) {
                break;
            }
        }
        api_retry++;
    }

    if (origin_response == NULL) {
        fprintf(stderr, "No data returned from jsonrpc server after retries.\n");
        goto cleanup;
    }

#ifdef DEBUG
    fprintf(stdout, "Decoding json: %s\n", origin_response);
#endif

    json_obj = json_loads(origin_response, 0, &error);
    if (json_obj == NULL) {
        fprintf(stderr, "Failed to decode json: %s\n", error.text);
    }

cleanup:
    if (request_data) json_decref(request_data);
    if (origin_response) free(origin_response);
    if (param_str) free(param_str);

    return json_obj;
}

char **read_variable(fm_var *varArray, int arrayLen)
{
    json_t *result_json;
    char **dataArray = (char **)malloc(arrayLen * sizeof(char *));
    if (dataArray == NULL)
    {
        return NULL; // malloc failed.
    }

    for (int i = 0; i < arrayLen; i++)
    {
        char *variable_name = varArray[i].varName;
        json_t *params_arr = json_array();
        json_array_append_new(params_arr, json_string(variable_name));
        result_json = callAPI(params_arr, "ReadVariable");

        if (result_json == NULL)
        {
            dataArray[i] = strdup(DEFAULT_STRING);
            if(dataArray[i] == NULL)
            {
                continue;
            }
        } else {
            int success = 0, retval = 0, errorCode = 0;
            char *id, *dataFormatted, *errorMessage = NULL;
            int data;
            json_error_t error;
            int res = json_unpack_ex(result_json, &error, 0, "{s:s, s:{s:b, s?F, s:{s:b, s?:s}, s?{s:i, s:s}}}",
                                    "id", &id, "result", "success", &success, "data", &data, "xtra", "retval", &retval,
                                    "formatted", &dataFormatted, "error", "code", &errorCode, "msg", &errorMessage);
            if (res == -1 || !success)
            {
                char * dispaly_error = errorMessage ? errorMessage : error.text;
                retry_count += 1;
                fprintf(stderr, "%s\n", dispaly_error);
                /* retry read the variable when the time over the CALL_RETRY_COUNT will alert the error message. */
                if (retry_count >= CALL_RETRY_COUNT)
                {
                    prompt_display(dispaly_error);
                }
                /* call the api failed will be set the default string */
                dataArray[i] = strdup(DEFAULT_STRING);
                continue;
            }

            if (success && res != -1)
            {
                if (lv_obj_is_valid(gg_prompt))
                {
                    lv_obj_del(gg_prompt);
                    gg_prompt = NULL;
                    retry_count = 0;
                }
            }
            dataArray[i] = strdup(dataFormatted);
            if (dataArray[i] == NULL)
            {
                // If memory allocation fails, release the allocated memory and return NULL.
                for (int j = 0; j < i; j++)
                {
                    free(dataArray[j]);
                }
                free(dataArray);
                return NULL;
            }
            json_decref(result_json);
        }
    }
    return dataArray;
}

void write_variable(char *varName, int value)
{
    json_t *params_arr = json_array();
    json_array_append_new(params_arr, json_string(varName));
    json_array_append_new(params_arr, json_integer(value));
    callAPI(params_arr, "WriteVariable");
    json_decref(params_arr);  /* Automatically release the memory occupied by the object when the reference count becomes 0 */
    return;
}
#endif
