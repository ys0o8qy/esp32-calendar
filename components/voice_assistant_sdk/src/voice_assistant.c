#include "voice_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool valid_string(const char *value)
{
    return value != NULL && value[0] != '\0';
}

static void copy_string(char *dest, size_t dest_size, const char *src)
{
    if (dest_size == 0) {
        return;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    snprintf(dest, dest_size, "%s", src);
}

void voice_assistant_emit(
    voice_assistant_handle_t handle,
    voice_assistant_event_type_t type,
    voice_assistant_state_t state,
    const char *text,
    const char *tool_name,
    const char *arguments_json)
{
    if (handle == NULL || handle->config.event_cb == NULL) {
        return;
    }

    voice_assistant_event_t event = {
        .type = type,
        .state = state,
        .text = text,
        .tool_name = tool_name,
        .arguments_json = arguments_json,
    };
    handle->config.event_cb(&event, handle->config.user_ctx);
}

voice_assistant_handle_t voice_assistant_start(const voice_assistant_config_t *config)
{
    if (config == NULL || !valid_string(config->backend_url) || !valid_string(config->device_id) ||
        config->event_cb == NULL) {
        return NULL;
    }

    voice_assistant_handle_t handle = (voice_assistant_handle_t)calloc(1, sizeof(*handle));
    if (handle == NULL) {
        return NULL;
    }

    handle->config = *config;
    copy_string(handle->backend_url, sizeof(handle->backend_url), config->backend_url);
    copy_string(handle->device_id, sizeof(handle->device_id), config->device_id);
    handle->config.backend_url = handle->backend_url;
    handle->config.device_id = handle->device_id;
    handle->state = VOICE_ASSISTANT_STATE_CONNECTING;

    if (handle->config.audio_port.init != NULL && handle->config.audio_port.init(handle->config.audio_port.ctx) != ESP_OK) {
        free(handle);
        return NULL;
    }

    if (voice_assistant_transport_connect(handle) != ESP_OK) {
        if (handle->config.audio_port.deinit != NULL) {
            handle->config.audio_port.deinit(handle->config.audio_port.ctx);
        }
        free(handle);
        return NULL;
    }

    voice_assistant_emit(handle, VOICE_ASSISTANT_EVENT_CONNECTED, VOICE_ASSISTANT_STATE_CONNECTING, NULL, NULL, NULL);
    handle->state = VOICE_ASSISTANT_STATE_IDLE;
    voice_assistant_emit(handle, VOICE_ASSISTANT_EVENT_IDLE, VOICE_ASSISTANT_STATE_IDLE, NULL, NULL, NULL);
    return handle;
}

esp_err_t voice_assistant_stop(voice_assistant_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    voice_assistant_audio_stop(handle);
    if (handle->config.audio_port.deinit != NULL) {
        handle->config.audio_port.deinit(handle->config.audio_port.ctx);
    }
    free(handle);
    return ESP_OK;
}

esp_err_t voice_assistant_listen(voice_assistant_handle_t handle, voice_assistant_listen_mode_t mode)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (mode != VOICE_ASSISTANT_LISTEN_MANUAL && mode != VOICE_ASSISTANT_LISTEN_WAKE_WORD) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t audio_ret = voice_assistant_audio_start(handle);
    if (audio_ret != ESP_OK) {
        voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_ERROR);
        return audio_ret;
    }

    const char *mode_text = mode == VOICE_ASSISTANT_LISTEN_WAKE_WORD ? "wake_word" : "manual";
    char json[96];
    snprintf(json, sizeof(json), "{\"type\":\"listen_start\",\"mode\":\"%s\"}", mode_text);
    voice_assistant_transport_send_json(handle, json);
    return voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_LISTENING);
}

esp_err_t voice_assistant_abort(voice_assistant_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    voice_assistant_audio_stop(handle);
    voice_assistant_transport_send_json(handle, "{\"type\":\"abort\"}");
    return voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_IDLE);
}

esp_err_t voice_assistant_register_tool(voice_assistant_handle_t handle, const voice_assistant_tool_t *tool)
{
    if (handle == NULL || tool == NULL || !valid_string(tool->name) || tool->invoke == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handle->tool_count >= VOICE_ASSISTANT_MAX_TOOLS) {
        return ESP_ERR_NO_MEM;
    }

    handle->tools[handle->tool_count++] = *tool;
    return ESP_OK;
}

voice_assistant_state_t voice_assistant_state(voice_assistant_handle_t handle)
{
    return handle == NULL ? VOICE_ASSISTANT_STATE_ERROR : handle->state;
}
