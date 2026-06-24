#include "voice_assistant.h"

#if defined(ESP_PLATFORM) && CONFIG_VOICE_ASSISTANT_ESP_SR_LOCAL_RECOGNIZER

#include "esp_log.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_mn_speech_commands.h"
#include "model_path.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define VOICE_ESP_SR_COMMAND_COUNT 5
#define VOICE_ESP_SR_DURATION_MS 3000
#define VOICE_ESP_SR_THRESHOLD 0.20f

static const char *TAG = "voice_esp_sr";

typedef struct {
    const char *phrase;
    const char *text;
} voice_esp_sr_command_t;

typedef struct {
    srmodel_list_t *models;
    esp_mn_iface_t *multinet;
    model_iface_data_t *model_data;
    bool running;
    bool waiting_command;
    int command_offset;
} voice_esp_sr_ctx_t;

static voice_esp_sr_ctx_t s_esp_sr;

static const voice_esp_sr_command_t s_commands[VOICE_ESP_SR_COMMAND_COUNT] = {
    {CONFIG_VOICE_ASSISTANT_WAKE_PHRASE_PINYIN, CONFIG_VOICE_ASSISTANT_WAKE_PHRASE_TEXT},
    {"xian shi tian qi", "显示天气"},
    {"da kai ri li", "打开日历"},
    {"jin tian ri cheng", "今天日程"},
    {"ting zhi", "停止"},
};

static esp_err_t voice_esp_sr_init(void *ctx)
{
    voice_esp_sr_ctx_t *recognizer = (voice_esp_sr_ctx_t *)ctx;
    if (recognizer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (recognizer->model_data != NULL) {
        return ESP_OK;
    }

    recognizer->models = esp_srmodel_init("model");
    if (recognizer->models == NULL || recognizer->models->num == -1) {
        ESP_LOGE(TAG, "failed to load ESP-SR models from model partition");
        return ESP_FAIL;
    }

    char *mn_name = esp_srmodel_filter(recognizer->models, ESP_MN_PREFIX, "cn");
    if (mn_name == NULL) {
        mn_name = esp_srmodel_filter(recognizer->models, ESP_MN_PREFIX, NULL);
    }
    if (mn_name == NULL) {
        ESP_LOGE(TAG, "no ESP-SR MultiNet model found");
        return ESP_FAIL;
    }

    recognizer->multinet = esp_mn_handle_from_name(mn_name);
    if (recognizer->multinet == NULL) {
        ESP_LOGE(TAG, "failed to resolve MultiNet handle for %s", mn_name);
        return ESP_FAIL;
    }

    recognizer->model_data = recognizer->multinet->create(mn_name, VOICE_ESP_SR_DURATION_MS);
    if (recognizer->model_data == NULL) {
        ESP_LOGE(TAG, "failed to create MultiNet model data");
        return ESP_FAIL;
    }
    recognizer->multinet->set_det_threshold(recognizer->model_data, VOICE_ESP_SR_THRESHOLD);

    esp_mn_commands_clear();
    for (int i = 0; i < VOICE_ESP_SR_COMMAND_COUNT; i++) {
        esp_mn_commands_add(i + 1, s_commands[i].phrase);
    }
    esp_mn_commands_update();
    recognizer->multinet->print_active_speech_commands(recognizer->model_data);
    recognizer->command_offset = 1;
    return ESP_OK;
}

static esp_err_t voice_esp_sr_deinit(void *ctx)
{
    voice_esp_sr_ctx_t *recognizer = (voice_esp_sr_ctx_t *)ctx;
    if (recognizer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (recognizer->model_data != NULL && recognizer->multinet != NULL) {
        recognizer->multinet->destroy(recognizer->model_data);
    }
    if (recognizer->models != NULL) {
        esp_srmodel_deinit(recognizer->models);
    }
    memset(recognizer, 0, sizeof(*recognizer));
    return ESP_OK;
}

static esp_err_t voice_esp_sr_start(void *ctx)
{
    voice_esp_sr_ctx_t *recognizer = (voice_esp_sr_ctx_t *)ctx;
    if (recognizer == NULL || recognizer->model_data == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    recognizer->running = true;
    return ESP_OK;
}

static esp_err_t voice_esp_sr_stop(void *ctx)
{
    voice_esp_sr_ctx_t *recognizer = (voice_esp_sr_ctx_t *)ctx;
    if (recognizer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    recognizer->running = false;
    recognizer->waiting_command = false;
    if (recognizer->model_data != NULL && recognizer->multinet != NULL) {
        recognizer->multinet->clean(recognizer->model_data);
    }
    return ESP_OK;
}

static voice_assistant_local_result_t voice_esp_sr_process_pcm(void *ctx, const int16_t *samples, size_t sample_count)
{
    (void)sample_count;
    voice_esp_sr_ctx_t *recognizer = (voice_esp_sr_ctx_t *)ctx;
    voice_assistant_local_result_t result = {
        .type = VOICE_ASSISTANT_LOCAL_RESULT_NONE,
    };

    if (recognizer == NULL || recognizer->model_data == NULL || recognizer->multinet == NULL ||
        !recognizer->running || samples == NULL) {
        return result;
    }

    esp_mn_state_t state = recognizer->multinet->detect(recognizer->model_data, (int16_t *)samples);
    if (state == ESP_MN_STATE_DETECTING) {
        return result;
    }
    if (state == ESP_MN_STATE_TIMEOUT) {
        recognizer->waiting_command = false;
        recognizer->multinet->clean(recognizer->model_data);
        return result;
    }
    if (state != ESP_MN_STATE_DETECTED) {
        return result;
    }

    esp_mn_results_t *mn_result = recognizer->multinet->get_results(recognizer->model_data);
    recognizer->multinet->clean(recognizer->model_data);
    if (mn_result == NULL || mn_result->num <= 0) {
        return result;
    }

    int command_id = mn_result->command_id[0] - recognizer->command_offset;
    if (command_id < 0 || command_id >= VOICE_ESP_SR_COMMAND_COUNT) {
        return result;
    }
    if (command_id == 0) {
        recognizer->waiting_command = true;
        result.type = VOICE_ASSISTANT_LOCAL_RESULT_WAKE_WORD;
        result.wake_word = s_commands[0].text;
        return result;
    }
    if (!recognizer->waiting_command) {
        return result;
    }

    recognizer->waiting_command = false;
    result.type = VOICE_ASSISTANT_LOCAL_RESULT_COMMAND;
    result.wake_word = s_commands[0].text;
    result.text = s_commands[command_id].text;
    return result;
}

voice_assistant_local_recognizer_t voice_assistant_esp_sr_local_recognizer(void)
{
    voice_assistant_local_recognizer_t recognizer = {
        .init = voice_esp_sr_init,
        .deinit = voice_esp_sr_deinit,
        .start = voice_esp_sr_start,
        .stop = voice_esp_sr_stop,
        .process_pcm = voice_esp_sr_process_pcm,
        .ctx = &s_esp_sr,
    };
    return recognizer;
}

#else

voice_assistant_local_recognizer_t voice_assistant_esp_sr_local_recognizer(void)
{
    voice_assistant_local_recognizer_t recognizer = {0};
    return recognizer;
}

#endif
