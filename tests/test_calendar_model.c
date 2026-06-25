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
}

static void test_sample_model_initializes_board_data_fields(void)
{
    calendar_model_t model = calendar_model_sample();

    assert(model.time_valid == true);
    assert(model.rtc_available == true);
    assert(model.shtc3_available == true);
    assert(model.indoor_valid == true);
    assert(model.event_day_count == 1);
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
    test_iso_week_number_handles_year_boundaries();
    test_png_writer();
    puts("calendar_model tests passed");
    return 0;
}
