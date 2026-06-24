#include "voice_internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool extract_json_string(const char *json, const char *key, char *out, size_t out_size)
{
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);

    const char *start = strstr(json, pattern);
    if (start == NULL || out_size == 0) {
        return false;
    }
    start += strlen(pattern);

    const char *end = strchr(start, '"');
    if (end == NULL) {
        return false;
    }

    size_t len = (size_t)(end - start);
    if (len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out, start, len);
    out[len] = '\0';
    return true;
}

static esp_err_t apply_state(voice_assistant_handle_t handle, const char *state)
{
    if (strcmp(state, "idle") == 0) {
        return voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_IDLE);
    }
    if (strcmp(state, "listening") == 0) {
        return voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_LISTENING);
    }
    if (strcmp(state, "thinking") == 0) {
        return voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_THINKING);
    }
    if (strcmp(state, "speaking") == 0) {
        return voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_SPEAKING);
    }
    if (strcmp(state, "error") == 0) {
        return voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_ERROR);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t voice_assistant_handle_backend_json(voice_assistant_handle_t handle, const char *json)
{
    char type[32];
    char value[VOICE_ASSISTANT_MAX_TEXT];

    if (handle == NULL || json == NULL || json[0] != '{') {
        return ESP_ERR_INVALID_ARG;
    }
    if (!extract_json_string(json, "type", type, sizeof(type))) {
        return ESP_ERR_INVALID_ARG;
    }

    if (strcmp(type, "state") == 0) {
        if (!extract_json_string(json, "state", value, sizeof(value))) {
            return ESP_ERR_INVALID_ARG;
        }
        return apply_state(handle, value);
    }

    if (strcmp(type, "transcript") == 0) {
        if (!extract_json_string(json, "text", handle->last_text, sizeof(handle->last_text))) {
            return ESP_ERR_INVALID_ARG;
        }
        voice_assistant_emit(
            handle,
            VOICE_ASSISTANT_EVENT_TRANSCRIPT_DELTA,
            handle->state,
            handle->last_text,
            NULL,
            NULL);
        return ESP_OK;
    }

    if (strcmp(type, "assistant_text") == 0) {
        if (!extract_json_string(json, "text", handle->last_text, sizeof(handle->last_text))) {
            return ESP_ERR_INVALID_ARG;
        }
        voice_assistant_emit(
            handle,
            VOICE_ASSISTANT_EVENT_ASSISTANT_TEXT,
            handle->state,
            handle->last_text,
            NULL,
            NULL);
        return ESP_OK;
    }

    if (strcmp(type, "tool_call") == 0) {
        char tool_name[64];
        char arguments[VOICE_ASSISTANT_MAX_TEXT];
        if (!extract_json_string(json, "name", tool_name, sizeof(tool_name))) {
            return ESP_ERR_INVALID_ARG;
        }
        if (!extract_json_string(json, "arguments", arguments, sizeof(arguments))) {
            arguments[0] = '\0';
        }
        voice_assistant_emit(
            handle,
            VOICE_ASSISTANT_EVENT_TOOL_CALL,
            handle->state,
            NULL,
            tool_name,
            arguments);
        return ESP_OK;
    }

    if (strcmp(type, "error") == 0) {
        if (!extract_json_string(json, "text", handle->last_text, sizeof(handle->last_text))) {
            handle->last_text[0] = '\0';
        }
        handle->state = VOICE_ASSISTANT_STATE_ERROR;
        voice_assistant_emit(
            handle,
            VOICE_ASSISTANT_EVENT_ERROR,
            VOICE_ASSISTANT_STATE_ERROR,
            handle->last_text,
            NULL,
            NULL);
        return ESP_OK;
    }

    return ESP_ERR_NOT_SUPPORTED;
}
