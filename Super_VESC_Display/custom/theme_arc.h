/*
    Copyright 2026 Super VESC Display

    "Twin Arc" dashboard theme — the modern HMI trend: two large 270° arcs
    (speed left, power right) framing a compact battery/range column, with the
    same top status bar and bottom telemetry strip as the cockpit. Hand-written
    in LVGL; no GUI Guider screen and no new fonts.

    800x480 only (registered from custom.c under SVD_WIDE_UI).
*/
#ifndef THEME_ARC_H_
#define THEME_ARC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Register the twin-arc theme. Call once from custom_init_once() (custom.c). */
void theme_arc_register(void);

#ifdef __cplusplus
}
#endif
#endif /* THEME_ARC_H_ */
