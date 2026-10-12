/**
 * @file adc_driver.h
 * @brief ADC Driver for ACS712 Current Sensor
 *
 * Migrated from v7.1.1_STABLE_NO_OTA.ino
 * Original: #define CURRENT_SENSOR_PIN 34, SENSITIVITY 0.185
 */

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

// ACS712 5A Sensor Parameters (from v7.1.1)
#define CURRENT_SENSOR_CHANNEL ADC_CHANNEL_6 // GPIO 34 = ADC1_CH6
#define ADC_SENSITIVITY 0.185f               // V/A for 5A model
#define ADC_VOLTAGE_REF 3.3f
#define ADC_RESOLUTION 4095.0f
#define NOISE_GATE_THRESHOLD                                                   \
  50.0f // ADC units (increased from 5.0 to reduce noise)

// Configurable thresholds (from v7.1.1)
typedef struct {
  float min_load;          // Minimum expected current when ON
  float anomaly_threshold; // Max current allowed when OFF
  float max_critical;      // Absolute maximum (overload)
} current_thresholds_t;

// ADC Calibration (persistent in NVS)
typedef struct {
  float zero_offset;  // ADC value at zero current (~2048)
  float scale_factor; // Multiplicative correction factor (default 1.0)
  uint8_t is_valid;   // 1 if calibration data is valid

  // === NOUVEAUX CHAMPS (Auto-Calibration v10.1) ===
  uint32_t
      calibration_timestamp; // Timestamp de la dernière calibration (secondes)
  uint8_t vref_type;         // Type Vref: 0=eFuse, 1=TwoPoint, 2=Default
  uint32_t actual_vref_mv;   // Vref réel utilisé (mV)
  float noise_level_rms;     // Niveau de bruit mesuré (ADC counts RMS)
} adc_calibration_t;

// Statistiques de calibration (pour validation/debug)
typedef struct {
  float mean;            // Moyenne des échantillons
  float stddev;          // Écart-type
  int min;               // Valeur min
  int max;               // Valeur max
  int samples_count;     // Nombre d'échantillons
  int outliers_rejected; // Outliers rejetés (méthode 3-sigma)
} adc_calib_stats_t;

/**
 * @brief Initialize ADC for current sensor
 */
void adc_driver_init(void);

/**
 * @brief Read current in Amps (RMS calculation)
 * @note This is a blocking call (~200ms) that updates the internal filter
 * @return Current in Amps (Smoothed)
 */
float adc_read_current_amps(void);

/**
 * @brief Get the last sampled (smoothed) current value (Non-blocking)
 * @return Last known current in Amps
 */
float adc_get_last_current(void);

/**
 * @brief Get raw ADC value
 * @return Raw 12-bit ADC value
 */
int adc_read_raw(void);

/**
 * @brief Set current thresholds
 * @param thresholds Pointer to thresholds structure
 */
void adc_set_thresholds(const current_thresholds_t *thresholds);

/**
 * @brief Get current thresholds
 * @return Pointer to thresholds structure
 */
const current_thresholds_t *adc_get_thresholds(void);

/**
 * @brief Save current thresholds to NVS
 * @return ESP_OK on success
 */
esp_err_t adc_save_thresholds(void);

/**
 * @brief Load current thresholds from NVS
 * @return ESP_OK on success
 */
esp_err_t adc_load_thresholds(void);

/**
 * @brief Get current calibration data
 * @return Pointer to calibration structure
 */
const adc_calibration_t *adc_get_calibration(void);

/**
 * @brief Set calibration data (does NOT save to NVS)
 * @param calibration Pointer to calibration structure
 */
void adc_set_calibration(const adc_calibration_t *calibration);

/**
 * @brief Save calibration data to NVS
 * @return ESP_OK on success
 */
esp_err_t adc_save_calibration(void);

/**
 * @brief Load calibration data from NVS
 * @return ESP_OK on success
 */
esp_err_t adc_load_calibration(void);

/**
 * @brief Perform zero calibration (measures current zero point)
 * @note MANUEL - Appelé par le bouton (PRÉSERVÉ INTACT)
 * @return Measured zero offset
 */
float adc_calibrate_zero(void);

// === NOUVELLES FONCTIONS (Auto-Calibration v10.1) ===

/**
 * @brief Auto-calibration complète au démarrage
 * @note Calibration avancée avec stats et eFuse Vref
 * @return ESP_OK on success
 */
esp_err_t adc_auto_calibrate_startup(void);

/**
 * @brief Valide une calibration existante
 * @param cal Pointeur vers calibration à valider
 * @return true si valide, false sinon
 */
bool adc_validate_calibration(const adc_calibration_t *cal);

/**
 * @brief Obtenir statistiques de la dernière calibration
 * @param stats Pointeur vers structure stats (remplie par fonction)
 * @return ESP_OK on success
 */
esp_err_t adc_get_calibration_stats(adc_calib_stats_t *stats);

/**
 * @brief Log détaillé de la calibration actuelle
 */
void adc_log_calibration_details(void);

#endif // ADC_DRIVER_H
