#include "calendar_platform.h"

#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

static const char *TAG = "calendar-platform";
static const char *WEEKDAYS[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
static char g_weather_updated_at[6];
static EventGroupHandle_t g_network_events;
static i2c_master_bus_handle_t g_i2c_bus;
static i2c_master_dev_handle_t g_shtc3_dev;
static i2c_master_dev_handle_t g_rtc_dev;
static bool g_platform_initialized;
static bool g_wifi_started;
static bool g_wifi_connected;
static bool g_sntp_started;
static bool g_sntp_synced;
static bool g_sensor_valid;
static int g_temp_c;
static int g_humidity_percent;

#define WIFI_CONNECTED_BIT BIT0

#define CALENDAR_I2C_SDA_PIN GPIO_NUM_13
#define CALENDAR_I2C_SCL_PIN GPIO_NUM_14
#define CALENDAR_I2C_PORT I2C_NUM_0
#define SHTC3_I2C_ADDRESS 0x70
#define PCF85063_I2C_ADDRESS 0x51

#define SHTC3_WAKEUP 0x3517
#define SHTC3_SOFT_RESET 0x805D
#define SHTC3_SLEEP 0xB098
#define SHTC3_MEASURE_T_RH 0x7866

#define PCF85063_TIME_REG 0x04

#define CALENDAR_I2C_DATA_TIMEOUT_MS 5000
#define CALENDAR_I2C_DONE_TIMEOUT_MS 1000
#define SHTC3_WAKE_DELAY_MS 50
#define SHTC3_RESET_DELAY_MS 20
#define SHTC3_MEASURE_DELAY_MS 20

static void calendar_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        g_wifi_connected = false;
        ESP_LOGW(TAG, "Wi-Fi disconnected, reconnecting");
        esp_wifi_connect();
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        g_wifi_connected = true;
        if (g_network_events != NULL) {
            xEventGroupSetBits(g_network_events, WIFI_CONNECTED_BIT);
        }
    }
}

static void calendar_time_sync_cb(struct timeval *tv)
{
    (void)tv;
    g_sntp_synced = true;
}

static esp_err_t calendar_init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "NVS erase failed");
        ret = nvs_flash_init();
    }
    return ret;
}

static esp_err_t calendar_init_wifi(void)
{
    if (CONFIG_CALENDAR_WIFI_SSID[0] == '\0') {
        ESP_LOGW(TAG, "Wi-Fi SSID is empty; configure CALENDAR_WIFI_SSID to enable NTP");
        return ESP_OK;
    }

    if (g_wifi_started) {
        return ESP_OK;
    }

    g_network_events = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(g_network_events != NULL, ESP_ERR_NO_MEM, TAG, "failed to create Wi-Fi event group");

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Wi-Fi init failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, calendar_wifi_event_handler, NULL, NULL),
        TAG,
        "Wi-Fi event register failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, calendar_wifi_event_handler, NULL, NULL),
        TAG,
        "IP event register failed");

    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, CONFIG_CALENDAR_WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, CONFIG_CALENDAR_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    if (CONFIG_CALENDAR_WIFI_PASSWORD[0] != '\0') {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Wi-Fi STA mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Wi-Fi config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Wi-Fi start failed");
    g_wifi_started = true;

    EventBits_t bits = xEventGroupWaitBits(
        g_network_events,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(15000));
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Wi-Fi connected to SSID:%s", CONFIG_CALENDAR_WIFI_SSID);
    } else {
        ESP_LOGW(TAG, "Wi-Fi did not connect before timeout");
    }

    return ESP_OK;
}

static esp_err_t calendar_init_sntp(void)
{
    if (g_sntp_started || !g_wifi_connected) {
        return ESP_OK;
    }

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_CALENDAR_SNTP_SERVER);
    config.sync_cb = calendar_time_sync_cb;
    ESP_RETURN_ON_ERROR(esp_netif_sntp_init(&config), TAG, "SNTP init failed");
    g_sntp_started = true;

    esp_err_t ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000));
    if (ret == ESP_OK) {
        g_sntp_synced = true;
        ESP_LOGI(TAG, "SNTP time synced from %s", CONFIG_CALENDAR_SNTP_SERVER);
    } else {
        ESP_LOGW(TAG, "SNTP sync timeout from %s", CONFIG_CALENDAR_SNTP_SERVER);
    }
    return ESP_OK;
}

