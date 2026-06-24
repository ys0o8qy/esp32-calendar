#include "calendar_platform.h"

#include <stdio.h>
#include <time.h>

#include "esp_log.h"

static const char *TAG = "calendar-platform";
static const char *WEEKDAYS[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
static char g_weather_updated_at[6];

void calendar_platform_init(void)
{
    ESP_LOGI(TAG, "platform boundary initialized");
    ESP_LOGI(TAG, "using RTC/system clock model until Wi-Fi, NTP, weather, and sensor providers are configured");
}

calendar_model_t calendar_platform_read_model(void)
{
    time_t now = time(NULL);
    struct tm local_time;
    localtime_r(&now, &local_time);

    bool time_valid = local_time.tm_year + 1900 >= 2024;
    if (time_valid) {
        snprintf(g_weather_updated_at, sizeof(g_weather_updated_at), "%02d:%02d", local_time.tm_hour, local_time.tm_min);
    } else {
        snprintf(g_weather_updated_at, sizeof(g_weather_updated_at), "--:--");
    }

    calendar_model_t model = {
        .year = local_time.tm_year + 1900,
        .month = local_time.tm_mon + 1,
        .day = local_time.tm_mday,
        .hour = local_time.tm_hour,
        .minute = local_time.tm_min,
        .weekday_text = WEEKDAYS[local_time.tm_wday],
        .lunar_text = "农历待同步",
        .day_hint = time_valid ? "本地时间" : "等待RTC/NTP同步",
        .city = "本地",
        .weather_summary = "暂无天气",
        .temp_c = 0,
        .temp_low_c = 0,
        .temp_high_c = 0,
        .humidity_percent = 0,
        .battery_percent = 0,
        .wifi_connected = false,
        .ntp_synced = time_valid,
        .weather_updated_at = g_weather_updated_at,
        .next_event_text = "暂无日程",
        .event_day_count = 0,
    };

    return model;
}
