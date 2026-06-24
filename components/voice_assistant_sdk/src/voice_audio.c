#include "voice_internal.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#define VOICE_ASSISTANT_MAX_FRAME_SAMPLES 1024
#define VOICE_ASSISTANT_DEFAULT_TASK_STACK_SIZE 4096
#define VOICE_ASSISTANT_DEFAULT_TASK_PRIORITY 5
#define VOICE_ASSISTANT_IDLE_POLL_DELAY_MS 10

static bool has_pollable_audio(voice_assistant_handle_t handle)
{
    return handle != NULL && handle->config.audio_port.read_pcm != NULL &&
           handle->config.local_recognizer.process_pcm != NULL;
}

#ifdef ESP_PLATFORM
static void voice_assistant_audio_task(void *arg)
{
    voice_assistant_handle_t handle = (voice_assistant_handle_t)arg;

    while (handle != NULL && handle->audio_task_running) {
        esp_err_t ret = voice_assistant_poll_audio(handle);
        if (ret != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(VOICE_ASSISTANT_IDLE_POLL_DELAY_MS));
        }
    }

    if (handle != NULL) {
        handle->audio_task = NULL;
    }
    vTaskDelete(NULL);
}

static esp_err_t start_audio_task(voice_assistant_handle_t handle)
{
    if (!has_pollable_audio(handle) || handle->audio_task != NULL) {
        return ESP_OK;
    }

    size_t stack_size = handle->config.task_stack_size > 0 ? handle->config.task_stack_size :
                                                              VOICE_ASSISTANT_DEFAULT_TASK_STACK_SIZE;
    int priority = handle->config.task_priority > 0 ? handle->config.task_priority :
                                                      VOICE_ASSISTANT_DEFAULT_TASK_PRIORITY;
    handle->audio_task_running = true;
    BaseType_t created = xTaskCreate(
        voice_assistant_audio_task,
        "voice_audio",
        (uint32_t)stack_size,
        handle,
        (UBaseType_t)priority,
        &handle->audio_task);
    if (created != pdPASS) {
        handle->audio_task_running = false;
        handle->audio_task = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static void stop_audio_task(voice_assistant_handle_t handle)
{
    if (handle == NULL || handle->audio_task == NULL) {
        return;
    }

    TaskHandle_t current = xTaskGetCurrentTaskHandle();
    handle->audio_task_running = false;
    if (current == handle->audio_task) {
        return;
    }
    for (int i = 0; i < 20 && handle->audio_task != NULL; i++) {
        vTaskDelay(pdMS_TO_TICKS(VOICE_ASSISTANT_IDLE_POLL_DELAY_MS));
    }
}
#endif

esp_err_t voice_assistant_audio_start(voice_assistant_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!handle->audio_recording) {
        if (handle->config.audio_port.start_recording != NULL) {
            esp_err_t ret = handle->config.audio_port.start_recording(handle->config.audio_port.ctx);
            if (ret != ESP_OK) {
                return ret;
            }
        }
        handle->audio_recording = true;
    }
    if (!handle->local_recognizer_running) {
        if (handle->config.local_recognizer.start != NULL) {
            esp_err_t ret = handle->config.local_recognizer.start(handle->config.local_recognizer.ctx);
            if (ret != ESP_OK) {
                if (handle->audio_recording && handle->config.audio_port.stop_recording != NULL) {
                    handle->config.audio_port.stop_recording(handle->config.audio_port.ctx);
                }
                handle->audio_recording = false;
                return ret;
            }
        }
        handle->local_recognizer_running = true;
    }

#ifdef ESP_PLATFORM
    esp_err_t task_ret = start_audio_task(handle);
    if (task_ret != ESP_OK) {
        voice_assistant_audio_stop(handle);
    }
    return task_ret;
#else
    return ESP_OK;
#endif
}

esp_err_t voice_assistant_audio_stop(voice_assistant_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
#ifdef ESP_PLATFORM
    stop_audio_task(handle);
#endif
    if (handle->local_recognizer_running && handle->config.local_recognizer.stop != NULL) {
        handle->config.local_recognizer.stop(handle->config.local_recognizer.ctx);
    }
    handle->local_recognizer_running = false;
    if (handle->config.audio_port.stop_playback != NULL) {
        handle->config.audio_port.stop_playback(handle->config.audio_port.ctx);
    }
    if (handle->audio_recording && handle->config.audio_port.stop_recording != NULL) {
        esp_err_t ret = handle->config.audio_port.stop_recording(handle->config.audio_port.ctx);
        handle->audio_recording = false;
        return ret;
    }
    handle->audio_recording = false;
    return ESP_OK;
}

esp_err_t voice_assistant_poll_audio(voice_assistant_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handle->config.audio_port.read_pcm == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int frame_samples = 960;
    if (handle->config.sample_rate_hz > 0 && handle->config.frame_ms > 0) {
        frame_samples = (handle->config.sample_rate_hz * handle->config.frame_ms) / 1000;
    }
    if (frame_samples <= 0 || frame_samples > VOICE_ASSISTANT_MAX_FRAME_SAMPLES) {
        return ESP_ERR_INVALID_ARG;
    }

    int16_t samples[VOICE_ASSISTANT_MAX_FRAME_SAMPLES];
    int read = handle->config.audio_port.read_pcm(handle->config.audio_port.ctx, samples, (size_t)frame_samples);
    if (read <= 0) {
        return ESP_FAIL;
    }

    return voice_assistant_process_audio_frame(handle, samples, (size_t)read);
}
