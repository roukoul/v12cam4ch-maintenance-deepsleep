/**
 * @file gpio_driver.c
 * @brief Implementation of GPIO controls for AepBill (Multi-Relay)
 */

#include "gpio_driver.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "buzzer_pattern.h"

static const char *TAG = "GPIO_DRV";

// Helper array for relay pins iteration
static const gpio_num_t RELAY_PINS[4] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN,
                                         RELAY4_PIN};

// Shadow state for output-only pins
static int s_relay_shadow_state[4] = {RELAY_OFF, RELAY_OFF, RELAY_OFF,
                                      RELAY_OFF};
#define NUM_RELAYS 4

void gpio_driver_init(void) {
  ESP_LOGI(TAG, "Initializing GPIO pins (Multi-Relay)...");

  // 1. Configure Relays (All 4)
  gpio_config_t io_conf = {0};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  // Bitmask for all relay pins
  io_conf.pin_bit_mask = ((1ULL << RELAY1_PIN) | (1ULL << RELAY2_PIN) |
                          (1ULL << RELAY3_PIN) | (1ULL << RELAY4_PIN));
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  // Initialize all relays to OFF
  for (int i = 0; i < NUM_RELAYS; i++) {
    gpio_set_level(RELAY_PINS[i], RELAY_OFF);
    s_relay_shadow_state[i] = RELAY_OFF;
  }

  // 2. Configure Buzzer
  gpio_reset_pin(BUZZER_PIN);
  gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(BUZZER_PIN, 0);

  // 3. Configure Buttons (Reset & Restart)
  gpio_config_t btn_conf = {0};
  btn_conf.intr_type = GPIO_INTR_DISABLE;
  btn_conf.mode = GPIO_MODE_INPUT;
  btn_conf.pin_bit_mask = ((1ULL << RESET_PIN) | (1ULL << RESTART_BTN_PIN));
  btn_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  btn_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&btn_conf);

  ESP_LOGI(TAG,
           "GPIOs Initialized: 4 Relays (16,4,26,32), Buzzer(25), Keys(23,27)");
}

void relay_set(int relay_index, int state) {
  if (relay_index < 0 || relay_index >= NUM_RELAYS) {
    ESP_LOGW(TAG, "Invalid relay index: %d", relay_index);
    return;
  }
  gpio_set_level(RELAY_PINS[relay_index], state);
  s_relay_shadow_state[relay_index] = state; // Update shadow state
  ESP_LOGI(TAG, "Relay %d set to %s (Shadow Update)", relay_index + 1,
           state ? "ON" : "OFF");

  // SYNCHRONIZE BUZZER WITH RELAY 1 (Sonnette - Index 0)
  if (relay_index == 0) {
      if (state == RELAY_ON) {
          ESP_LOGI(TAG, "Sync: Starting internal Buzzer with Relay 1 (Sonnette)");
          buzzer_start(BUZZER_PATTERN_CONTINUOUS, 86400000); // Max duration basically
      } else {
          ESP_LOGI(TAG, "Sync: Stopping internal Buzzer with Relay 1 (Sonnette)");
          buzzer_stop();
      }
  }
}

int relay_get_state(int relay_index) {
  if (relay_index < 0 || relay_index >= NUM_RELAYS)
    return 0;
  // Return shadow state because GPIOs might be Output-only
  return s_relay_shadow_state[relay_index];
}

void buzzer_on(void) { gpio_set_level(BUZZER_PIN, 1); }

void buzzer_off(void) { gpio_set_level(BUZZER_PIN, 0); }

bool is_reset_button_pressed(void) { return (gpio_get_level(RESET_PIN) == 0); }

bool is_restart_button_pressed(void) {
  return (gpio_get_level(RESTART_BTN_PIN) == 0);
}

bool is_factory_reset_requested(void) {
  static uint32_t press_start = 0;
  static bool was_pressed = false;
  const uint32_t LONG_PRESS_MS = 5000;

  bool is_pressed = (gpio_get_level(RESET_PIN) == 0);
  uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

  if (is_pressed && !was_pressed) {
    press_start = now;
    was_pressed = true;
  } else if (!is_pressed && was_pressed) {
    was_pressed = false;
    press_start = 0;
  } else if (is_pressed && was_pressed) {
    uint32_t duration = now - press_start;
    if (duration > LONG_PRESS_MS) {
      was_pressed = false;
      press_start = 0;
      return true;
    }
  }
  return false;
}
