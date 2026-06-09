#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Boot splash GIF.
 *
 * splash_screen_show() displays /vescfs/splash.gif as a full-screen overlay on
 * the LVGL top layer (so it survives the idle→dashboard lv_scr_load that runs
 * while the dashboard builds). It is a no-op if the storage FS isn't ready in
 * time or the file is absent. A safety timer auto-hides it so it can never get
 * stuck. Call once at boot, after the display is up.
 *
 * splash_screen_hide() removes the overlay — call it once the real UI underneath
 * is ready (after the dashboard is built). Safe to call when nothing is shown.
 *
 * Both take bsp_display_lock internally; call from the main/LVGL context. */
void splash_screen_show(void);
void splash_screen_hide(void);

#ifdef __cplusplus
}
#endif