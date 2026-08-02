#include "ota_http.h"

#include "sdkconfig.h"

#if CONFIG_OTA_HTTP_ENABLED

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/sha256.h"

#include "ota_screen.h"
#include "board.h"

static const char *TAG = "ota_http";
static httpd_handle_t s_server;

/* 4 KiB chunk — comfortable for esp_ota_write (must be ≥ 16 bytes) and
 * keeps heap pressure low while the AA pipeline still runs alongside. */
#define OTA_RX_BUF_SIZE 4096

static esp_err_t index_get_handler(httpd_req_t *req)
{
    /* HTML <form enctype=application/octet-stream> is NOT a real value —
     * browsers fall back to x-www-form-urlencoded and send only the
     * filename. So we POST raw bytes via XHR (XHR gives us upload
     * progress events, which fetch() still doesn't). */
    static const char html[] =
        "<!doctype html><html><head><meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>ESP32-P4 OTA</title><style>"
        "*{box-sizing:border-box}"
        "body{margin:0;min-height:100vh;font-family:-apple-system,BlinkMacSystemFont,"
        "'Segoe UI',system-ui,sans-serif;background:#0b0d10;color:#e6e8eb;"
        "display:flex;align-items:center;justify-content:center;padding:1.5em}"
        ".card{width:100%;max-width:32em;background:#14181d;border-radius:14px;"
        "padding:2em;box-shadow:0 8px 32px rgba(0,0,0,.4);"
        "border:1px solid #20262d}"
        "h1{margin:0 0 .2em;font-weight:600;font-size:1.4em}"
        ".sub{color:#8a939c;font-size:.9em;margin-bottom:1.4em}"
        "input[type=file]{display:none}"
        ".file{margin-top:1em;padding:.8em 1em;background:#10141a;"
        "border-radius:8px;display:none;font-size:.9em}"
        ".file.on{display:flex;justify-content:space-between;gap:1em}"
        ".file .nm{color:#e6e8eb;overflow:hidden;text-overflow:ellipsis;"
        "white-space:nowrap}"
        ".file .sz{color:#8a939c;flex-shrink:0}"
        "button{width:100%;margin-top:1em;padding:.85em;border:0;border-radius:8px;"
        "font-weight:600;font-size:1em;cursor:pointer;transition:.15s}"
        ".btn-pick{background:#2a3138;color:#e6e8eb}"
        ".btn-pick:hover{background:#343c44}"
        ".btn-up{background:#4ea1ff;color:#0b0d10}"
        ".btn-up:hover:not(:disabled){background:#6cb3ff}"
        "button:disabled{background:#2a3138;color:#5a6068;cursor:not-allowed}"
        ".bar{margin-top:1em;height:6px;background:#10141a;border-radius:3px;"
        "overflow:hidden;display:none}"
        ".bar.on{display:block}"
        ".bar .fill{height:100%;background:#4ea1ff;width:0;transition:width .1s}"
        ".bar .fill.ok{background:#3dd68c}"
        ".bar .fill.err{background:#ff5e5e}"
        ".status{margin-top:.8em;font-size:.9em;color:#8a939c;min-height:1.2em;"
        "font-variant-numeric:tabular-nums}"
        ".status.ok{color:#3dd68c}"
        ".status.err{color:#ff5e5e}"
        ".foot{margin-top:1.6em;padding-top:1.2em;border-top:1px solid #20262d;"
        "font-size:.82em;color:#6b7480}"
        ".foot a{color:#4ea1ff;text-decoration:none}"
        ".foot a:hover{text-decoration:underline}"
        ".foot details{margin-top:.5em}"
        ".foot summary{cursor:pointer;color:#8a939c}"
        ".foot pre{background:#10141a;padding:.7em;border-radius:6px;"
        "overflow-x:auto;font-size:.85em;color:#b8c0c8;margin:.5em 0 0}"
        "</style></head><body><div class=card>"
        "<h1>ESP32-P4 OTA</h1>"
        "<div class=sub>upload firmware image to the next partition</div>"
        "<input type=file id=f accept='.bin'>"
        "<button id=p class=btn-pick>Select firmware (.bin)</button>"
        "<div class=file id=fi><span class=nm id=fn></span>"
        "<span class=sz id=fs></span></div>"
        "<button id=b class=btn-up disabled>Upload &amp; reboot</button>"
        "<div class=bar id=ba><div class=fill id=bf></div></div>"
        "<div class=status id=st></div>"
        "<div class=foot>"
        "<a href=/info>&#9432; device info</a> &nbsp; "
        "<a href=/files>&#128193; files</a> &nbsp; "
        "<a href=/lisp>&#955; LISP editor</a>"
        "<details><summary>shell upload</summary>"
        "<pre>curl --data-binary @build/esp32p4_android_auto.bin \\\n"
        "  -H 'Content-Type: application/octet-stream' \\\n"
        "  http://&lt;device&gt;/ota</pre></details>"
        "</div></div>"
        "<script>"
        "const $=id=>document.getElementById(id),"
        "f=$('f'),p=$('p'),fi=$('fi'),fn=$('fn'),fs=$('fs'),"
        "b=$('b'),ba=$('ba'),bf=$('bf'),st=$('st');"
        "const fmt=n=>n<1024?n+' B':n<1048576?(n/1024).toFixed(1)+' KiB':"
        "(n/1048576).toFixed(2)+' MiB';"
        "let file=null;"
        "p.onclick=()=>f.click();"
        "f.onchange=()=>{const x=f.files[0];if(!x)return;file=x;"
        "fn.textContent=x.name;fs.textContent=fmt(x.size);"
        "fi.classList.add('on');b.disabled=false;"
        "st.textContent='';st.className='status';"
        "bf.className='fill';bf.style.width='0';ba.classList.remove('on');};"
        "b.onclick=()=>{if(!file)return;"
        "b.disabled=true;p.disabled=true;ba.classList.add('on');"
        "st.className='status';st.textContent='uploading...';"
        "const x=new XMLHttpRequest(),t0=performance.now();"
        "x.upload.onprogress=e=>{if(!e.lengthComputable)return;"
        "const pr=e.loaded/e.total,dt=(performance.now()-t0)/1000,"
        "sp=e.loaded/dt;bf.style.width=(pr*100).toFixed(1)+'%';"
        "st.textContent=fmt(e.loaded)+' / '+fmt(e.total)+"
        "'  ('+fmt(sp)+'/s)';};"
        "x.onload=()=>{const ok=x.status>=200&&x.status<300;"
        "bf.className='fill '+(ok?'ok':'err');"
        "st.className='status '+(ok?'ok':'err');"
        "st.textContent=ok?'OK — rebooting...':"
        "'error '+x.status+': '+x.responseText;"
        "if(!ok){b.disabled=false;p.disabled=false}};"
        "x.onerror=()=>{bf.className='fill err';"
        "st.className='status err';st.textContent='network error';"
        "b.disabled=false;p.disabled=false};"
        "x.open('POST','/ota');"
        "x.setRequestHeader('Content-Type','application/octet-stream');"
        "x.send(file);};"
        "</script></body></html>";
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, sizeof(html) - 1);
}

