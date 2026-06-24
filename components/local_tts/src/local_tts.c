#include "local_tts.h"

#include <stdbool.h>
#include <string.h>

#define LOCAL_TTS_DEFAULT_SAMPLE_RATE_HZ 16000
#define LOCAL_TTS_DEFAULT_AMPLITUDE 9000
#define LOCAL_TTS_DEFAULT_CHAR_MS 120
#define LOCAL_TTS_DEFAULT_GAP_MS 20
#define LOCAL_TTS_MIN_SAMPLE_RATE_HZ 8000
#define LOCAL_TTS_MAX_SAMPLE_RATE_HZ 24000
#define LOCAL_TTS_MIN_AMPLITUDE 512
#define LOCAL_TTS_MAX_AMPLITUDE 16000
#define LOCAL_TTS_MIN_CHAR_MS 20
#define LOCAL_TTS_MAX_CHAR_MS 240
#define LOCAL_TTS_MIN_GAP_MS 0
#define LOCAL_TTS_MAX_GAP_MS 80

typedef struct {
    uint32_t codepoint;
    size_t bytes;
    bool whitespace;
} local_tts_token_t;

static int clamp_int(int value, int min, int max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

static local_tts_config_t normalize_config(const local_tts_config_t *config)
{
    local_tts_config_t normalized = local_tts_default_config();
    if (config != NULL) {
        normalized = *config;
    }
    normalized.sample_rate_hz =
        clamp_int(normalized.sample_rate_hz, LOCAL_TTS_MIN_SAMPLE_RATE_HZ, LOCAL_TTS_MAX_SAMPLE_RATE_HZ);
    normalized.amplitude = clamp_int(normalized.amplitude, LOCAL_TTS_MIN_AMPLITUDE, LOCAL_TTS_MAX_AMPLITUDE);
    normalized.char_duration_ms =
        clamp_int(normalized.char_duration_ms, LOCAL_TTS_MIN_CHAR_MS, LOCAL_TTS_MAX_CHAR_MS);
    normalized.gap_duration_ms = clamp_int(normalized.gap_duration_ms, LOCAL_TTS_MIN_GAP_MS, LOCAL_TTS_MAX_GAP_MS);
    return normalized;
}

static size_t samples_for_ms(const local_tts_config_t *config, int duration_ms)
{
    return ((size_t)config->sample_rate_hz * (size_t)duration_ms) / 1000U;
}

static bool ascii_whitespace(unsigned char ch)
{
    return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

static local_tts_token_t next_token(const char *text)
{
    const unsigned char *input = (const unsigned char *)text;
    local_tts_token_t token = {
        .codepoint = input[0],
        .bytes = 1,
        .whitespace = ascii_whitespace(input[0]),
    };

    if (input[0] < 0x80) {
        return token;
    }
    if ((input[0] & 0xE0) == 0xC0 && (input[1] & 0xC0) == 0x80) {
        token.codepoint = ((uint32_t)(input[0] & 0x1F) << 6) | (uint32_t)(input[1] & 0x3F);
        token.bytes = 2;
        return token;
    }
    if ((input[0] & 0xF0) == 0xE0 && (input[1] & 0xC0) == 0x80 && (input[2] & 0xC0) == 0x80) {
        token.codepoint = ((uint32_t)(input[0] & 0x0F) << 12) | ((uint32_t)(input[1] & 0x3F) << 6) |
                          (uint32_t)(input[2] & 0x3F);
        token.bytes = 3;
        return token;
    }
    if ((input[0] & 0xF8) == 0xF0 && (input[1] & 0xC0) == 0x80 && (input[2] & 0xC0) == 0x80 &&
        (input[3] & 0xC0) == 0x80) {
        token.codepoint = ((uint32_t)(input[0] & 0x07) << 18) | ((uint32_t)(input[1] & 0x3F) << 12) |
                          ((uint32_t)(input[2] & 0x3F) << 6) | (uint32_t)(input[3] & 0x3F);
        token.bytes = 4;
        return token;
    }
    return token;
}

static int16_t synth_sample(uint32_t codepoint, size_t index, size_t sample_count, const local_tts_config_t *config)
{
    uint32_t frequency = 170U + (codepoint % 260U);
    uint32_t phase = ((uint32_t)index * frequency * 2U) / (uint32_t)config->sample_rate_hz;
    int sign = (phase & 1U) == 0U ? 1 : -1;
    int envelope = config->amplitude;

    size_t fade_samples = sample_count / 8U;
    if (fade_samples > 0U && index < fade_samples) {
        envelope = (int)(((size_t)envelope * index) / fade_samples);
    } else if (fade_samples > 0U && index + fade_samples >= sample_count) {
        size_t remaining = sample_count - index;
        envelope = (int)(((size_t)envelope * remaining) / fade_samples);
    }

    uint32_t buzz = (uint32_t)((index * (codepoint % 17U + 3U)) & 0x1F);
    int shaped = (envelope * (24 + (int)buzz)) / 40;
    return (int16_t)(sign * shaped);
}

local_tts_config_t local_tts_default_config(void)
{
    local_tts_config_t config = {
        .sample_rate_hz = LOCAL_TTS_DEFAULT_SAMPLE_RATE_HZ,
        .amplitude = LOCAL_TTS_DEFAULT_AMPLITUDE,
        .char_duration_ms = LOCAL_TTS_DEFAULT_CHAR_MS,
        .gap_duration_ms = LOCAL_TTS_DEFAULT_GAP_MS,
    };
    return config;
}

size_t local_tts_estimate_samples(const local_tts_config_t *config, const char *text)
{
    if (text == NULL || text[0] == '\0') {
        return 0;
    }

    local_tts_config_t normalized = normalize_config(config);
    size_t char_samples = samples_for_ms(&normalized, normalized.char_duration_ms);
    size_t gap_samples = samples_for_ms(&normalized, normalized.gap_duration_ms);
    size_t total = 0;

    while (*text != '\0') {
        local_tts_token_t token = next_token(text);
        total += token.whitespace ? gap_samples : char_samples + gap_samples;
        text += token.bytes;
    }
    return total;
}

esp_err_t local_tts_synthesize(
    const local_tts_config_t *config,
    const char *text,
    int16_t *pcm,
    size_t max_samples,
    size_t *written_samples)
{
    if (written_samples != NULL) {
        *written_samples = 0;
    }
    if (text == NULL || pcm == NULL || written_samples == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    local_tts_config_t normalized = normalize_config(config);
    size_t required_samples = local_tts_estimate_samples(&normalized, text);
    if (required_samples > max_samples) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (required_samples == 0) {
        return ESP_OK;
    }

    size_t char_samples = samples_for_ms(&normalized, normalized.char_duration_ms);
    size_t gap_samples = samples_for_ms(&normalized, normalized.gap_duration_ms);
    size_t offset = 0;

    while (*text != '\0') {
        local_tts_token_t token = next_token(text);
        if (token.whitespace) {
            memset(&pcm[offset], 0, gap_samples * sizeof(*pcm));
            offset += gap_samples;
        } else {
            for (size_t i = 0; i < char_samples; i++) {
                pcm[offset++] = synth_sample(token.codepoint, i, char_samples, &normalized);
            }
            memset(&pcm[offset], 0, gap_samples * sizeof(*pcm));
            offset += gap_samples;
        }
        text += token.bytes;
    }

    *written_samples = offset;
    return ESP_OK;
}
