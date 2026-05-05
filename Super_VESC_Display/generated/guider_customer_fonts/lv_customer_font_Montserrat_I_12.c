/*******************************************************************************
 * Size: 12 px
 * Bpp: 4
 * Opts: undefined
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef LV_CUSTOMER_FONT_MONTSERRAT_I_12
#define LV_CUSTOMER_FONT_MONTSERRAT_I_12 1
#endif

#if LV_CUSTOMER_FONT_MONTSERRAT_I_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0031 "1" */
    0xab, 0xe0, 0x3, 0xb0, 0x6, 0x70, 0x9, 0x40,
    0xd, 0x10, 0xd, 0x0, 0x3a, 0x0, 0x76, 0x0,

    /* U+0032 "2" */
    0x0, 0x3b, 0xcc, 0xa1, 0x0, 0x61, 0x0, 0x78,
    0x0, 0x0, 0x0, 0x67, 0x0, 0x0, 0x2, 0xc1,
    0x0, 0x0, 0x4c, 0x20, 0x0, 0x8, 0xa0, 0x0,
    0x1, 0xb6, 0x0, 0x0, 0xe, 0xdb, 0xbb, 0x90,

    /* U+0033 "3" */
    0x0, 0x6b, 0xbb, 0xdb, 0x0, 0x0, 0x3, 0xb0,
    0x0, 0x0, 0x4b, 0x0, 0x0, 0x3, 0xf7, 0x0,
    0x0, 0x0, 0x26, 0xc0, 0x0, 0x0, 0x0, 0xe0,
    0x5, 0x0, 0x4, 0xb0, 0x8, 0xcc, 0xc9, 0x10
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 69, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 16, .adv_w = 109, .box_w = 8, .box_h = 8, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 48, .adv_w = 108, .box_w = 8, .box_h = 8, .ofs_x = -1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 49, .range_length = 3, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 0, 1
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 1, 1
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    -1
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 1,
    .right_class_cnt     = 1,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_customer_font_Montserrat_I_12 = {
#else
lv_font_t lv_customer_font_Montserrat_I_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 12,          /*The maximum line height required by the font*/
    .base_line = 1,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_CUSTOMER_FONT_MONTSERRAT_I_12*/

