/**
 * @file alarm_manager.c
 * @brief Alarm Logic Implementation
 */

#include "alarm_manager.h"
#include "drivers/gpio_driver.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "predictive_maintenance.h" // IA Maintenance Prédictive
#include <string.h>
#include <sys/time.h>
#include <time.h>


static const char *TAG = "ALARM_MGR";

// Global Schedule Data
BellSchedule schedule[NUM_DEVICES][7];
bool alarmsEnabled[NUM_DEVICES][7];

// Mutex for protecting access to schedule and alarmsEnabled
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
static SemaphoreHandle_t s_alarm_mutex = NULL;

void alarm_lock(void) {
  if (s_alarm_mutex)
    xSemaphoreTake(s_alarm_mutex, portMAX_DELAY);
}

void alarm_unlock(void) {
  if (s_alarm_mutex)
    xSemaphoreGive(s_alarm_mutex);
}

// Cache for check logic (one boolean per relay)
// We have 4 relays max
typedef struct {
  bool relayState[4];
  unsigned long lastUpdate;
} AlarmCache;

static AlarmCache alarmCache = {{false, false, false, false}, 0};

// NVS Keys
#define NVS_NAMESPACE "storage"
// We use dynamic keys for daily blocks: sch_k%d_d%d and en_k%d_d%d

// Helper to get milliseconds roughly compatible with Arduino millis()
static unsigned long get_millis() {
  return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void alarm_manager_factory_reset(void) {
  for (int k = 0; k < NUM_DEVICES; k++) {
    for (int i = 0; i < 7; i++) {
      alarmsEnabled[k][i] = true;
      for (int j = 0; j < MAX_ALARMS_PER_DAY; j++) {
        schedule[k][i].startHour[j] = -1;
        schedule[k][i].startMinute[j] = -1;
        schedule[k][i].startSecond[j] = -1;
        schedule[k][i].endHour[j] = -1;
        schedule[k][i].endMinute[j] = -1;
        schedule[k][i].endSecond[j] = -1;
        schedule[k][i].endDayOffset[j] = 0;
        schedule[k][i].relayIdx[j] = 0; // Default to Relay 1
      }
    }
  }
  // We dont save here if called from init, but normally we do
  ESP_LOGI(TAG, "Memory initialized to defaults");
}

void alarm_manager_init(void) {
  esp_err_t err;
  nvs_handle_t my_handle;

  if (s_alarm_mutex == NULL) {
    s_alarm_mutex = xSemaphoreCreateMutex();
  }

  alarm_lock();
  // Initialize memory with defaults
  alarm_manager_factory_reset();
  alarm_unlock();

  err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error opening NVS handle (Init)!");
    return;
  }

  size_t required_size;

  alarm_lock();
  // Load per-day schedule and enabled states
  for (int k = 0; k < NUM_DEVICES; k++) {
    for (int day = 0; day < 7; day++) {
      char key_sch[16];
      char key_en[16];
      snprintf(key_sch, sizeof(key_sch), "sch_k%d_d%d", k, day);
      snprintf(key_en, sizeof(key_en), "en_k%d_d%d", k, day);

      // Load Schedule Day
      required_size = sizeof(schedule[k][day]);
      err = nvs_get_blob(my_handle, key_sch, &schedule[k][day], &required_size);
      if (err != ESP_OK || required_size != sizeof(schedule[k][day])) {
        ESP_LOGD(TAG, "Missing/invalid schedule for device %d day %d", k, day);
        // default already set
      }

      // Load Enabled State Day
      required_size = sizeof(alarmsEnabled[k][day]);
      err = nvs_get_blob(my_handle, key_en, &alarmsEnabled[k][day], &required_size);
      if (err != ESP_OK) {
        ESP_LOGD(TAG, "Missing enabled state for device %d day %d", k, day);
        // default already set
      }
    }
  }
  ESP_LOGI(TAG, "Schedule & Enabled states loaded from NVS (daily blocks)");
  alarm_unlock();

  nvs_close(my_handle);
}

