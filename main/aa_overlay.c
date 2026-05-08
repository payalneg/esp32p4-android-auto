#include "aa_overlay.h"

#include <stdio.h>
#include <string.h>

#include "custom.h"
#include "lvgl.h"

/* Panel-native (portrait) dimensions, mirrored from display_video.c. The
 * AA frame in `fb` was rotated 90° CW into this layout, so the user sees
 * USER_W × USER_H landscape. */
#define PANEL_W   480
#define PANEL_H   800
#define USER_W    800
#define USER_H    480

#define MARGIN_X        16
#define CENTER_Y        (USER_H / 2)
#define COLOR_TEXT      0x0000u     /* RGB565 black */
#define COLOR_OUTLINE   0xFFFFu     /* RGB565 white halo so digits stay legible
                                       on dark video backgrounds too */
#define GLOBAL_ALPHA    220u        /* 0..255 — slight transparency over video */

/* Antonio Regular 64 — the project's display font (Super_VESC_Display
 * generated_fonts), already compiled in for the dashboard widgets.
 * Tall/narrow numeric look that reads at a glance even over busy video. */
extern const lv_font_t lv_font_Antonio_Regular_64;

/* 4-bpp anti-aliased opacity table — same numbers LVGL uses internally
 * (lv_draw_sw_letter.c::_lv_bpp4_opa_table). Lets us share the rasteriser
 * with whatever bpp the font happens to use. */
static const uint8_t bpp4_opa[16] = {
      0, 17, 34, 51, 68, 85, 102, 119,
    136, 153, 170, 187, 204, 221, 238, 255
};

static inline void blend_user_pixel(uint16_t *fb, int ux, int uy,
                                    uint16_t color, uint8_t alpha)
{
    if (alpha == 0) return;
    if ((unsigned)ux >= USER_W || (unsigned)uy >= USER_H) return;
    /* User landscape (ux, uy) → panel-native (px, py) for ROTATE_90 CW.
     * Same mapping touch_input.c uses: px = (USER_H-1) - uy, py = ux. */
    int px = (USER_H - 1) - uy;
    int py = ux;
    int idx = py * PANEL_W + px;
    if (alpha == 255) { fb[idx] = color; return; }

    uint16_t bg = fb[idx];
    int r1 = (color >> 11) & 0x1F;
    int g1 = (color >> 5)  & 0x3F;
    int b1 =  color        & 0x1F;
    int r2 = (bg    >> 11) & 0x1F;
    int g2 = (bg    >> 5)  & 0x3F;
    int b2 =  bg           & 0x1F;
    int r = (r1 * alpha + r2 * (255 - alpha)) / 255;
    int g = (g1 * alpha + g2 * (255 - alpha)) / 255;
    int b = (b1 * alpha + b2 * (255 - alpha)) / 255;
    fb[idx] = (r << 11) | (g << 5) | b;
}

/* Render one glyph at (pen_x, line_top_y) in user-landscape coords; return
 * the advance width so caller can chain glyphs along a baseline. */
