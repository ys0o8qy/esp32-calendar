#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "calendar_model.h"
#include "calendar_display.h"
#include "calendar_platform.h"
#include "voice_assistant.h"

static const char *TAG = "esp32-calendar";

#if CONFIG_VOICE_ASSISTANT_ENABLE
#define ASSISTANT_TEXT_SIZE 64

typedef struct {
    voice_assistant_event_type_t type;
    char text[ASSISTANT_TEXT_SIZE];
} assistant_event_msg_t;

typedef struct {
    char state_text[ASSISTANT_TEXT_SIZE];
    char caption[ASSISTANT_TEXT_SIZE];
    char error[ASSISTANT_TEXT_SIZE];
    bool active;
} assistant_snapshot_t;

static QueueHandle_t g_assistant_events;
static voice_assistant_handle_t g_assistant;
static assistant_snapshot_t g_assistant_snapshot = {
    .state_text = "语音待机",
    .caption = "",
    .error = "",
    .active = false,
};

static void copy_text(char *dest, size_t dest_size, const char *src)
{
    if (dest_size == 0) {
        return;
    }
    snprintf(dest, dest_size, "%s", src == NULL ? "" : src);
}

static const char *assistant_state_label(voice_assistant_event_type_t type)
{
    switch (type) {
    case VOICE_ASSISTANT_EVENT_CONNECTED:
    case VOICE_ASSISTANT_EVENT_IDLE:
        return "语音待机";
    case VOICE_ASSISTANT_EVENT_LISTENING:
        return "正在聆听";
    case VOICE_ASSISTANT_EVENT_THINKING:
        return "正在思考";
    case VOICE_ASSISTANT_EVENT_SPEAKING:
        return "正在播报";
    case VOICE_ASSISTANT_EVENT_ERROR:
        return "语音错误";
    case VOICE_ASSISTANT_EVENT_TRANSCRIPT_DELTA:
    case VOICE_ASSISTANT_EVENT_ASSISTANT_TEXT:
    case VOICE_ASSISTANT_EVENT_TOOL_CALL:
    default:
        return NULL;
    }
}

static void assistant_event_cb(const voice_assistant_event_t *event, void *user_ctx)
{
    (void)user_ctx;
    if (event == NULL || g_assistant_events == NULL) {
        return;
    }

    assistant_event_msg_t msg = {
        .type = event->type,
    };
    copy_text(msg.text, sizeof(msg.text), event->text);
    xQueueSend(g_assistant_events, &msg, 0);
}

static const char *assistant_token_provider(void *user_ctx)
{
    (void)user_ctx;
    return "";
}

static void assistant_start(void)
{
    if (CONFIG_VOICE_ASSISTANT_BACKEND_URL[0] == '\0') {
        ESP_LOGW(TAG, "Voice assistant disabled because backend URL is empty");
        return;
    }

    g_assistant_events = xQueueCreate(8, sizeof(assistant_event_msg_t));
    if (g_assistant_events == NULL) {
        ESP_LOGE(TAG, "Voice assistant event queue allocation failed");
        return;
    }

    voice_assistant_config_t config = {
        .backend_url = CONFIG_VOICE_ASSISTANT_BACKEND_URL,
        .device_id = CONFIG_VOICE_ASSISTANT_DEVICE_ID,
        .token_provider = assistant_token_provider,
        .event_cb = assistant_event_cb,
        .audio_port = voice_assistant_waveshare_rlcd_4_2_audio_port(),
        .sample_rate_hz = 16000,
        .frame_ms = 60,
        .task_stack_size = 4096,
        .task_priority = 5,
    };

    g_assistant = voice_assistant_start(&config);
    if (g_assistant == NULL) {
        ESP_LOGE(TAG, "Voice assistant start failed");
    }
}

static void assistant_drain_events(void)
{
    if (g_assistant_events == NULL) {
        return;
    }

    assistant_event_msg_t msg;
    while (xQueueReceive(g_assistant_events, &msg, 0) == pdTRUE) {
        const char *state_label = assistant_state_label(msg.type);
        if (state_label != NULL) {
            copy_text(g_assistant_snapshot.state_text, sizeof(g_assistant_snapshot.state_text), state_label);
        }

        switch (msg.type) {
        case VOICE_ASSISTANT_EVENT_LISTENING:
        case VOICE_ASSISTANT_EVENT_THINKING:
        case VOICE_ASSISTANT_EVENT_SPEAKING:
            g_assistant_snapshot.active = true;
            g_assistant_snapshot.error[0] = '\0';
            break;
        case VOICE_ASSISTANT_EVENT_TRANSCRIPT_DELTA:
        case VOICE_ASSISTANT_EVENT_ASSISTANT_TEXT:
            copy_text(g_assistant_snapshot.caption, sizeof(g_assistant_snapshot.caption), msg.text);
            break;
        case VOICE_ASSISTANT_EVENT_ERROR:
            g_assistant_snapshot.active = false;
            copy_text(g_assistant_snapshot.error, sizeof(g_assistant_snapshot.error), msg.text);
            break;
        case VOICE_ASSISTANT_EVENT_CONNECTED:
        case VOICE_ASSISTANT_EVENT_IDLE:
        default:
            g_assistant_snapshot.active = false;
            break;
        }
    }
}

static void assistant_apply_to_model(calendar_model_t *model)
{
    if (model == NULL) {
        return;
    }

    assistant_drain_events();
    model->assistant_state_text = g_assistant_snapshot.state_text;
    model->assistant_caption = g_assistant_snapshot.caption;
    model->assistant_error = g_assistant_snapshot.error;
    model->assistant_active = g_assistant_snapshot.active;
}
#else
static void assistant_start(void)
{
}

static void assistant_apply_to_model(calendar_model_t *model)
{
    (void)model;
}
#endif

void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    ESP_LOGI(TAG, "Booting ESP32-S3-RLCD-4.2 calendar firmware");
    ESP_LOGI(TAG, "Cores: %d, silicon revision: %d", chip_info.cores, chip_info.revision);
    ESP_LOGI(TAG, "Flash: %lu MB", (unsigned long)(flash_size / (1024 * 1024)));
    calendar_platform_init();
    assistant_start();
    calendar_model_t model = calendar_platform_read_model();
    assistant_apply_to_model(&model);
    ESP_ERROR_CHECK(calendar_display_start(&model));

#if CONFIG_SPIRAM
    ESP_LOGI(TAG, "PSRAM initialized: %s", esp_psram_is_initialized() ? "yes" : "no");
    ESP_LOGI(TAG, "Free PSRAM: %u bytes", (unsigned int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#else
    ESP_LOGW(TAG, "PSRAM is not enabled in sdkconfig");
#endif

    while (true) {
        model = calendar_platform_read_model();
        assistant_apply_to_model(&model);
        ESP_ERROR_CHECK(calendar_display_update(&model));
        char status[96];
        calendar_model_status_text(&model, status, sizeof(status));
        ESP_LOGI(TAG, "%s %02d:%02d %s", model.weekday_text, model.hour, model.minute, status);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
