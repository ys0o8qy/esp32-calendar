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

static void test_month_grid_marks_today_and_events(void)
{
    calendar_month_grid_t grid;

    calendar_model_month_grid(2026, 6, 23, &grid);

    assert(grid.cells[3][1].day == 23);
    assert(grid.cells[3][1].is_today);
    assert(grid.cells[3][4].day == 26);
    assert(grid.cells[3][4].has_event);
    assert(grid.cells[4][1].day == 30);
    assert(grid.cells[4][1].has_event);
}

static void test_month_grid_uses_adjacent_month_days(void)
{
    calendar_month_grid_t grid;

    calendar_model_month_grid(2026, 6, 23, &grid);

    assert(grid.cells[4][2].day == 1);
    assert(!grid.cells[4][2].in_current_month);
    assert(grid.cells[4][6].day == 5);
    assert(!grid.cells[4][6].in_current_month);
}

int main(void)
{
    test_status_text_mentions_connectivity_and_cache();
    test_month_grid_marks_today_and_events();
    test_month_grid_uses_adjacent_month_days();
    test_rlcd_mono_buffer();
    test_png_writer();
    puts("calendar_model tests passed");
    return 0;
}
