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
    test_iso_week_number_handles_year_boundaries();
    test_rlcd_mono_buffer();
    test_png_writer();
    puts("calendar_model tests passed");
    return 0;
}
