#pragma once

#include <stdbool.h>
#include <stddef.h>

#define CALENDAR_WEEK_ROWS 6
#define CALENDAR_WEEK_DAYS 7
#define CALENDAR_MAX_EVENTS 8

typedef struct {
    int day;
    bool in_current_month;
    bool is_today;
    bool has_event;
} calendar_day_cell_t;

typedef struct {
    calendar_day_cell_t cells[CALENDAR_WEEK_ROWS][CALENDAR_WEEK_DAYS];
} calendar_month_grid_t;

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    const char *weekday_text;
    const char *lunar_text;
    const char *day_hint;
    const char *city;
    const char *weather_summary;
    int temp_c;
    int temp_low_c;
    int temp_high_c;
    int humidity_percent;
    int battery_percent;
    bool wifi_connected;
    bool ntp_synced;
    const char *weather_updated_at;
    const char *next_event_text;
    int event_days[CALENDAR_MAX_EVENTS];
    size_t event_day_count;
} calendar_model_t;

calendar_model_t calendar_model_sample(void);
void calendar_model_status_text(const calendar_model_t *model, char *buffer, size_t buffer_size);
void calendar_model_month_grid(int year, int month, int today, calendar_month_grid_t *grid);
