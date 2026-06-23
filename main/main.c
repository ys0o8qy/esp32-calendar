#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "calendar_model.h"
#include "calendar_display.h"
#include "calendar_platform.h"

static const char *TAG = "esp32-calendar";

void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    ESP_LOGI(TAG, "Booting ESP32-S3-RLCD-4.2 calendar scaffold");
    ESP_LOGI(TAG, "Cores: %d, silicon revision: %d", chip_info.cores, chip_info.revision);
    ESP_LOGI(TAG, "Flash: %lu MB", (unsigned long)(flash_size / (1024 * 1024)));
    calendar_platform_init();
    calendar_model_t model = calendar_platform_read_model();
    ESP_ERROR_CHECK(calendar_display_start(&model));

#if CONFIG_SPIRAM
    ESP_LOGI(TAG, "PSRAM initialized: %s", esp_psram_is_initialized() ? "yes" : "no");
    ESP_LOGI(TAG, "Free PSRAM: %u bytes", (unsigned int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#else
    ESP_LOGW(TAG, "PSRAM is not enabled in sdkconfig");
#endif

    while (true) {
        model = calendar_platform_read_model();
        char status[96];
        calendar_model_status_text(&model, status, sizeof(status));
        ESP_LOGI(TAG, "%s %02d:%02d %s", model.weekday_text, model.hour, model.minute, status);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
