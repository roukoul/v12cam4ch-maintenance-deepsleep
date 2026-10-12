#pragma once

#include "driver/i2c.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DS3231_ADDR 0x68

/**
 * @brief Initialise le bus I2C en mode Maître pour le DS3231
 *
 * @param i2c_num Port I2C (ex: I2C_NUM_0)
 * @param sda_io_num Broche GPIO pour SDA
 * @param scl_io_num Broche GPIO pour SCL
 * @return esp_err_t ESP_OK si succès
 */
esp_err_t ds3231_init(i2c_port_t i2c_num, int sda_io_num, int scl_io_num);

/**
 * @brief Lit l'heure actuelle depuis le module RTC DS3231
 *
 * @param i2c_num Port I2C utilisé
 * @param timeinfo Structure contenant l'heure lue
 * @return esp_err_t ESP_OK si succès
 */
esp_err_t ds3231_get_time(i2c_port_t i2c_num, struct tm *timeinfo);

/**
 * @brief Configure une nouvelle heure dans le module RTC DS3231
 *
 * @param i2c_num Port I2C utilisé
 * @param timeinfo Structure contenant la nouvelle heure
 * @return esp_err_t ESP_OK si succès
 */
esp_err_t ds3231_set_time(i2c_port_t i2c_num, const struct tm *timeinfo);

#ifdef __cplusplus
}
#endif
