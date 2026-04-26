/**
 * @file logging.h
 * @brief Header for circular buffer logging system
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>
#include <stddef.h>


/**
 * @brief Initialize the log buffer
 */
void log_buffer_init(void);

/**
 * @brief Append a formatted log entry to the buffer
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void log_buffer_append(const char *format, ...);

/**
 * @brief Get the current log buffer contents
 * @param length Pointer to store the buffer length
 * @return Pointer to log buffer (read-only)
 */
const char *log_buffer_get(size_t *length);

/**
 * @brief Clear all logs from the buffer
 */
void log_buffer_clear(void);

/**
 * @brief Lock the log buffer for thread-safe reading
 */
void log_lock(void);

/**
 * @brief Unlock the log buffer
 */
void log_unlock(void);

#endif // LOGGING_H
