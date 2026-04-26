#ifndef PREDICTIVE_MAINTENANCE_H
#define PREDICTIVE_MAINTENANCE_H

#include <stdint.h>

/**
 * @brief Initialize Predictive Maintenance module (Temp sensor & NVS load)
 */
void pm_init(void);

/**
 * @brief Get current relay health (0.0% to 100.0%)
 */
float pm_get_health(void);

/**
 * @brief Get ESP32 internal temperature
 */
float pm_get_temp(void);

/**
 * @brief Record a ring duration and degrade health accordingly
 * @param duration_ms Duration of the ring in milliseconds
 */
void pm_record_ring(uint32_t duration_ms);

/**
 * @brief Calculate the estimated remaining days before relay reaches critical health.
 * Based on the current programmed schedule and average temperature.
 * @return Remaining days, or 9999 if infinite/unknown.
 */
int pm_get_days_remaining(void);

#endif /* PREDICTIVE_MAINTENANCE_H */
