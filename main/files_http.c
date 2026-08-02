#include "files_http.h"

#include "sdkconfig.h"

#if CONFIG_OTA_HTTP_ENABLED

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bsp/esp-bsp.h"
#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"

#include "app_fs.h"   /* app_fs_base() — the /vescfs LittleFS mount */

static const char *TAG = "files_http";

#define MAX_PATH        384
#define UPLOAD_CHUNK    4096
#define DOWNLOAD_CHUNK  8192
#define JSON_BODY_MAX   4096

/* ---------- URL / path helpers ---------- */

static int hex2int(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* In-place URL decode. Handles %XX and '+' → space. */
static void url_decode(char *s)
{
    char *o = s;
    while (*s) {
        if (*s == '%' && s[1] && s[2]) {
            int a = hex2int((unsigned char)s[1]);
            int b = hex2int((unsigned char)s[2]);
            if (a >= 0 && b >= 0) {
                *o++ = (char)((a << 4) | b);
                s += 3;
                continue;
            }
        }
        if (*s == '+') { *o++ = ' '; s++; continue; }
        *o++ = *s++;
    }
    *o = '\0';
}

/* The browser exposes a synthetic root "/" listing the two drives:
 * "vescfs" (internal LittleFS, app_fs_base()) and "sdcard"
 * (BSP_SD_MOUNT_POINT, when a card is mounted). Below a drive it's plain
 * absolute POSIX paths. */
static const char *vescfs_root(void) { return app_fs_base(); }      /* "/vescfs" */

static bool is_under(const char *path, const char *root)
{
    size_t n = strlen(root);
    return strncmp(path, root, n) == 0 && (path[n] == '\0' || path[n] == '/');
}
static bool is_under_vescfs(const char *p) { return is_under(p, vescfs_root()); }
static bool is_under_sdcard(const char *p) { return is_under(p, BSP_SD_MOUNT_POINT); }

/* microSD mounts lazily (idempotent in the BSP). Mount it whenever a /sdcard
 * path is touched so a card inserted after boot still works. */
static void maybe_mount_sd(const char *p)
{
    if (is_under_sdcard(p)) bsp_sdcard_mount();
}

/* Join with a runtime size arg so -Werror=format-truncation (which fires on
 * compile-time-known sizes) stays quiet; snprintf truncates safely anyway. */
static void join_path(char *out, size_t out_sz, const char *dir, const char *name)
{
    snprintf(out, out_sz, "%s/%s", dir, name);
}

/* True iff path is non-empty, lives under /vescfs or /sdcard, and has no
 * "..", "./" or empty segments. Trims trailing slashes (except a mount root).
 * Strict so a malicious URL can't escape via "/vescfs/../etc/passwd". */
static bool path_safe(char *path)
{
    if (!path || !*path) return false;
    bool u1 = is_under_vescfs(path);
    bool u2 = is_under_sdcard(path);
    if (!u1 && !u2) return false;
    const char *p = path;
    while ((p = strchr(p, '/')) != NULL) {
        p++;
        if (p[0] == '.' && (p[1] == '\0' || p[1] == '/')) return false;
        if (p[0] == '.' && p[1] == '.' &&
            (p[2] == '\0' || p[2] == '/')) return false;
        if (p[0] == '\0') break;  /* trailing slash, no segment */
    }
    size_t rlen = strlen(u1 ? vescfs_root() : BSP_SD_MOUNT_POINT);
    size_t l = strlen(path);
    while (l > rlen && path[l - 1] == '/') {
        path[--l] = '\0';
    }
    return true;
}

static esp_err_t send_json_err(httpd_req_t *req, int code, const char *msg)
{
    const char *status =
        (code == 400) ? "400 Bad Request"   :
        (code == 403) ? "403 Forbidden"     :
        (code == 404) ? "404 Not Found"     :
        (code == 409) ? "409 Conflict"      :
                        "500 Internal Server Error";
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json");
    /* cJSON would be overkill here — escape the message just enough so
     * an errno string can't break the JSON envelope. */
    char body[192];
    int n = snprintf(body, sizeof body, "{\"error\":\"");
    for (const char *s = msg ? msg : ""; *s && n < (int)sizeof body - 8; ++s) {
        if (*s == '"' || *s == '\\') body[n++] = '\\';
        body[n++] = *s;
    }
    n += snprintf(body + n, sizeof body - n, "\"}");
    return httpd_resp_send(req, body, n);
}

/* Reads ?path=... from query string, URL-decodes into `out`, and validates
 * via path_safe(). Returns ESP_OK on success, anything else on parse /
 * safety failure. */
static esp_err_t get_query_path(httpd_req_t *req, char *out, size_t out_sz)
{
    size_t qlen = httpd_req_get_url_query_len(req);
    if (qlen == 0 || qlen > 1023) return ESP_ERR_INVALID_ARG;
    char *q = malloc(qlen + 1);
    if (!q) return ESP_ERR_NO_MEM;
    if (httpd_req_get_url_query_str(req, q, qlen + 1) != ESP_OK) {
        free(q);
        return ESP_FAIL;
    }
    esp_err_t r = httpd_query_key_value(q, "path", out, out_sz);
    free(q);
    if (r != ESP_OK) return r;
    url_decode(out);
    if (!path_safe(out)) return ESP_ERR_INVALID_ARG;
    return ESP_OK;
}

/* Reads up to JSON_BODY_MAX bytes of request body and parses as JSON.
 * Caller frees with cJSON_Delete. NULL on any error. */
static cJSON *read_json_body(httpd_req_t *req)
{
    int len = req->content_len;
    if (len <= 0 || len > JSON_BODY_MAX) return NULL;
    char *buf = malloc(len + 1);
    if (!buf) return NULL;
    int off = 0;
    while (off < len) {
        int n = httpd_req_recv(req, buf + off, len - off);
        if (n == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (n <= 0) { free(buf); return NULL; }
        off += n;
    }
    buf[len] = '\0';
    cJSON *j = cJSON_Parse(buf);
    free(buf);
    return j;
}

/* Extract a required string field from a JSON object into `out`, validate
 * as a safe SD path. Returns ESP_OK on success, ESP_FAIL with the JSON
 * body already wired through send_json_err on failure. */
static esp_err_t json_get_safe_path(httpd_req_t *req, cJSON *j,
                                    const char *field,
                                    char *out, size_t out_sz)
{
    cJSON *v = cJSON_GetObjectItem(j, field);
    if (!cJSON_IsString(v) || !v->valuestring || !v->valuestring[0]) {
        send_json_err(req, 400, "missing field");
        return ESP_FAIL;
    }
    strncpy(out, v->valuestring, out_sz - 1);
    out[out_sz - 1] = '\0';
    if (!path_safe(out)) {
        send_json_err(req, 400, "bad path");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* ---------- GET /files/api/list?path=... ---------- */

static esp_err_t list_handler(httpd_req_t *req)
{
    /* Read ?path= (default to the synthetic root "/"). We don't go through
     * get_query_path() here because that rejects "/" via path_safe. */
    char path[MAX_PATH] = "/";
    size_t qlen = httpd_req_get_url_query_len(req);
    if (qlen > 0 && qlen <= 1023) {
        char *q = malloc(qlen + 1);
        if (q) {
            if (httpd_req_get_url_query_str(req, q, qlen + 1) == ESP_OK &&
                httpd_query_key_value(q, "path", path, sizeof path) == ESP_OK) {
                url_decode(path);
            }
            free(q);
        }
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "path", path);
    cJSON_AddStringToObject(root, "root", "/");
    cJSON *arr = cJSON_AddArrayToObject(root, "entries");

    if (strcmp(path, "/") == 0) {
        /* Synthetic root: the mounted drives as directories. */
        cJSON *ev = cJSON_CreateObject();
        cJSON_AddStringToObject(ev, "name", "vescfs");
        cJSON_AddBoolToObject(ev, "dir", true);
        cJSON_AddNumberToObject(ev, "size", 0);
        cJSON_AddItemToArray(arr, ev);
        esp_err_t sd = bsp_sdcard_mount();
        if (sd == ESP_OK || sd == ESP_ERR_INVALID_STATE) {
            cJSON *es = cJSON_CreateObject();
            cJSON_AddStringToObject(es, "name", "sdcard");
            cJSON_AddBoolToObject(es, "dir", true);
            cJSON_AddNumberToObject(es, "size", 0);
            cJSON_AddItemToArray(arr, es);
        }
    } else {
        if (!path_safe(path)) {
            cJSON_Delete(root);
            return send_json_err(req, 400, "bad path");
        }
        maybe_mount_sd(path);
        DIR *d = opendir(path);
        if (!d) {
            cJSON_Delete(root);
            return send_json_err(req, 404, strerror(errno));
        }
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (de->d_name[0] == '.' &&
                (de->d_name[1] == '\0' ||
                 (de->d_name[1] == '.' && de->d_name[2] == '\0'))) continue;
            char full[MAX_PATH];
            join_path(full, sizeof full, path, de->d_name);
            struct stat st;
            cJSON *e = cJSON_CreateObject();
            cJSON_AddStringToObject(e, "name", de->d_name);
            if (stat(full, &st) == 0) {
                cJSON_AddBoolToObject(e, "dir", S_ISDIR(st.st_mode));
                cJSON_AddNumberToObject(e, "size", (double)st.st_size);
            } else {
                cJSON_AddBoolToObject(e, "dir", false);
                cJSON_AddNumberToObject(e, "size", 0);
            }
            cJSON_AddItemToArray(arr, e);
        }
        closedir(d);
    }

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!out) return send_json_err(req, 500, "json oom");
    httpd_resp_set_type(req, "application/json");
    esp_err_t r = httpd_resp_sendstr(req, out);
    free(out);
    return r;
}

/* ---------- GET /files/api/download?path=... ---------- */

static esp_err_t download_handler(httpd_req_t *req)
{
    char path[MAX_PATH];
    if (get_query_path(req, path, sizeof path) != ESP_OK) {
        return send_json_err(req, 400, "bad path");
    }
    maybe_mount_sd(path);
    struct stat st;
    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        return send_json_err(req, 404, "not a file");
    }
    FILE *f = fopen(path, "rb");
    if (!f) return send_json_err(req, 404, strerror(errno));

    const char *slash = strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;
    char cd[MAX_PATH + 48];   /* big enough that the %s can't truncate (-Werror) */
    /* Quote the filename. We don't try RFC 5987 here — ASCII names are
     * the common case on this device, anything fancier the browser will
     * still save as something sensible from the URL. */
    snprintf(cd, sizeof cd, "attachment; filename=\"%s\"", name);
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Content-Disposition", cd);

    char *buf = malloc(DOWNLOAD_CHUNK);
    if (!buf) { fclose(f); return send_json_err(req, 500, "oom"); }
    size_t n;
    esp_err_t err = ESP_OK;
    while ((n = fread(buf, 1, DOWNLOAD_CHUNK, f)) > 0) {
        if (httpd_resp_send_chunk(req, buf, n) != ESP_OK) {
            err = ESP_FAIL;
            break;
        }
    }
    free(buf);
    fclose(f);
    if (err != ESP_OK) return err;
    return httpd_resp_send_chunk(req, NULL, 0);
}

/* ---------- POST /files/api/upload?path=/sdcard/.../filename ---------- */

static esp_err_t upload_handler(httpd_req_t *req)
{
    char path[MAX_PATH];
    if (get_query_path(req, path, sizeof path) != ESP_OK) {
        return send_json_err(req, 400, "bad path");
    }
    maybe_mount_sd(path);
    /* Refuse to overwrite a directory with a file. Plain file overwrite
     * is allowed — same behaviour as `cp -f`. */
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return send_json_err(req, 409, "path is a directory");
    }
    FILE *f = fopen(path, "wb");
    if (!f) return send_json_err(req, 500, strerror(errno));

    int remaining = req->content_len;
    char *buf = malloc(UPLOAD_CHUNK);
    if (!buf) { fclose(f); unlink(path); return send_json_err(req, 500, "oom"); }
    while (remaining > 0) {
        int want = remaining < UPLOAD_CHUNK ? remaining : UPLOAD_CHUNK;
        int n = httpd_req_recv(req, buf, want);
        if (n == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (n <= 0) {
            free(buf); fclose(f); unlink(path);
            return send_json_err(req, 500, "recv failed");
        }
        if (fwrite(buf, 1, n, f) != (size_t)n) {
            free(buf); fclose(f); unlink(path);
            return send_json_err(req, 500, "write failed");
        }
        remaining -= n;
    }
    free(buf);
    fclose(f);
    ESP_LOGI(TAG, "uploaded %d bytes -> %s", req->content_len, path);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---------- POST /files/api/rename {src, dst} ---------- */

static esp_err_t rename_handler(httpd_req_t *req)
{
    cJSON *j = read_json_body(req);
    if (!j) return send_json_err(req, 400, "bad json");
    char src[MAX_PATH], dst[MAX_PATH];
    if (json_get_safe_path(req, j, "src", src, sizeof src) != ESP_OK ||
        json_get_safe_path(req, j, "dst", dst, sizeof dst) != ESP_OK) {
        cJSON_Delete(j);
        return ESP_FAIL;
    }
    cJSON_Delete(j);
    maybe_mount_sd(src);
    maybe_mount_sd(dst);
    if (rename(src, dst) != 0) {
        ESP_LOGW(TAG, "rename %s -> %s: errno=%d", src, dst, errno);
        return send_json_err(req, 500, strerror(errno));
    }
    ESP_LOGI(TAG, "renamed %s -> %s", src, dst);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---------- POST /files/api/delete {path} ---------- */

static esp_err_t delete_handler(httpd_req_t *req)
{
    cJSON *j = read_json_body(req);
    if (!j) return send_json_err(req, 400, "bad json");
    char path[MAX_PATH];
    if (json_get_safe_path(req, j, "path", path, sizeof path) != ESP_OK) {
        cJSON_Delete(j);
        return ESP_FAIL;
    }
    cJSON_Delete(j);
    if (strcmp(path, BSP_SD_MOUNT_POINT) == 0 ||
        strcmp(path, vescfs_root()) == 0) {
        return send_json_err(req, 403, "refusing to delete a drive root");
    }
    maybe_mount_sd(path);
    struct stat st;
    if (stat(path, &st) != 0) return send_json_err(req, 404, strerror(errno));
    int r = S_ISDIR(st.st_mode) ? rmdir(path) : unlink(path);
    if (r != 0) {
        ESP_LOGW(TAG, "delete %s: errno=%d", path, errno);
        return send_json_err(req, 500, strerror(errno));
    }
    ESP_LOGI(TAG, "deleted %s", path);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---------- POST /files/api/mkdir {path} ---------- */

static esp_err_t mkdir_handler(httpd_req_t *req)
{
    cJSON *j = read_json_body(req);
    if (!j) return send_json_err(req, 400, "bad json");
    char path[MAX_PATH];
    if (json_get_safe_path(req, j, "path", path, sizeof path) != ESP_OK) {
        cJSON_Delete(j);
        return ESP_FAIL;
    }
    cJSON_Delete(j);
    maybe_mount_sd(path);
    if (mkdir(path, 0755) != 0) {
        ESP_LOGW(TAG, "mkdir %s: errno=%d", path, errno);
        return send_json_err(req, 500, strerror(errno));
    }
    ESP_LOGI(TAG, "mkdir %s", path);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---------- GET /files — single-page HTML UI ---------- */

static esp_err_t index_handler(httpd_req_t *req)
{
    /* Vanilla-JS SPA. Calls the /files/api endpoints above, no external
     * deps. Style matches the OTA page so the two feel like one tool. */
    static const char html[] =
        "<!doctype html><html><head><meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>ESP32-P4 Files</title><style>"
        "*{box-sizing:border-box}"
        "body{margin:0;min-height:100vh;font-family:-apple-system,BlinkMacSystemFont,"
        "'Segoe UI',system-ui,sans-serif;background:#0b0d10;color:#e6e8eb;"
        "padding:1em}"
        ".wrap{max-width:60em;margin:0 auto}"
        "h1{margin:.2em 0;font-weight:600;font-size:1.3em}"
        ".bar{display:flex;gap:.5em;flex-wrap:wrap;align-items:center;"
        "margin:.6em 0}"
        ".path{flex:1;min-width:14em;background:#14181d;border:1px solid #20262d;"
        "padding:.5em .7em;border-radius:6px;font-family:ui-monospace,monospace;"
        "font-size:.9em;overflow-x:auto;white-space:nowrap}"
        "button,label.btn{background:#2a3138;color:#e6e8eb;border:0;"
        "padding:.55em .9em;border-radius:6px;cursor:pointer;font-size:.9em;"
        "font-weight:500}"
        "button:hover,label.btn:hover{background:#343c44}"
        "button.pri{background:#4ea1ff;color:#0b0d10}"
        "button.pri:hover{background:#6cb3ff}"
        "button.danger{background:#b00020;color:#fff}"
        "button.danger:hover{background:#cc1133}"
        "button:disabled{opacity:.5;cursor:not-allowed}"
        "input[type=file]{display:none}"
        "table{width:100%;border-collapse:collapse;background:#14181d;"
        "border-radius:8px;overflow:hidden;border:1px solid #20262d;"
        "margin-top:.5em}"
        "th,td{padding:.55em .8em;text-align:left;border-bottom:1px solid #1a1f25}"
        "tr:last-child td{border-bottom:0}"
        "tr.row{cursor:pointer}"
        "tr.row:hover{background:#1a1f25}"
        "th{font-size:.75em;text-transform:uppercase;color:#8a939c;"
        "letter-spacing:.05em;font-weight:600}"
        ".nm{display:flex;align-items:center;gap:.5em}"
        ".ic{width:1.2em;text-align:center;color:#8a939c}"
        ".ic.dir{color:#4ea1ff}"
        ".sz{color:#8a939c;font-variant-numeric:tabular-nums;width:7em}"
        ".ac{text-align:right;width:1px;white-space:nowrap}"
        ".ac button{padding:.3em .6em;font-size:.8em;margin-left:.25em}"
        ".msg{margin-top:.6em;font-size:.9em;color:#8a939c;min-height:1.2em}"
        ".msg.ok{color:#3dd68c}.msg.err{color:#ff5e5e}"
        ".bar2{height:6px;background:#10141a;border-radius:3px;overflow:hidden;"
        "margin-top:.4em;display:none}"
        ".bar2.on{display:block}"
        ".bar2 .fl{height:100%;background:#4ea1ff;width:0;transition:width .1s}"
        ".bar2 .fl.ok{background:#3dd68c}.bar2 .fl.err{background:#ff5e5e}"
        ".modal{position:fixed;inset:0;background:rgba(0,0,0,.6);"
        "display:none;align-items:center;justify-content:center;z-index:10;"
        "padding:1em}"
        ".modal.on{display:flex}"
        ".mbox{background:#14181d;border:1px solid #20262d;border-radius:10px;"
        "padding:1.4em;max-width:32em;width:100%}"
        ".mbox h2{margin:0 0 .5em;font-size:1.1em;font-weight:600}"
        ".mbox p{margin:.4em 0;color:#b8c0c8;word-break:break-all;font-size:.9em}"
        ".mbox input{width:100%;margin-top:.4em;padding:.55em .7em;"
        "background:#10141a;border:1px solid #20262d;border-radius:6px;"
        "color:#e6e8eb;font-size:.95em;font-family:ui-monospace,monospace}"
        ".mbtns{display:flex;gap:.5em;justify-content:flex-end;margin-top:1em}"
        ".tv{background:#10141a;padding:.7em;border-radius:6px;max-height:50vh;"
        "overflow:auto;font-family:ui-monospace,monospace;font-size:.82em;"
        "white-space:pre-wrap;word-break:break-all;color:#b8c0c8}"
        "</style></head><body><div class=wrap>"
        "<h1><a href=/ style=\"color:#8a939c;text-decoration:none;"
        "margin-right:.3em\">\xe2\x86\x90</a>ESP32-P4 Files"
        "<a href=/lisp style=\"float:right;font-size:.6em;color:#4ea1ff;"
        "text-decoration:none;line-height:2.4\">LISP editor \xe2\x86\x92</a></h1>"
        "<div class=bar>"
          "<button id=up>\xe2\x86\x91 Up</button>"
          "<div class=path id=cp></div>"
          "<button id=nd>New folder</button>"
          "<label class=btn for=fu>Upload</label>"
          "<input id=fu type=file multiple>"
        "</div>"
        "<div class=bar2 id=bb><div class=fl id=bf></div></div>"
        "<div class=msg id=ms></div>"
        "<table><thead><tr><th>Name</th><th class=sz>Size</th>"
        "<th class=ac>Actions</th></tr></thead>"
        "<tbody id=tb></tbody></table>"
        "</div>"
        "<div class=modal id=md><div class=mbox>"
          "<h2 id=mt></h2><div id=mc></div>"
          "<div class=mbtns id=mbtns></div>"
        "</div></div>"
        "<script>"
        "const $=id=>document.getElementById(id);"
        "let ROOT='/',cwd='/';"
        "const join=(d,n)=>(d==='/'?'':d)+'/'+n;"
        "const fmt=n=>n<1024?n+' B':n<1048576?(n/1024).toFixed(1)+' KiB':"
        "(n/1048576).toFixed(2)+' MiB';"
        "const setMsg=(t,cls)=>{$('ms').className='msg '+(cls||'');"
        "$('ms').textContent=t||'';};"
        "const esc=s=>s.replace(/[&<>\"']/g,c=>({'&':'&amp;','<':'&lt;',"
        "'>':'&gt;','\"':'&quot;',\"'\":'&#39;'}[c]));"
        "async function jget(u){const r=await fetch(u);"
        "if(!r.ok){let j={};try{j=await r.json()}catch(e){}"
        "throw new Error(j.error||r.status);}return r.json();}"
        "async function jpost(u,b){const r=await fetch(u,{method:'POST',"
        "headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});"
        "if(!r.ok){let j={};try{j=await r.json()}catch(e){}"
        "throw new Error(j.error||r.status);}return r.json();}"
        "async function load(p){setMsg('loading...');"
        "try{const d=await jget('/files/api/list?path='+encodeURIComponent(p));"
        "cwd=d.path;ROOT=d.root;$('cp').textContent=cwd;"
        "$('up').disabled=(cwd===ROOT);"
        "d.entries.sort((a,b)=>a.dir!==b.dir?(a.dir?-1:1):"
        "a.name.localeCompare(b.name,undefined,{sensitivity:'base'}));"
        "const tb=$('tb');tb.innerHTML='';"
        "if(!d.entries.length){tb.innerHTML="
        "'<tr><td colspan=3 style=\"color:#8a939c;padding:1em\">"
        "(empty directory)</td></tr>';}"
        "for(const e of d.entries){"
        "const tr=document.createElement('tr');tr.className='row';"
        "const ic=e.dir?'\xf0\x9f\x93\x81':(isImg(e.name)?"
        "'\xf0\x9f\x96\xbc\xef\xb8\x8f':'\xf0\x9f\x93\x84');"
        "const cls=e.dir?'ic dir':'ic';"
        "tr.innerHTML='<td><div class=nm><span class=\"'+cls+'\">'+ic+"
        "'</span>'+esc(e.name)+'</div></td>"
        "<td class=sz>'+(e.dir?'—':fmt(e.size))+'</td>"
        "<td class=ac></td>';"
        "const full=join(cwd,e.name);"
        "tr.querySelector('.nm').onclick=()=>{"
        "if(e.dir)load(full);else openView(full,e.size);};"
        "const ac=tr.querySelector('.ac');"
        "if(!e.dir){const b=document.createElement('button');b.textContent='\xe2\xac\x87';"
        "b.title='Download';b.onclick=(ev)=>{ev.stopPropagation();"
        "window.location='/files/api/download?path='+encodeURIComponent(full);};"
        "ac.appendChild(b);}"
        "const r=document.createElement('button');r.textContent='\xe2\x9c\x8f';"
        "r.title='Rename';r.onclick=(ev)=>{ev.stopPropagation();renameAsk(full,e.name);};"
        "ac.appendChild(r);"
        "const d2=document.createElement('button');d2.textContent='\xe2\x9c\x95';"
        "d2.title='Delete';d2.className='danger';"
        "d2.onclick=(ev)=>{ev.stopPropagation();delAsk(full,e.name,e.dir);};"
        "ac.appendChild(d2);"
        "tb.appendChild(tr);}"
        "setMsg(d.entries.length+' item'+(d.entries.length===1?'':'s'));"
        "}catch(e){setMsg('list failed: '+e.message,'err');}}"
        "$('up').onclick=()=>{if(cwd===ROOT)return;"
        "const i=cwd.lastIndexOf('/');"
        "load(i<=ROOT.length-1?ROOT:cwd.slice(0,i));};"
        "function modal(title,bodyHtml,buttons){"
        "$('mt').textContent=title;$('mc').innerHTML=bodyHtml;"
        "const mb=$('mbtns');mb.innerHTML='';"
        "for(const b of buttons){const el=document.createElement('button');"
        "el.textContent=b.label;if(b.cls)el.className=b.cls;"
        "el.onclick=()=>b.act();mb.appendChild(el);}"
        "$('md').classList.add('on');}"
        "function closeModal(){$('md').classList.remove('on');}"
        "function delAsk(full,name,isDir){"
        "modal(isDir?'Delete folder?':'Delete file?',"
        "'<p>'+esc(full)+'</p>'+(isDir?"
        "'<p style=\"color:#ff8080\">(must be empty)</p>':''),"
        "[{label:'Cancel',act:closeModal},"
        "{label:'Delete',cls:'danger',act:async()=>{closeModal();"
        "try{await jpost('/files/api/delete',{path:full});"
        "setMsg('deleted','ok');load(cwd);}"
        "catch(e){setMsg('delete failed: '+e.message,'err');}}}]);}"
        "function renameAsk(full,oldName){"
        "modal('Rename / move',"
        "'<p>'+esc(full)+'</p><input id=rn value=\"'+esc(oldName)+'\">"
        "<p style=\"color:#8a939c\">A plain name renames in place; "
        "a full path starting with / moves the entry there.</p>',"
        "[{label:'Cancel',act:closeModal},"
        "{label:'Save',cls:'pri',act:async()=>{"
        "const v=document.getElementById('rn').value.trim();"
        "if(!v){setMsg(\"name can't be empty\",'err');return;}"
        "if(v.indexOf('/')>=0&&v[0]!=='/'){"
        "setMsg('use a plain name, or a full path starting with /','err');return;}"
        "const dst=v[0]==='/'?v:join(cwd,v);closeModal();"
        "try{await jpost('/files/api/rename',{src:full,dst});"
        "setMsg('renamed','ok');load(cwd);}"
        "catch(e){setMsg('rename failed: '+e.message,'err');}}}]);"
        "setTimeout(()=>{const i=document.getElementById('rn');"
        "if(i){i.focus();i.select();}},50);}"
        "$('nd').onclick=()=>{"
        "modal('New folder','<input id=nn placeholder=\"folder name\">',"
        "[{label:'Cancel',act:closeModal},"
        "{label:'Create',cls:'pri',act:async()=>{"
        "const v=document.getElementById('nn').value.trim();"
        "if(!v||v.indexOf('/')>=0){setMsg(\"bad name\",'err');return;}"
        "closeModal();"
        "try{await jpost('/files/api/mkdir',{path:join(cwd,v)});"
        "setMsg('created','ok');load(cwd);}"
        "catch(e){setMsg('mkdir failed: '+e.message,'err');}}}]);"
        "setTimeout(()=>{const i=document.getElementById('nn');"
        "if(i)i.focus();},50);};"
        "const isImg=n=>/\\.(png|jpe?g|gif|bmp|webp|svg|ico)$/i.test(n);"
        "async function openView(full,size){"
        "const PREV=65536;"
        "const url='/files/api/download?path='+encodeURIComponent(full);"
        "const dlBtn={label:'Download',cls:'pri',act:()=>{closeModal();"
        "window.location=url;}};"
        "const closeBtn={label:'Close',act:closeModal};"
        "if(isImg(full)){"
        "const body='<p style=\"font-size:.85em;color:#8a939c\">'+esc(full)+"
        "' \xc2\xb7 '+fmt(size)+'</p>"
        "<div style=\"display:flex;justify-content:center;background:#10141a;"
        "border-radius:6px;padding:.6em;max-height:60vh;overflow:auto\">"
        "<img src=\"'+url+'\" style=\"max-width:100%;max-height:55vh;"
        "image-rendering:auto\" alt=\"image\"></div>';"
        "modal(full.split('/').pop(),body,[dlBtn,closeBtn]);return;}"
        "if(size>PREV){"
        "modal(full.split('/').pop(),"
        "'<p>'+esc(full)+'</p><p>Size: '+fmt(size)+"
        "'</p><p style=\"color:#8a939c\">(too large to preview — download instead)</p>',"
        "[dlBtn,closeBtn]);return;}"
        "try{const r=await fetch(url);if(!r.ok)throw new Error(r.status);"
        "const txt=await r.text();"
        "const bin=/[\\x00-\\x08\\x0e-\\x1f]/.test(txt.slice(0,2048));"
        "const body=(bin?'<p style=\"color:#ff8080\">(binary content)</p>':'')+"
        "'<div class=tv>'+esc(txt)+'</div>';"
        "modal(full.split('/').pop(),body,[dlBtn,closeBtn]);}"
        "catch(e){setMsg('open failed: '+e.message,'err');}}"
        "$('fu').onchange=async()=>{"
        "const files=Array.from($('fu').files);$('fu').value='';"
        "if(!files.length)return;"
        "const bb=$('bb'),bf=$('bf');bb.classList.add('on');"
        "let done=0;"
        "for(const f of files){"
        "setMsg('uploading '+f.name+' ('+(done+1)+'/'+files.length+')');"
        "bf.className='fl';bf.style.width='0';"
        "await new Promise((res,rej)=>{const x=new XMLHttpRequest();"
        "x.upload.onprogress=e=>{if(e.lengthComputable)"
        "bf.style.width=((e.loaded/e.total)*100).toFixed(1)+'%';};"
        "x.onload=()=>{if(x.status>=200&&x.status<300){res();}"
        "else{bf.className='fl err';"
        "let m=x.status;try{m=JSON.parse(x.responseText).error||m}catch(e){}"
        "rej(new Error(m));}};"
        "x.onerror=()=>{bf.className='fl err';rej(new Error('network'));};"
        "x.open('POST','/files/api/upload?path='+"
        "encodeURIComponent(join(cwd,f.name)));"
        "x.setRequestHeader('Content-Type','application/octet-stream');"
        "x.send(f);}).catch(e=>{setMsg('upload '+f.name+': '+e.message,'err');});"
        "done++;}"
        "bf.className='fl ok';"
        "setMsg(done+' file(s) uploaded',done===files.length?'ok':'err');"
        "setTimeout(()=>bb.classList.remove('on'),1500);"
        "load(cwd);};"
        "$('md').onclick=e=>{if(e.target===$('md'))closeModal();};"
        "load('/');"
        "</script></body></html>";
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, sizeof html - 1);
}

/* ---------- Registration ---------- */

esp_err_t files_http_register(httpd_handle_t server)
{
    if (!server) {
        ESP_LOGE(TAG, "no server handle — call after ota_http_start()");
        return ESP_ERR_INVALID_STATE;
    }

    static const httpd_uri_t routes[] = {
        { .uri = "/files",              .method = HTTP_GET,
          .handler = index_handler,    .user_ctx = NULL },
        { .uri = "/files/api/list",     .method = HTTP_GET,
          .handler = list_handler,     .user_ctx = NULL },
        { .uri = "/files/api/download", .method = HTTP_GET,
          .handler = download_handler, .user_ctx = NULL },
        { .uri = "/files/api/upload",   .method = HTTP_POST,
          .handler = upload_handler,   .user_ctx = NULL },
        { .uri = "/files/api/rename",   .method = HTTP_POST,
          .handler = rename_handler,   .user_ctx = NULL },
        { .uri = "/files/api/delete",   .method = HTTP_POST,
          .handler = delete_handler,   .user_ctx = NULL },
        { .uri = "/files/api/mkdir",    .method = HTTP_POST,
          .handler = mkdir_handler,    .user_ctx = NULL },
    };
    for (size_t i = 0; i < sizeof routes / sizeof routes[0]; ++i) {
        esp_err_t e = httpd_register_uri_handler(server, &routes[i]);
        if (e != ESP_OK) {
            ESP_LOGE(TAG, "register %s: %s",
                     routes[i].uri, esp_err_to_name(e));
            return e;
        }
    }
    ESP_LOGI(TAG, "web file manager attached at /files");
    return ESP_OK;
}

#else  /* !CONFIG_OTA_HTTP_ENABLED */

esp_err_t files_http_register(httpd_handle_t server)
{
    (void)server;
    return ESP_OK;
}

#endif
