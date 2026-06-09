#ifndef FILES_SCREEN_H
#define FILES_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Open the SD-card file browser on top of the current screen. Modal —
 * tear-down on Close button returns to the previously-active screen
 * (typically VESC Settings). Safe to call from LVGL thread only; takes
 * bsp_display_lock internally. */
void files_screen_show(void);

#ifdef __cplusplus
}
#endif

#endif /* FILES_SCREEN_H */
