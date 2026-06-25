#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "SDL.h"
#include "calendar_model.h"
#include "calendar_ui.h"
#include "lvgl.h"
#include "png_writer.h"

#define SIM_WIDTH 400
#define SIM_HEIGHT 300

static SDL_Window *g_window;
static SDL_Renderer *g_renderer;
static SDL_Texture *g_texture;
static uint32_t g_pixels[SIM_WIDTH * SIM_HEIGHT];
static const char *SIM_WEEKDAYS[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};

static calendar_model_t calendar_model_from_host_time(void)
{
    calendar_model_t model = calendar_model_sample();
    time_t now_time = time(NULL);
    struct tm local_time = {0};

    if (localtime_r(&now_time, &local_time) == NULL || local_time.tm_year + 1900 < 2024) {
        model.time_valid = false;
        model.weekday_text = "待同步";
        model.day_hint = "等待系统时间或RTC";
        model.rtc_available = false;
        model.rtc_fallback_used = false;
        model.shtc3_available = false;
        model.indoor_valid = false;
        return model;
    }

    model.year = local_time.tm_year + 1900;
    model.month = local_time.tm_mon + 1;
    model.day = local_time.tm_mday;
    model.hour = local_time.tm_hour;
    model.minute = local_time.tm_min;
    model.time_valid = true;
    model.weekday_text = SIM_WEEKDAYS[local_time.tm_wday];
    model.day_hint = "系统时间 · 室内已更新";
    model.rtc_available = true;
    model.rtc_fallback_used = false;
    model.shtc3_available = true;
    model.indoor_valid = true;
    model.temp_c = 26;
    model.humidity_percent = 46;
    model.event_day_count = 1;
    model.event_days[0] = model.day;
    return model;
}

static void sdl_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    (void)disp_drv;

    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            if (x < 0 || x >= SIM_WIDTH || y < 0 || y >= SIM_HEIGHT) {
                color_p++;
                continue;
            }

            uint8_t r = color_p->ch.red;
            uint8_t g = color_p->ch.green;
            uint8_t b = color_p->ch.blue;
            g_pixels[y * SIM_WIDTH + x] = 0xff000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
            color_p++;
        }
    }

    SDL_UpdateTexture(g_texture, NULL, g_pixels, SIM_WIDTH * (int)sizeof(uint32_t));
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
    lv_disp_flush_ready(disp_drv);
}

static bool sdl_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    g_window = SDL_CreateWindow(
        "ESP32 Calendar LVGL Simulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SIM_WIDTH * 2,
        SIM_HEIGHT * 2,
        SDL_WINDOW_SHOWN);
    if (!g_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) {
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!g_renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_RenderSetLogicalSize(g_renderer, SIM_WIDTH, SIM_HEIGHT);
    g_texture = SDL_CreateTexture(
        g_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SIM_WIDTH,
        SIM_HEIGHT);
    if (!g_texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

int main(int argc, char **argv)
{
    static lv_color_t draw_buffer_1[SIM_WIDTH * 32];
    static lv_color_t draw_buffer_2[SIM_WIDTH * 32];
    static lv_disp_draw_buf_t draw_buffer;
    static lv_disp_drv_t display_driver;

    bool smoke_test = false;
    const char *dump_png_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--smoke-test") == 0) {
            smoke_test = true;
        } else if (strcmp(argv[i], "--dump-png") == 0 && i + 1 < argc) {
            dump_png_path = argv[++i];
            smoke_test = true;
        } else {
            fprintf(stderr, "usage: %s [--smoke-test] [--dump-png PATH]\n", argv[0]);
            return 2;
        }
    }

    if (!sdl_init()) {
        return 1;
    }

    lv_init();
    lv_disp_draw_buf_init(&draw_buffer, draw_buffer_1, draw_buffer_2, SIM_WIDTH * 32);
    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = SIM_WIDTH;
    display_driver.ver_res = SIM_HEIGHT;
    display_driver.flush_cb = sdl_flush;
    display_driver.draw_buf = &draw_buffer;
    lv_disp_drv_register(&display_driver);

    calendar_model_t model = calendar_model_from_host_time();
    calendar_ui_t ui;
    calendar_ui_create(&ui, &model);

    bool running = true;
    uint32_t last_tick = SDL_GetTicks();
    uint32_t last_model_update = last_tick;
    int iterations = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        uint32_t now = SDL_GetTicks();
        lv_tick_inc(now - last_tick);
        last_tick = now;
        if (!smoke_test && now - last_model_update >= 1000) {
            model = calendar_model_from_host_time();
            calendar_ui_update(&ui, &model);
            last_model_update = now;
        }
        lv_timer_handler();
        SDL_Delay(5);

        if (smoke_test && ++iterations > 10) {
            running = false;
        }
    }

    if (dump_png_path != NULL) {
        char error[160] = {0};
        if (!png_write_argb8888(dump_png_path, g_pixels, SIM_WIDTH, SIM_HEIGHT, error, sizeof(error))) {
            fprintf(stderr, "PNG dump failed: %s\n", error);
            SDL_DestroyTexture(g_texture);
            SDL_DestroyRenderer(g_renderer);
            SDL_DestroyWindow(g_window);
            SDL_Quit();
            return 1;
        }
        printf("Wrote render PNG: %s\n", dump_png_path);
    }

    SDL_DestroyTexture(g_texture);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
