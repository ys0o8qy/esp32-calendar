#include "voice_assistant.h"

#if defined(ESP_PLATFORM) && CONFIG_VOICE_ASSISTANT_WAVESHARE_CODEC_PORT

#include "codec_board.h"
#include "codec_init.h"
#include "esp_codec_dev.h"
#include "esp_log.h"

#include <stdbool.h>
#include <string.h>

static const char *TAG = "voice_board_audio";

typedef struct {
    esp_codec_dev_handle_t playback;
    esp_codec_dev_handle_t record;
    bool initialized;
    bool recording_open;
    bool playback_open;
} waveshare_audio_ctx_t;

static waveshare_audio_ctx_t s_audio;

static esp_codec_dev_sample_info_t sample_info(void)
{
    esp_codec_dev_sample_info_t info = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16,
    };
    return info;
}

static esp_err_t waveshare_audio_init(void *ctx)
{
    waveshare_audio_ctx_t *audio = (waveshare_audio_ctx_t *)ctx;
    if (audio == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (audio->initialized) {
        return ESP_OK;
    }

    set_codec_board_type("S3_RLCD_4_2");
    codec_init_cfg_t codec_cfg = {
        .in_mode = CODEC_I2S_MODE_TDM,
        .out_mode = CODEC_I2S_MODE_TDM,
        .in_use_tdm = false,
        .reuse_dev = false,
    };
    if (init_codec(&codec_cfg) != 0) {
        ESP_LOGE(TAG, "failed to initialize ES7210/ES8311 codec board");
        return ESP_FAIL;
    }

    audio->playback = get_playback_handle();
    audio->record = get_record_handle();
    if (audio->record == NULL) {
        ESP_LOGE(TAG, "ES7210 record handle unavailable");
        deinit_codec();
        memset(audio, 0, sizeof(*audio));
        return ESP_FAIL;
    }

    if (audio->playback != NULL) {
        esp_codec_dev_set_out_vol(audio->playback, CONFIG_VOICE_ASSISTANT_DEFAULT_VOLUME);
    }
    esp_codec_dev_set_in_gain(audio->record, 35.0);
    audio->initialized = true;
    return ESP_OK;
}

static esp_err_t waveshare_audio_deinit(void *ctx)
{
    waveshare_audio_ctx_t *audio = (waveshare_audio_ctx_t *)ctx;
    if (audio == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (audio->recording_open && audio->record != NULL) {
        esp_codec_dev_close(audio->record);
    }
    if (audio->playback_open && audio->playback != NULL) {
        esp_codec_dev_close(audio->playback);
    }
    deinit_codec();
    memset(audio, 0, sizeof(*audio));
    return ESP_OK;
}

static esp_err_t waveshare_start_recording(void *ctx)
{
    waveshare_audio_ctx_t *audio = (waveshare_audio_ctx_t *)ctx;
    if (audio == NULL || audio->record == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (audio->recording_open) {
        return ESP_OK;
    }
    esp_codec_dev_sample_info_t info = sample_info();
    int ret = esp_codec_dev_open(audio->record, &info);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "failed to open ES7210 record stream: %d", ret);
        return ESP_FAIL;
    }
    audio->recording_open = true;
    return ESP_OK;
}

static esp_err_t waveshare_stop_recording(void *ctx)
{
    waveshare_audio_ctx_t *audio = (waveshare_audio_ctx_t *)ctx;
    if (audio == NULL || audio->record == NULL || !audio->recording_open) {
        return ESP_OK;
    }
    esp_codec_dev_close(audio->record);
    audio->recording_open = false;
    return ESP_OK;
}

static int waveshare_read_pcm(void *ctx, int16_t *samples, size_t sample_count)
{
    waveshare_audio_ctx_t *audio = (waveshare_audio_ctx_t *)ctx;
    if (audio == NULL || audio->record == NULL || samples == NULL || sample_count == 0) {
        return -1;
    }
    int bytes = (int)(sample_count * sizeof(int16_t));
    int ret = esp_codec_dev_read(audio->record, samples, bytes);
    if (ret != ESP_CODEC_DEV_OK) {
        return -1;
    }
    return (int)sample_count;
}

static esp_err_t waveshare_play_pcm(void *ctx, const int16_t *samples, size_t sample_count)
{
    waveshare_audio_ctx_t *audio = (waveshare_audio_ctx_t *)ctx;
    if (audio == NULL || audio->playback == NULL || samples == NULL || sample_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!audio->playback_open) {
        esp_codec_dev_sample_info_t info = sample_info();
        int open_ret = esp_codec_dev_open(audio->playback, &info);
        if (open_ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "failed to open ES8311 playback stream: %d", open_ret);
            return ESP_FAIL;
        }
        audio->playback_open = true;
    }
    int bytes = (int)(sample_count * sizeof(int16_t));
    int ret = esp_codec_dev_write(audio->playback, (void *)samples, bytes);
    return ret == ESP_CODEC_DEV_OK ? ESP_OK : ESP_FAIL;
}

static esp_err_t waveshare_stop_playback(void *ctx)
{
    waveshare_audio_ctx_t *audio = (waveshare_audio_ctx_t *)ctx;
    if (audio == NULL || audio->playback == NULL || !audio->playback_open) {
        return ESP_OK;
    }
    esp_codec_dev_close(audio->playback);
    audio->playback_open = false;
    return ESP_OK;
}

#endif

voice_assistant_audio_port_t voice_assistant_waveshare_rlcd_4_2_audio_port(void)
{
#if defined(ESP_PLATFORM) && CONFIG_VOICE_ASSISTANT_WAVESHARE_CODEC_PORT
    voice_assistant_audio_port_t port = {
        .init = waveshare_audio_init,
        .deinit = waveshare_audio_deinit,
        .start_recording = waveshare_start_recording,
        .stop_recording = waveshare_stop_recording,
        .read_pcm = waveshare_read_pcm,
        .play_pcm = waveshare_play_pcm,
        .stop_playback = waveshare_stop_playback,
        .ctx = &s_audio,
    };
    return port;
#else
    voice_assistant_audio_port_t port = {0};
    return port;
#endif
}
