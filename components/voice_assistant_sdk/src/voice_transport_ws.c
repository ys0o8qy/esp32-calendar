#include "voice_internal.h"

#include <stdio.h>
#include <string.h>

esp_err_t voice_assistant_transport_connect(voice_assistant_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->backend_url[0] == '\0') {
        snprintf(
            handle->last_outbound_json,
            sizeof(handle->last_outbound_json),
            "{\"type\":\"hello\",\"device_id\":\"%s\",\"sample_rate\":%d,\"frame_ms\":%d,\"mode\":\"local\"}",
            handle->device_id,
            handle->config.sample_rate_hz > 0 ? handle->config.sample_rate_hz : 16000,
            handle->config.frame_ms > 0 ? handle->config.frame_ms : 60);
        return ESP_OK;
    }

    const char *token = NULL;
    if (handle->config.token_provider != NULL) {
        token = handle->config.token_provider(handle->config.user_ctx);
    }

    snprintf(
        handle->last_outbound_json,
        sizeof(handle->last_outbound_json),
        "{\"type\":\"hello\",\"device_id\":\"%s\",\"sample_rate\":%d,\"frame_ms\":%d,\"token\":\"%s\"}",
        handle->device_id,
        handle->config.sample_rate_hz > 0 ? handle->config.sample_rate_hz : 16000,
        handle->config.frame_ms > 0 ? handle->config.frame_ms : 60,
        token == NULL ? "" : token);
    return ESP_OK;
}

esp_err_t voice_assistant_transport_send_json(voice_assistant_handle_t handle, const char *json)
{
    if (handle == NULL || json == NULL || json[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    snprintf(handle->last_outbound_json, sizeof(handle->last_outbound_json), "%s", json);
    return ESP_OK;
}

const char *voice_assistant_last_outbound_json(voice_assistant_handle_t handle)
{
    if (handle == NULL) {
        return "";
    }
    return handle->last_outbound_json;
}