void alarm_manager_save(void) {
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error opening NVS for saving!");
    return;
  }

  esp_err_t err_final = ESP_OK;

  alarm_lock();
  for (int k = 0; k < NUM_DEVICES; k++) {
    for (int day = 0; day < 7; day++) {
      char key_sch[16];
      char key_en[16];
      snprintf(key_sch, sizeof(key_sch), "sch_k%d_d%d", k, day);
      snprintf(key_en, sizeof(key_en), "en_k%d_d%d", k, day);

      esp_err_t err1 = nvs_set_blob(my_handle, key_sch, &schedule[k][day], sizeof(schedule[k][day]));
      esp_err_t err2 = nvs_set_blob(my_handle, key_en, &alarmsEnabled[k][day], sizeof(alarmsEnabled[k][day]));

      if (err1 != ESP_OK || err2 != ESP_OK) {
        err_final = ESP_FAIL;
      }
    }
  }
  alarm_unlock();

  if (err_final == ESP_OK) {
    nvs_commit(my_handle);
    ESP_LOGI(TAG, "Schedule saved to NVS (daily blocks)");
  } else {
    ESP_LOGE(TAG, "Failed to save schedule");
  }
  nvs_close(my_handle);
}

static time_t create_alarm_timestamp(time_t now, int dayOfWeek, int hour,
                                     int minute, int second) {
  struct tm *now_tm = localtime(&now);
  struct tm alarm_tm = *now_tm; // Copy current time components

  // Calculate current day index (0=Monday...6=Sunday)
  int current_dow = (now_tm->tm_wday + 6) % 7;
  int days_diff = dayOfWeek - current_dow;

  // Adjust target date
  alarm_tm.tm_mday += days_diff;
  alarm_tm.tm_hour = hour;
  alarm_tm.tm_min = minute;
  alarm_tm.tm_sec = second;
  alarm_tm.tm_isdst = -1;

  return mktime(&alarm_tm);
}

void alarm_manager_check(void) {
  static unsigned long lastScheduleCheck = 0;
  unsigned long current_ms = get_millis();

  if (current_ms - lastScheduleCheck < 1000)
    return;
  lastScheduleCheck = current_ms;

  time_t now;
  time(&now);

  int k = 0; // Device index (only 1 supported)

  // We need to check all relays
  bool shouldBeOn[4] = {false, false, false, false};
  bool stateMismatch = false;

  for (int r = 0; r < 4; r++) {
    if (alarmCache.relayState[r] != (relay_get_state(r) == 1)) {
      stateMismatch = true;
      break;
    }
  }

  // Re-calculate cache every 1s (increased frequency for better precision)
  // or if manual relay state changed externally
  if ((current_ms - alarmCache.lastUpdate >= 1000) || stateMismatch) {

    struct tm *timeinfo = localtime(&now);
    int currentDay = (timeinfo->tm_wday + 6) % 7; // 0=Monday

    // Check Today (0) and Yesterday (-1) to handle midnight overlaps
    alarm_lock();
    for (int dayOffset = -1; dayOffset <= 0; dayOffset++) {
      int day = (currentDay + dayOffset + 7) % 7;

      if (!alarmsEnabled[k][day])
        continue;

      for (int i = 0; i < MAX_ALARMS_PER_DAY; i++) {
        if (schedule[k][day].startHour[i] == -1)
          continue;

        int rIdx = schedule[k][day].relayIdx[i];
        if (rIdx < 0 || rIdx > 3)
          rIdx = 0;

        time_t start_ts = create_alarm_timestamp(
            now, day, schedule[k][day].startHour[i],
            schedule[k][day].startMinute[i], schedule[k][day].startSecond[i]);

        int targetEndDay = (day + schedule[k][day].endDayOffset[i]) % 7;
        time_t end_ts = create_alarm_timestamp(
            now, targetEndDay, schedule[k][day].endHour[i],
            schedule[k][day].endMinute[i], schedule[k][day].endSecond[i]);

        // If end time is before start time, it likely means next day
        // (if endDayOffset was 0 but endSec < startSec, which shouldn't happen
        // with web validation, but let's be robust)
        if (end_ts <= start_ts) {
          end_ts += 86400; // Add 24h
        }

        if (now >= start_ts && now < end_ts) {
          if (rIdx == 0) {
            float health = pm_get_health();
            float temp = pm_get_temp();
            // Auto-protection condition:
            if (health < 5.0f || temp > 65.0f) {
              // Limit the ring duration to protect system (e.g. 3 seconds max)
              if (now >= start_ts + 3) {
                shouldBeOn[rIdx] = false;
              } else {
                shouldBeOn[rIdx] = true;
              }
            } else {
              shouldBeOn[rIdx] = true;
            }
          } else {
            shouldBeOn[rIdx] = true;
          }
        }

        // Handle weekly cycle: if we are at start of Monday,
        // a Sunday alarm spanning midnight might be in the future (next week)
        // relative to 'now' calculation, so we check previous week too.
        if (now >= (start_ts - 604800) && now < (end_ts - 604800)) {
          shouldBeOn[rIdx] = true;
        }
      }
    }
    alarm_unlock();

    // Update Cache and Hardware
    static unsigned long s_relay_on_time[4] = {0, 0, 0, 0};

    for (int r = 0; r < 4; r++) {
      alarmCache.relayState[r] = shouldBeOn[r];
      int current = relay_get_state(r);
      if (shouldBeOn[r] && current == 0) {
        relay_set(r, 1);
        s_relay_on_time[r] = current_ms;
        ESP_LOGI(TAG, "Relay %d ON by alarm", r + 1);
      } else if (!shouldBeOn[r] && current == 1) {
        relay_set(r, 0);
        ESP_LOGI(TAG, "Relay %d OFF by alarm", r + 1);
        if (r == 0 && s_relay_on_time[r] > 0) {
          uint32_t duration = current_ms - s_relay_on_time[r];
          pm_record_ring(duration);
          s_relay_on_time[r] = 0;
        }
      }
    }
    alarmCache.lastUpdate = current_ms;
  }
}

