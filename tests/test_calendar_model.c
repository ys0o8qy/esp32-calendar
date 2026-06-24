#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "calendar_model.h"

void test_rlcd_mono_buffer(void);
void test_png_writer(void);

static void test_status_text_mentions_connectivity_and_cache(void)
{
    calendar_model_t model = calendar_model_sample();
    char buffer[96];

    calendar_model_status_text(&model, buffer, sizeof(buffer));

    assert(strstr(buffer, "Wi-Fi") != NULL);
    assert(strstr(buffer, "NTP") != NULL);
    assert(strstr(buffer, "08:12") != NULL);
    assert(strstr(buffer, "82%") != NULL);
}

static void test_sample_model_initializes_voice_assistant_fields(void)
{
    calendar_model_t model = calendar_model_sample();

    assert(model.assistant_active == false);
    assert(strcmp(model.assistant_state_text, "语音待机") == 0);
    assert(strcmp(model.assistant_caption, "") == 0);
    assert(strcmp(model.assistant_error, "") == 0);
}

static void test_month_grid_marks_today_and_events(void)
{
    calendar_model_t model = calendar_model_sample();
    calendar_month_grid_t grid;

    model.event_days[0] = 24;
    model.event_days[1] = 30;
    model.event_day_count = 2;

    calendar_model_month_grid(&model, &grid);

    assert(grid.cells[3][1].day == 23);
    assert(grid.cells[3][1].is_today);
    assert(grid.cells[3][2].day == 24);
    assert(grid.cells[3][2].has_event);
    assert(grid.cells[3][4].day == 26);
    assert(!grid.cells[3][4].has_event);
    assert(grid.cells[4][1].day == 30);
    assert(grid.cells[4][1].has_event);
}

static void test_month_grid_uses_adjacent_month_days(void)
{
    calendar_model_t model = calendar_model_sample();
    calendar_month_grid_t grid;

    calendar_model_month_grid(&model, &grid);

    assert(grid.cells[4][2].day == 1);
    assert(!grid.cells[4][2].in_current_month);
    assert(grid.cells[4][6].day == 5);
    assert(!grid.cells[4][6].in_current_month);
}

static void test_month_grid_keeps_sixth_week_for_long_months(void)
{
    calendar_model_t model = calendar_model_sample();
    calendar_month_grid_t grid;

    model.year = 2026;
    model.month = 3;
    model.day = 31;
    model.event_day_count = 0;

    calendar_model_month_grid(&model, &grid);

    assert(grid.cells[5][0].day == 30);
    assert(grid.cells[5][0].in_current_month);
    assert(grid.cells[5][1].day == 31);
    assert(grid.cells[5][1].is_today);
    assert(grid.cells[5][2].day == 1);
    assert(!grid.cells[5][2].in_current_month);
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
    test_status_text_mentions_connectivity_and_cache();
    test_sample_model_initializes_voice_assistant_fields();
    test_month_grid_marks_today_and_events();
    test_month_grid_uses_adjacent_month_days();
    test_month_grid_keeps_sixth_week_for_long_months();
    test_iso_week_number_handles_year_boundaries();
    test_rlcd_mono_buffer();
    test_png_writer();
    puts("calendar_model tests passed");
    return 0;
}
