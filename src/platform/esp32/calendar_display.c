#include "calendar_display.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

#include "calendar_ui.h"
#include "rlcd_mono_buffer.h"

#define CALENDAR_LCD_WIDTH 400
#define CALENDAR_LCD_HEIGHT 300
#define CALENDAR_LCD_DC_PIN GPIO_NUM_5
#define CALENDAR_LCD_CS_PIN GPIO_NUM_40
#define CALENDAR_LCD_SCK_PIN GPIO_NUM_11
#define CALENDAR_LCD_MOSI_PIN GPIO_NUM_12
#define CALENDAR_LCD_RST_PIN GPIO_NUM_41
#define CALENDAR_LCD_SPI_HOST SPI3_HOST
#define CALENDAR_LVGL_TICK_MS 5
#define CALENDAR_LVGL_TASK_MIN_MS 5
#define CALENDAR_LVGL_TASK_MAX_MS 50

static const char *TAG = "calendar-display";

static esp_lcd_panel_io_handle_t g_io;
static uint8_t *g_mono_buffer;
static size_t g_mono_buffer_len;
static lv_disp_draw_buf_t g_draw_buffer;
static lv_disp_drv_t g_display_driver;
static SemaphoreHandle_t g_lvgl_lock;
static calendar_ui_t g_ui;
#ifdef CONFIG_CALENDAR_DUMP_RLCD_FRAME
static bool g_dumped_mono_frame;
#endif

static void lcd_send_cmd(uint8_t cmd)
{
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(g_io, cmd, NULL, 0));
}

static void lcd_send_data(uint8_t data)
{
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(g_io, -1, &data, 1));
}

static void lcd_reset(void)
{
    gpio_set_level(CALENDAR_LCD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(CALENDAR_LCD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(CALENDAR_LCD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

static void lcd_send_frame(void)
{
    lcd_send_cmd(0x2A);
    lcd_send_data(0x12);
    lcd_send_data(0x2A);

    lcd_send_cmd(0x2B);
    lcd_send_data(0x00);
    lcd_send_data(0xC7);

    lcd_send_cmd(0x2C);
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_color(g_io, -1, g_mono_buffer, g_mono_buffer_len));
}

#ifdef CONFIG_CALENDAR_DUMP_RLCD_FRAME
static void dump_mono_frame_once(void)
{
    if (g_dumped_mono_frame || g_mono_buffer == NULL || g_mono_buffer_len == 0) {
        return;
    }
    g_dumped_mono_frame = true;

    ESP_LOGI(
        TAG,
        "CALENDAR_RLCD_FRAME_BEGIN width=%d height=%d bytes=%u format=st7305-landscape-1bpp-hex",
        CALENDAR_LCD_WIDTH,
        CALENDAR_LCD_HEIGHT,
        (unsigned)g_mono_buffer_len);

    for (size_t offset = 0; offset < g_mono_buffer_len; offset += 32) {
        size_t chunk_len = g_mono_buffer_len - offset;
        if (chunk_len > 32) {
            chunk_len = 32;
        }

        char hex[65];
        for (size_t i = 0; i < chunk_len; i++) {
            snprintf(hex + i * 2, sizeof(hex) - i * 2, "%02x", g_mono_buffer[offset + i]);
        }
        ESP_LOGI(TAG, "CALENDAR_RLCD_FRAME_HEX offset=%u data=%s", (unsigned)offset, hex);
    }
    ESP_LOGI(TAG, "CALENDAR_RLCD_FRAME_END");
}
#endif

static esp_err_t lcd_bus_init(void)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = CALENDAR_LCD_MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = CALENDAR_LCD_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CALENDAR_LCD_WIDTH * CALENDAR_LCD_HEIGHT,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(CALENDAR_LCD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO), TAG, "SPI bus init failed");

    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = CALENDAR_LCD_CS_PIN,
        .dc_gpio_num = CALENDAR_LCD_DC_PIN,
        .spi_mode = 0,
        .pclk_hz = 10 * 1000 * 1000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)CALENDAR_LCD_SPI_HOST, &io_config, &g_io),
        TAG,
        "LCD panel IO init failed");

    gpio_config_t reset_config = {
        .pin_bit_mask = 1ULL << CALENDAR_LCD_RST_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&reset_config), TAG, "LCD reset GPIO init failed");
    return ESP_OK;
}

