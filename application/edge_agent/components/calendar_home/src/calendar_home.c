#include "calendar_home.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "calendar_board_data.h"
#include "calendar_model.h"
#include "calendar_ui.h"
#include "display_arbiter.h"
#include "esp_board_manager_includes.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lua_lvgl_runtime.h"
#include "lvgl.h"

static const char *TAG = "calendar_home";

#define CALENDAR_HOME_BUFFER_LINES 24
#define CALENDAR_HOME_LVGL_TICK_MS 5
#define CALENDAR_HOME_TASK_STACK 8192
#define CALENDAR_HOME_TASK_PRIO 3

typedef struct {
    esp_lcd_panel_handle_t panel;
    lv_display_t *display;
    void *draw_buf_1;
    void *draw_buf_2;
    size_t draw_buf_size;
    int width;
    int height;
    calendar_ui_t ui;
    esp_timer_handle_t tick_timer;
    TaskHandle_t task;
    bool started;
    bool tick_running;
} calendar_home_state_t;

static calendar_home_state_t s_home;
static char s_day_hint[64];
static const char *WEEKDAYS[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};

static int calendar_round_float(float value)
{
    return (int)(value + (value >= 0.0f ? 0.5f : -0.5f));
}

static void calendar_home_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(CALENDAR_HOME_LVGL_TICK_MS);
}

static esp_err_t calendar_home_start_tick(void)
{
    if (s_home.tick_running || !s_home.tick_timer) {
        return ESP_OK;
    }

    esp_err_t err = esp_timer_start_periodic(s_home.tick_timer, CALENDAR_HOME_LVGL_TICK_MS * 1000ULL);
    if (err == ESP_ERR_INVALID_STATE) {
        s_home.tick_running = true;
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(err, TAG, "start tick timer failed");
    s_home.tick_running = true;
    return ESP_OK;
}

static void calendar_home_stop_tick(void)
{
    if (!s_home.tick_running || !s_home.tick_timer) {
        return;
    }

    esp_err_t err = esp_timer_stop(s_home.tick_timer);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "stop tick timer failed: %s", esp_err_to_name(err));
    }
    s_home.tick_running = false;
}

static void calendar_home_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    (void)display;

    if (s_home.panel && display_arbiter_is_owner(DISPLAY_ARBITER_OWNER_CALENDAR)) {
        esp_err_t err = esp_lcd_panel_draw_bitmap(s_home.panel,
                                                  area->x1,
                                                  area->y1,
                                                  area->x2 + 1,
                                                  area->y2 + 1,
                                                  px_map);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "LCD flush failed: %s", esp_err_to_name(err));
        }
    }

    lv_display_flush_ready(display);
}

static esp_err_t calendar_home_load_board_display(void)
{
#if !CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
    return ESP_ERR_NOT_SUPPORTED;
#else
    void *lcd_handle = NULL;
    void *lcd_config = NULL;

    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, &lcd_handle),
                        TAG,
                        "get display handle failed");
    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, &lcd_config),
                        TAG,
                        "get display config failed");

    dev_display_lcd_handles_t *lcd_handles = (dev_display_lcd_handles_t *)lcd_handle;
    dev_display_lcd_config_t *lcd_cfg = (dev_display_lcd_config_t *)lcd_config;

    ESP_RETURN_ON_FALSE(lcd_handles && lcd_cfg && lcd_handles->panel_handle,
                        ESP_ERR_INVALID_STATE,
                        TAG,
                        "display_lcd handle/config is NULL");

    s_home.panel = lcd_handles->panel_handle;
    s_home.width = lcd_cfg->lcd_width;
    s_home.height = lcd_cfg->lcd_height;
    return ESP_OK;
#endif
}

static calendar_model_t calendar_home_model_from_snapshot(const calendar_board_snapshot_t *snapshot)
{
    calendar_model_t model = calendar_model_sample();

    model.year = snapshot->local_time.tm_year + 1900;
    model.month = snapshot->local_time.tm_mon + 1;
    model.day = snapshot->local_time.tm_mday;
    model.hour = snapshot->local_time.tm_hour;
    model.minute = snapshot->local_time.tm_min;
    model.time_valid = snapshot->time_valid;
    model.rtc_available = snapshot->rtc_available;
    model.rtc_fallback_used = snapshot->rtc_fallback_used;
    model.shtc3_available = snapshot->shtc3_available;
    model.indoor_valid = snapshot->indoor_valid;
    model.weekday_text = snapshot->time_valid && snapshot->local_time.tm_wday >= 0 && snapshot->local_time.tm_wday <= 6
                         ? WEEKDAYS[snapshot->local_time.tm_wday]
                         : "待同步";
    model.temp_c = snapshot->indoor_valid ? calendar_round_float(snapshot->temperature_c) : 0;
    model.humidity_percent = snapshot->indoor_valid ? calendar_round_float(snapshot->humidity_percent) : 0;
    model.event_day_count = snapshot->time_valid ? 1 : 0;
    model.event_days[0] = snapshot->time_valid ? model.day : 0;

    if (!snapshot->time_valid) {
        model.day_hint = "等待系统时间或RTC";
    } else if (snapshot->rtc_fallback_used) {
        model.day_hint = "RTC fallback";
    } else {
        model.day_hint = "系统时间";
    }

    if (snapshot->indoor_valid) {
        snprintf(s_day_hint, sizeof(s_day_hint), "%s · 室内已更新", model.day_hint);
        model.day_hint = s_day_hint;
    }

    return model;
}

