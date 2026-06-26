#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "calendar_model.h"

void test_png_writer(void);

static void test_status_text_mentions_board_data_sources(void)
{
    calendar_model_t model = calendar_model_sample();
    char buffer[96];

    calendar_model_status_text(&model, buffer, sizeof(buffer));

    assert(strstr(buffer, "RTC") != NULL);
    assert(strstr(buffer, "SHTC3") != NULL);
    assert(strstr(buffer, "已读取") != NULL);
    assert(strstr(buffer, "麦克风 开启") != NULL);
}

static void test_sample_model_initializes_board_data_fields(void)
{
    calendar_model_t model = calendar_model_sample();

    assert(model.time_valid == true);
    assert(model.rtc_available == true);
    assert(model.shtc3_available == true);
    assert(model.indoor_valid == true);
    assert(model.wifi_configured == true);
    assert(model.wifi_connected == true);
    assert(model.battery_valid == true);
    assert(model.mic_muted == false);
    assert(model.assistant_state == CALENDAR_ASSISTANT_IDLE);
    assert(strcmp(model.assistant_detail, "") == 0);
    assert(model.battery_percent == 82);
    assert(strcmp(model.day_hint, "时间已同步") == 0);
    assert(model.event_day_count == 1);
}

static void test_assistant_state_text(void)
{
    assert(strcmp(calendar_model_assistant_state_text(CALENDAR_ASSISTANT_IDLE), "待命") == 0);
    assert(strcmp(calendar_model_assistant_state_text(CALENDAR_ASSISTANT_LISTENING), "正在听") == 0);
    assert(strcmp(calendar_model_assistant_state_text(CALENDAR_ASSISTANT_THINKING), "思考中") == 0);
    assert(strcmp(calendar_model_assistant_state_text(CALENDAR_ASSISTANT_SPEAKING), "回复中") == 0);
    assert(strcmp(calendar_model_assistant_state_text(CALENDAR_ASSISTANT_DONE), "已完成") == 0);
    assert(strcmp(calendar_model_assistant_state_text(CALENDAR_ASSISTANT_ERROR), "出错") == 0);
}

static void test_iso_week_number_handles_year_boundaries(void)
{
    assert(calendar_model_iso_week(2026, 6, 23) == 26);
    assert(calendar_model_iso_week(2026, 1, 1) == 1);
    assert(calendar_model_iso_week(2021, 1, 1) == 53);
    assert(calendar_model_iso_week(2027, 1, 1) == 53);
}

int main(void)
{
    test_status_text_mentions_board_data_sources();
    test_sample_model_initializes_board_data_fields();
    test_assistant_state_text();
    test_iso_week_number_handles_year_boundaries();
    test_png_writer();
    puts("calendar_model tests passed");
    return 0;
}
