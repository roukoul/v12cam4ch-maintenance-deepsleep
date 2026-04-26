/**
 * @file buzzer_pattern.c
 * @brief Implementation of non-blocking buzzer with PWM patterns
 *
 * CRITICAL: Task priorité BASSE (1) pour ne pas bloquer LCD (priorité 5+)
 */

#include "buzzer_pattern.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>


static const char *TAG = "BUZZER";

// État global du buzzer
static buzzer_state_t s_buzzer_state = {.pattern = BUZZER_PATTERN_OFF,
                                        .active = false,
                                        .start_time_ms = 0,
                                        .max_duration_ms =
                                            300000, // 5 minutes par défaut
                                        .timeout_reached = false};

// Handle de la task FreeRTOS
static TaskHandle_t s_buzzer_task_handle = NULL;

// Mutex pour thread-safety (optionnel mais recommandé)
static SemaphoreHandle_t s_buzzer_mutex = NULL;

/**
 * @brief Configure LEDC PWM pour le buzzer
 */
static void buzzer_ledc_init(void) {
  // Configuration du timer LEDC
  ledc_timer_config_t ledc_timer = {.speed_mode = BUZZER_LEDC_MODE,
                                    .timer_num = BUZZER_LEDC_TIMER,
                                    .duty_resolution = BUZZER_LEDC_DUTY_RES,
                                    .freq_hz = BUZZER_FREQUENCY_HZ,
                                    .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // Configuration du channel LEDC
  ledc_channel_config_t ledc_channel = {.channel = BUZZER_LEDC_CHANNEL,
                                        .duty = 0, // Start silent
                                        .gpio_num = BUZZER_GPIO,
                                        .speed_mode = BUZZER_LEDC_MODE,
                                        .hpoint = 0,
                                        .timer_sel = BUZZER_LEDC_TIMER};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ESP_LOGI(TAG, "LEDC PWM initialized: GPIO=%d, Freq=%d Hz", BUZZER_GPIO,
           BUZZER_FREQUENCY_HZ);
}

/**
 * @brief Active le buzzer (PWM ON)
 */
static void buzzer_pwm_on(void) {
  ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, BUZZER_DUTY_CYCLE);
  ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
}

/**
 * @brief Désactive le buzzer (PWM OFF)
 */
static void buzzer_pwm_off(void) {
  ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0);
  ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
}

/**
 * @brief Task FreeRTOS gérant les patterns
 * @note Priorité BASSE (1) - Ne bloque jamais le LCD!
 */
