#pragma once

#include "lvgl.h"

/* Project-specific Montserrat-Medium subsets with extended coverage:
 *   - ASCII basic         (0x20-0x7E)
 *   - Russian Cyrillic    (А-я + Ё/ё)
 *   - Polish diacritics   (ą ć ę ł ń ó ś ź ż + uppercase)
 *
 * Generated with lv_font_conv from the Montserrat Medium TTF, bpp 4 for
 * the small text size and bpp 2 for the big ones (AA detail is wasted
 * past 32 px). Use these in place of lv_font_montserrat_* anywhere
 * non-ASCII can land — title/artist of the music tile, notification
 * body, etc. */

#ifdef __cplusplus
extern "C" {
#endif

LV_FONT_DECLARE(aabridge_font_24);
LV_FONT_DECLARE(aabridge_font_32);
LV_FONT_DECLARE(aabridge_font_38);
LV_FONT_DECLARE(aabridge_font_48);

#ifdef __cplusplus
}
#endif
