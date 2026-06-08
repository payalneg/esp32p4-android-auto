/*
    Copyright 2026 Super VESC Display

    "Cockpit (Amber)" dashboard theme — the cyberpunk-amber reskin of the
    cockpit, authored as its own GUI Guider screen (dashboard_amber) and wired
    into the dashboard-theme registry. See dashboard_theme.h for the model and
    theme_ref.c for the template this follows.
*/
#ifndef THEME_DASHBOARD_AMBER_H_
#define THEME_DASHBOARD_AMBER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Register the amber theme. Call once from custom_init_once() (custom.c). */
void theme_dashboard_amber_register(void);

#ifdef __cplusplus
}
#endif
#endif /* THEME_DASHBOARD_AMBER_H_ */
