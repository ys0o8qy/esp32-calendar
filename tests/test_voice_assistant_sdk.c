#include "voice_assistant.h"
#include "voice_internal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    voice_assistant_event_type_t events[16];
    char payloads[16][64];
    size_t count;
} event_log_t;

typedef struct {
    voice_assistant_local_result_t results[4];
    size_t result_count;
    size_t next_result;
    int init_count;
    int deinit_count;
    int start_count;
    int stop_count;
    int process_count;
} scripted_local_recognizer_t;

typedef struct {
    int start_count;
    int stop_count;
    int read_count;
    int16_t samples[32];
    int read_result;
} scripted_audio_port_t;

static void record_event(const voice_assistant_event_t *event, void *user_ctx)
{
    event_log_t *log = (event_log_t *)user_ctx;
    assert(log->count < 16);
    log->events[log->count] = event->type;
    if (event->text != NULL) {
        snprintf(log->payloads[log->count], sizeof(log->payloads[log->count]), "%s", event->text);
    }
    log->count++;
}

static const char *token_provider(void *user_ctx)
{
    (void)user_ctx;
    return "short-lived-token";
}

static esp_err_t scripted_audio_start_recording(void *ctx)
{
    scripted_audio_port_t *audio = (scripted_audio_port_t *)ctx;
    audio->start_count++;
    return ESP_OK;
}

static esp_err_t scripted_audio_stop_recording(void *ctx)
{
    scripted_audio_port_t *audio = (scripted_audio_port_t *)ctx;
    audio->stop_count++;
    return ESP_OK;
}

static int scripted_audio_read_pcm(void *ctx, int16_t *samples, size_t sample_count)
{
    scripted_audio_port_t *audio = (scripted_audio_port_t *)ctx;
    audio->read_count++;

    size_t count = audio->read_result > 0 ? (size_t)audio->read_result : 0;
    if (count > sample_count) {
        count = sample_count;
    }
    for (size_t i = 0; i < count; i++) {
        samples[i] = audio->samples[i];
    }
    return (int)count;
}

static voice_assistant_audio_port_t make_scripted_audio_port(scripted_audio_port_t *audio)
{
    voice_assistant_audio_port_t port = {
        .start_recording = scripted_audio_start_recording,
        .stop_recording = scripted_audio_stop_recording,
        .read_pcm = scripted_audio_read_pcm,
        .ctx = audio,
    };
    return port;
}

static esp_err_t scripted_local_init(void *ctx)
{
    scripted_local_recognizer_t *recognizer = (scripted_local_recognizer_t *)ctx;
    recognizer->init_count++;
    return ESP_OK;
}

static esp_err_t scripted_local_deinit(void *ctx)
{
    scripted_local_recognizer_t *recognizer = (scripted_local_recognizer_t *)ctx;
    recognizer->deinit_count++;
    return ESP_OK;
}

static esp_err_t scripted_local_start(void *ctx)
{
    scripted_local_recognizer_t *recognizer = (scripted_local_recognizer_t *)ctx;
    recognizer->start_count++;
    return ESP_OK;
}

static esp_err_t scripted_local_stop(void *ctx)
{
    scripted_local_recognizer_t *recognizer = (scripted_local_recognizer_t *)ctx;
    recognizer->stop_count++;
    return ESP_OK;
}

static voice_assistant_local_result_t scripted_local_process(void *ctx, const int16_t *samples, size_t sample_count)
{
    (void)samples;
    assert(sample_count > 0);

    scripted_local_recognizer_t *recognizer = (scripted_local_recognizer_t *)ctx;
    recognizer->process_count++;
    if (recognizer->next_result >= recognizer->result_count) {
        voice_assistant_local_result_t result = {
            .type = VOICE_ASSISTANT_LOCAL_RESULT_NONE,
        };
        return result;
    }
    return recognizer->results[recognizer->next_result++];
}

static voice_assistant_local_recognizer_t make_scripted_local_recognizer(scripted_local_recognizer_t *recognizer)
{
    voice_assistant_local_recognizer_t local = {
        .init = scripted_local_init,
        .deinit = scripted_local_deinit,
        .start = scripted_local_start,
        .stop = scripted_local_stop,
        .process_pcm = scripted_local_process,
        .ctx = recognizer,
    };
    return local;
}

