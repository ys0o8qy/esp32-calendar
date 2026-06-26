#include "calendar_board_data.h"

#include <string.h>
#include <sys/time.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "calendar_board";

#define CALENDAR_I2C_SDA_PIN GPIO_NUM_13
#define CALENDAR_I2C_SCL_PIN GPIO_NUM_14
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

static i2c_master_bus_handle_t s_i2c_bus;
static i2c_master_dev_handle_t s_shtc3_dev;
static i2c_master_dev_handle_t s_rtc_dev;
static bool s_initialized;
static bool s_shtc3_available;
static bool s_rtc_available;
static bool s_created_i2c_bus;
static bool s_rtc_synced_this_boot;
static bool s_system_time_from_rtc_fallback;

static bool calendar_get_system_time(struct tm *local_time);

static esp_err_t calendar_i2c_wait_done(void)
{
    ESP_RETURN_ON_FALSE(s_i2c_bus != NULL, ESP_ERR_INVALID_STATE, TAG, "I2C bus is NULL");
    return i2c_master_bus_wait_all_done(s_i2c_bus, CALENDAR_I2C_DONE_TIMEOUT_MS);
}

static esp_err_t calendar_i2c_write_cmd(i2c_master_dev_handle_t dev, uint16_t cmd)
{
    uint8_t bytes[2] = {(uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xff)};
    ESP_RETURN_ON_ERROR(calendar_i2c_wait_done(), TAG, "I2C wait failed");
    return i2c_master_transmit(dev, bytes, sizeof(bytes), CALENDAR_I2C_DATA_TIMEOUT_MS);
}

