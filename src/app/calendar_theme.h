#pragma once

#include "lvgl.h"

typedef struct {
    lv_style_t screen;
    lv_style_t panel;
    lv_style_t muted;
    lv_style_t inverse;
    lv_style_t today;
} calendar_theme_t;

void calendar_theme_init(calendar_theme_t *theme);
