#pragma once

#include "voice_assistant.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#define VOICE_ASSISTANT_MAX_TOOLS 4
#define VOICE_ASSISTANT_MAX_TEXT 96

struct voice_assistant {
    voice_assistant_config_t config;
    voice_assistant_state_t state;
    voice_assistant_tool_t tools[VOICE_ASSISTANT_MAX_TOOLS];
    size_t tool_count;
    char backend_url[128];
    char device_id[64];
    char last_text[VOICE_ASSISTANT_MAX_TEXT];
    char last_wake_word[VOICE_ASSISTANT_MAX_TEXT];
    char last_outbound_json[192];
    bool audio_recording;
    bool local_recognizer_running;
#ifdef ESP_PLATFORM
    TaskHandle_t audio_task;
    bool audio_task_running;
#endif
};

void voice_assistant_emit(
    voice_assistant_handle_t handle,
    voice_assistant_event_type_t type,
    voice_assistant_state_t state,
    const char *text,
    const char *tool_name,
    const char *arguments_json);
esp_err_t voice_assistant_set_state(voice_assistant_handle_t handle, voice_assistant_state_t state);
esp_err_t voice_assistant_handle_backend_json(voice_assistant_handle_t handle, const char *json);
esp_err_t voice_assistant_audio_start(voice_assistant_handle_t handle);
esp_err_t voice_assistant_audio_stop(voice_assistant_handle_t handle);
esp_err_t voice_assistant_transport_connect(voice_assistant_handle_t handle);
esp_err_t voice_assistant_transport_send_json(voice_assistant_handle_t handle, const char *json);
const char *voice_assistant_last_outbound_json(voice_assistant_handle_t handle);