bool alarm_is_scheduled_today(int deviceIndex) {
  time_t now;
  time(&now);
  struct tm *timeinfo = localtime(&now);
  int todayIndex = (timeinfo->tm_wday + 6) % 7;

  bool result = false;
  alarm_lock();
  if (alarmsEnabled[deviceIndex][todayIndex]) {
    for (int j = 0; j < MAX_ALARMS_PER_DAY; j++) {
      if (schedule[deviceIndex][todayIndex].startHour[j] != -1) {
        result = true;
        break;
      }
    }
  }
  alarm_unlock();
  return result;
}

void alarm_get_next_time_str(int deviceIndex, int relayIndex, char *buffer,
                             size_t size) {
  time_t now;
  time(&now);
  struct tm *timeinfo = localtime(&now);
  int currentDay = (timeinfo->tm_wday + 6) % 7;
  int currentSec =
      timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec;

  alarm_lock();
  if (!alarmsEnabled[deviceIndex][currentDay]) {
    alarm_unlock();
    buffer[0] = '\0';
    return;
  }

  int bestIdx = -1;
  int minDiff = 24 * 3600 + 1; // Any diff > 1 day is larger than max possible

  for (int i = 0; i < MAX_ALARMS_PER_DAY; i++) {
    int h = schedule[deviceIndex][currentDay].startHour[i];
    if (h == -1)
      continue;

    // Filter by relay ID if specified (relayIndex >= 0)
    // If relayIndex == -1, check ALL alarms
    if (relayIndex != -1 &&
        schedule[deviceIndex][currentDay].relayIdx[i] != relayIndex)
      continue;

    int startSec = h * 3600 +
                   schedule[deviceIndex][currentDay].startMinute[i] * 60 +
                   schedule[deviceIndex][currentDay].startSecond[i];

    if (startSec > currentSec) {
      int diff = startSec - currentSec;
      if (diff < minDiff) {
        minDiff = diff;
        bestIdx = i;
      }
    }
  }

  if (bestIdx != -1) {
    snprintf(buffer, size, "%02d:%02d:%02d",
             schedule[deviceIndex][currentDay].startHour[bestIdx],
             schedule[deviceIndex][currentDay].startMinute[bestIdx],
             schedule[deviceIndex][currentDay].startSecond[bestIdx]);
    alarm_unlock();
    return;
  }
  alarm_unlock();
  buffer[0] = '\0';
}

