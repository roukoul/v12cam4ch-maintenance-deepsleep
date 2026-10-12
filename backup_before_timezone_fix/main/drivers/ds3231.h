#ifndef DS3231_DRIVER_H
#define DS3231_DRIVER_H

#include "i2c_manager.h"
#include <time.h>

/**
 * @brief Initialize DS3231 Logic
 *
 * Does NOT install I2C driver (uses I2C Manager).
 * Checks if device is present.
 *
 * @return ESP_OK if device found, ESP_ERR_NOT_FOUND otherwise
 */
esp_err_t ds3231_init(void);

/**
 * @brief Get time from RTC
 */
esp_err_t ds3231_get_time(struct tm *timeinfo);

/**
 * @brief Set time to RTC
 */
esp_err_t ds3231_set_time(struct tm *timeinfo);

/**
 * @brief Get temperature from DS3231
 */
float ds3231_get_temp(void);

/**
 * @brief Synchronize System time FROM RTC
 */
void ds3231_sync_to_system(void);

/**
 * @brief Synchronize RTC FROM System time
 */
void ds3231_sync_from_system(void);

/**
 * @brief Set Alarm 1 (HH:MM:SS)
 * When the alarm triggers, the INT/SQW pin goes LOW.
 */
esp_err_t ds3231_set_alarm(int hour, int minute);

/**
 * @brief Clear Alarm 1 and reset the interrupt flag.
 */
esp_err_t ds3231_clear_alarm(void);

/**
 * @brief Check the Oscillator Stop Flag (OSF).
 * If true, the clock has lost power and the time is invalid.
 * @return true if power was lost.
 */
bool ds3231_check_power_lost(void);

#endif // DS3231_DRIVER_H
