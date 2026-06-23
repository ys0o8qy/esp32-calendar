#pragma once

#include "calendar_model.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t *screen;
} calendar_ui_t;

void calendar_ui_create(calendar_ui_t *ui, const calendar_model_t *model);
