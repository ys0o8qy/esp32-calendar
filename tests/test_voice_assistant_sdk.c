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
    test_tool_registration_requires_name_and_callback();
    puts("voice_assistant_sdk tests passed");
    return 0;
}
