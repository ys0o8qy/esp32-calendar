#include "local_tts.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static bool pcm_contains_nonzero_sample(const int16_t *pcm, size_t sample_count)
{
    for (size_t i = 0; i < sample_count; i++) {
        if (pcm[i] != 0) {
            return true;
        }
    }
    return false;
}

static void test_estimate_samples_counts_utf8_text_and_gaps(void)
{
    local_tts_config_t config = local_tts_default_config();
    config.sample_rate_hz = 8000;
    config.char_duration_ms = 20;
    config.gap_duration_ms = 5;

    size_t samples = local_tts_estimate_samples(&config, "天气晴");

    assert(samples == 600);
}

static void test_synthesize_generates_nonzero_pcm_for_chinese_text(void)
{
    local_tts_config_t config = local_tts_default_config();
    config.sample_rate_hz = 8000;
    config.char_duration_ms = 20;
    config.gap_duration_ms = 5;
    int16_t pcm[1024] = {0};
    size_t written = 0;

    assert(local_tts_synthesize(&config, "天气晴", pcm, 1024, &written) == ESP_OK);

    assert(written == 600);
    assert(pcm_contains_nonzero_sample(pcm, written));
}

static void test_synthesize_rejects_too_small_buffer(void)
{
    local_tts_config_t config = local_tts_default_config();
    config.sample_rate_hz = 8000;
    config.char_duration_ms = 20;
    config.gap_duration_ms = 5;
    int16_t pcm[8] = {0};
    size_t written = 99;

    assert(local_tts_synthesize(&config, "天气晴", pcm, 8, &written) == ESP_ERR_INVALID_SIZE);
    assert(written == 0);
}

int main(void)
{
    test_estimate_samples_counts_utf8_text_and_gaps();
    test_synthesize_generates_nonzero_pcm_for_chinese_text();
    test_synthesize_rejects_too_small_buffer();
    puts("local_tts tests passed");
    return 0;
}
