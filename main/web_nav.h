#pragma once

/* Shared three-tab strip for the on-device web UI.
 *
 *   /ota    firmware update  (ota_http.c)
 *   /files  file manager     (files_http.c)
 *   /lisp   LISP editor      (lisp_http.c + web/lisp_editor.html)
 *
 * Three separate pages, one identical strip on top of each: every tab is a
 * plain link to a sibling page, the current one gets " on" appended to its
 * class. No JS, so it costs nothing and survives a reload.
 *
 * The LISP editor is a standalone .html file (gzipped at build time) and
 * cannot include a C header — it carries its own copy of this markup and
 * CSS. Keep the two in sync when touching the strip.
 */

/* Colours mirror the palette both pages already use (#14181d panel,
 * #4ea1ff accent) so the strip needs no per-page theming. */
#define WEB_NAV_CSS \
    ".nav{display:flex;align-items:stretch;background:#0b0d10;" \
    "border-bottom:1px solid #20262d;position:sticky;top:0;z-index:20}" \
    /* min-width:0 — without it a nowrap tab can't shrink below its
       min-content width and the strip pushes the page wider than a phone
       viewport instead of ellipsising. */ \
    ".nav a{flex:1 1 0;min-width:0;text-align:center;padding:.85em .6em;color:#8a939c;" \
    "text-decoration:none;font-size:.95em;font-weight:600;white-space:nowrap;" \
    "overflow:hidden;text-overflow:ellipsis;line-height:1.25;" \
    "border-bottom:2px solid transparent;transition:.12s}" \
    ".nav a:hover{color:#e6e8eb;background:#14181d}" \
    ".nav a.on{color:#e6e8eb;background:#14181d;border-bottom-color:#4ea1ff}" \
    ".nav a .ic{margin-right:.45em;opacity:.9;font-weight:400}"

/* Pass " on" for the current page, "" for the other two. */
#define WEB_NAV(a_fw, a_files, a_lisp) \
    "<nav class=nav>" \
    "<a class='tab" a_fw "' href=/ota>" \
    "<span class=ic>&#8593;</span>Firmware</a>" \
    "<a class='tab" a_files "' href=/files>" \
    "<span class=ic>&#128193;</span>Files</a>" \
    "<a class='tab" a_lisp "' href=/lisp>" \
    "<span class=ic>&#955;</span>LISP</a>" \
    "</nav>"

#define WEB_NAV_FIRMWARE  WEB_NAV(" on", "", "")
#define WEB_NAV_FILES     WEB_NAV("", " on", "")
