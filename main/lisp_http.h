#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/* Attaches the web LISP editor to an already-running httpd instance (the
 * one started by ota_http_start, same as the file manager). Safe to call
 * once; a second call returns ESP_ERR_INVALID_STATE.
 *
 * Endpoints (gated by CONFIG_LISP_HTTP_ENABLED):
 *   GET  /lisp                    the editor page (gzip blob in .rodata)
 *   GET  /lisp/api/state          JSON: job progress, target id, stats, seqs
 *   POST /lisp/api/read           start reading the script off the VESC
 *   GET  /lisp/api/code           text/plain — code from the last read
 *   POST /lisp/api/upload?run=0|1 body = code text, start erase+write
 *   POST /lisp/api/run?run=0|1    start / stop the stored script
 *   POST /lisp/api/repl           body = expression → COMM_LISP_REPL_CMD
 *   GET  /lisp/api/console?since= JSON: script output since that sequence
 *   POST /lisp/api/console/clear  drop the console ring
 *
 * The VESC transfers are asynchronous: the POSTs return 202 immediately and
 * the page polls /lisp/api/state. Blocking inside a handler would stall the
 * whole server — httpd serves every connection from one task and an upload
 * takes tens of seconds. */
esp_err_t lisp_http_register(httpd_handle_t server);