static esp_err_t calendar_i2c_wait_done(void)
{
    if (g_i2c_bus == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return i2c_master_bus_wait_all_done(g_i2c_bus, CALENDAR_I2C_DONE_TIMEOUT_MS);
}

static esp_err_t calendar_i2c_write_cmd(i2c_master_dev_handle_t dev, uint16_t cmd)
{
    uint8_t bytes[2] = {(uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xff)};
    ESP_RETURN_ON_ERROR(calendar_i2c_wait_done(), TAG, "I2C bus wait failed");
    return i2c_master_transmit(dev, bytes, sizeof(bytes), CALENDAR_I2C_DATA_TIMEOUT_MS);
}

static esp_err_t calendar_shtc3_wake(void)
{
    ESP_RETURN_ON_ERROR(calendar_i2c_write_cmd(g_shtc3_dev, SHTC3_WAKEUP), TAG, "SHTC3 wake failed");
    vTaskDelay(pdMS_TO_TICKS(SHTC3_WAKE_DELAY_MS));
    return ESP_OK;
}

static void calendar_probe_shtc3(void)
{
    esp_err_t ret = calendar_shtc3_wake();
    if (ret == ESP_OK) {
        ret = calendar_i2c_write_cmd(g_shtc3_dev, SHTC3_SOFT_RESET);
    }
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(SHTC3_RESET_DELAY_MS));
        ret = calendar_i2c_write_cmd(g_shtc3_dev, SHTC3_SLEEP);
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SHTC3 init probe failed: %s", esp_err_to_name(ret));
    }
}

static uint8_t calendar_shtc3_crc(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xff;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static int calendar_round_float(float value)
{
    return (int)(value + (value >= 0.0f ? 0.5f : -0.5f));
}

static esp_err_t calendar_init_i2c(void)
{
    if (g_i2c_bus != NULL) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port = CALENDAR_I2C_PORT,
        .sda_io_num = CALENDAR_I2C_SDA_PIN,
        .scl_io_num = CALENDAR_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &g_i2c_bus), TAG, "I2C bus init failed");

    i2c_device_config_t shtc3_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHTC3_I2C_ADDRESS,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(g_i2c_bus, &shtc3_config, &g_shtc3_dev), TAG, "SHTC3 add failed");

    i2c_device_config_t rtc_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PCF85063_I2C_ADDRESS,
        .scl_speed_hz = 300000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(g_i2c_bus, &rtc_config, &g_rtc_dev), TAG, "PCF85063 add failed");

    calendar_probe_shtc3();
    return ESP_OK;
}

