/*
    Copyright 2026 Super VESC Display

    "Mono" dashboard theme — one central 270° speedometer with a thin battery
    ring, flanked by battery/range and power/current cards, a wide now-playing
    card and the five-cell telemetry strip.

    800x480 only (registered from custom.c under SVD_WIDE_UI).
*/
#ifndef THEME_MONO_H_
#define THEME_MONO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Register the Mono theme. Call once from custom_init_once() (custom.c). */
void theme_mono_register(void);

#ifdef __cplusplus
}
#endif
#endif /* THEME_MONO_H_ */