static void lcd_panel_init(void)
{
    lcd_reset();

    lcd_send_cmd(0xD6);
    lcd_send_data(0x17);
    lcd_send_data(0x02);

    lcd_send_cmd(0xD1);
    lcd_send_data(0x01);

    lcd_send_cmd(0xC0);
    lcd_send_data(0x11);
    lcd_send_data(0x04);

    lcd_send_cmd(0xC1);
    lcd_send_data(0x69);
    lcd_send_data(0x69);
    lcd_send_data(0x69);
    lcd_send_data(0x69);

    lcd_send_cmd(0xC2);
    lcd_send_data(0x19);
    lcd_send_data(0x19);
    lcd_send_data(0x19);
    lcd_send_data(0x19);

    lcd_send_cmd(0xC4);
    lcd_send_data(0x4B);
    lcd_send_data(0x4B);
    lcd_send_data(0x4B);
    lcd_send_data(0x4B);

    lcd_send_cmd(0xC5);
    lcd_send_data(0x19);
    lcd_send_data(0x19);
    lcd_send_data(0x19);
    lcd_send_data(0x19);

    lcd_send_cmd(0xD8);
    lcd_send_data(0x80);
    lcd_send_data(0xE9);

    lcd_send_cmd(0xB2);
    lcd_send_data(0x02);

    lcd_send_cmd(0xB3);
    const uint8_t b3[] = {0xE5, 0xF6, 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
    for (size_t i = 0; i < sizeof(b3); i++) {
        lcd_send_data(b3[i]);
    }

    lcd_send_cmd(0xB4);
    const uint8_t b4[] = {0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
    for (size_t i = 0; i < sizeof(b4); i++) {
        lcd_send_data(b4[i]);
    }

    lcd_send_cmd(0x62);
    lcd_send_data(0x32);
    lcd_send_data(0x03);
    lcd_send_data(0x1F);

    lcd_send_cmd(0xB7);
    lcd_send_data(0x13);

    lcd_send_cmd(0xB0);
    lcd_send_data(0x64);

    lcd_send_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(200));

    lcd_send_cmd(0xC9);
    lcd_send_data(0x00);

    lcd_send_cmd(0x36);
    lcd_send_data(0x48);

    lcd_send_cmd(0x3A);
    lcd_send_data(0x11);

    lcd_send_cmd(0xB9);
    lcd_send_data(0x20);

    lcd_send_cmd(0xB8);
    lcd_send_data(0x29);

    lcd_send_cmd(0x21);

    lcd_send_cmd(0x2A);
    lcd_send_data(0x12);
    lcd_send_data(0x2A);

    lcd_send_cmd(0x2B);
    lcd_send_data(0x00);
    lcd_send_data(0xC7);

    lcd_send_cmd(0x35);
    lcd_send_data(0x00);

    lcd_send_cmd(0xD0);
    lcd_send_data(0xFF);

    lcd_send_cmd(0x38);
    lcd_send_cmd(0x29);

    rlcd_mono_buffer_fill(g_mono_buffer, g_mono_buffer_len, RLCD_MONO_WHITE);
    lcd_send_frame();
}

static void lvgl_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_map)
{
    lv_color_t *pixel = color_map;

    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            bool white = rlcd_mono_pixel_is_white(lv_color_brightness(*pixel));
            rlcd_mono_buffer_set_landscape_pixel(
                g_mono_buffer,
                CALENDAR_LCD_WIDTH,
                CALENDAR_LCD_HEIGHT,
                (uint16_t)x,
                (uint16_t)y,
                white);
            pixel++;
        }
    }

    lcd_send_frame();
#ifdef CONFIG_CALENDAR_DUMP_RLCD_FRAME
    dump_mono_frame_once();
#endif
    lv_disp_flush_ready(disp_drv);
}

static void lvgl_tick(void *arg)
{
    (void)arg;
    lv_tick_inc(CALENDAR_LVGL_TICK_MS);
}