static uint16_t draw_glyph(uint16_t *fb, const lv_font_t *font, uint32_t letter,
                           int pen_x, int line_top_y,
                           uint16_t color, uint8_t global_alpha)
{
    lv_font_glyph_dsc_t g;
    if (!lv_font_get_glyph_dsc(font, &g, letter, 0)) return 0;
    if (g.box_w == 0 || g.box_h == 0) return g.adv_w;
    const uint8_t *bm = lv_font_get_glyph_bitmap(font, letter);
    if (!bm) return g.adv_w;

    /* Same vertical placement formula as lv_draw_sw_letter.c:
     *   gy0 = line_top + (line_height - base_line) - box_h - ofs_y
     */
    int ascent = font->line_height - font->base_line;
    int gy0 = line_top_y + ascent - g.box_h - g.ofs_y;
    int gx0 = pen_x + g.ofs_x;
    int bpp = g.bpp;

    for (int row = 0; row < g.box_h; row++) {
        for (int col = 0; col < g.box_w; col++) {
            int bit_ofs = (row * g.box_w + col) * bpp;
            int byte_idx = bit_ofs >> 3;
            int bit_in_byte = bit_ofs & 7;
            uint8_t a = 0;
            switch (bpp) {
            case 4:
                a = bpp4_opa[(bm[byte_idx] >> (4 - bit_in_byte)) & 0x0F];
                break;
            case 8:
                a = bm[byte_idx];
                break;
            case 2:
                a = (uint8_t)(((bm[byte_idx] >> (6 - bit_in_byte)) & 0x03) * 85);
                break;
            case 1:
                a = ((bm[byte_idx] >> (7 - bit_in_byte)) & 0x01) ? 255 : 0;
                break;
            default:
                continue;
            }
            if (a == 0) continue;
            uint8_t eff = (uint16_t)a * global_alpha / 255;
            blend_user_pixel(fb, gx0 + col, gy0 + row, color, eff);
        }
    }
    return g.adv_w;
}

static int measure_text(const lv_font_t *font, const char *s)
{
    int w = 0;
    while (*s) {
        lv_font_glyph_dsc_t g;
        if (lv_font_get_glyph_dsc(font, &g, (unsigned char)*s++, 0)) {
            w += g.adv_w;
        }
    }
    return w;
}

static void draw_text(uint16_t *fb, const lv_font_t *font,
                      int pen_x, int line_top_y, const char *s,
                      uint16_t color, uint8_t alpha)
{
    while (*s) {
        pen_x += draw_glyph(fb, font, (unsigned char)*s++, pen_x, line_top_y,
                            color, alpha);
    }
}

/* Black text + a 1-px white halo (N/S/E/W). 4 outline passes is enough to
 * keep dark digits readable against dark video backgrounds; a full 8-way
 * outline would be twice the cost for diagonal pixels barely visible at
 * this font size. */
static void draw_text_with_shadow(uint16_t *fb, const lv_font_t *font,
                                  int pen_x, int line_top_y, const char *s)
{
    draw_text(fb, font, pen_x - 1, line_top_y,     s, COLOR_OUTLINE, GLOBAL_ALPHA);
    draw_text(fb, font, pen_x + 1, line_top_y,     s, COLOR_OUTLINE, GLOBAL_ALPHA);
    draw_text(fb, font, pen_x,     line_top_y - 1, s, COLOR_OUTLINE, GLOBAL_ALPHA);
    draw_text(fb, font, pen_x,     line_top_y + 1, s, COLOR_OUTLINE, GLOBAL_ALPHA);
    draw_text(fb, font, pen_x,     line_top_y,     s, COLOR_TEXT,    GLOBAL_ALPHA);
}

void aa_overlay_draw(uint16_t *fb)
{
    if (!fb) return;

    /* Pull the same numbers the cockpit shows. cockpit_get_*() return the
     * last value passed to update_speed / update_battery_proc, so AA HUD
     * agrees with the dashboard, demo mode included. The values are stale
     * while the user is in AA (vesc_ui_updater is an LVGL timer and the
     * worker is paused) — that's fine, they reflect the latest reading
     * the cockpit got, and refresh as soon as the user toggles back. */
    char speed_buf[8];
    char batt_buf[8];
    snprintf(speed_buf, sizeof speed_buf, "%d",
             cockpit_get_speed_value());
    snprintf(batt_buf,  sizeof batt_buf,  "%d%%",
             cockpit_get_battery_proc_value());

    const lv_font_t *f = &lv_font_Antonio_Regular_64;
    int line_top = CENTER_Y - f->line_height / 2;

    /* Speed: anchored at left margin. */
    draw_text_with_shadow(fb, f, MARGIN_X, line_top, speed_buf);

    /* Battery: right-aligned at right margin. */
    int pen_x = USER_W - MARGIN_X - measure_text(f, batt_buf);
    draw_text_with_shadow(fb, f, pen_x, line_top, batt_buf);
}
