/*
 * Copyright 2024-2025 NXP
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
 * accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
 * terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef CONNECT_UTILS_H
#define CONNECT_UTILS_H

#include <curl/curl.h>

/*
   * @param   {char*} ws_url: websocket link eg: ws://127.0.0.1:41000.
   * @returns {CURL* } the pointer of the curl connection.
*/
CURL * websocket_connect(const char *ws_url);

/*
   * @param   {CURL *} ws_curl: the pointer of the curl connection.
   * @param   {char *} parm: the request data with json string.
   * @returns {char *} the websocket response data with json string.
*/
char *websocket_request(CURL *ws_curl, char *parm);

/*
   * for close the websocket connection.
   * @param  {CURL *} ws_curl: the pointer of the curl connection.
*/
void websocket_close(CURL *ws_curl);

/*
   * for http request function.
   * @param   {const char *} url: http link eg: http://127.0.0.1:41000.
   * @param   {const char *} parm: http post request parameters.
*/
char *http_request(const char *url, const char *parm);

#endif
