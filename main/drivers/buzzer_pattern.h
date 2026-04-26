/**
 * @file buzzer_pattern.h
 * @brief Advanced Buzzer Driver with PWM Patterns (Non-Blocking)
 *
 * Architecture:
 * - Utilise LEDC hardware PWM (pas de CPU blocking)
 * - Task FreeRTOS dédiée (priorité basse pour ne pas perturber LCD)
 * - State machine pour patterns
 *
 * IMPORTANT: Ne bloque JAMAIS le LCD refresh!
 */

#ifndef BUZZER_PATTERN_H
#define BUZZER_PATTERN_H

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>


// GPIO du buzzer (déjà utilisé dans gpio_driver.h)
#define BUZZER_GPIO 25

// Configuration PWM
#define BUZZER_LEDC_TIMER LEDC_TIMER_1
#define BUZZER_LEDC_MODE LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_1
#define BUZZER_LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define BUZZER_FREQUENCY_HZ 2000 // 2 kHz (ton d'alarme)
#define BUZZER_DUTY_CYCLE 512    // 50% duty (1024 = 100%)

// Patterns
typedef enum {
  BUZZER_PATTERN_OFF = 0,    // Silence
  BUZZER_PATTERN_CONTINUOUS, // Continu (alarme startup)
  BUZZER_PATTERN_FAST_BEEP,  // Rapide 100ms ON / 100ms OFF
  BUZZER_PATTERN_SLOW_BEEP,  // Lent 500ms ON / 1500ms OFF (défaut runtime)
} buzzer_pattern_t;

// État de l'alarme
typedef struct {
  buzzer_pattern_t pattern;
  bool active;
  uint32_t start_time_ms;
  uint32_t max_duration_ms; // Timeout (default 5 min)
  bool timeout_reached;
} buzzer_state_t;

/**
 * @brief Initialize buzzer with PWM (LEDC)
 * @note Non-blocking, hardware PWM
 */
void buzzer_pattern_init(void);

/**
 * @brief Start buzzer pattern
 * @param pattern Pattern type
 * @param duration_ms Maximum duration (0 = infinite)
 */
void buzzer_start(buzzer_pattern_t pattern, uint32_t duration_ms);

/**
 * @brief Stop buzzer immediately
 */
void buzzer_stop(void);

/**
 * @brief Alarm at startup if anomaly detected
 * @note 5s continuous then switch to intermittent
 */
void buzzer_alarm_startup(void);

/**
 * @brief Get current buzzer state
 */
buzzer_state_t buzzer_get_state(void);

/**
 * @brief Check if buzzer is active
 */
bool buzzer_is_active(void);

#endif // BUZZER_PATTERN_H