void alarm_get_next_time_str_with_relay(int deviceIndex, int relayIndex, char *buffer,
                             size_t size) {
  time_t now;
  time(&now);
  struct tm *timeinfo = localtime(&now);
  int currentDay = (timeinfo->tm_wday + 6) % 7;
  int currentSec =
      timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec;

  alarm_lock();
  if (!alarmsEnabled[deviceIndex][currentDay]) {
    alarm_unlock();
    buffer[0] = '\0';
    return;
  }

  int bestIdx = -1;
  int minDiff = 24 * 3600 + 1;

  for (int i = 0; i < MAX_ALARMS_PER_DAY; i++) {
    int h = schedule[deviceIndex][currentDay].startHour[i];
    if (h == -1)
      continue;

    if (relayIndex != -1 &&
        schedule[deviceIndex][currentDay].relayIdx[i] != relayIndex)
      continue;

    int startSec = h * 3600 +
                   schedule[deviceIndex][currentDay].startMinute[i] * 60 +
                   schedule[deviceIndex][currentDay].startSecond[i];

    if (startSec > currentSec) {
      int diff = startSec - currentSec;
      if (diff < minDiff) {
        minDiff = diff;
        bestIdx = i;
      }
    }
  }

  if (bestIdx != -1) {
    int rIdx = schedule[deviceIndex][currentDay].relayIdx[bestIdx];
    const char *rName = (rIdx == 0) ? "S" : (rIdx == 1) ? "A" : (rIdx == 2) ? "B" : (rIdx == 3) ? "C" : "?";
    snprintf(buffer, size, "%s | %02d:%02d:%02d",
             rName,
             schedule[deviceIndex][currentDay].startHour[bestIdx],
             schedule[deviceIndex][currentDay].startMinute[bestIdx],
             schedule[deviceIndex][currentDay].startSecond[bestIdx]);
    alarm_unlock();
    return;
  }
  alarm_unlock();
  buffer[0] = '\0';
}

bool alarm_is_active(int deviceIndex, int relayIndex) {
  (void)deviceIndex;
  bool active = false;
  alarm_lock();
  if (relayIndex >= 0 && relayIndex < 4) {
    active = alarmCache.relayState[relayIndex];
  } else {
    active = (alarmCache.relayState[0] | alarmCache.relayState[1] |
              alarmCache.relayState[2] | alarmCache.relayState[3]);
  }
  alarm_unlock();
  return active;
}

uint32_t alarm_calculate_weekly_ring_seconds(void) {
  uint32_t total_seconds = 0;
  alarm_lock();
  for (int day = 0; day < 7; day++) {
    if (!alarmsEnabled[0][day])
      continue;

    for (int i = 0; i < MAX_ALARMS_PER_DAY; i++) {
      if (schedule[0][day].startHour[i] == -1)
        continue;

      // We only care about Relay 0 (the bell) for predictive maintenance
      if (schedule[0][day].relayIdx[i] != 0)
        continue;

      int startSec = schedule[0][day].startHour[i] * 3600 +
                     schedule[0][day].startMinute[i] * 60 +
                     schedule[0][day].startSecond[i];

      int endSec = schedule[0][day].endHour[i] * 3600 +
                   schedule[0][day].endMinute[i] * 60 +
                   schedule[0][day].endSecond[i];

      if (schedule[0][day].endDayOffset[i] > 0) {
        endSec += 86400 * schedule[0][day].endDayOffset[i];
      }

      if (endSec > startSec) {
        total_seconds += (endSec - startSec);
      } else if (endSec <= startSec) {
        // Fallback if offset was missing but end is clearly next day
        total_seconds += (endSec + 86400 - startSec);
      }
    }
  }
  alarm_unlock();
  return total_seconds;
}
time_t alarm_get_next_timestamp(void) {
  time_t now;
  time(&now);
  struct tm *timeinfo = localtime(&now);
  int currentDay = (timeinfo->tm_wday + 6) % 7;

  time_t best_ts = 0;

  alarm_lock();
  // Check next 7 days
  for (int dayOffset = 0; dayOffset < 7; dayOffset++) {
    int day = (currentDay + dayOffset) % 7;
    if (!alarmsEnabled[0][day])
      continue;

    for (int i = 0; i < MAX_ALARMS_PER_DAY; i++) {
      if (schedule[0][day].startHour[i] == -1)
        continue;

      time_t alarm_ts = create_alarm_timestamp(
          now, day, schedule[0][day].startHour[i],
          schedule[0][day].startMinute[i], schedule[0][day].startSecond[i]);

      // If it's today but already passed, move to next week
      if (dayOffset == 0 && alarm_ts <= now) {
        alarm_ts += 604800; // 7 days later
      } else if (dayOffset > 0 && alarm_ts <= now) {
        // This shouldn't happen with correct create_alarm_timestamp logic but
        // let's be safe
        alarm_ts += 604800;
      }

      if (best_ts == 0 || alarm_ts < best_ts) {
        best_ts = alarm_ts;
      }
    }
  }
  alarm_unlock();

  return best_ts;
}