static esp_err_t calendar_shtc3_wake(void)
{
    ESP_RETURN_ON_ERROR(calendar_i2c_write_cmd(s_shtc3_dev, SHTC3_WAKEUP), TAG, "SHTC3 wake failed");
    vTaskDelay(pdMS_TO_TICKS(SHTC3_WAKE_DELAY_MS));
    return ESP_OK;
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

static uint8_t calendar_to_bcd(int value)
{
    return (uint8_t)(((value / 10) << 4) | (value % 10));
}

static int calendar_from_bcd(uint8_t value)
{
    return ((value >> 4) * 10) + (value & 0x0f);
}

static bool calendar_bcd_is_valid(uint8_t value)
{
    return (value & 0x0f) <= 9 && ((value >> 4) & 0x0f) <= 9;
}

static bool calendar_is_leap_year(int year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static int calendar_days_in_month(int year, int month)
{
    static const int days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31,
    };

    if (month < 1 || month > 12) {
        return 0;
    }

    if (month == 2 && calendar_is_leap_year(year)) {
        return 29;
    }
    return days[month - 1];
}

static bool calendar_date_components_valid(int year, int month, int day, int hour, int minute, int second)
{
    if (year < 2024 || month < 1 || month > 12) {
        return false;
    }
    if (day < 1 || day > calendar_days_in_month(year, month)) {
        return false;
    }
    return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59;
}

static bool calendar_time_is_valid(const struct tm *timeinfo)
{
    if (timeinfo == NULL || timeinfo->tm_wday < 0 || timeinfo->tm_wday > 6) {
        return false;
    }

    if (!calendar_date_components_valid(
            timeinfo->tm_year + 1900,
            timeinfo->tm_mon + 1,
            timeinfo->tm_mday,
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec)) {
        return false;
    }

    struct tm normalized = *timeinfo;
    normalized.tm_isdst = -1;
    if (mktime(&normalized) == (time_t)-1) {
        return false;
    }

    return normalized.tm_year == timeinfo->tm_year &&
           normalized.tm_mon == timeinfo->tm_mon &&
           normalized.tm_mday == timeinfo->tm_mday &&
           normalized.tm_hour == timeinfo->tm_hour &&
           normalized.tm_min == timeinfo->tm_min &&
           normalized.tm_sec == timeinfo->tm_sec;
}

static void calendar_set_unknown_local_time(struct tm *local_time)
{
    memset(local_time, 0, sizeof(*local_time));
    local_time->tm_year = 124;
    local_time->tm_mon = 0;
    local_time->tm_mday = 1;
    local_time->tm_wday = 1;
    local_time->tm_isdst = -1;
}

static esp_err_t calendar_read_rtc_time(struct tm *timeinfo)
{
#if CONFIG_CALENDAR_HOME_ENABLE_RTC
    if (s_rtc_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t reg = PCF85063_TIME_REG;
    uint8_t data[7] = {0};
    ESP_RETURN_ON_ERROR(calendar_i2c_wait_done(), TAG, "I2C wait failed");
    ESP_RETURN_ON_ERROR(
        i2c_master_transmit_receive(s_rtc_dev, &reg, 1, data, sizeof(data), CALENDAR_I2C_DATA_TIMEOUT_MS),
        TAG,
        "PCF85063 read failed");

    if ((data[0] & 0x80) != 0 ||
        !calendar_bcd_is_valid(data[0] & 0x7f) ||
        !calendar_bcd_is_valid(data[1] & 0x7f) ||
        !calendar_bcd_is_valid(data[2] & 0x3f) ||
        !calendar_bcd_is_valid(data[3] & 0x3f) ||
        (data[4] & 0x07) > 6 ||
        !calendar_bcd_is_valid(data[5] & 0x1f) ||
        !calendar_bcd_is_valid(data[6])) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    memset(timeinfo, 0, sizeof(*timeinfo));
    timeinfo->tm_sec = calendar_from_bcd(data[0] & 0x7f);
    timeinfo->tm_min = calendar_from_bcd(data[1] & 0x7f);
    timeinfo->tm_hour = calendar_from_bcd(data[2] & 0x3f);
    timeinfo->tm_mday = calendar_from_bcd(data[3] & 0x3f);
    timeinfo->tm_mon = calendar_from_bcd(data[5] & 0x1f) - 1;
    timeinfo->tm_year = calendar_from_bcd(data[6]) + 100;
    timeinfo->tm_isdst = -1;

    if (!calendar_time_is_valid(timeinfo)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    struct tm normalized = *timeinfo;
    time_t rtc_time = mktime(&normalized);
    if (rtc_time == (time_t)-1 || normalized.tm_mday != timeinfo->tm_mday) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    *timeinfo = normalized;
    return ESP_OK;
#else
    (void)timeinfo;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t calendar_write_rtc_time(const struct tm *timeinfo)
{
#if CONFIG_CALENDAR_HOME_ENABLE_RTC
    if (s_rtc_dev == NULL || !calendar_time_is_valid(timeinfo)) {
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
    ESP_RETURN_ON_ERROR(calendar_i2c_wait_done(), TAG, "I2C wait failed");
    return i2c_master_transmit(s_rtc_dev, data, sizeof(data), CALENDAR_I2C_DATA_TIMEOUT_MS);
#else
    (void)timeinfo;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t calendar_sync_rtc_from_system_time(void)
{
#if CONFIG_CALENDAR_HOME_ENABLE_RTC
    struct tm local_time = {0};

    if (!s_rtc_available || s_rtc_dev == NULL || s_created_i2c_bus) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!calendar_get_system_time(&local_time)) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = calendar_write_rtc_time(&local_time);
    if (ret == ESP_OK) {
        s_rtc_synced_this_boot = true;
        s_system_time_from_rtc_fallback = false;
    }
    return ret;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static bool calendar_get_system_time(struct tm *local_time)
{
    time_t now = time(NULL);
    localtime_r(&now, local_time);
    return calendar_time_is_valid(local_time);
}

static esp_err_t calendar_read_shtc3(float *temperature_c, float *humidity_percent)
{
#if CONFIG_CALENDAR_HOME_ENABLE_SHTC3
    if (s_shtc3_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[6] = {0};
    ESP_RETURN_ON_ERROR(calendar_shtc3_wake(), TAG, "SHTC3 wake failed");
    ESP_RETURN_ON_ERROR(calendar_i2c_write_cmd(s_shtc3_dev, SHTC3_MEASURE_T_RH), TAG, "SHTC3 measure failed");
    vTaskDelay(pdMS_TO_TICKS(SHTC3_MEASURE_DELAY_MS));
    ESP_RETURN_ON_ERROR(calendar_i2c_wait_done(), TAG, "I2C wait failed");
    ESP_RETURN_ON_ERROR(i2c_master_receive(s_shtc3_dev, data, sizeof(data), CALENDAR_I2C_DATA_TIMEOUT_MS),
                        TAG,
                        "SHTC3 read failed");
    (void)calendar_i2c_write_cmd(s_shtc3_dev, SHTC3_SLEEP);

    if (calendar_shtc3_crc(data, 2) != data[2] || calendar_shtc3_crc(&data[3], 2) != data[5]) {
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t raw_temp = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_humidity = ((uint16_t)data[3] << 8) | data[4];
    *temperature_c = 175.0f * (float)raw_temp / 65536.0f - 45.0f;
    *humidity_percent = 100.0f * (float)raw_humidity / 65536.0f;
    if (*humidity_percent < 0.0f) {
        *humidity_percent = 0.0f;
    } else if (*humidity_percent > 100.0f) {
        *humidity_percent = 100.0f;
    }
    return ESP_OK;
#else
    (void)temperature_c;
    (void)humidity_percent;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static void calendar_probe_shtc3(void)
{
#if CONFIG_CALENDAR_HOME_ENABLE_SHTC3
    esp_err_t ret = calendar_shtc3_wake();
    if (ret == ESP_OK) {
        ret = calendar_i2c_write_cmd(s_shtc3_dev, SHTC3_SOFT_RESET);
    }
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(SHTC3_RESET_DELAY_MS));
        ret = calendar_i2c_write_cmd(s_shtc3_dev, SHTC3_SLEEP);
    }
    s_shtc3_available = (ret == ESP_OK);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SHTC3 init probe failed: %s", esp_err_to_name(ret));
    }
#endif
}

static esp_err_t calendar_init_i2c(void)
{
    if (s_i2c_bus != NULL) {
        return ESP_OK;
    }

    i2c_port_t port = (i2c_port_t)CONFIG_CALENDAR_HOME_I2C_PORT;
    esp_err_t ret = i2c_master_get_bus_handle(port, &s_i2c_bus);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Using board manager I2C bus on port %d", (int)port);
    } else if (ret == ESP_ERR_NOT_FOUND) {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = port,
            .sda_io_num = CALENDAR_I2C_SDA_PIN,
            .scl_io_num = CALENDAR_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &s_i2c_bus), TAG, "I2C bus init failed");
        s_created_i2c_bus = true;
        ESP_LOGW(TAG, "Created fallback I2C bus on port %d; board support did not expose one", (int)port);
    } else {
        ESP_RETURN_ON_ERROR(ret, TAG, "I2C bus lookup failed");
    }

#if CONFIG_CALENDAR_HOME_ENABLE_SHTC3
    i2c_device_config_t shtc3_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHTC3_I2C_ADDRESS,
        .scl_speed_hz = 400000,
    };
    esp_err_t shtc3_ret = i2c_master_bus_add_device(s_i2c_bus, &shtc3_config, &s_shtc3_dev);
    if (shtc3_ret != ESP_OK) {
        ESP_LOGW(TAG, "SHTC3 add failed: %s", esp_err_to_name(shtc3_ret));
    }
#endif

#if CONFIG_CALENDAR_HOME_ENABLE_RTC
    i2c_device_config_t rtc_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PCF85063_I2C_ADDRESS,
        .scl_speed_hz = 300000,
    };
    esp_err_t rtc_ret = i2c_master_bus_add_device(s_i2c_bus, &rtc_config, &s_rtc_dev);
    s_rtc_available = (rtc_ret == ESP_OK);
    if (rtc_ret != ESP_OK) {
        ESP_LOGW(TAG, "PCF85063 add failed: %s", esp_err_to_name(rtc_ret));
    }
#endif

    if (s_shtc3_dev != NULL) {
        calendar_probe_shtc3();
    }

    return ESP_OK;
}

esp_err_t calendar_board_data_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    esp_err_t ret = calendar_init_i2c();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Calendar I2C init failed: %s", esp_err_to_name(ret));
    }
    s_initialized = true;
    return ESP_OK;
}

