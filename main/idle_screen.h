#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Two-line "we're alive but nothing's happening" screen. Used at boot
 * ("Android Auto" / "Initialising Wi-Fi…"), once we're listening
 * ("Waiting for phone" / "<IP> | port"), and again after AA disconnects.
 *
 * Sits on lv_scr_act() as a hidden full-screen container — display_init()
 * must have run first.
 *
 * Idempotent. */
esp_err_t idle_screen_init(void);

/* Cache the two strings, apply them, and unhide the idle root. The cache
 * is what idle_screen_refresh() re-applies later. */
void idle_screen_show(const char *line1, const char *line2);

/* Re-apply the last `show` strings and force LVGL to repaint the active
 * screen. Call after the AA video sink wrote the panel framebuffer
 * directly so the stale frame gets replaced by the idle text. */
void idle_screen_refresh(void);

void idle_screen_hide(void);

/* Show or hide a "Connect" button under the subtitle. Used in "Waiting
 * for phone" state to give the user a manual override for paging the
 * paired phone (BT agent's auto-reconnect can be off, or stalled in
 * backoff). Tap routes through bt_link_request_connect_now(). */
void idle_screen_set_connect_visible(bool visible);

/* Link state, shown as the title in its own colour (red / amber / green)
 * with `detail` as the subtitle. Cached like show()'s strings, so a later
 * refresh re-applies it. Does not change visibility. ASCII only. */
typedef enum {
    IDLE_STATE_DISCONNECTED = 0,
    IDLE_STATE_CONNECTING,
    IDLE_STATE_CONNECTED,
} idle_screen_state_t;
void idle_screen_set_state(idle_screen_state_t state, const char *detail);

/* Small grey third line — the AP / IP / port reference. Cached. */
void idle_screen_set_info(const char *info);

#ifdef __cplusplus
}
#endif
