#include "voice_internal.h"

esp_err_t voice_assistant_audio_start(voice_assistant_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handle->config.audio_port.start_recording == NULL) {
        return ESP_OK;
    }
    return handle->config.audio_port.start_recording(handle->config.audio_port.ctx);
}

esp_err_t voice_assistant_audio_stop(voice_assistant_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handle->config.audio_port.stop_playback != NULL) {
        handle->config.audio_port.stop_playback(handle->config.audio_port.ctx);
    }
    if (handle->config.audio_port.stop_recording != NULL) {
        return handle->config.audio_port.stop_recording(handle->config.audio_port.ctx);
    }
    return ESP_OK;
}
