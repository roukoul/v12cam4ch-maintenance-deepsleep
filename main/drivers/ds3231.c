#define _POSIX_C_SOURCE 200809L
#include "ds3231.h"
#include <sys/time.h>
#include <time.h>

static const char *TAG = "DS3231";
static bool s_rtc_found = false;

// BCD Helpers
static uint8_t dec2bcd(uint8_t val) { return ((val / 10 * 16) + (val % 10)); }
static uint8_t bcd2dec(uint8_t val) { return ((val / 16 * 10) + (val % 16)); }

esp_err_t ds3231_init(void) {
  // Check if device exists on shared bus
  if (i2c_manager_check_device(I2C_ADDR_DS3231)) {
    ESP_LOGI(TAG, "DS3231 found at 0x%02X", I2C_ADDR_DS3231);
    s_rtc_found = true;
    return ESP_OK;
  } else {
    ESP_LOGW(TAG, "DS3231 NOT found at 0x%02X", I2C_ADDR_DS3231);
    s_rtc_found = false;
    return ESP_ERR_NOT_FOUND;
  }
}

esp_err_t ds3231_get_time(struct tm *timeinfo) {
  if (!s_rtc_found)
    return ESP_ERR_NOT_FOUND;

  uint8_t data[7];
  esp_err_t err = i2c_manager_read_reg(I2C_ADDR_DS3231, 0x00, data, 7);
  if (err != ESP_OK)
    return err;

  timeinfo->tm_sec = bcd2dec(data[0]);
  timeinfo->tm_min = bcd2dec(data[1]);
  timeinfo->tm_hour = bcd2dec(data[2]);
  timeinfo->tm_wday = bcd2dec(data[3]) - 1; // 1-7 -> 0-6
  timeinfo->tm_mday = bcd2dec(data[4]);
  timeinfo->tm_mon = bcd2dec(data[5] & 0x7F) - 1; // 1-12 -> 0-11
  timeinfo->tm_year = bcd2dec(data[6]) + 100; // 00-99 -> 2000-2099 (100+offset)
  timeinfo->tm_isdst = -1;

  return ESP_OK;
}

esp_err_t ds3231_set_time(struct tm *timeinfo) {
  if (!s_rtc_found)
    return ESP_ERR_NOT_FOUND;

  uint8_t data[7];
  data[0] = dec2bcd(timeinfo->tm_sec);
  data[1] = dec2bcd(timeinfo->tm_min);
  data[2] = dec2bcd(timeinfo->tm_hour);
  data[3] = dec2bcd(timeinfo->tm_wday + 1);
  data[4] = dec2bcd(timeinfo->tm_mday);
  data[5] = dec2bcd(timeinfo->tm_mon + 1);
  data[6] = dec2bcd(timeinfo->tm_year - 100);

  return i2c_manager_write_reg(I2C_ADDR_DS3231, 0x00, data, 7);
}

void ds3231_sync_to_system(void) {
  struct tm t;
  esp_err_t err = ds3231_get_time(&t);
  if (err == ESP_OK) {
    time_t now = mktime(&t);
    struct timeval tv = {.tv_sec = now, .tv_usec = 0};
    settimeofday(&tv, NULL);
    ESP_LOGD(TAG, "System time synced from RTC");
  } else {
    ESP_LOGE(TAG, "ERREUR I2C: Impossible de lire le DS3231! Code: %d", err);
  }
}

void ds3231_sync_from_system(void) {
  time_t now;
  struct tm t;
  time(&now);
  localtime_r(&now, &t);
  if (ds3231_set_time(&t) == ESP_OK) {
    ESP_LOGI(TAG, "RTC synced from System time");
  }
}

float ds3231_get_temp(void) {
  if (!s_rtc_found)
    return 0.0f;

  uint8_t data[2];
  if (i2c_manager_read_reg(I2C_ADDR_DS3231, 0x11, data, 2) == ESP_OK) {
    return data[0] + (data[1] >> 6) * 0.25f;
  }
  return 0.0f;
}

esp_err_t ds3231_set_alarm(int hour, int minute) {
    if (!s_rtc_found) return ESP_ERR_NOT_FOUND;

    // Alarm 1 mask: match Hours, Minutes, Seconds
    // Registry 0x07 (sec), 0x08 (min), 0x09 (hour), 0x0A (day/date)
    uint8_t data[4];
    data[0] = dec2bcd(0);      // 0 seconds
    data[1] = dec2bcd(minute); 
    data[2] = dec2bcd(hour);
    data[3] = 0x80;            // Alarm 1 when hours, minutes, seconds match (bit 7=1)

    esp_err_t err = i2c_manager_write_reg(I2C_ADDR_DS3231, 0x07, data, 4);
    if (err != ESP_OK) return err;

    // Disable Alarm 2 Interrupt just in case, and enable Alarm 1
    uint8_t ctrl;
    i2c_manager_read_reg(I2C_ADDR_DS3231, 0x0E, &ctrl, 1);
    ctrl &= ~0x02; // Clear A2IE
    ctrl |= 0x05;  // Set A1IE and INTCN
    return i2c_manager_write_reg(I2C_ADDR_DS3231, 0x0E, &ctrl, 1);
}

esp_err_t ds3231_clear_alarm(void) {
    if (!s_rtc_found) return ESP_ERR_NOT_FOUND;
    
    // Clear both Alarm 1 and Alarm 2 Flags (A1F=bit0, A2F=bit1)
    uint8_t status;
    i2c_manager_read_reg(I2C_ADDR_DS3231, 0x0F, &status, 1);
    status &= ~0x03; // Clear A1F and A2F
    return i2c_manager_write_reg(I2C_ADDR_DS3231, 0x0F, &status, 1);
}

bool ds3231_check_power_lost(void) {
    if (!s_rtc_found) return true;
    
    // Check OSF (bit 7 of 0x0F)
    uint8_t status;
    if (i2c_manager_read_reg(I2C_ADDR_DS3231, 0x0F, &status, 1) == ESP_OK) {
        if (status & 0x80) {
            // Power was lost. Clear OSF for next time.
            status &= ~0x80;
            i2c_manager_write_reg(I2C_ADDR_DS3231, 0x0F, &status, 1);
            return true;
        }
    }
    return false;
}
