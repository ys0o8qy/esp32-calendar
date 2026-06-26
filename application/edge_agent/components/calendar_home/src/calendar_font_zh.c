/*******************************************************************************
 * Size: 10 px
 * Bpp: 1
 * Opts: --no-compress --no-prefilter --bpp 1 --size 10 --font assets/fonts/fusion-pixel-10px-monospaced-zh_hans.ttf --symbols  %+-./0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz°·一三二五保六取可同周四就已年度待或据接数新日时更月未步温湿用电等系统绪置读连配量间 --format lvgl -o application/edge_agent/components/calendar_home/src/calendar_font_zh.c --lv-include calendar_font_zh.h --lv-font-name calendar_font_zh_16 --force-fast-kern-format
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "calendar_font_zh.h"
#endif

#ifndef CALENDAR_FONT_ZH_16
#define CALENDAR_FONT_ZH_16 1
#endif

#if CALENDAR_FONT_ZH_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0025 "%" */
    0x91, 0x22, 0x48, 0x90,

    /* U+002B "+" */
    0x22, 0xf2, 0x20,

    /* U+002D "-" */
    0xf0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x11, 0x22, 0x44, 0x48, 0x80,

    /* U+0030 "0" */
    0x69, 0xbd, 0x99, 0x60,

    /* U+0031 "1" */
    0x59, 0x24, 0xb8,

    /* U+0032 "2" */
    0x69, 0x12, 0x48, 0xf0,

    /* U+0033 "3" */
    0x69, 0x12, 0x19, 0x60,

    /* U+0034 "4" */
    0x35, 0x59, 0x9f, 0x10,

    /* U+0035 "5" */
    0xf8, 0x8e, 0x11, 0xe0,

    /* U+0036 "6" */
    0x69, 0x8e, 0x99, 0x60,

    /* U+0037 "7" */
    0xf1, 0x12, 0x24, 0x40,

    /* U+0038 "8" */
    0x69, 0x96, 0x99, 0x60,

    /* U+0039 "9" */
    0x69, 0x97, 0x19, 0x60,

    /* U+003A ":" */
    0x88,

    /* U+0041 "A" */
    0x69, 0x99, 0xf9, 0x90,

    /* U+0042 "B" */
    0xe9, 0x9e, 0x99, 0xe0,

    /* U+0043 "C" */
    0x69, 0x88, 0x89, 0x60,

    /* U+0044 "D" */
    0xe9, 0x99, 0x99, 0xe0,

    /* U+0045 "E" */
    0xf8, 0x8e, 0x88, 0xf0,

    /* U+0046 "F" */
    0xf8, 0x8e, 0x88, 0x80,

    /* U+0047 "G" */
    0x69, 0x88, 0xb9, 0x70,

    /* U+0048 "H" */
    0x99, 0x9f, 0x99, 0x90,

    /* U+0049 "I" */
    0xf2, 0x22, 0x22, 0xf0,

    /* U+004A "J" */
    0x11, 0x11, 0x99, 0x60,

    /* U+004B "K" */
    0x99, 0xac, 0xa9, 0x90,

    /* U+004C "L" */
    0x88, 0x88, 0x88, 0xf0,

    /* U+004D "M" */
    0x9f, 0xf9, 0x99, 0x90,

    /* U+004E "N" */
    0x9d, 0xdb, 0xb9, 0x90,

    /* U+004F "O" */
    0x69, 0x99, 0x99, 0x60,

    /* U+0050 "P" */
    0xe9, 0x9e, 0x88, 0x80,

    /* U+0051 "Q" */
    0x69, 0x99, 0x9a, 0x50,

    /* U+0052 "R" */
    0xe9, 0x9e, 0xa9, 0x90,

    /* U+0053 "S" */
    0x69, 0x86, 0x19, 0x60,

    /* U+0054 "T" */
    0xf2, 0x22, 0x22, 0x20,

    /* U+0055 "U" */
    0x99, 0x99, 0x99, 0x60,

    /* U+0056 "V" */
    0x99, 0x9a, 0xaa, 0xc0,

    /* U+0057 "W" */
    0x99, 0x99, 0xff, 0x90,

    /* U+0058 "X" */
    0x99, 0x96, 0x99, 0x90,

    /* U+0059 "Y" */
    0x99, 0x55, 0x22, 0x20,

    /* U+005A "Z" */
    0xf1, 0x24, 0x48, 0xf0,

    /* U+0061 "a" */
    0x61, 0x79, 0x70,

    /* U+0062 "b" */
    0x88, 0xe9, 0x99, 0xe0,

    /* U+0063 "c" */
    0x69, 0x89, 0x60,

    /* U+0064 "d" */
    0x11, 0x79, 0x99, 0x70,

    /* U+0065 "e" */
    0x69, 0xf8, 0x60,

    /* U+0066 "f" */
    0x34, 0xf4, 0x44, 0x40,

    /* U+0067 "g" */
    0x79, 0x97, 0x16,

    /* U+0068 "h" */
    0x88, 0xe9, 0x99, 0x90,

    /* U+0069 "i" */
    0x20, 0xe2, 0x22, 0xf0,

    /* U+006A "j" */
    0x23, 0x92, 0x4e,

    /* U+006B "k" */
    0x88, 0x9a, 0xca, 0x90,

    /* U+006C "l" */
    0xc4, 0x44, 0x44, 0x30,

    /* U+006D "m" */
    0x9f, 0xf9, 0x90,

    /* U+006E "n" */
    0xe9, 0x99, 0x90,

    /* U+006F "o" */
    0x69, 0x99, 0x60,

    /* U+0070 "p" */
    0xe9, 0x9e, 0x88,

    /* U+0071 "q" */
    0x79, 0x97, 0x11,

    /* U+0072 "r" */
    0xbc, 0x88, 0x80,

    /* U+0073 "s" */
    0x78, 0x61, 0xe0,

    /* U+0074 "t" */
    0x44, 0xf4, 0x44, 0x30,

    /* U+0075 "u" */
    0x99, 0x99, 0x70,

    /* U+0076 "v" */
    0x99, 0xaa, 0xc0,

    /* U+0077 "w" */
    0x99, 0xff, 0x90,

    /* U+0078 "x" */
    0x99, 0x69, 0x90,

    /* U+0079 "y" */
    0x99, 0x55, 0x2c,

    /* U+007A "z" */
    0xf2, 0x48, 0xf0,

    /* U+00B0 "°" */
    0x55, 0x0,

    /* U+00B7 "·" */
    0xf0,

    /* U+4E00 "一" */
    0xff, 0x80,

    /* U+4E09 "三" */
    0x7f, 0x0, 0x0, 0x0, 0x3, 0xe0, 0x0, 0x0,
    0x0, 0xff, 0x80,

    /* U+4E8C "二" */
    0x7f, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0xfe,

    /* U+4E94 "五" */
    0x7f, 0x4, 0x2, 0x1, 0x7, 0xf0, 0x88, 0x44,
    0x22, 0xff, 0x80,

    /* U+4FDD "保" */
    0x2f, 0x94, 0x52, 0x39, 0xf4, 0x22, 0xfd, 0x1c,
    0x95, 0x52, 0x0,

    /* U+516D "六" */
    0x8, 0x4, 0x3f, 0xe0, 0x0, 0x1, 0x8, 0x84,
    0x81, 0x80, 0x80,

    /* U+53D6 "取" */
    0xff, 0xa4, 0x5e, 0x29, 0x57, 0xaa, 0x49, 0x25,
    0xf5, 0xc, 0x80,

    /* U+53EF "可" */
    0xff, 0x80, 0x9e, 0x49, 0x24, 0x92, 0x49, 0xe4,
    0x2, 0x7, 0x0,

    /* U+540C "同" */
    0xff, 0xc0, 0x6f, 0xb0, 0x1b, 0xed, 0x16, 0xfb,
    0x1, 0x81, 0x80,

    /* U+5468 "周" */
    0x7f, 0xa4, 0x5f, 0xa9, 0x17, 0xfa, 0x5, 0xfb,
    0x45, 0xbf, 0x80,

    /* U+56DB "四" */
    0xff, 0xca, 0x65, 0x32, 0x99, 0x4d, 0x1f, 0x3,
    0x1, 0xff, 0x80,

    /* U+5C31 "就" */
    0x22, 0xf9, 0x3, 0xfe, 0x49, 0x57, 0xa8, 0x95,
    0x6a, 0x69, 0x80,

    /* U+5DF2 "已" */
    0xff, 0x0, 0xa0, 0x50, 0x2f, 0xf4, 0x2, 0x1,
    0x1, 0x7f, 0x80,

    /* U+5E74 "年" */
    0x20, 0x1f, 0xd1, 0x17, 0xe2, 0x41, 0x23, 0xfe,
    0x8, 0x4, 0x0,

    /* U+5EA6 "度" */
    0x4, 0x3f, 0xd2, 0x8f, 0xf4, 0xa2, 0xf9, 0x44,
    0x9c, 0xb1, 0x80,

    /* U+5F85 "待" */
    0x44, 0x4f, 0xc1, 0x7, 0xf4, 0x16, 0xfd, 0x4,
    0xa2, 0x4b, 0x0,

    /* U+6216 "或" */
    0x4, 0xff, 0xc0, 0x8f, 0x54, 0xab, 0xc8, 0xd,
    0xc9, 0x18, 0x80,

    /* U+636E "据" */
    0x5f, 0xa8, 0x7f, 0xea, 0x45, 0xfe, 0x91, 0xbe,
    0x91, 0xcf, 0x80,

    /* U+63A5 "接" */
    0x42, 0x2f, 0xfa, 0x4b, 0xf4, 0x43, 0xff, 0x24,
    0x9c, 0xd1, 0x80,

    /* U+6570 "数" */
    0xaa, 0x59, 0xff, 0x2d, 0x5a, 0x2f, 0xe9, 0x24,
    0xe5, 0x8c, 0x80,

    /* U+65B0 "新" */
    0x27, 0xfe, 0x15, 0xa, 0xff, 0xd1, 0x2b, 0xd4,
    0xd2, 0xa9, 0x0,

    /* U+65E5 "日" */
    0xff, 0x6, 0xc, 0x1f, 0xf0, 0x60, 0xc1, 0xfe,

    /* U+65F6 "时" */
    0x1, 0x70, 0xab, 0xf4, 0x2e, 0x95, 0x2a, 0x85,
    0xc2, 0x3, 0x0,

    /* U+66F4 "更" */
    0xff, 0x84, 0x1f, 0xc9, 0x27, 0xf2, 0x49, 0xfc,
    0x50, 0xf7, 0x80,

    /* U+6708 "月" */
    0x3f, 0x90, 0x4f, 0xe4, 0x12, 0x9, 0xfc, 0x82,
    0x81, 0x81, 0x80,

    /* U+672A "未" */
    0x8, 0x3f, 0x82, 0x1, 0xf, 0xf8, 0xe0, 0xa9,
    0x93, 0x8, 0x0,

    /* U+6B65 "步" */
    0x8, 0x27, 0x92, 0x1f, 0xf0, 0x82, 0x42, 0x66,
    0xc, 0x78, 0x0,

    /* U+6E29 "温" */
    0x9f, 0x28, 0x87, 0xd2, 0x25, 0xf0, 0x0, 0xfe,
    0xd5, 0xbf, 0x80,

    /* U+6E7F "湿" */
    0x9f, 0xa8, 0x47, 0xf2, 0x15, 0xf8, 0x50, 0x6a,
    0x94, 0xbf, 0x80,

    /* U+7528 "用" */
    0x7f, 0xa4, 0x5f, 0xe9, 0x14, 0x8b, 0xfd, 0x22,
    0x91, 0x89, 0x80,

    /* U+7535 "电" */
    0x10, 0x7f, 0xa4, 0x5f, 0xe9, 0x14, 0x8b, 0xfc,
    0x21, 0xf, 0x80,

    /* U+7B49 "等" */
    0x44, 0x3b, 0xea, 0x41, 0x7, 0xf0, 0x13, 0xfe,
    0x44, 0x16, 0x0,

    /* U+7CFB "系" */
    0x7f, 0x8, 0x8, 0x82, 0x80, 0x97, 0xf4, 0x20,
    0x92, 0x88, 0x80,

    /* U+7EDF "统" */
    0x44, 0x2f, 0xe1, 0x1d, 0x25, 0xff, 0x50, 0x28,
    0x55, 0xd3, 0x80,

    /* U+7EEA "绪" */
    0x24, 0xaf, 0xe1, 0x3c, 0xa5, 0xfc, 0x47, 0x7e,
    0x11, 0xef, 0x80,

    /* U+7F6E "置" */
    0x7f, 0x2a, 0xbf, 0xe1, 0x7, 0xf2, 0x9, 0xfc,
    0x82, 0xff, 0x80,

    /* U+8BFB "读" */
    0x82, 0x27, 0xc0, 0x9b, 0xf4, 0x4a, 0x51, 0x7e,
    0xca, 0x58, 0x80,

    /* U+8FDE "连" */
    0x82, 0x2f, 0xc1, 0x1, 0x4c, 0xfa, 0x11, 0x7f,
    0x44, 0x9f, 0x80,

    /* U+914D "配" */
    0xf7, 0xa0, 0x7c, 0x3a, 0xfb, 0x44, 0xa3, 0xd3,
    0x29, 0xf7, 0x80,

    /* U+91CF "量" */
    0x7f, 0x20, 0xbf, 0xe9, 0x27, 0xf2, 0x49, 0xfc,
    0x10, 0xff, 0x80,

    /* U+95F4 "间" */
    0x9f, 0xa0, 0x4f, 0xb4, 0x5b, 0xed, 0x16, 0x8b,
    0x7d, 0x81, 0x80
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 80, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 5, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 8, .adv_w = 80, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 9, .adv_w = 80, .box_w = 1, .box_h = 1, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 10, .adv_w = 80, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 15, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 19, .adv_w = 80, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 22, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 26, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 34, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 42, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 46, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 50, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 54, .adv_w = 80, .box_w = 1, .box_h = 5, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 59, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 67, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 71, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 79, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 103, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 127, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 131, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 135, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 143, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 147, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 159, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 162, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 169, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 183, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 191, .adv_w = 80, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 194, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 205, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 214, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 217, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 230, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 233, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 236, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 242, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 80, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 247, .adv_w = 160, .box_w = 2, .box_h = 2, .ofs_x = 4, .ofs_y = 2},
    {.bitmap_index = 248, .adv_w = 160, .box_w = 9, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 250, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 261, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 269, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 280, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 291, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 302, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 313, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 324, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 335, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 346, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 357, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 368, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 379, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 390, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 401, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 412, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 423, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 434, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 445, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 456, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 467, .adv_w = 160, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 475, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 486, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 497, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 508, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 519, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 530, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 541, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 552, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 563, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 574, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 585, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 596, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 607, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 618, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 629, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 640, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 651, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 662, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 673, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x5, 0xb
};

