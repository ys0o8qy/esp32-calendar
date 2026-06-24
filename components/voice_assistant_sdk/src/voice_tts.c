#include "voice_internal.h"
#include "local_tts.h"

#include <stdlib.h>

#define VOICE_ASSISTANT_TTS_MAX_SAMPLES 96000U

static size_t local_tts_estimate_adapter(void *ctx, const char *text)
{
    return local_tts_estimate_samples((const local_tts_config_t *)ctx, text);
}

static esp_err_t local_tts_synthesize_adapter(
    void *ctx,
    const char *text,
    int16_t *pcm,
    size_t max_samples,
    size_t *written_samples)
{
    return local_tts_synthesize((const local_tts_config_t *)ctx, text, pcm, max_samples, written_samples);
}

voice_assistant_tts_t voice_assistant_local_tts(void)
{
    voice_assistant_tts_t tts = {
        .estimate_samples = local_tts_estimate_adapter,
        .synthesize = local_tts_synthesize_adapter,
        .ctx = NULL,
    };
    return tts;
}

esp_err_t voice_assistant_speak_text(voice_assistant_handle_t handle, const char *text)
{
    if (handle == NULL || text == NULL || text[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (handle->config.tts.estimate_samples == NULL || handle->config.tts.synthesize == NULL ||
        handle->config.audio_port.play_pcm == NULL) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    size_t sample_count = handle->config.tts.estimate_samples(handle->config.tts.ctx, text);
    if (sample_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (sample_count > VOICE_ASSISTANT_TTS_MAX_SAMPLES) {
        return ESP_ERR_INVALID_SIZE;
    }

    int16_t *pcm = (int16_t *)malloc(sample_count * sizeof(*pcm));
    if (pcm == NULL) {
        return ESP_ERR_NO_MEM;
    }

    size_t written_samples = 0;
    esp_err_t synth_ret =
        handle->config.tts.synthesize(handle->config.tts.ctx, text, pcm, sample_count, &written_samples);
    if (synth_ret != ESP_OK || written_samples == 0) {
        free(pcm);
        return synth_ret == ESP_OK ? ESP_ERR_INVALID_STATE : synth_ret;
    }

    voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_SPEAKING);
    esp_err_t play_ret = handle->config.audio_port.play_pcm(handle->config.audio_port.ctx, pcm, written_samples);
    if (handle->config.audio_port.stop_playback != NULL) {
        handle->config.audio_port.stop_playback(handle->config.audio_port.ctx);
    }
    free(pcm);

    if (play_ret != ESP_OK) {
        voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_ERROR);
        return play_ret;
    }
    return voice_assistant_set_state(handle, VOICE_ASSISTANT_STATE_IDLE);
}
