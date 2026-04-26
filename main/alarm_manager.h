/**
 * @file alarm_manager.h
 * @brief Alarm Logic and Schedule Management for AepBill
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <time.h>

// Constants from original code
#define NUM_DEVICES 1
#define MAX_ALARMS_PER_DAY 60 // Increased to 60 for 4-relay pool

// Schedule Structure
typedef struct {
  int8_t startHour[MAX_ALARMS_PER_DAY];
  int8_t startMinute[MAX_ALARMS_PER_DAY];
  int8_t startSecond[MAX_ALARMS_PER_DAY];
  int8_t endHour[MAX_ALARMS_PER_DAY];
  int8_t endMinute[MAX_ALARMS_PER_DAY];
  int8_t endSecond[MAX_ALARMS_PER_DAY];
  int8_t endDayOffset[MAX_ALARMS_PER_DAY]; // 0=Same day, 1=Next day
  int8_t relayIdx[MAX_ALARMS_PER_DAY];     // 0=Relay1, 1=Relay2, ...
} BellSchedule;

// Global schedule variables (extern to be accessible by HTTP handlers)
extern BellSchedule schedule[NUM_DEVICES][7];
extern bool alarmsEnabled[NUM_DEVICES][7];

/**
 * @brief Lock the alarm schedule for thread-safe access
 */
void alarm_lock(void);

/**
 * @brief Unlock the alarm schedule
 */
void alarm_unlock(void);

/**
 * @brief Initialize alarm manager, load schedule from NVS
 */
void alarm_manager_init(void);

/**
 * @brief Save current schedule and enabled states to NVS
 */
void alarm_manager_save(void);

/**
 * @brief Check schedule and update relay state
 * Should be called periodically from main loop
 */
void alarm_manager_check(void);

/**
 * @brief Initialize default schedule (clears all alarms)
 */
void alarm_manager_factory_reset(void);

/**
 * @brief Check if device is scheduled for today (for display)
 */
bool alarm_is_scheduled_today(int deviceIndex);

/**
 * @brief Get next alarm time string (HH:MM:SS) for display
 * @param relayIndex Index of relay to check (0-3), or -1 for ANY relay
 */
void alarm_get_next_time_str(int deviceIndex, int relayIndex, char *buffer,
                             size_t size);

/**
 * @brief Get next alarm time string with relay prefix (e.g. "S | 14:30:00")
 * @param relayIndex Index of relay to check (0-3), or -1 for ANY relay
 */
void alarm_get_next_time_str_with_relay(int deviceIndex, int relayIndex, char *buffer,
                                        size_t size);

/**
 * @brief Get the timestamp of the very next alarm across all relays
 * @return time_t of next alarm, or 0 if none found in next 7 days
 */
time_t alarm_get_next_timestamp(void);

/**
 * @brief Check if alarm is currently active (relay ON due to schedule)
 * @param relayIndex Index of relay to check (0-3)
 * @return true if relay should be ON according to current time and schedule
 */
bool alarm_is_active(int deviceIndex, int relayIndex);

/**
 * @brief Calculate the total expected ring time in seconds for a standard week
 * based on the currently active schedule for Relay 0.
 * @return Total seconds per week
 */
uint32_t alarm_calculate_weekly_ring_seconds(void);