static esp_err_t calendar_read_shtc3(void)
{
    if (g_shtc3_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[6] = {0};
    ESP_RETURN_ON_ERROR(calendar_shtc3_wake(), TAG, "SHTC3 wake failed");
    ESP_RETURN_ON_ERROR(calendar_i2c_write_cmd(g_shtc3_dev, SHTC3_MEASURE_T_RH), TAG, "SHTC3 measure failed");
    vTaskDelay(pdMS_TO_TICKS(SHTC3_MEASURE_DELAY_MS));
    ESP_RETURN_ON_ERROR(calendar_i2c_wait_done(), TAG, "I2C bus wait failed");
    ESP_RETURN_ON_ERROR(
        i2c_master_receive(g_shtc3_dev, data, sizeof(data), CALENDAR_I2C_DATA_TIMEOUT_MS),
        TAG,
        "SHTC3 read failed");
    calendar_i2c_write_cmd(g_shtc3_dev, SHTC3_SLEEP);

    if (calendar_shtc3_crc(data, 2) != data[2] || calendar_shtc3_crc(&data[3], 2) != data[5]) {
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t raw_temp = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_humidity = ((uint16_t)data[3] << 8) | data[4];
    float temp_c = 175.0f * (float)raw_temp / 65536.0f - 45.0f - 4.0f;
    float humidity = 100.0f * (float)raw_humidity / 65536.0f;

    g_temp_c = calendar_round_float(temp_c);
    g_humidity_percent = calendar_round_float(humidity);
    if (g_humidity_percent < 0) {
        g_humidity_percent = 0;
    } else if (g_humidity_percent > 100) {
        g_humidity_percent = 100;
    }
    g_sensor_valid = true;
    return ESP_OK;
}

static uint8_t calendar_to_bcd(int value)
{
    return (uint8_t)(((value / 10) << 4) | (value % 10));
}

static int calendar_from_bcd(uint8_t value)
{
    return ((value >> 4) * 10) + (value & 0x0f);
}

static bool calendar_time_is_valid(const struct tm *timeinfo)
{
    return timeinfo->tm_year + 1900 >= 2024;
}

static esp_err_t calendar_read_rtc_time(struct tm *timeinfo)
{
    if (g_rtc_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t reg = PCF85063_TIME_REG;
    uint8_t data[7] = {0};
    ESP_RETURN_ON_ERROR(calendar_i2c_wait_done(), TAG, "I2C bus wait failed");
    ESP_RETURN_ON_ERROR(
        i2c_master_transmit_receive(g_rtc_dev, &reg, 1, data, sizeof(data), CALENDAR_I2C_DATA_TIMEOUT_MS),
        TAG,
        "PCF85063 read failed");

    memset(timeinfo, 0, sizeof(*timeinfo));
    timeinfo->tm_sec = calendar_from_bcd(data[0] & 0x7f);
    timeinfo->tm_min = calendar_from_bcd(data[1] & 0x7f);
    timeinfo->tm_hour = calendar_from_bcd(data[2] & 0x3f);
    timeinfo->tm_mday = calendar_from_bcd(data[3] & 0x3f);
    timeinfo->tm_wday = data[4] & 0x07;
    timeinfo->tm_mon = calendar_from_bcd(data[5] & 0x1f) - 1;
    timeinfo->tm_year = calendar_from_bcd(data[6]) + 100;
    timeinfo->tm_isdst = -1;

    return calendar_time_is_valid(timeinfo) ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t calendar_write_rtc_time(const struct tm *timeinfo)
{
    if (g_rtc_dev == NULL || !calendar_time_is_valid(timeinfo)) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[8] = {
        PCF85063_TIME_REG,
        calendar_to_bcd(timeinfo->tm_sec),
        calendar_to_bcd(timeinfo->tm_min),
        calendar_to_bcd(timeinfo->tm_hour),
        calendar_to_bcd(timeinfo->tm_mday),
        calendar_to_bcd(timeinfo->tm_wday),
        calendar_to_bcd(timeinfo->tm_mon + 1),
        calendar_to_bcd((timeinfo->tm_year + 1900) % 100),
    };
    ESP_RETURN_ON_ERROR(calendar_i2c_wait_done(), TAG, "I2C bus wait failed");
    return i2c_master_transmit(g_rtc_dev, data, sizeof(data), CALENDAR_I2C_DATA_TIMEOUT_MS);
}

static bool calendar_get_local_time(struct tm *local_time)
{
    time_t now = time(NULL);
    localtime_r(&now, local_time);
    if (calendar_time_is_valid(local_time)) {
        return true;
    }

    if (calendar_read_rtc_time(local_time) == ESP_OK) {
        time_t rtc_time = mktime(local_time);
        struct timeval tv = {
            .tv_sec = rtc_time,
            .tv_usec = 0,
        };
        settimeofday(&tv, NULL);
        return true;
    }

    return false;
}

void calendar_platform_init(void)
{
    if (g_platform_initialized) {
        return;
    }

    setenv("TZ", "CST-8", 1);
    tzset();

    ESP_ERROR_CHECK(calendar_init_nvs());
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t event_ret = esp_event_loop_create_default();
    if (event_ret != ESP_OK && event_ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(event_ret);
    }

    ESP_ERROR_CHECK(calendar_init_i2c());
    ESP_ERROR_CHECK(calendar_init_wifi());
    ESP_ERROR_CHECK(calendar_init_sntp());

    struct tm local_time = {0};
    if (calendar_get_local_time(&local_time) && g_sntp_synced) {
        esp_err_t rtc_ret = calendar_write_rtc_time(&local_time);
        if (rtc_ret != ESP_OK) {
            ESP_LOGW(TAG, "PCF85063 update failed: %s", esp_err_to_name(rtc_ret));
        }
    }

    esp_err_t sensor_ret = calendar_read_shtc3();
    if (sensor_ret != ESP_OK) {
        ESP_LOGW(TAG, "SHTC3 read failed during init: %s", esp_err_to_name(sensor_ret));
    }

    g_platform_initialized = true;
}

calendar_model_t calendar_platform_read_model(void)
{
    if (!g_platform_initialized) {
        calendar_platform_init();
    }

    struct tm local_time = {0};
    bool time_valid = calendar_get_local_time(&local_time);
    if (!time_valid) {
        time_t now = time(NULL);
        localtime_r(&now, &local_time);
    }

    esp_err_t sensor_ret = calendar_read_shtc3();
    if (sensor_ret != ESP_OK) {
        ESP_LOGW(TAG, "SHTC3 read failed: %s", esp_err_to_name(sensor_ret));
    }

    if (time_valid) {
        snprintf(g_weather_updated_at, sizeof(g_weather_updated_at), "%02d:%02d", local_time.tm_hour, local_time.tm_min);
    } else {
        snprintf(g_weather_updated_at, sizeof(g_weather_updated_at), "--:--");
    }

    calendar_model_t model = {
        .year = local_time.tm_year + 1900,
        .month = local_time.tm_mon + 1,
        .day = local_time.tm_mday,
        .hour = local_time.tm_hour,
        .minute = local_time.tm_min,
        .weekday_text = WEEKDAYS[local_time.tm_wday],
        .lunar_text = "农历待同步",
        .day_hint = time_valid ? (g_sntp_synced ? "NTP已同步" : "RTC保时") : "等待RTC/NTP同步",
        .city = "本地",
        .weather_summary = g_sensor_valid ? "室内温湿度" : "传感器待连接",
        .temp_c = g_sensor_valid ? g_temp_c : 0,
        .temp_low_c = g_sensor_valid ? g_temp_c : 0,
        .temp_high_c = g_sensor_valid ? g_temp_c : 0,
        .humidity_percent = g_sensor_valid ? g_humidity_percent : 0,
        .battery_percent = 0,
        .wifi_connected = g_wifi_connected,
        .ntp_synced = time_valid && g_sntp_synced,
        .weather_updated_at = g_weather_updated_at,
        .next_event_text = "暂无日程",
        .assistant_state_text = "语音待机",
        .assistant_caption = "",
        .assistant_error = "",
        .assistant_active = false,
        .event_day_count = 0,
    };

    return model;
}
