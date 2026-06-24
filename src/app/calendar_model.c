#include "calendar_model.h"

#include <stdio.h>
#include <string.h>

static bool is_leap_year(int year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static int days_in_month(int year, int month)
{
    static const int days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31,
    };

    if (month == 2 && is_leap_year(year)) {
        return 29;
    }

    return days[month - 1];
}

static int day_of_year(int year, int month, int day)
{
    int ordinal = day;
    for (int m = 1; m < month; m++) {
        ordinal += days_in_month(year, m);
    }
    return ordinal;
}

static int weekday_monday_first(int year, int month, int day)
{
    if (month < 3) {
        month += 12;
        year -= 1;
    }

    int k = year % 100;
    int j = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    int sunday_first = (h + 6) % 7;
    return (sunday_first + 6) % 7;
}

static int iso_weeks_in_year(int year)
{
    int jan1_weekday = weekday_monday_first(year, 1, 1);
    return jan1_weekday == 3 || (jan1_weekday == 2 && is_leap_year(year)) ? 53 : 52;
}

calendar_model_t calendar_model_sample(void)
{
    calendar_model_t model = {
        .year = 2026,
        .month = 6,
        .day = 23,
        .hour = 8,
        .minute = 36,
        .weekday_text = "周二",
        .lunar_text = "农历五月初九",
        .day_hint = "今日无雨，适合通勤",
        .city = "北京",
        .weather_summary = "晴转多云",
        .temp_c = 28,
        .temp_low_c = 19,
        .temp_high_c = 31,
        .humidity_percent = 46,
        .battery_percent = 82,
        .wifi_connected = true,
        .ntp_synced = true,
        .weather_updated_at = "08:12",
        .next_event_text = "10:30 项目同步",
        .assistant_state_text = "语音待机",
        .assistant_caption = "",
        .assistant_error = "",
        .assistant_active = false,
        .event_days = {26, 30},
        .event_day_count = 2,
    };

    return model;
}

void calendar_model_status_text(const calendar_model_t *model, char *buffer, size_t buffer_size)
{
    if (buffer_size == 0) {
        return;
    }

    snprintf(
        buffer,
        buffer_size,
        "Wi-Fi %s  NTP %s  天气 %s  电量 %d%%",
        model->wifi_connected ? "已连接" : "离线",
        model->ntp_synced ? "已同步" : "未同步",
        model->weather_updated_at,
        model->battery_percent);
}

int calendar_model_iso_week(int year, int month, int day)
{
    int ordinal = day_of_year(year, month, day);
    int weekday = weekday_monday_first(year, month, day);
    int week = (ordinal - weekday + 9) / 7;

    if (week < 1) {
        return iso_weeks_in_year(year - 1);
    }

    int weeks_this_year = iso_weeks_in_year(year);
    if (week > weeks_this_year) {
        return 1;
    }

    return week;
}