static esp_err_t info_get_handler(httpd_req_t *req)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next    = esp_ota_get_next_update_partition(NULL);
    esp_app_desc_t desc = {0};
    if (running) esp_ota_get_partition_description(running, &desc);

    char body[384];
    int n = snprintf(body, sizeof(body),
        "running: %s @ 0x%08" PRIx32 " (size %" PRIu32 ")\n"
        "version: %s\n"
        "idf:     %s\n"
        "model:   %s\n"
        "next:    %s @ 0x%08" PRIx32 " (size %" PRIu32 ")\n",
        running ? running->label : "?",
        running ? running->address : 0,
        running ? running->size : 0,
        desc.version, desc.idf_ver,
        BOARD_MODEL_ID,
        next ? next->label : "(none)",
        next ? next->address : 0,
        next ? next->size : 0);
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, body, n);
}

static esp_err_t ota_post_handler(httpd_req_t *req)
{
    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    if (!next) {
        ESP_LOGE(TAG, "no next OTA partition — wrong partition table?");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "no ota partition (rebuild partitions.csv)");
        return ESP_FAIL;
    }

    int remaining = req->content_len;
    if (remaining <= 0) {
        httpd_resp_send_err(req, HTTPD_411_LENGTH_REQUIRED,
                            "Content-Length required");
        return ESP_FAIL;
    }
    if ((uint32_t)remaining > next->size) {
        ESP_LOGE(TAG, "image %d > slot %" PRIu32, remaining, next->size);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "image larger than ota slot");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA begin: %d bytes -> %s @ 0x%08" PRIx32,
             remaining, next->label, next->address);

    /* On-screen progress. Idempotent init in case nothing else has shown
     * the overlay yet. Title is set once at the start of the session. */
    ota_screen_init();
    ota_screen_set_title("Firmware update");
    ota_screen_show("Receiving from browser");
    ota_screen_set_status("");

    /* Stage the full image in PSRAM first, then flush to flash in one
     * concentrated pass. The previous flow interleaved recv -> esp_ota_write
     * every 4 KiB; that meant ~150 seconds of esp_flash_*-induced cache
     * disables while SDIO/WiFi RX was also active. Decoupling RX from
     * flash writes:
     *   - During RX (~150 s): only WiFi / SDIO traffic, no flash bus hits.
     *   - After RX done: ~5-10 s tight loop of esp_ota_write from PSRAM,
     *     no concurrent network or other big bus users.
     * Diagnostic side benefit: if the verify glitch persists even with
     * receive isolated from flash, then SDIO/WiFi isn't the suspect — the
     * problem lives entirely in the final write+verify window. */
    uint8_t *stage = heap_caps_malloc(remaining,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!stage) {
        ESP_LOGE(TAG, "PSRAM staging alloc %d bytes failed", remaining);
        ota_screen_set_status_error("Out of memory");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "psram staging alloc failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "staging buffer allocated in PSRAM @%p", stage);

    /* SHA-256 of the received byte stream — logged at the end so the user
     * can `shasum -a 256 build/esp32p4_android_auto.bin` locally and verify
     * whether bytes arrived intact (TCP/HTTP) or got mangled on the wire. */
    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts(&sha, 0);

    int total = req->content_len;
    int written = 0;
    int next_log_at = 0x40000;  /* log every 256 KiB */
    int next_ui_at  = 0;        /* refresh progress every ~64 KiB */
    while (remaining > 0) {
        int want = remaining < OTA_RX_BUF_SIZE ? remaining : OTA_RX_BUF_SIZE;
        int n = httpd_req_recv(req, (char *)(stage + written), want);
        if (n == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (n <= 0) {
            ESP_LOGE(TAG, "recv error %d after %d/%d bytes",
                     n, written, written + remaining);
            ota_screen_set_status_error("Network error");
            heap_caps_free(stage);
            mbedtls_sha256_free(&sha);
            return ESP_FAIL;
        }
        mbedtls_sha256_update(&sha, stage + written, n);
        written   += n;
        remaining -= n;
        if (written >= next_log_at) {
            ESP_LOGI(TAG, "...%d bytes received", written);
            next_log_at += 0x40000;
        }
        if (written >= next_ui_at) {
            ota_screen_set_progress(written, total);
            next_ui_at += 0x10000;  /* 64 KiB granularity is plenty smooth */
        }
    }
    ota_screen_set_progress(total, total);

    unsigned char digest[32];
    mbedtls_sha256_finish(&sha, digest);
    mbedtls_sha256_free(&sha);
    char hex[65];
    for (int i = 0; i < 32; i++) {
        snprintf(hex + i * 2, 3, "%02x", digest[i]);
    }
    ESP_LOGI(TAG, "received %d / %d bytes, sha256=%s", written, total, hex);

    /* Flush staged image to flash in one concentrated pass — no
     * concurrent network or other big bus users at this point. The flash
     * erase / program windows briefly stall the MIPI-DSI DMA fetching the
     * PSRAM framebuffer, so the panel may flash cyan/blue while this runs.
     * Warn the user so it doesn't look like a defect. */
    ota_screen_show("Erasing flash - screen may flicker");
    ota_screen_set_status("Don't power off");
    ota_screen_set_progress(0, written);

    esp_ota_handle_t handle = 0;
    esp_err_t err = esp_ota_begin(next, written, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin: %s", esp_err_to_name(err));
        ota_screen_set_status_error("Erase failed");
        heap_caps_free(stage);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "esp_ota_begin failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "flushing %d bytes to flash...", written);
    ota_screen_show("Writing firmware - screen may flicker");
    int64_t t_flush_us = esp_timer_get_time();
    int next_write_ui_at = 0;
    for (int off = 0; off < written; ) {
        int chunk = (written - off) < OTA_RX_BUF_SIZE
                    ? (written - off) : OTA_RX_BUF_SIZE;
        err = esp_ota_write(handle, stage + off, chunk);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write @%d: %s", off, esp_err_to_name(err));
            ota_screen_set_status_error("Write failed");
            esp_ota_abort(handle);
            heap_caps_free(stage);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                "flash write failed");
            return ESP_FAIL;
        }
        off += chunk;
        if (off >= next_write_ui_at) {
            ota_screen_set_progress(off, written);
            next_write_ui_at += 0x40000;  /* 256 KiB cadence — write is fast */
        }
    }
    heap_caps_free(stage);
    ota_screen_set_progress(written, written);
    ESP_LOGI(TAG, "flush done in %lld ms",
             (esp_timer_get_time() - t_flush_us) / 1000);

    ota_screen_show("Verifying");
    err = esp_ota_end(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end: %s", esp_err_to_name(err));
        ota_screen_set_status_error("Verify failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "image verify failed");
        return ESP_FAIL;
    }
    err = esp_ota_set_boot_partition(next);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_boot_partition: %s", esp_err_to_name(err));
        ota_screen_set_status_error("Commit failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "set_boot_partition failed");
        return ESP_FAIL;
    }

    ota_screen_show("Rebooting...");
    ota_screen_set_status("");
    ESP_LOGW(TAG, "OTA OK (%d bytes) -> %s, rebooting in 500 ms",
             written, next->label);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "OK rebooting\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;  /* unreachable */
}

esp_err_t ota_http_start(void)
{
    ESP_LOGI(TAG, "ota_http_start() called");
    if (s_server) {
        ESP_LOGI(TAG, "already running, skipping");
        return ESP_OK;
    }

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port       = CONFIG_OTA_HTTP_PORT;
    cfg.recv_wait_timeout = 30;
    cfg.send_wait_timeout = 30;
    cfg.lru_purge_enable  = true;
    cfg.stack_size        = 8192;
    /* OTA registers 4 handlers; the web file manager (files_http_register)
     * adds 7 and the web LISP editor (lisp_http_register) 10 more → bump
     * well past the default 8. */
    cfg.max_uri_handlers  = 24;

    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start: %s", esp_err_to_name(err));
        s_server = NULL;
        return err;
    }

    /* Serve the upload page on both "/" and "/ota" so QR codes that
     * point straight at /ota land on the form instead of a 405. */
    static const httpd_uri_t ix = {
        .uri = "/",     .method = HTTP_GET,  .handler = index_get_handler,
    };
    static const httpd_uri_t ix_ota = {
        .uri = "/ota",  .method = HTTP_GET,  .handler = index_get_handler,
    };
    static const httpd_uri_t in = {
        .uri = "/info", .method = HTTP_GET,  .handler = info_get_handler,
    };
    static const httpd_uri_t up = {
        .uri = "/ota",  .method = HTTP_POST, .handler = ota_post_handler,
    };
    httpd_register_uri_handler(s_server, &ix);
    httpd_register_uri_handler(s_server, &ix_ota);
    httpd_register_uri_handler(s_server, &in);
    httpd_register_uri_handler(s_server, &up);

    ESP_LOGI(TAG, "OTA HTTP server on :%d", cfg.server_port);
    return ESP_OK;
}

httpd_handle_t ota_http_get_server(void) { return s_server; }

#else  /* !CONFIG_OTA_HTTP_ENABLED */

esp_err_t ota_http_start(void) { return ESP_OK; }
httpd_handle_t ota_http_get_server(void) { return NULL; }

#endif
