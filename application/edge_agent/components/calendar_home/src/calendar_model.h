#pragma once

#include <stdbool.h>
#include <stddef.h>

#define CALENDAR_MAX_EVENTS 8

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    bool time_valid;
    bool rtc_available;
    bool rtc_fallback_used;
    bool shtc3_available;
    bool indoor_valid;
    const char *weekday_text;
    const char *day_hint;
    int temp_c;
    int humidity_percent;
    int event_days[CALENDAR_MAX_EVENTS];
    size_t event_day_count;
} calendar_model_t;

calendar_model_t calendar_model_sample(void);
void calendar_model_status_text(const calendar_model_t *model, char *buffer, size_t buffer_size);
int calendar_model_iso_week(int year, int month, int day);
