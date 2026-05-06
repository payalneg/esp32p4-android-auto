#ifndef UI_MODE_H
#define UI_MODE_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_MODE_AA = 0,    /* Android Auto (waiting screen + later, video output) */
    UI_MODE_VESC,      /* Super VESC dashboard */
} ui_mode_t;

esp_err_t ui_mode_init(void);   /* must be called after display_init() */

ui_mode_t ui_mode_get(void);
void      ui_mode_set(ui_mode_t mode);
void      ui_mode_toggle(void);

#ifdef __cplusplus
}
#endif

#endif