static voice_assistant_config_t make_config(event_log_t *log)
{
    voice_assistant_config_t config = {
        .backend_url = "wss://assistant.example.test/session",
        .device_id = "calendar-test-device",
        .token_provider = token_provider,
        .event_cb = record_event,
        .user_ctx = log,
        .sample_rate_hz = 16000,
        .frame_ms = 60,
        .task_stack_size = 4096,
        .task_priority = 5,
    };
    return config;
}

static void test_start_validates_required_config(void)
{
    event_log_t log = {0};
    voice_assistant_config_t config = make_config(&log);

    config.backend_url = NULL;
    assert(voice_assistant_start(&config) == NULL);

    config = make_config(&log);
    config.device_id = NULL;
    assert(voice_assistant_start(&config) == NULL);

    config = make_config(&log);
    config.event_cb = NULL;
    assert(voice_assistant_start(&config) == NULL);
}

static void test_start_emits_connected_then_idle(void)
{
    event_log_t log = {0};
    voice_assistant_config_t config = make_config(&log);
    voice_assistant_handle_t assistant = voice_assistant_start(&config);

    assert(assistant != NULL);
    assert(voice_assistant_state(assistant) == VOICE_ASSISTANT_STATE_IDLE);
    assert(log.count == 2);
    assert(log.events[0] == VOICE_ASSISTANT_EVENT_CONNECTED);
    assert(log.events[1] == VOICE_ASSISTANT_EVENT_IDLE);
    assert(strstr(voice_assistant_last_outbound_json(assistant), "\"type\":\"hello\"") != NULL);
    assert(strstr(voice_assistant_last_outbound_json(assistant), "\"device_id\":\"calendar-test-device\"") != NULL);

    assert(voice_assistant_stop(assistant) == ESP_OK);
}

static void test_listen_and_abort_emit_state_events(void)
{
    event_log_t log = {0};
    voice_assistant_config_t config = make_config(&log);
    voice_assistant_handle_t assistant = voice_assistant_start(&config);

    assert(assistant != NULL);
    assert(voice_assistant_listen(assistant, VOICE_ASSISTANT_LISTEN_MANUAL) == ESP_OK);
    assert(voice_assistant_state(assistant) == VOICE_ASSISTANT_STATE_LISTENING);
    assert(log.events[2] == VOICE_ASSISTANT_EVENT_LISTENING);
    assert(strstr(voice_assistant_last_outbound_json(assistant), "\"type\":\"listen_start\"") != NULL);
    assert(strstr(voice_assistant_last_outbound_json(assistant), "\"mode\":\"manual\"") != NULL);

    assert(voice_assistant_abort(assistant) == ESP_OK);
    assert(voice_assistant_state(assistant) == VOICE_ASSISTANT_STATE_IDLE);
    assert(log.events[3] == VOICE_ASSISTANT_EVENT_IDLE);
    assert(strstr(voice_assistant_last_outbound_json(assistant), "\"type\":\"abort\"") != NULL);

    assert(voice_assistant_stop(assistant) == ESP_OK);
}

