/**
 * @file logging.c
 * @brief Circular buffer logging system for remote debugging
 */

#include "logging.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LOG_BUFFER_SIZE 8192
#define MAX_LOG_LINE 256

static char log_buffer[LOG_BUFFER_SIZE];
static size_t log_write_pos = 0;
static bool buffer_wrapped = false;

static const char *TAG = "LOGGING";

static SemaphoreHandle_t s_log_mutex = NULL;

void log_lock(void) {
  if (s_log_mutex)
    xSemaphoreTake(s_log_mutex, portMAX_DELAY);
}

void log_unlock(void) {
  if (s_log_mutex)
    xSemaphoreGive(s_log_mutex);
}

void log_buffer_init(void) {
  if (s_log_mutex == NULL) {
    s_log_mutex = xSemaphoreCreateMutex();
  }
  log_lock();
  memset(log_buffer, 0, LOG_BUFFER_SIZE);
  log_write_pos = 0;
  buffer_wrapped = false;
  log_unlock();
  ESP_LOGI(TAG, "Log buffer initialized (%d bytes)", LOG_BUFFER_SIZE);
}

void log_buffer_append(const char *format, ...) {
  char temp[MAX_LOG_LINE];
  va_list args;

  // Format the log message
  va_start(args, format);
  int msg_len = vsnprintf(temp, sizeof(temp) - 1, format, args);
  va_end(args);

  if (msg_len <= 0)
    return;

  // Add timestamp
  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm timeinfo;
  localtime_r(&tv.tv_sec, &timeinfo);

  char log_line[MAX_LOG_LINE + 50];
  int line_len =
      snprintf(log_line, sizeof(log_line), "[%02d:%02d:%02d.%03ld] %s\n",
               timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
               tv.tv_usec / 1000, temp);

  if (line_len <= 0)
    return;

  // Write to circular buffer
  log_lock();
  size_t space_left = LOG_BUFFER_SIZE - log_write_pos;

  if (space_left >= line_len) {
    // Fits in remaining space
    memcpy(log_buffer + log_write_pos, log_line, line_len);
    log_write_pos += line_len;
  } else {
    // Need to wrap around
    memcpy(log_buffer + log_write_pos, log_line, space_left);
    memcpy(log_buffer, log_line + space_left, line_len - space_left);
    log_write_pos = line_len - space_left;
    buffer_wrapped = true;
  }
  log_unlock();
}

const char *log_buffer_get(size_t *length) {
  if (buffer_wrapped) {
    // Buffer has wrapped, return from write position to end
    *length = LOG_BUFFER_SIZE;
    // Create a temporary ordered buffer (optional, for better readability)
    static char ordered_buffer[LOG_BUFFER_SIZE];
    memcpy(ordered_buffer, log_buffer + log_write_pos,
           LOG_BUFFER_SIZE - log_write_pos);
    memcpy(ordered_buffer + (LOG_BUFFER_SIZE - log_write_pos), log_buffer,
           log_write_pos);
    return ordered_buffer;
  } else {
    // Buffer hasn't wrapped, return from start to write position
    *length = log_write_pos;
    return log_buffer;
  }
}

void log_buffer_clear(void) {
  log_lock();
  memset(log_buffer, 0, LOG_BUFFER_SIZE);
  log_write_pos = 0;
  buffer_wrapped = false;
  log_unlock();
}
