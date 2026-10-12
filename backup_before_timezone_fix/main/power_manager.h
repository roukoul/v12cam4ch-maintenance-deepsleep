#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    POWER_MODE_NORMAL = 0,
    POWER_MODE_ECO = 1,    // Modem Sleep
    POWER_MODE_STEALTH = 2,// Deep Sleep Scheduled (Daily)
    POWER_MODE_HOLIDAY = 3,// Deep Sleep on specific Dates (Extended)
    POWER_MODE_WEEKLY = 4  // Deep Sleep Scheduled (Weekly)
} power_mode_t;

typedef struct {
    int32_t start_hour;
    int32_t start_min;
    int32_t start_sec;
    int32_t end_hour;
    int32_t end_min;
    int32_t end_sec;
    // Extended Calendar Support (Holiday Mode)
    int32_t start_day;
    int32_t start_month;
    int32_t start_year;
    int32_t end_day;
    int32_t end_month;
    int32_t end_year;
} sleep_window_t;

typedef struct {
    bool enabled;
    int32_t start_hour;
    int32_t start_min;
    int32_t start_sec;
    int32_t end_hour;
    int32_t end_min;
    int32_t end_sec;
} daily_sleep_window_t;

typedef struct {
    daily_sleep_window_t days[7]; // 0=Sunday, 1=Monday... 6=Saturday
} weekly_sleep_schedule_t;

typedef struct {
    power_mode_t mode;
    sleep_window_t window;
} power_settings_t;

/**
 * @brief Initialize Power Manager and load settings from NVS
 */
void power_manager_init(void);

/**
 * @brief Set the energy mode
 */
void power_manager_set_mode(power_mode_t mode);

/**
 * @brief Get the current energy mode
 */
power_mode_t power_manager_get_mode(void);

void power_manager_set_window(sleep_window_t window);
sleep_window_t power_manager_get_window(void);

void power_manager_set_weekly(weekly_sleep_schedule_t weekly);
weekly_sleep_schedule_t power_manager_get_weekly(void);

// Periodically check if we should enter stealth mode or stay in modem sleep
void power_manager_check(void);

// Force entry into stealth mode (deep sleep)
void power_manager_enter_stealth(void);

/**
 * @brief Apply power saving logic (called periodically)
 */
void power_manager_apply(void);

#endif // POWER_MANAGER_H