static esp_err_t calendar_home_refresh_model(void)
{
    calendar_board_snapshot_t snapshot = {0};
    ESP_RETURN_ON_ERROR(calendar_board_data_read(&snapshot), TAG, "read board data failed");

    calendar_model_t model = calendar_home_model_from_snapshot(&snapshot);
    calendar_ui_update(&s_home.ui, &model);
    return ESP_OK;
}

static void calendar_home_owner_changed(display_arbiter_owner_t owner, void *user_ctx)
{
    (void)user_ctx;

    if (!s_home.started) {
        return;
    }

    if (owner == DISPLAY_ARBITER_OWNER_CALENDAR) {
        esp_err_t err = calendar_home_start_tick();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "start LVGL tick after owner switch failed: %s", esp_err_to_name(err));
        }
        if (lua_lvgl_lock() == ESP_OK) {
            lv_obj_invalidate(s_home.ui.screen);
            lua_lvgl_unlock();
        }
    } else {
        calendar_home_stop_tick();
    }
}

static void calendar_home_task(void *arg)
{
    (void)arg;

    TickType_t last_refresh = 0;
    while (1) {
        if (display_arbiter_is_owner(DISPLAY_ARBITER_OWNER_CALENDAR)) {
            if (lua_lvgl_lock() == ESP_OK) {
                TickType_t now = xTaskGetTickCount();
                if (last_refresh == 0 ||
                    now - last_refresh >= pdMS_TO_TICKS(CONFIG_CALENDAR_HOME_REFRESH_MS)) {
                    esp_err_t err = calendar_home_refresh_model();
                    if (err != ESP_OK) {
                        ESP_LOGW(TAG, "refresh model failed: %s", esp_err_to_name(err));
                    }
                    last_refresh = now;
                }
                (void)lv_timer_handler();
                lua_lvgl_unlock();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static esp_err_t calendar_home_create_lvgl_display(void)
{
#if LVGL_VERSION_MAJOR < 9
    return ESP_ERR_NOT_SUPPORTED;
#else
    ESP_RETURN_ON_ERROR(lua_lvgl_ensure_initialized(), TAG, "init LVGL failed");

    s_home.draw_buf_size = (size_t)s_home.width * CALENDAR_HOME_BUFFER_LINES * sizeof(lv_color_t);
    s_home.draw_buf_1 = heap_caps_malloc(s_home.draw_buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    s_home.draw_buf_2 = heap_caps_malloc(s_home.draw_buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!s_home.draw_buf_1) {
        s_home.draw_buf_1 = heap_caps_malloc(s_home.draw_buf_size, MALLOC_CAP_8BIT);
    }
    if (!s_home.draw_buf_2) {
        s_home.draw_buf_2 = heap_caps_malloc(s_home.draw_buf_size, MALLOC_CAP_8BIT);
    }
    ESP_RETURN_ON_FALSE(s_home.draw_buf_1 && s_home.draw_buf_2, ESP_ERR_NO_MEM, TAG, "LVGL draw buffer alloc failed");

    s_home.display = lv_display_create(s_home.width, s_home.height);
    ESP_RETURN_ON_FALSE(s_home.display != NULL, ESP_ERR_NO_MEM, TAG, "LVGL display create failed");

    lv_display_set_buffers(s_home.display,
                           s_home.draw_buf_1,
                           s_home.draw_buf_2,
                           (uint32_t)s_home.draw_buf_size,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_home.display, calendar_home_flush_cb);
    lv_display_set_default(s_home.display);
    return ESP_OK;
#endif
}

esp_err_t calendar_home_start(void)
{
    if (s_home.started) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(calendar_home_load_board_display(), TAG, "load board display failed");
    ESP_RETURN_ON_ERROR(calendar_board_data_init(), TAG, "init board data failed");
    ESP_RETURN_ON_ERROR(calendar_home_create_lvgl_display(), TAG, "create LVGL display failed");

    calendar_board_snapshot_t snapshot = {0};
    ESP_RETURN_ON_ERROR(calendar_board_data_read(&snapshot), TAG, "read initial data failed");
    calendar_model_t model = calendar_home_model_from_snapshot(&snapshot);

    ESP_RETURN_ON_ERROR(lua_lvgl_lock(), TAG, "lock LVGL failed");
    calendar_ui_create(&s_home.ui, &model);
    lua_lvgl_unlock();

    const esp_timer_create_args_t timer_args = {
        .callback = calendar_home_tick_cb,
        .name = "calendar_lvgl_tick",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &s_home.tick_timer), TAG, "create tick timer failed");
    ESP_RETURN_ON_ERROR(display_arbiter_register_owner_changed_callback(calendar_home_owner_changed, NULL),
                        TAG,
                        "register display owner callback failed");
    ESP_RETURN_ON_ERROR(display_arbiter_acquire(DISPLAY_ARBITER_OWNER_CALENDAR), TAG, "acquire calendar display failed");
    ESP_RETURN_ON_ERROR(calendar_home_start_tick(), TAG, "start tick timer failed");

    BaseType_t task_ok = xTaskCreate(calendar_home_task,
                                     "calendar_home",
                                     CALENDAR_HOME_TASK_STACK,
                                     NULL,
                                     CALENDAR_HOME_TASK_PRIO,
                                     &s_home.task);
    ESP_RETURN_ON_FALSE(task_ok == pdPASS, ESP_ERR_NO_MEM, TAG, "create calendar task failed");

    s_home.started = true;
    ESP_LOGI(TAG, "Calendar home started on %dx%d display", s_home.width, s_home.height);
    return ESP_OK;
}
