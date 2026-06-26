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

static void load_screen(lv_obj_t *screen)
{
#if LVGL_VERSION_MAJOR >= 9
    lv_screen_load(screen);
#else
    lv_scr_load(screen);
#endif
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

static void style_calendar_button_matrix(lv_obj_t *button_matrix)
{
    lv_obj_remove_style_all(button_matrix);
    lv_obj_set_size(button_matrix, 168, 152);
    lv_obj_set_style_bg_opa(button_matrix, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(button_matrix, 0, 0);
    lv_obj_set_style_pad_all(button_matrix, 0, 0);
    lv_obj_set_style_pad_row(button_matrix, 2, 0);
    lv_obj_set_style_pad_column(button_matrix, 2, 0);
    lv_obj_set_style_text_font(button_matrix, &calendar_font_zh_16, 0);
    lv_obj_set_style_text_color(button_matrix, lv_color_hex(0x171717), LV_PART_ITEMS);
    lv_obj_set_style_text_color(button_matrix, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(button_matrix, LV_OPA_TRANSP, LV_PART_ITEMS);
    lv_obj_set_style_border_color(button_matrix, lv_color_hex(0x171717), LV_PART_ITEMS);
    lv_obj_set_style_border_width(button_matrix, 1, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_opa(button_matrix, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_radius(button_matrix, 3, LV_PART_ITEMS);
}

static void add_status_bar(lv_obj_t *parent, const calendar_model_t *model)
{
    char text[128];
    calendar_model_status_text(model, text, sizeof(text));

    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_pos(bar, 10, 258);
    lv_obj_set_size(bar, 380, 34);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(0x171717), 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);

    lv_obj_t *label = make_label_box(bar, text, 8, 7, 364, 20);
    lv_obj_add_style(label, &g_theme.muted, 0);
}

static void add_top_status(lv_obj_t *parent, const calendar_model_t *model)
{
    char text[32];

    snprintf(
        text,
        sizeof(text),
        "WiFi %s",
        model->wifi_connected ? "已连接" : (model->wifi_configured ? "未连接" : "未配置"));
    lv_obj_t *wifi = make_label_box(parent, text, 10, 8, 150, 22);
    lv_obj_add_style(wifi, &g_theme.muted, 0);

    const char *battery_format = model->battery_valid ? "电量 %d%%" : "电量 --";
    if (model->battery_valid) {
        snprintf(text, sizeof(text), battery_format, model->battery_percent);
    } else {
        snprintf(text, sizeof(text), "%s", battery_format);
    }
    lv_obj_t *battery = make_label_box(parent, text, 288, 8, 92, 22);
    lv_obj_add_style(battery, &g_theme.muted, 0);
}

static void add_sensor_tile(lv_obj_t *parent, int x, int y, const char *label_text, const char *value_text)
{
    lv_obj_t *panel = make_panel(parent, x, y, 76, 76);

    lv_obj_t *label = make_label_box(panel, label_text, 8, 7, 60, 18);
    lv_obj_add_style(label, &g_theme.muted, 0);

    lv_obj_t *value = make_label_box(panel, value_text, 8, 34, 60, 30);
    lv_obj_set_style_text_font(value, &calendar_font_fusion_28, 0);
}

static void add_sensor_tiles(lv_obj_t *parent, const calendar_model_t *model)
{
    char temp_text[16];
    char humidity_text[16];

    if (model->indoor_valid) {
        snprintf(temp_text, sizeof(temp_text), "%d°C", model->temp_c);
        snprintf(humidity_text, sizeof(humidity_text), "%d%%", model->humidity_percent);
    } else {
        snprintf(temp_text, sizeof(temp_text), "--°C");
        snprintf(humidity_text, sizeof(humidity_text), "--%%");
    }

    add_sensor_tile(parent, 18, 172, "温度", temp_text);
    add_sensor_tile(parent, 102, 172, "湿度", humidity_text);
}

static void add_month_calendar(lv_obj_t *parent, const calendar_model_t *model)
{
    static const char *weekdays[] = {"一", "二", "三", "四", "五", "六", "日"};
    char text[24];

    lv_obj_t *panel = make_panel(parent, 198, 50, 184, 198);
    if (!model->time_valid) {
        make_label_box(panel, "等待同步", 8, 5, 120, 22);
        lv_obj_t *source = make_label_box(panel, "RTC/系统时间", 8, 32, 128, 22);
        lv_obj_add_style(source, &g_theme.muted, 0);
        return;
    }

    snprintf(text, sizeof(text), "%d年%d月", model->year, model->month);
    make_label_box(panel, text, 8, 5, 112, 22);
    snprintf(text, sizeof(text), "%d周", calendar_model_iso_week(model->year, model->month, model->day));
    lv_obj_t *week = make_label_box(panel, text, 134, 5, 42, 22);
    lv_obj_add_style(week, &g_theme.muted, 0);

    lv_obj_t *calendar = lv_calendar_create(panel);
    lv_obj_remove_style_all(calendar);
    lv_obj_set_pos(calendar, 8, 31);
    lv_obj_set_size(calendar, 168, 154);
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
    add_top_status(ui->screen, model);

    if (model->time_valid) {
        snprintf(text, sizeof(text), "%s  %d月%d日", model->weekday_text, model->month, model->day);
    } else {
        snprintf(text, sizeof(text), "等待时间同步");
    }
    make_label(ui->screen, text, 18, 45);

    snprintf(text, sizeof(text), model->time_valid ? "%02d:%02d" : "--:--", model->hour, model->minute);
    lv_obj_t *time = make_label(ui->screen, text, 18, 78);
    lv_obj_set_style_text_font(time, &calendar_font_fusion_48, 0);

    lv_obj_t *hint = make_label_box(ui->screen, model->day_hint, 21, 139, 154, 22);
    lv_obj_add_style(hint, &g_theme.muted, 0);

    add_sensor_tiles(ui->screen, model);
    add_month_calendar(ui->screen, model);
    add_status_bar(ui->screen, model);
}

void calendar_ui_create(calendar_ui_t *ui, const calendar_model_t *model)
{
    ensure_theme();
    ui->screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(ui->screen);
    lv_obj_add_style(ui->screen, &g_theme.screen, 0);
    lv_obj_set_size(ui->screen, 400, 300);
    calendar_ui_update(ui, model);
    load_screen(ui->screen);
}
