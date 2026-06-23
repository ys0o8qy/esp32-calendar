#include "calendar_ui.h"

#include <stdio.h>

#include "calendar_theme.h"

static calendar_theme_t g_theme;

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, int x, int y)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    return label;
}

static lv_obj_t *make_panel(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_add_style(panel, &g_theme.panel, 0);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, w, h);
    return panel;
}

static void add_month_grid(lv_obj_t *parent, const calendar_model_t *model)
{
    static const char *weekdays[] = {"一", "二", "三", "四", "五", "六", "日"};
    calendar_month_grid_t grid;
    char text[16];

    calendar_model_month_grid(model->year, model->month, model->day, &grid);

    lv_obj_t *panel = make_panel(parent, 190, 45, 190, 201);
    snprintf(text, sizeof(text), "%d 年 %d 月", model->year, model->month);
    make_label(panel, text, 8, 4);
    snprintf(text, sizeof(text), "第 26 周");
    lv_obj_t *week = make_label(panel, text, 126, 4);
    lv_obj_add_style(week, &g_theme.muted, 0);

    for (int col = 0; col < CALENDAR_WEEK_DAYS; col++) {
        lv_obj_t *label = make_label(panel, weekdays[col], 16 + col * 24, 36);
        lv_obj_add_style(label, &g_theme.muted, 0);
    }

    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < CALENDAR_WEEK_DAYS; col++) {
            calendar_day_cell_t *cell = &grid.cells[row][col];
            snprintf(text, sizeof(text), "%d", cell->day);
            lv_obj_t *label = make_label(panel, text, 14 + col * 24, 62 + row * 24);
            if (!cell->in_current_month) {
                lv_obj_add_style(label, &g_theme.muted, 0);
            }
            if (cell->is_today) {
                lv_obj_add_style(label, &g_theme.today, 0);
            }
            if (cell->has_event) {
                lv_obj_t *dot = lv_obj_create(panel);
                lv_obj_remove_style_all(dot);
                lv_obj_set_style_bg_color(dot, lv_color_hex(0x171717), 0);
                lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
                lv_obj_set_size(dot, 4, 4);
                lv_obj_set_pos(dot, 20 + col * 24, 78 + row * 24);
            }
        }
    }
}

void calendar_ui_create(calendar_ui_t *ui, const calendar_model_t *model)
{
    char text[96];

    calendar_theme_init(&g_theme);
    ui->screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(ui->screen);
    lv_obj_add_style(ui->screen, &g_theme.screen, 0);
    lv_obj_set_size(ui->screen, 400, 300);

    calendar_model_status_text(model, text, sizeof(text));
    lv_obj_t *status = make_label(ui->screen, text, 10, 8);
    lv_obj_add_style(status, &g_theme.muted, 0);

    snprintf(text, sizeof(text), "%s  %d月%d日", model->weekday_text, model->month, model->day);
    make_label(ui->screen, text, 18, 45);

    snprintf(text, sizeof(text), "%02d:%02d", model->hour, model->minute);
    lv_obj_t *time = make_label(ui->screen, text, 18, 78);
    lv_obj_set_style_text_font(time, &lv_font_montserrat_48, 0);

    make_label(ui->screen, model->lunar_text, 21, 135);
    lv_obj_t *hint = make_label(ui->screen, model->day_hint, 21, 153);
    lv_obj_add_style(hint, &g_theme.muted, 0);

    lv_obj_t *weather = make_panel(ui->screen, 18, 166, 154, 82);
    snprintf(text, sizeof(text), "%s天气", model->city);
    make_label(weather, text, 8, 4);
    snprintf(text, sizeof(text), "%dC", model->temp_c);
    lv_obj_t *temp = make_label(weather, text, 92, 28);
    lv_obj_set_style_text_font(temp, &lv_font_montserrat_28, 0);
    snprintf(text, sizeof(text), "%s %d-%dC", model->weather_summary, model->temp_low_c, model->temp_high_c);
    lv_obj_t *summary = make_label(weather, text, 8, 48);
    lv_obj_add_style(summary, &g_theme.muted, 0);
    snprintf(text, sizeof(text), "湿度 %d%% 体感舒适", model->humidity_percent);
    lv_obj_t *humidity = make_label(weather, text, 8, 64);
    lv_obj_add_style(humidity, &g_theme.muted, 0);

    lv_obj_t *event = lv_obj_create(ui->screen);
    lv_obj_remove_style_all(event);
    lv_obj_add_style(event, &g_theme.inverse, 0);
    lv_obj_set_pos(event, 18, 256);
    lv_obj_set_size(event, 154, 24);
    snprintf(text, sizeof(text), "下一项 %s", model->next_event_text);
    make_label(event, text, 8, 4);

    add_month_grid(ui->screen, model);

    lv_obj_t *offline = make_panel(ui->screen, 190, 256, 190, 24);
    make_label(offline, "离线策略: RTC 保时 天气保留最近缓存", 4, 3);

    lv_scr_load(ui->screen);
}