static void buzzer_pattern_task(void *pvParameters) {
  ESP_LOGI(TAG, "Buzzer task started (priority=%d)", uxTaskPriorityGet(NULL));

  while (1) {
    // Prendre le mutex
    if (xSemaphoreTake(s_buzzer_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {

      if (!s_buzzer_state.active) {
        // Buzzer inactif, dormir
        xSemaphoreGive(s_buzzer_mutex);
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
      }

      // Vérifier timeout
      uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
      uint32_t elapsed = now_ms - s_buzzer_state.start_time_ms;

      if (s_buzzer_state.max_duration_ms > 0 &&
          elapsed >= s_buzzer_state.max_duration_ms) {
        // Timeout atteint
        ESP_LOGW(TAG, "Buzzer timeout reached (%u ms)", elapsed);
        s_buzzer_state.timeout_reached = true;
        s_buzzer_state.active = false;
        buzzer_pwm_off();
        xSemaphoreGive(s_buzzer_mutex);
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
      }

      // Exécuter pattern
      switch (s_buzzer_state.pattern) {
      case BUZZER_PATTERN_CONTINUOUS:
        // Continu - juste ON constant
        buzzer_pwm_on();
        xSemaphoreGive(s_buzzer_mutex);
        vTaskDelay(pdMS_TO_TICKS(100)); // Vérifier toutes les 100ms
        break;

      case BUZZER_PATTERN_FAST_BEEP:
        // Rapide: 100ms ON, 100ms OFF
        buzzer_pwm_on();
        xSemaphoreGive(s_buzzer_mutex);
        vTaskDelay(pdMS_TO_TICKS(100)); // ON

        if (xSemaphoreTake(s_buzzer_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
          buzzer_pwm_off();
          xSemaphoreGive(s_buzzer_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // OFF
        break;

      case BUZZER_PATTERN_SLOW_BEEP:
        // Lent: 500ms ON, 1500ms OFF (cycle 2s)
        buzzer_pwm_on();
        xSemaphoreGive(s_buzzer_mutex);
        vTaskDelay(pdMS_TO_TICKS(500)); // ON

        if (xSemaphoreTake(s_buzzer_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
          buzzer_pwm_off();
          xSemaphoreGive(s_buzzer_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1500)); // OFF
        break;

      case BUZZER_PATTERN_OFF:
      default:
        buzzer_pwm_off();
        s_buzzer_state.active = false;
        xSemaphoreGive(s_buzzer_mutex);
        vTaskDelay(pdMS_TO_TICKS(100));
        break;
      }
    } else {
      // Mutex timeout - retry
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

/**
 * @brief Initialize buzzer system
 */
void buzzer_pattern_init(void) {
  ESP_LOGI(TAG, "Initializing advanced buzzer driver...");

  // Init PWM
  buzzer_ledc_init();

  // Créer mutex
  s_buzzer_mutex = xSemaphoreCreateMutex();
  if (s_buzzer_mutex == NULL) {
    ESP_LOGE(TAG, "Failed to create mutex!");
    return;
  }

  // Créer task avec PRIORITÉ BASSE (1)
  // IMPORTANT: Priorité basse pour ne pas bloquer LCD (priorité 5+)
  BaseType_t ret = xTaskCreate(buzzer_pattern_task, "buzzer_task",
                               2048, // Stack size
                               NULL,
                               1, // PRIORITÉ BASSE = 1 (LCD a priorité 5+)
                               &s_buzzer_task_handle);

  if (ret != pdPASS) {
    ESP_LOGE(TAG, "Failed to create buzzer task!");
    return;
  }

  ESP_LOGI(TAG, "Buzzer task created (priority=1, non-blocking)");
}

/**
 * @brief Start buzzer with pattern
 */
void buzzer_start(buzzer_pattern_t pattern, uint32_t duration_ms) {
  if (xSemaphoreTake(s_buzzer_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    s_buzzer_state.pattern = pattern;
    s_buzzer_state.active = true;
    s_buzzer_state.start_time_ms = (uint32_t)(esp_timer_get_time() / 1000);
    s_buzzer_state.max_duration_ms = duration_ms;
    s_buzzer_state.timeout_reached = false;

    ESP_LOGI(TAG, "Buzzer started: pattern=%d, duration=%u ms", pattern,
             duration_ms);

    xSemaphoreGive(s_buzzer_mutex);
  }
}

/**
 * @brief Stop buzzer immediately
 */
void buzzer_stop(void) {
  if (xSemaphoreTake(s_buzzer_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    s_buzzer_state.pattern = BUZZER_PATTERN_OFF;
    s_buzzer_state.active = false;
    buzzer_pwm_off();

    ESP_LOGI(TAG, "Buzzer stopped");

    xSemaphoreGive(s_buzzer_mutex);
  }
}

/**
 * @brief Alarme startup (5s continu puis intermittent)
 */
void buzzer_alarm_startup(void) {
  ESP_LOGW(TAG, "🚨 STARTUP ALARM - Anomaly detected!");

  // Phase 1: 5 secondes continues
  buzzer_start(BUZZER_PATTERN_CONTINUOUS, 5000);

  // Attendre 5 secondes (non-bloquant car géré par task)
  // Après 5s, la task va timeout et s'arrêter

  // Dans main.c, après 5s, appeler:
  // buzzer_start(BUZZER_PATTERN_SLOW_BEEP, 300000);  // 5 min intermittent
}

/**
 * @brief Get buzzer state
 */
buzzer_state_t buzzer_get_state(void) {
  buzzer_state_t state;
  if (xSemaphoreTake(s_buzzer_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    state = s_buzzer_state;
    xSemaphoreGive(s_buzzer_mutex);
  } else {
    memset(&state, 0, sizeof(state));
  }
  return state;
}

/**
 * @brief Check if buzzer active
 */
bool buzzer_is_active(void) {
  bool active = false;
  if (xSemaphoreTake(s_buzzer_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    active = s_buzzer_state.active;
    xSemaphoreGive(s_buzzer_mutex);
  }
  return active;
}