static void lvgl_task(void *arg)
{
    (void)arg;

    while (true) {
        uint32_t delay_ms = CALENDAR_LVGL_TASK_MAX_MS;
        if (xSemaphoreTake(g_lvgl_lock, portMAX_DELAY) == pdTRUE) {
            delay_ms = lv_timer_handler();
            xSemaphoreGive(g_lvgl_lock);
        }
        if (delay_ms < CALENDAR_LVGL_TASK_MIN_MS) {
            delay_ms = CALENDAR_LVGL_TASK_MIN_MS;
        } else if (delay_ms > CALENDAR_LVGL_TASK_MAX_MS) {
            delay_ms = CALENDAR_LVGL_TASK_MAX_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

static esp_err_t lvgl_port_init(void)
{
    lv_init();

    g_lvgl_lock = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(g_lvgl_lock != NULL, ESP_ERR_NO_MEM, TAG, "LVGL mutex allocation failed");

    size_t draw_pixels = CALENDAR_LCD_WIDTH * CALENDAR_LCD_HEIGHT;
    lv_color_t *draw_buffer_1 = heap_caps_malloc(draw_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    lv_color_t *draw_buffer_2 = heap_caps_malloc(draw_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_RETURN_ON_FALSE(draw_buffer_1 != NULL && draw_buffer_2 != NULL, ESP_ERR_NO_MEM, TAG, "LVGL draw buffer allocation failed");

    lv_disp_draw_buf_init(&g_draw_buffer, draw_buffer_1, draw_buffer_2, draw_pixels);
    lv_disp_drv_init(&g_display_driver);
    g_display_driver.hor_res = CALENDAR_LCD_WIDTH;
    g_display_driver.ver_res = CALENDAR_LCD_HEIGHT;
    g_display_driver.flush_cb = lvgl_flush;
    g_display_driver.draw_buf = &g_draw_buffer;
    g_display_driver.full_refresh = 1;
    lv_disp_drv_register(&g_display_driver);

    esp_timer_handle_t tick_timer = NULL;
    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick,
        .name = "lvgl_tick",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&tick_args, &tick_timer), TAG, "LVGL tick timer create failed");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(tick_timer, CALENDAR_LVGL_TICK_MS * 1000), TAG, "LVGL tick timer start failed");

    BaseType_t task_ok = xTaskCreatePinnedToCore(lvgl_task, "calendar_lvgl", 8192, NULL, 5, NULL, 0);
    ESP_RETURN_ON_FALSE(task_ok == pdPASS, ESP_ERR_NO_MEM, TAG, "LVGL task create failed");
    return ESP_OK;
}

esp_err_t calendar_display_start(const calendar_model_t *model)
{
    ESP_LOGI(TAG, "initializing Waveshare ESP32-S3-RLCD-4.2 display");

    g_mono_buffer_len = rlcd_mono_buffer_size(CALENDAR_LCD_WIDTH, CALENDAR_LCD_HEIGHT);
    g_mono_buffer = heap_caps_malloc(g_mono_buffer_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_RETURN_ON_FALSE(g_mono_buffer != NULL, ESP_ERR_NO_MEM, TAG, "RLCD mono buffer allocation failed");

    ESP_RETURN_ON_ERROR(lcd_bus_init(), TAG, "LCD bus init failed");
    lcd_panel_init();
    ESP_LOGI(TAG, "ST7305 RLCD panel initialized");

    ESP_RETURN_ON_ERROR(lvgl_port_init(), TAG, "LVGL port init failed");

    if (xSemaphoreTake(g_lvgl_lock, portMAX_DELAY) == pdTRUE) {
        calendar_ui_create(&g_ui, model);
        xSemaphoreGive(g_lvgl_lock);
    }
    ESP_LOGI(TAG, "calendar UI loaded on RLCD");
    return ESP_OK;
}

esp_err_t calendar_display_update(const calendar_model_t *model)
{
    ESP_RETURN_ON_FALSE(g_lvgl_lock != NULL, ESP_ERR_INVALID_STATE, TAG, "LVGL port is not initialized");
    ESP_RETURN_ON_FALSE(g_ui.screen != NULL, ESP_ERR_INVALID_STATE, TAG, "calendar UI is not initialized");

    if (xSemaphoreTake(g_lvgl_lock, portMAX_DELAY) == pdTRUE) {
        calendar_ui_update(&g_ui, model);
        xSemaphoreGive(g_lvgl_lock);
        return ESP_OK;
    }

    return ESP_FAIL;
}
