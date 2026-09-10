#ifndef UI_GEOM_H
#define UI_GEOM_H

/* Screen geometry for the hand-written screens.
 *
 * The GUI Guider screens exist once per panel geometry (Super_VESC_Display at
 * 800x480 for the ESP32-P4 head units, Super_VESC_Display_480 at 480x480 for
 * the ESP32-S3 board). Everything in custom/ — Settings, Statistics, the VESC
 * Tool menu, the LISP editor and panel, PAS and speed setup — is built in C and
 * compiled once for both, so it has to ask the display how wide it is instead
 * of assuming 800.
 *
 * UI_W / UI_H are the live resolution. UI_SX() scales an x coordinate or a
 * width that was measured against the original 800-wide layout, which is what
 * the column anchors in these screens are. Vertical geometry is left alone:
 * both panels are 480 tall.
 *
 * Nothing here is a substitute for looking at the result — a column that only
 * just fitted at 800 px can end up cramped at 480, and that is a design call.
 * The desktop simulator (Super_VESC_Display_480/lvgl-simulator) is the fast way
 * to see it.
 */

#include "lvgl.h"

#define UI_DESIGN_W  800     /* width the code screens were laid out against */

#define UI_W  ((lv_coord_t)lv_disp_get_hor_res(NULL))
#define UI_H  ((lv_coord_t)lv_disp_get_ver_res(NULL))

/* Scale an x / width from the 800-wide design to this screen. */
#define UI_SX(v)  ((lv_coord_t)(((int32_t)(v) * (int32_t)UI_W) / UI_DESIGN_W))

/* True on a panel that is as tall as it is wide (the ESP32-S3 board), where a
 * layout built as a wide strip needs different treatment rather than just
 * narrower columns. */
#define UI_IS_SQUARE  (UI_W == UI_H)

#endif /* UI_GEOM_H */
