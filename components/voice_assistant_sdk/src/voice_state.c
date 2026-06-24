#include "voice_internal.h"

const char *voice_assistant_state_text(voice_assistant_state_t state)
{
    switch (state) {
    case VOICE_ASSISTANT_STATE_IDLE:
        return "idle";
    case VOICE_ASSISTANT_STATE_CONNECTING:
        return "connecting";
    case VOICE_ASSISTANT_STATE_LISTENING:
        return "listening";
    case VOICE_ASSISTANT_STATE_THINKING:
        return "thinking";
    case VOICE_ASSISTANT_STATE_SPEAKING:
        return "speaking";
    case VOICE_ASSISTANT_STATE_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

static voice_assistant_event_type_t event_for_state(voice_assistant_state_t state)
{
    switch (state) {
    case VOICE_ASSISTANT_STATE_LISTENING:
        return VOICE_ASSISTANT_EVENT_LISTENING;
    case VOICE_ASSISTANT_STATE_THINKING:
        return VOICE_ASSISTANT_EVENT_THINKING;
    case VOICE_ASSISTANT_STATE_SPEAKING:
        return VOICE_ASSISTANT_EVENT_SPEAKING;
    case VOICE_ASSISTANT_STATE_ERROR:
        return VOICE_ASSISTANT_EVENT_ERROR;
    case VOICE_ASSISTANT_STATE_IDLE:
    case VOICE_ASSISTANT_STATE_CONNECTING:
    default:
        return VOICE_ASSISTANT_EVENT_IDLE;
    }
}

esp_err_t voice_assistant_set_state(voice_assistant_handle_t handle, voice_assistant_state_t state)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    handle->state = state;
    voice_assistant_emit(handle, event_for_state(state), state, NULL, NULL, NULL);
    return ESP_OK;
}
