#include "calendar_ui.h"

#include <stdbool.h>
#include <stdio.h>

#include "calendar_font_zh.h"
#include "calendar_theme.h"

static calendar_theme_t g_theme;
static bool g_theme_initialized;
static lv_calendar_date_t g_highlighted_dates[CALENDAR_MAX_EVENTS];

static void ensure_theme(void)
{
    if (!g_theme_initialized) {
        calendar_theme_init(&g_theme);
        g_theme_initialized = true;
    }
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, int x, int y)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    return label;
}

static lv_obj_t *make_label_box(lv_obj_t *parent, const char *text, int x, int y, int w, int h)
{
    lv_obj_t *label = make_label(parent, text, x, y);
    lv_obj_set_size(label, w, h);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
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

static lv_obj_t *make_bottom_status(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_pos(bar, 10, 264);
    lv_obj_set_size(bar, 380, 28);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(0x171717), 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    return bar;
}

static uint16_t collect_highlighted_dates(const calendar_model_t *model)
{
    size_t count = model->event_day_count;
    if (count > CALENDAR_MAX_EVENTS) {
        count = CALENDAR_MAX_EVENTS;
    }

    uint16_t date_count = 0;
    for (size_t i = 0; i < count; i++) {
        int day = model->event_days[i];
        if (day < 1 || day > 31) {
            continue;
        }
        g_highlighted_dates[date_count].year = (uint16_t)model->year;
        g_highlighted_dates[date_count].month = (int8_t)model->month;
        g_highlighted_dates[date_count].day = (int8_t)day;
        date_count++;
    }
    return date_count;
}

static void calendar_draw_part_begin(lv_event_t *event)
{
    lv_obj_t *button_matrix = lv_event_get_target(event);
    lv_obj_draw_part_dsc_t *draw = lv_event_get_param(event);
    if (draw->part != LV_PART_ITEMS) {
        return;
    }

    draw->rect_dsc->bg_opa = LV_OPA_TRANSP;
    draw->rect_dsc->border_opa = LV_OPA_TRANSP;
    if (draw->id < 7) {
        draw->label_dsc->color = lv_color_hex(0x5a5a54);
    } else if (lv_btnmatrix_has_btn_ctrl(button_matrix, draw->id, LV_BTNMATRIX_CTRL_DISABLED)) {
        draw->label_dsc->color = lv_color_hex(0xffffff);
    }

    if (lv_btnmatrix_has_btn_ctrl(button_matrix, draw->id, LV_BTNMATRIX_CTRL_CUSTOM_2)) {
        draw->rect_dsc->border_opa = LV_OPA_COVER;
        draw->rect_dsc->border_color = lv_color_hex(0x171717);
        draw->rect_dsc->border_width = 2;
        draw->rect_dsc->border_side = LV_BORDER_SIDE_BOTTOM;
    }

    if (lv_btnmatrix_has_btn_ctrl(button_matrix, draw->id, LV_BTNMATRIX_CTRL_CUSTOM_1)) {
        draw->rect_dsc->bg_opa = LV_OPA_COVER;
        draw->rect_dsc->bg_color = lv_color_hex(0x171717);
        draw->rect_dsc->border_opa = LV_OPA_TRANSP;
        draw->rect_dsc->radius = 3;
        draw->label_dsc->color = lv_color_hex(0xffffff);
    }
}

static void style_calendar_button_matrix(lv_obj_t *button_matrix)
{
    lv_obj_remove_style_all(button_matrix);
    lv_obj_set_size(button_matrix, 174, 158);
    lv_obj_set_style_bg_opa(button_matrix, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(button_matrix, 0, 0);
    lv_obj_set_style_pad_all(button_matrix, 0, 0);
    lv_obj_set_style_pad_row(button_matrix, 1, 0);
    lv_obj_set_style_pad_column(button_matrix, 3, 0);
    lv_obj_set_style_text_font(button_matrix, &calendar_font_zh_16, 0);
    lv_obj_set_style_text_color(button_matrix, lv_color_hex(0x171717), LV_PART_ITEMS);
    lv_obj_set_style_text_color(button_matrix, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(button_matrix, LV_OPA_TRANSP, LV_PART_ITEMS);
    lv_obj_set_style_border_opa(button_matrix, LV_OPA_TRANSP, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(button_matrix, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(button_matrix, 3, LV_PART_ITEMS);
    lv_obj_add_event_cb(button_matrix, calendar_draw_part_begin, LV_EVENT_DRAW_PART_BEGIN, NULL);
}

static void add_month_calendar(lv_obj_t *parent, const calendar_model_t *model)
{
    static const char *weekdays[] = {"一", "二", "三", "四", "五", "六", "日"};
    char text[16];

    lv_obj_t *panel = make_panel(parent, 190, 42, 190, 210);
    snprintf(text, sizeof(text), "%d 年 %d 月", model->year, model->month);
    make_label_box(panel, text, 8, 4, 120, 22);
    snprintf(text, sizeof(text), "%d周", calendar_model_iso_week(model->year, model->month, model->day));
    lv_obj_t *week = make_label_box(panel, text, 138, 4, 44, 22);
    lv_obj_add_style(week, &g_theme.muted, 0);

    lv_obj_t *calendar = lv_calendar_create(panel);
    lv_obj_remove_style_all(calendar);
    lv_obj_set_pos(calendar, 8, 32);
    lv_obj_set_size(calendar, 174, 160);
    lv_calendar_set_day_names(calendar, weekdays);
    lv_calendar_set_showed_date(calendar, (uint32_t)model->year, (uint32_t)model->month);
    lv_calendar_set_today_date(calendar, (uint32_t)model->year, (uint32_t)model->month, (uint32_t)model->day);
    lv_calendar_set_highlighted_dates(calendar, g_highlighted_dates, collect_highlighted_dates(model));

    lv_obj_t *button_matrix = lv_calendar_get_btnmatrix(calendar);
    style_calendar_button_matrix(button_matrix);
}

void calendar_ui_update(calendar_ui_t *ui, const calendar_model_t *model)
{
    char text[96];

    lv_obj_clean(ui->screen);

    snprintf(
        text,
        sizeof(text),
        "Wi-Fi%s · NTP%s · 电%d%%",
        model->wifi_connected ? "" : "离线",
        model->ntp_synced ? "" : "未同",
        model->battery_percent);
    lv_obj_t *status = make_label_box(ui->screen, text, 10, 8, 190, 22);
    lv_obj_add_style(status, &g_theme.muted, 0);

    snprintf(text, sizeof(text), "%s  %d月%d日", model->weekday_text, model->month, model->day);
    make_label(ui->screen, text, 18, 45);

    snprintf(text, sizeof(text), "%02d:%02d", model->hour, model->minute);
    lv_obj_t *time = make_label(ui->screen, text, 18, 78);
    lv_obj_set_style_text_font(time, &calendar_font_fusion_48, 0);

    make_label_box(ui->screen, model->lunar_text, 21, 135, 154, 22);
    lv_obj_t *hint = make_label_box(ui->screen, model->day_hint, 21, 158, 154, 22);
    lv_obj_add_style(hint, &g_theme.muted, 0);

    lv_obj_t *weather = make_panel(ui->screen, 18, 181, 154, 66);
    snprintf(text, sizeof(text), "%s %s", model->city, model->weather_summary);
    make_label_box(weather, text, 8, 4, 138, 22);
    snprintf(text, sizeof(text), "湿%d%%", model->humidity_percent);
    lv_obj_t *humidity = make_label_box(weather, text, 8, 24, 82, 20);
    lv_obj_add_style(humidity, &g_theme.muted, 0);
    snprintf(text, sizeof(text), "%d°C", model->temp_c);
    lv_obj_t *temp = make_label_box(weather, text, 62, 20, 72, 34);
    lv_obj_set_style_text_font(temp, &calendar_font_fusion_28, 0);
    lv_obj_set_style_text_align(temp, LV_TEXT_ALIGN_RIGHT, 0);
    snprintf(text, sizeof(text), "%d-%d°C", model->temp_low_c, model->temp_high_c);
    lv_obj_t *summary = make_label_box(weather, text, 8, 42, 82, 20);
    lv_obj_add_style(summary, &g_theme.muted, 0);
    snprintf(text, sizeof(text), "更新%s", model->weather_updated_at);
    lv_obj_t *updated = make_label_box(weather, text, 92, 42, 54, 20);
    lv_obj_add_style(updated, &g_theme.muted, 0);

    snprintf(text, sizeof(text), "下一项 %s", model->next_event_text);
    lv_obj_t *event = make_label_box(ui->screen, text, 18, 252, 164, 20);
    lv_obj_add_style(event, &g_theme.muted, 0);

    add_month_calendar(ui->screen, model);

    lv_obj_t *assistant = make_bottom_status(ui->screen);
    const char *assistant_text = model->assistant_error[0] != '\0' ? model->assistant_error : model->assistant_caption;
    if (assistant_text[0] == '\0') {
        assistant_text = model->assistant_active ? "等待语音结果" : "可按键语音输入";
    }
    snprintf(text, sizeof(text), "%s %s", model->assistant_state_text, assistant_text);
    make_label_box(assistant, text, 8, 3, 364, 22);
}

void calendar_ui_create(calendar_ui_t *ui, const calendar_model_t *model)
{
    ensure_theme();
    ui->screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(ui->screen);
    lv_obj_add_style(ui->screen, &g_theme.screen, 0);
    lv_obj_set_size(ui->screen, 400, 300);
    calendar_ui_update(ui, model);
    lv_scr_load(ui->screen);
}
