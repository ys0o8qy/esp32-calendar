#pragma once

#include <stdbool.h>
#include <time.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool time_valid;
    bool rtc_available;
    bool rtc_fallback_used;
    bool shtc3_available;
    bool indoor_valid;
    struct tm local_time;
    float temperature_c;
    float humidity_percent;
} calendar_board_snapshot_t;

esp_err_t calendar_board_data_init(void);
esp_err_t calendar_board_data_read(calendar_board_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif
