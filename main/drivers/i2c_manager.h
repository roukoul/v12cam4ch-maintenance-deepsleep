#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdbool.h>

// I2C Configuration for ESP32-WROOM-32
#define I2C_MANAGER_PORT I2C_NUM_0
#define I2C_MANAGER_SDA_PIN GPIO_NUM_21
#define I2C_MANAGER_SCL_PIN GPIO_NUM_22
#define I2C_MANAGER_FREQ_HZ 100000 // 100kHz standard speed

// Default Device Addresses
#define I2C_ADDR_DS3231 0x68
#define I2C_ADDR_LCD 0x27

/**
 * @brief Initialize the shared I2C bus (GPIO 21/22)
 *
 * This MUST be called ONLY ONCE at startup.
 * It installs the ESP-IDF I2C driver.
 *
 * @return ESP_OK on success
 */
esp_err_t i2c_manager_init(void);

/**
 * @brief Check if a device exists on the bus
 *
 * Sends a dummy write to the address to see if ACK is received.
 *
 * @param address I2C address (7-bit)
 * @return true if device responded (ACK)
 * @return false if no response (NACK)
 */
bool i2c_manager_check_device(uint8_t address);

/**
 * @brief Scan the entire I2C bus and log found devices
 *
 * Useful for debugging wiring connections.
 */
void i2c_manager_scan_bus(void);

/**
 * @brief Send data to an I2C slave
 */
esp_err_t i2c_manager_write(uint8_t addr, const uint8_t *data, size_t len);

/**
 * @brief Read data from an I2C slave
 */
esp_err_t i2c_manager_read(uint8_t addr, uint8_t *data, size_t len);

/**
 * @brief Write register(s) to an I2C slave
 */
esp_err_t i2c_manager_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data,
                                size_t len);

/**
 * @brief Read register(s) from an I2C slave
 */
esp_err_t i2c_manager_read_reg(uint8_t addr, uint8_t reg, uint8_t *data,
                               size_t len);

#endif // I2C_MANAGER_H