static void test_protocol_parses_backend_messages(void)
{
    event_log_t log = {0};
    voice_assistant_config_t config = make_config(&log);
    voice_assistant_handle_t assistant = voice_assistant_start(&config);

    assert(assistant != NULL);
    assert(voice_assistant_handle_backend_json(assistant, "{\"type\":\"state\",\"state\":\"thinking\"}") == ESP_OK);
    assert(voice_assistant_state(assistant) == VOICE_ASSISTANT_STATE_THINKING);

    assert(voice_assistant_handle_backend_json(assistant, "{\"type\":\"transcript\",\"text\":\"打开日历\"}") == ESP_OK);
    assert(log.events[3] == VOICE_ASSISTANT_EVENT_TRANSCRIPT_DELTA);
    assert(strcmp(log.payloads[3], "打开日历") == 0);

    assert(voice_assistant_handle_backend_json(assistant, "{\"type\":\"assistant_text\",\"text\":\"今天没有日程\"}") == ESP_OK);
    assert(log.events[4] == VOICE_ASSISTANT_EVENT_ASSISTANT_TEXT);
    assert(strcmp(log.payloads[4], "今天没有日程") == 0);

    assert(voice_assistant_handle_backend_json(assistant, "{\"type\":\"error\",\"text\":\"backend unavailable\"}") == ESP_OK);
    assert(voice_assistant_state(assistant) == VOICE_ASSISTANT_STATE_ERROR);
    assert(log.events[5] == VOICE_ASSISTANT_EVENT_ERROR);

    assert(voice_assistant_handle_backend_json(assistant, "{\"type\":\"unknown\"}") == ESP_ERR_NOT_SUPPORTED);
    assert(voice_assistant_handle_backend_json(assistant, "not-json") == ESP_ERR_INVALID_ARG);

    assert(voice_assistant_stop(assistant) == ESP_OK);
}

static void test_local_recognizer_lifecycle_follows_assistant_lifecycle(void)
{
    event_log_t log = {0};
    scripted_local_recognizer_t recognizer = {0};
    voice_assistant_config_t config = make_config(&log);
    config.local_recognizer = make_scripted_local_recognizer(&recognizer);

    voice_assistant_handle_t assistant = voice_assistant_start(&config);

    assert(assistant != NULL);
    assert(recognizer.init_count == 1);
    assert(voice_assistant_stop(assistant) == ESP_OK);
    assert(recognizer.deinit_count == 1);
}

static void test_local_recognizer_allows_backend_free_startup(void)
{
    event_log_t log = {0};
    scripted_local_recognizer_t recognizer = {0};
    voice_assistant_config_t config = make_config(&log);
    config.backend_url = "";
    config.local_recognizer = make_scripted_local_recognizer(&recognizer);

    voice_assistant_handle_t assistant = voice_assistant_start(&config);

    assert(assistant != NULL);
    assert(voice_assistant_state(assistant) == VOICE_ASSISTANT_STATE_IDLE);
    assert(strstr(voice_assistant_last_outbound_json(assistant), "\"type\":\"hello\"") != NULL);
    assert(strstr(voice_assistant_last_outbound_json(assistant), "\"mode\":\"local\"") != NULL);

    assert(voice_assistant_stop(assistant) == ESP_OK);
}

static void test_local_wake_word_detection_starts_wake_word_listening(void)
{
    event_log_t log = {0};
    scripted_local_recognizer_t recognizer = {
        .results = {
            {
                .type = VOICE_ASSISTANT_LOCAL_RESULT_WAKE_WORD,
                .wake_word = "小王小王",
            },
        },
        .result_count = 1,
    };
    voice_assistant_config_t config = make_config(&log);
    config.local_recognizer = make_scripted_local_recognizer(&recognizer);
    voice_assistant_handle_t assistant = voice_assistant_start(&config);
    int16_t samples[16] = {0};

    assert(assistant != NULL);
    assert(voice_assistant_process_audio_frame(assistant, samples, 16) == ESP_OK);
    assert(recognizer.process_count == 1);
    assert(voice_assistant_state(assistant) == VOICE_ASSISTANT_STATE_LISTENING);
    assert(log.events[2] == VOICE_ASSISTANT_EVENT_WAKE_WORD);
    assert(strcmp(log.payloads[2], "小王小王") == 0);
    assert(log.events[3] == VOICE_ASSISTANT_EVENT_LISTENING);
    assert(strstr(voice_assistant_last_outbound_json(assistant), "\"type\":\"listen_start\"") != NULL);
    assert(strstr(voice_assistant_last_outbound_json(assistant), "\"mode\":\"wake_word\"") != NULL);
    assert(strstr(voice_assistant_last_outbound_json(assistant), "\"wake_word\":\"小王小王\"") != NULL);

    assert(voice_assistant_stop(assistant) == ESP_OK);
}

