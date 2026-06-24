#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef ESP_PLATFORM
#include "esp_err.h"
#else
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_NOT_SUPPORTED 0x106
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct voice_assistant *voice_assistant_handle_t;

typedef enum {
    VOICE_ASSISTANT_STATE_IDLE = 0,
    VOICE_ASSISTANT_STATE_CONNECTING,
    VOICE_ASSISTANT_STATE_LISTENING,
    VOICE_ASSISTANT_STATE_THINKING,
    VOICE_ASSISTANT_STATE_SPEAKING,
    VOICE_ASSISTANT_STATE_ERROR,
} voice_assistant_state_t;

typedef enum {
    VOICE_ASSISTANT_LISTEN_MANUAL = 0,
    VOICE_ASSISTANT_LISTEN_WAKE_WORD,
} voice_assistant_listen_mode_t;

typedef enum {
    VOICE_ASSISTANT_EVENT_CONNECTED = 0,
    VOICE_ASSISTANT_EVENT_LISTENING,
    VOICE_ASSISTANT_EVENT_THINKING,
    VOICE_ASSISTANT_EVENT_SPEAKING,
    VOICE_ASSISTANT_EVENT_IDLE,
    VOICE_ASSISTANT_EVENT_WAKE_WORD,
    VOICE_ASSISTANT_EVENT_TRANSCRIPT_DELTA,
    VOICE_ASSISTANT_EVENT_ASSISTANT_TEXT,
    VOICE_ASSISTANT_EVENT_TOOL_CALL,
    VOICE_ASSISTANT_EVENT_ERROR,
} voice_assistant_event_type_t;

typedef struct {
    voice_assistant_event_type_t type;
    voice_assistant_state_t state;
    const char *text;
    const char *tool_name;
    const char *arguments_json;
} voice_assistant_event_t;

typedef void (*voice_assistant_event_cb_t)(const voice_assistant_event_t *event, void *user_ctx);
typedef const char *(*voice_assistant_token_provider_t)(void *user_ctx);
typedef esp_err_t (*voice_assistant_tool_invoke_cb_t)(
    const char *arguments_json,
    char *response_json,
    size_t response_size,
    void *user_ctx);

typedef enum {
    VOICE_ASSISTANT_LOCAL_RESULT_NONE = 0,
    VOICE_ASSISTANT_LOCAL_RESULT_WAKE_WORD,
    VOICE_ASSISTANT_LOCAL_RESULT_COMMAND,
} voice_assistant_local_result_type_t;

typedef struct {
    voice_assistant_local_result_type_t type;
    const char *wake_word;
    const char *text;
} voice_assistant_local_result_t;

typedef struct {
    esp_err_t (*init)(void *ctx);
    esp_err_t (*deinit)(void *ctx);
    esp_err_t (*start)(void *ctx);
    esp_err_t (*stop)(void *ctx);
    voice_assistant_local_result_t (*process_pcm)(void *ctx, const int16_t *samples, size_t sample_count);
    void *ctx;
} voice_assistant_local_recognizer_t;

typedef struct {
    esp_err_t (*init)(void *ctx);
    esp_err_t (*deinit)(void *ctx);
    esp_err_t (*start_recording)(void *ctx);
    esp_err_t (*stop_recording)(void *ctx);
    int (*read_pcm)(void *ctx, int16_t *samples, size_t sample_count);
    esp_err_t (*play_pcm)(void *ctx, const int16_t *samples, size_t sample_count);
    esp_err_t (*stop_playback)(void *ctx);
    void *ctx;
} voice_assistant_audio_port_t;

typedef struct {
    const char *name;
    const char *description;
    voice_assistant_tool_invoke_cb_t invoke;
    void *user_ctx;
} voice_assistant_tool_t;

typedef struct {
    const char *backend_url;
    const char *device_id;
    voice_assistant_token_provider_t token_provider;
    voice_assistant_event_cb_t event_cb;
    void *user_ctx;
    voice_assistant_audio_port_t audio_port;
    voice_assistant_local_recognizer_t local_recognizer;
    int sample_rate_hz;
    int frame_ms;
    size_t task_stack_size;
    int task_priority;
} voice_assistant_config_t;

voice_assistant_handle_t voice_assistant_start(const voice_assistant_config_t *config);
esp_err_t voice_assistant_stop(voice_assistant_handle_t handle);
esp_err_t voice_assistant_listen(voice_assistant_handle_t handle, voice_assistant_listen_mode_t mode);
esp_err_t voice_assistant_abort(voice_assistant_handle_t handle);
esp_err_t voice_assistant_register_tool(voice_assistant_handle_t handle, const voice_assistant_tool_t *tool);
esp_err_t voice_assistant_poll_audio(voice_assistant_handle_t handle);
esp_err_t voice_assistant_process_audio_frame(voice_assistant_handle_t handle, const int16_t *samples, size_t sample_count);
voice_assistant_state_t voice_assistant_state(voice_assistant_handle_t handle);
const char *voice_assistant_state_text(voice_assistant_state_t state);
voice_assistant_audio_port_t voice_assistant_waveshare_rlcd_4_2_audio_port(void);
voice_assistant_local_recognizer_t voice_assistant_esp_sr_local_recognizer(void);

#ifdef __cplusplus
}
#endif
