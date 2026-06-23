#include "calendar_platform.h"

#include "esp_log.h"

static const char *TAG = "calendar-platform";

void calendar_platform_init(void)
{
    ESP_LOGI(TAG, "platform boundary initialized");
    ESP_LOGI(TAG, "Wi-Fi, NTP, weather HTTP, RTC, and SHTC3 providers will feed calendar_model_t here");
}

calendar_model_t calendar_platform_read_model(void)
{
    return calendar_model_sample();
}