static const uint16_t unicode_list_4[] = {
    0x0, 0x7, 0x4d50, 0x4d59, 0x4ddc, 0x4de4, 0x4f2d, 0x50bd,
    0x5326, 0x533f, 0x535c, 0x53b8, 0x562b, 0x5b81, 0x5d42, 0x5dc4,
    0x5df6, 0x5ed5, 0x6166, 0x62be, 0x62f5, 0x64c0, 0x6500, 0x6535,
    0x6546, 0x6644, 0x6658, 0x667a, 0x6ab5, 0x6d79, 0x6dcf, 0x7478,
    0x7485, 0x7a99, 0x7c4b, 0x7e2f, 0x7e3a, 0x7ebe, 0x8b4b, 0x8f2e,
    0x909d, 0x911f, 0x9544
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 12, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 3, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 45, .range_length = 14, .glyph_id_start = 4,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 65, .range_length = 26, .glyph_id_start = 18,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 97, .range_length = 26, .glyph_id_start = 44,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 176, .range_length = 38213, .glyph_id_start = 70,
        .unicode_list = unicode_list_4, .glyph_id_ofs_list = NULL, .list_length = 43, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
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
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 5,
    .bpp = 1,
    .kern_classes = 0,
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
const lv_font_t calendar_font_zh_16 = {
#else
lv_font_t calendar_font_zh_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 9,          /*The maximum line height required by the font*/
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



#endif /*#if CALENDAR_FONT_ZH_16*/

