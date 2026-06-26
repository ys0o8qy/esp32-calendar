#include "calendar_theme.h"

#include "calendar_font_zh.h"

void calendar_theme_init(calendar_theme_t *theme)
{
    lv_style_init(&theme->screen);
    lv_style_set_bg_color(&theme->screen, lv_color_hex(0xffffff));
    lv_style_set_text_color(&theme->screen, lv_color_hex(0x171717));
    lv_style_set_text_font(&theme->screen, &calendar_font_zh_16);
    lv_style_set_pad_all(&theme->screen, 8);

    lv_style_init(&theme->panel);
    lv_style_set_bg_color(&theme->panel, lv_color_hex(0xffffff));
    lv_style_set_border_color(&theme->panel, lv_color_hex(0x171717));
    lv_style_set_border_width(&theme->panel, 1);
    lv_style_set_radius(&theme->panel, 4);
    lv_style_set_pad_all(&theme->panel, 8);

    lv_style_init(&theme->muted);
    lv_style_set_text_color(&theme->muted, lv_color_hex(0x5a5a54));

    lv_style_init(&theme->inverse);
    lv_style_set_bg_color(&theme->inverse, lv_color_hex(0x171717));
    lv_style_set_text_color(&theme->inverse, lv_color_hex(0xffffff));
    lv_style_set_radius(&theme->inverse, 4);
    lv_style_set_pad_hor(&theme->inverse, 8);
    lv_style_set_pad_ver(&theme->inverse, 4);

    lv_style_init(&theme->today);
    lv_style_set_bg_color(&theme->today, lv_color_hex(0x171717));
    lv_style_set_bg_opa(&theme->today, LV_OPA_COVER);
    lv_style_set_text_color(&theme->today, lv_color_hex(0xffffff));
    lv_style_set_radius(&theme->today, 3);
    lv_style_set_pad_hor(&theme->today, 0);
    lv_style_set_pad_top(&theme->today, 5);
    lv_style_set_pad_bottom(&theme->today, 0);
}
