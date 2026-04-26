/**
 * @file gpio_driver.h
 * @brief GPIO Driver for Multi-Relay and Buzzer control
 */

#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "driver/gpio.h"
#include <stdbool.h>

// Pin Definitions
#define RELAY1_PIN GPIO_NUM_16 // Existing Relay
#define RELAY2_PIN GPIO_NUM_4  // New Relay
#define RELAY3_PIN GPIO_NUM_26 // New Relay
#define RELAY4_PIN GPIO_NUM_32 // New Relay

// Legacy alias (points to Relay 1)
#define RELAY_PIN RELAY1_PIN

#define BUZZER_PIN GPIO_NUM_25
#define RESET_PIN GPIO_NUM_23       // Factory reset (long press >5s)
#define RESTART_BTN_PIN GPIO_NUM_27 // External restart button (short press)
#define RTC_INTERRUPT_PIN GPIO_NUM_33 // From DS3231 SQW/INT (Pull-up required)

// Relay states
#define RELAY_ON 1
#define RELAY_OFF 0

/**
 * @brief Initialize all GPIOs (4 Relays, Buzzer, Buttons)
 */
void gpio_driver_init(void);

/**
 * @brief Set specific relay state
 * @param relay_index 0=Relay1, 1=Relay2, 2=Relay3, 3=Relay4
 * @param state RELAY_ON or RELAY_OFF
 */
void relay_set(int relay_index, int state);

/**
 * @brief Get specific relay state
 * @param relay_index 0=Relay1, 1=Relay2, 2=Relay3, 3=Relay4
 * @return RELAY_ON or RELAY_OFF (returns 0 if index invalid)
 */
int relay_get_state(int relay_index);

/**
 * @brief Activate buzzer
 */
void buzzer_on(void);

/**
 * @brief Deactivate buzzer
 */
void buzzer_off(void);

/**
 * @brief Check if reset button is pressed
 */
bool is_reset_button_pressed(void);

/**
 * @brief Check if external restart button pressed (GPIO 27)
 */
bool is_restart_button_pressed(void);

/**
 * @brief Check if factory reset requested (long press >5s on GPIO 23)
 */
bool is_factory_reset_requested(void);

#endif // GPIO_DRIVER_H