static void test_local_command_detection_emits_transcript_without_backend_asr(void)
{
    event_log_t log = {0};
    scripted_local_recognizer_t recognizer = {
        .results = {
            {
                .type = VOICE_ASSISTANT_LOCAL_RESULT_WAKE_WORD,
                .wake_word = "小王小王",
            },
            {
                .type = VOICE_ASSISTANT_LOCAL_RESULT_COMMAND,
                .wake_word = "小王小王",
                .text = "显示天气",
            },
        },
        .result_count = 2,
    };
    voice_assistant_config_t config = make_config(&log);
    config.local_recognizer = make_scripted_local_recognizer(&recognizer);
    voice_assistant_handle_t assistant = voice_assistant_start(&config);
    int16_t samples[16] = {0};

    assert(assistant != NULL);
    assert(voice_assistant_process_audio_frame(assistant, samples, 16) == ESP_OK);
    assert(voice_assistant_process_audio_frame(assistant, samples, 16) == ESP_OK);
    assert(log.events[4] == VOICE_ASSISTANT_EVENT_TRANSCRIPT_DELTA);
    assert(strcmp(log.payloads[4], "显示天气") == 0);
    assert(log.events[5] == VOICE_ASSISTANT_EVENT_IDLE);
    assert(voice_assistant_state(assistant) == VOICE_ASSISTANT_STATE_IDLE);

    assert(voice_assistant_stop(assistant) == ESP_OK);
}

static void test_audio_poll_feeds_captured_pcm_to_local_recognizer(void)
{
    event_log_t log = {0};
    scripted_audio_port_t audio = {
        .samples = {1, 2, 3, 4},
        .read_result = 4,
    };
    scripted_local_recognizer_t recognizer = {
        .results = {
            {
                .type = VOICE_ASSISTANT_LOCAL_RESULT_WAKE_WORD,
                .wake_word = "小王小王",
            },
        },
        .result_count = 1,
    };
    voice_assistant_config_t config = make_config(&log);
    config.audio_port = make_scripted_audio_port(&audio);
    config.local_recognizer = make_scripted_local_recognizer(&recognizer);
    config.frame_ms = 1;
    voice_assistant_handle_t assistant = voice_assistant_start(&config);

    assert(assistant != NULL);
    assert(voice_assistant_listen(assistant, VOICE_ASSISTANT_LISTEN_WAKE_WORD) == ESP_OK);
    assert(audio.start_count == 1);
    assert(voice_assistant_poll_audio(assistant) == ESP_OK);
    assert(audio.read_count == 1);
    assert(audio.start_count == 1);
    assert(recognizer.process_count == 1);
    assert(log.events[3] == VOICE_ASSISTANT_EVENT_WAKE_WORD);
    assert(strcmp(log.payloads[3], "小王小王") == 0);

    assert(voice_assistant_stop(assistant) == ESP_OK);
}

static void test_tool_registration_requires_name_and_callback(void)
{
    event_log_t log = {0};
    voice_assistant_config_t config = make_config(&log);
    voice_assistant_handle_t assistant = voice_assistant_start(&config);
    voice_assistant_tool_t tool = {
        .name = "calendar.status",
        .description = "Read the current calendar status",
    };

    assert(assistant != NULL);
    assert(voice_assistant_register_tool(assistant, &tool) == ESP_ERR_INVALID_ARG);

    tool.invoke = (voice_assistant_tool_invoke_cb_t)record_event;
    assert(voice_assistant_register_tool(assistant, &tool) == ESP_OK);

    assert(voice_assistant_stop(assistant) == ESP_OK);
}

int main(void)
{
    test_start_validates_required_config();
    test_start_emits_connected_then_idle();
    test_listen_and_abort_emit_state_events();
    test_protocol_parses_backend_messages();
    test_local_recognizer_lifecycle_follows_assistant_lifecycle();
    test_local_recognizer_allows_backend_free_startup();
    test_local_wake_word_detection_starts_wake_word_listening();
    test_local_command_detection_emits_transcript_without_backend_asr();
    test_audio_poll_feeds_captured_pcm_to_local_recognizer();
    test_tool_registration_requires_name_and_callback();
    puts("voice_assistant_sdk tests passed");
    return 0;
}
