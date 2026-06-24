#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef ESP_PLATFORM
#include "esp_err.h"
#else
#ifndef ESP_OK
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_INVALID_SIZE 0x104
#define ESP_ERR_NOT_SUPPORTED 0x106
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int sample_rate_hz;
    int amplitude;
    int char_duration_ms;
    int gap_duration_ms;
} local_tts_config_t;

local_tts_config_t local_tts_default_config(void);
size_t local_tts_estimate_samples(const local_tts_config_t *config, const char *text);
esp_err_t local_tts_synthesize(
    const local_tts_config_t *config,
    const char *text,
    int16_t *pcm,
    size_t max_samples,
    size_t *written_samples);

#ifdef __cplusplus
}
#endif