esp_err_t calendar_board_data_sync_rtc_from_system_time(void)
{
    ESP_RETURN_ON_ERROR(calendar_board_data_init(), TAG, "board data init failed");
    return calendar_sync_rtc_from_system_time();
}

esp_err_t calendar_board_data_read(calendar_board_snapshot_t *snapshot)
{
    ESP_RETURN_ON_FALSE(snapshot != NULL, ESP_ERR_INVALID_ARG, TAG, "snapshot is NULL");
    ESP_RETURN_ON_ERROR(calendar_board_data_init(), TAG, "board data init failed");

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->rtc_available = s_rtc_available;
    snapshot->shtc3_available = s_shtc3_available;

    if (calendar_get_system_time(&snapshot->local_time)) {
        snapshot->time_valid = true;
        snapshot->rtc_fallback_used = s_system_time_from_rtc_fallback;
        if (s_rtc_available &&
            !s_created_i2c_bus &&
            !s_rtc_synced_this_boot &&
            !s_system_time_from_rtc_fallback) {
            esp_err_t ret = calendar_sync_rtc_from_system_time();
            if (ret == ESP_OK) {
                s_rtc_synced_this_boot = true;
            } else {
                ESP_LOGW(TAG, "PCF85063 update failed: %s", esp_err_to_name(ret));
            }
        }
    } else if (calendar_read_rtc_time(&snapshot->local_time) == ESP_OK) {
        time_t rtc_time = mktime(&snapshot->local_time);
        struct timeval tv = {
            .tv_sec = rtc_time,
            .tv_usec = 0,
        };
        settimeofday(&tv, NULL);
        snapshot->time_valid = true;
        snapshot->rtc_fallback_used = true;
        s_system_time_from_rtc_fallback = true;
    } else {
        calendar_set_unknown_local_time(&snapshot->local_time);
    }

    if (s_shtc3_available) {
        esp_err_t ret = calendar_read_shtc3(&snapshot->temperature_c, &snapshot->humidity_percent);
        snapshot->indoor_valid = (ret == ESP_OK);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "SHTC3 read failed: %s", esp_err_to_name(ret));
        }
    }

    return ESP_OK;
}
