/**
 * @file adc_driver.c
 * @brief ADC Driver Implementation for ACS712 Current Sensor
 *
 * Implements RMS current measurement similar to v7.1.1 readCurrentAmps()
 */

#include "adc_driver.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <math.h>
#include <stdbool.h>

static const char *TAG = "ADC_DRV";

static adc_oneshot_unit_handle_t adc_handle;
static float virtual_zero = 2048.0f; // Mid-point for 12-bit ADC

// === NOUVEAU: eFuse Calibration Handle ===
static adc_cali_handle_t adc_cali_handle = NULL;
static bool adc_cali_enabled = false;

// === NOUVEAU: Stats dernière calibration ===
static adc_calib_stats_t last_calib_stats = {0};

// Default calibration (from v7.1.1)
static adc_calibration_t calibration = {.zero_offset = 2048.0f,
                                        .scale_factor = 1.0f,
                                        .is_valid = 0,
                                        // Nouveaux champs initialisés
                                        .calibration_timestamp = 0,
                                        .vref_type = 2, // Default
                                        .actual_vref_mv = 1100,
                                        .noise_level_rms = 0.0f};

// Default thresholds (from v7.1.1)
static current_thresholds_t thresholds = {
    .min_load = 0.08f, .anomaly_threshold = 0.60f, .max_critical = 4.0f};

// === FORWARD DECLARATIONS ===
static esp_err_t adc_init_with_efuse(void);

void adc_driver_init(void) {
  ESP_LOGI(TAG, "Initializing ADC for current sensor...");

  // Configure ADC unit
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

  // Configure channel
  adc_oneshot_chan_cfg_t chan_config = {
      .bitwidth = ADC_BITWIDTH_12,
      .atten = ADC_ATTEN_DB_12, // 0-3.3V range
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, CURRENT_SENSOR_CHANNEL,
                                             &chan_config));

  // === NOUVEAU: Init eFuse Vref Calibration ===
  esp_err_t efuse_err = adc_init_with_efuse();
  if (efuse_err != ESP_OK) {
    ESP_LOGW(TAG, "eFuse calibration not available, using raw ADC");
  }

  // Load calibration from NVS (if available)
  esp_err_t calib_err = adc_load_calibration();

  // === NOUVEAU: Validation et Auto-Calibration ===
  if (calib_err != ESP_OK || calibration.is_valid != 1) {
    ESP_LOGW(TAG, "No saved calibration found");
    ESP_LOGI(TAG, "Starting auto-calibration...");

    esp_err_t auto_calib_err = adc_auto_calibrate_startup();
    if (auto_calib_err == ESP_OK) {
      adc_save_calibration(); // Sauvegarder auto-calibration
      ESP_LOGI(TAG, "Auto-calibration saved to NVS");
    } else {
      ESP_LOGE(TAG, "Auto-calibration failed, using defaults");
      // Fallback sur ancien comportement
      adc_calibrate_zero();
      adc_save_calibration();
    }
  } else {
    // Calibration chargée depuis NVS - VALIDER
    ESP_LOGI(TAG, "Loaded calibration from NVS, validating...");

    if (!adc_validate_calibration(&calibration)) {
      ESP_LOGW(TAG, "Calibration validation failed, re-calibrating...");

      esp_err_t auto_calib_err = adc_auto_calibrate_startup();
      if (auto_calib_err == ESP_OK) {
        adc_save_calibration();
        ESP_LOGI(TAG, "Re-calibration complete and saved");
      } else {
        ESP_LOGW(TAG, "Auto-calibration failed, keeping old calibration");
      }
    } else {
      ESP_LOGI(TAG, "Calibration validation PASSED");
      ESP_LOGI(TAG,
               "Using saved calibration: zero=%.1f scale=%.3f age=%u hours",
               calibration.zero_offset, calibration.scale_factor,
               ((uint32_t)(esp_timer_get_time() / 1000000) -
                calibration.calibration_timestamp) /
                   3600);
      virtual_zero = calibration.zero_offset;
    }
  }

  // Load thresholds from NVS (if available)
  adc_load_thresholds();

  // === NOUVEAU: Log détaillé calibration ===
  adc_log_calibration_details();

  ESP_LOGI(TAG,
           "ADC initialized. Zero=%.1f Scale=%.3f Thresholds=[%.2f,%.2f,%.2f]",
           virtual_zero, calibration.scale_factor, thresholds.min_load,
           thresholds.anomaly_threshold, thresholds.max_critical);
}

int adc_read_raw(void) {
  if (adc_handle == NULL)
    return 0;
  int raw = 0;
  adc_oneshot_read(adc_handle, CURRENT_SENSOR_CHANNEL, &raw);
  return raw;
}

// Stored filtered current value
static float s_filtered_current = 0.0f;
static bool s_first_sample = true;

// Internal sampling function (Blocking call ~200ms)
static float adc_sample_raw_rms(void) {
  if (adc_handle == NULL)
    return 0.0f;
  // RMS measurement over ~200ms (10 cycles at 50Hz)
  const int measurement_duration_ms = 200;
  const int sample_delay_us = 40;

  double sum_squares = 0;
  int count = 0;

  int64_t start_time = esp_timer_get_time();
  int64_t end_time = start_time + (measurement_duration_ms * 1000);

  while (esp_timer_get_time() < end_time) {
    int raw;
    esp_err_t ret = adc_oneshot_read(adc_handle, CURRENT_SENSOR_CHANNEL, &raw);

    if (ret != ESP_OK) {
      count++;
      esp_rom_delay_us(sample_delay_us);
      continue;
    }

    float diff = (float)raw - virtual_zero;

    // Saturate diff
    if (diff > 2047.0f)
      diff = 2047.0f;
    if (diff < -2048.0f)
      diff = -2048.0f;

    // Noise gate
    if (fabs(diff) < NOISE_GATE_THRESHOLD) {
      diff = 0;
    }

    sum_squares += (double)(diff * diff);
    count++;

    esp_rom_delay_us(sample_delay_us);
  }

  if (count == 0)
    return 0.0f;

  double rms_adc = sqrt(sum_squares / (double)count);
  float voltage_rms = (float)((rms_adc / ADC_RESOLUTION) * ADC_VOLTAGE_REF);
  float amps_rms = voltage_rms / ADC_SENSITIVITY;

  if (calibration.is_valid == 1) {
    amps_rms *= calibration.scale_factor;
  }

  // Sanity check
  if (amps_rms > 10.0f || amps_rms < -0.1f) {
    ESP_LOGW(TAG, "Invalid current reading: %.2f A", amps_rms);
    return 0.0f;
  }

  return amps_rms;
}

// Public blocking function: Samples AND Updates Filter
float adc_read_current_amps(void) {
  float new_sample = adc_sample_raw_rms();

  // Apply EMA Filter (Alpha = 0.3)
  if (s_first_sample) {
    s_filtered_current = new_sample;
    s_first_sample = false;
  } else {
    s_filtered_current = 0.3f * new_sample + 0.7f * s_filtered_current;
  }

  return s_filtered_current;
}

// Public non-blocking getter
float adc_get_last_current(void) { return s_filtered_current; }

void adc_set_thresholds(const current_thresholds_t *new_thresholds) {
  if (new_thresholds) {
    thresholds = *new_thresholds;
    ESP_LOGI(TAG, "Thresholds updated: min=%.2f, anomaly=%.2f, max=%.2f",
             thresholds.min_load, thresholds.anomaly_threshold,
             thresholds.max_critical);
  }
}

const current_thresholds_t *adc_get_thresholds(void) { return &thresholds; }
// ========== NVS Calibration Functions ==========

const adc_calibration_t *adc_get_calibration(void) { return &calibration; }

void adc_set_calibration(const adc_calibration_t *new_cal) {
  if (new_cal) {
    calibration = *new_cal;
    ESP_LOGI(TAG, "Calibration updated: zero=%.1f scale=%.3f valid=%d",
             calibration.zero_offset, calibration.scale_factor,
             calibration.is_valid);
  }
}

esp_err_t adc_save_calibration(void) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS for calibration save: %s",
             esp_err_to_name(err));
    return err;
  }

  err = nvs_set_blob(handle, "adc_calib", &calibration, sizeof(calibration));
  if (err == ESP_OK) {
    err = nvs_commit(handle);
    ESP_LOGI(TAG, "Calibration saved to NVS: zero=%.1f scale=%.3f",
             calibration.zero_offset, calibration.scale_factor);
  } else {
    ESP_LOGE(TAG, "Failed to save calibration: %s", esp_err_to_name(err));
  }

  nvs_close(handle);
  return err;
}

esp_err_t adc_load_calibration(void) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "NVS not available (first boot?), using default calibration");
    return err;
  }

  size_t required_size = sizeof(calibration);
  adc_calibration_t temp;
  err = nvs_get_blob(handle, "adc_calib", &temp, &required_size);

  if (err == ESP_OK && temp.is_valid == 1) {
    calibration = temp;
    virtual_zero = calibration.zero_offset; // Apply loaded zero
    ESP_LOGI(TAG, "Calibration loaded from NVS: zero=%.1f scale=%.3f",
             calibration.zero_offset, calibration.scale_factor);
  } else {
    ESP_LOGW(TAG, "No valid calibration in NVS, using defaults");
  }

  nvs_close(handle);
  return err;
}

float adc_calibrate_zero(void) {
  ESP_LOGI(TAG, "Starting zero calibration (500 samples)...");
  long sum = 0;
  const int samples = 500;

  for (int i = 0; i < samples; i++) {
    int raw;
    adc_oneshot_read(adc_handle, CURRENT_SENSOR_CHANNEL, &raw);
    sum += raw;
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  float new_zero = (float)sum / samples;
  ESP_LOGI(TAG, "Zero calibration complete: %.1f (previous: %.1f)", new_zero,
           virtual_zero);

  // Update calibration structure
  calibration.zero_offset = new_zero;
  calibration.is_valid = 1;
  virtual_zero = new_zero;

  return new_zero;
}

// ========== NVS Thresholds Functions ==========

esp_err_t adc_save_thresholds(void) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS for threshold save: %s",
             esp_err_to_name(err));
    return err;
  }

  err = nvs_set_blob(handle, "adc_thresh", &thresholds, sizeof(thresholds));
  if (err == ESP_OK) {
    err = nvs_commit(handle);
    ESP_LOGI(TAG, "Thresholds saved to NVS: min=%.2f anom=%.2f max=%.2f",
             thresholds.min_load, thresholds.anomaly_threshold,
             thresholds.max_critical);
  } else {
    ESP_LOGE(TAG, "Failed to save thresholds: %s", esp_err_to_name(err));
  }

  nvs_close(handle);
  return err;
}

esp_err_t adc_load_thresholds(void) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "NVS not available, using default thresholds");
    return err;
  }

  size_t required_size = sizeof(thresholds);
  current_thresholds_t temp;
  err = nvs_get_blob(handle, "adc_thresh", &temp, &required_size);

  if (err == ESP_OK) {
    // Validate thresholds
    if (temp.min_load > 0 && temp.min_load < temp.anomaly_threshold &&
        temp.anomaly_threshold < temp.max_critical &&
        temp.max_critical <= 10.0f) {
      thresholds = temp;
      ESP_LOGI(TAG, "Thresholds loaded from NVS: min=%.2f anom=%.2f max=%.2f",
               thresholds.min_load, thresholds.anomaly_threshold,
               thresholds.max_critical);
    } else {
      ESP_LOGW(TAG, "Invalid thresholds in NVS, using defaults");
    }
  } else {
    ESP_LOGW(TAG, "No thresholds in NVS, using defaults");
  }

  nvs_close(handle);
  return err;
}

// ============================================================================
// === NOUVELLES FONCTIONS - AUTO-CALIBRATION v10.1 ===
// ============================================================================

/**
 * @brief Initialise ADC avec calibration eFuse Vref
 * @return ESP_OK on success
 */
static esp_err_t adc_init_with_efuse(void) {
  ESP_LOGI(TAG, "Initializing ADC calibration (eFuse Vref)...");

  // Configuration du scheme de calibration
  adc_cali_line_fitting_config_t cali_config = {
      .unit_id = ADC_UNIT_1,
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_12,
  };

  esp_err_t ret =
      adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle);

  if (ret == ESP_OK) {
    adc_cali_enabled = true;

    // Déterminer le type de Vref utilisé
    // Note: L'API ne retourne pas directement le type, on log juste le succès
    ESP_LOGI(TAG, "ADC calibration enabled (eFuse or curve fitting)");

    // Estimer le Vref utilisé (lecture d'un point connu)
    // Pour simplifier, on lit la valeur calibrée à ~2048 (mid-point)
    int raw_sample = 2048;
    int voltage_mv = 0; // API attend int*, pas uint32_t*
    adc_cali_raw_to_voltage(adc_cali_handle, raw_sample, &voltage_mv);

    // Calculer Vref approximatif
    // voltage_mv = (raw_sample / 4095) * Vref * (attenuation_factor)
    // Pour 12dB atten, factor ≈ 3.6
    calibration.actual_vref_mv =
        (uint32_t)((voltage_mv * 4095.0f) / (raw_sample * 3.6f));

    // Type: on suppose eFuse si disponible, sinon curve fitting
    calibration.vref_type = 0; // Assume eFuse (ESP-IDF choisit automatiquement)

    ESP_LOGI(TAG, "Estimated Vref: %u mV", calibration.actual_vref_mv);
  } else {
    adc_cali_enabled = false;
    ESP_LOGW(TAG, "ADC calibration failed, using raw values");
    calibration.vref_type = 2; // Default
    calibration.actual_vref_mv = 1100;
  }

  return ret;
}

/**
 * @brief Valide une structure de calibration
 * @param cal Pointeur vers calibration à valider
 * @return true si valide, false sinon
 */
bool adc_validate_calibration(const adc_calibration_t *cal) {
  if (!cal) {
    ESP_LOGW(TAG, "Validation failed: NULL pointer");
    return false;
  }

  // Check 1: Flag is_valid
  if (cal->is_valid != 1) {
    ESP_LOGW(TAG, "Validation failed: is_valid=%d", cal->is_valid);
    return false;
  }

  // Check 2: Zero offset dans plage raisonnable (12-bit ADC centered)
  if (cal->zero_offset < 1900.0f || cal->zero_offset > 2200.0f) {
    ESP_LOGW(TAG, "Validation failed: zero_offset=%.1f (expected 1900-2200)",
             cal->zero_offset);
    return false;
  }

  // Check 3: Scale factor raisonnable
  if (cal->scale_factor < 0.8f || cal->scale_factor > 1.2f) {
    ESP_LOGW(TAG, "Validation failed: scale_factor=%.3f (expected 0.8-1.2)",
             cal->scale_factor);
    return false;
  }

  // Check 4: Pas trop ancien (30 jours = 2592000 secondes)
  if (cal->calibration_timestamp > 0) {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000000);
    uint32_t age_sec = now - cal->calibration_timestamp;
    const uint32_t MAX_AGE = 30 * 24 * 3600; // 30 jours

    if (age_sec > MAX_AGE) {
      ESP_LOGW(TAG, "Validation failed: calibration too old (%u days)",
               age_sec / 86400);
      return false;
    }
  }

  // Check 5: Bruit acceptable (si mesuré)
  if (cal->noise_level_rms > 50.0f) {
    ESP_LOGW(TAG, "Validation failed: noise_level=%.1f (expected <50)",
             cal->noise_level_rms);
    return false;
  }

  ESP_LOGI(TAG, "Calibration validation: PASSED");
  return true;
}

/**
 * @brief Auto-calibration complète au démarrage
 * @return ESP_OK on success
 */
esp_err_t adc_auto_calibrate_startup(void) {
  ESP_LOGI(TAG, "=== Starting Auto-Calibration ===");

  const int WARMUP_SAMPLES = 100;
  const int SAMPLES = 1000;
  int raw_samples[SAMPLES];

  // PHASE 1: Warmup - jeter les premiers échantillons
  ESP_LOGI(TAG, "Phase 1: Warmup (%d samples)...", WARMUP_SAMPLES);
  for (int i = 0; i < WARMUP_SAMPLES; i++) {
    int dummy;
    adc_oneshot_read(adc_handle, CURRENT_SENSOR_CHANNEL, &dummy);
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  // PHASE 2: Collecte des échantillons
  ESP_LOGI(TAG, "Phase 2: Collecting %d samples...", SAMPLES);
  double sum = 0;
  double sum_sq = 0;
  int min_val = 4095;
  int max_val = 0;

  for (int i = 0; i < SAMPLES; i++) {
    int raw;
    esp_err_t ret = adc_oneshot_read(adc_handle, CURRENT_SENSOR_CHANNEL, &raw);

    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "ADC read error at sample %d", i);
      continue;
    }

    raw_samples[i] = raw;
    sum += raw;
    sum_sq += (raw * raw);

    if (raw < min_val)
      min_val = raw;
    if (raw > max_val)
      max_val = raw;

    vTaskDelay(pdMS_TO_TICKS(1));
  }

  // PHASE 3: Calculs statistiques
  ESP_LOGI(TAG, "Phase 3: Computing statistics...");
  double mean = sum / SAMPLES;
  double variance = (sum_sq / SAMPLES) - (mean * mean);
  double stddev = sqrt(variance);

  ESP_LOGI(TAG, "Stats: mean=%.1f stddev=%.2f min=%d max=%d", mean, stddev,
           min_val, max_val);

  // PHASE 4: Filtrage outliers (méthode 3-sigma)
  ESP_LOGI(TAG, "Phase 4: Filtering outliers (3-sigma)...");
  double filtered_sum = 0;
  int filtered_count = 0;
  double threshold = 3.0 * stddev;

  for (int i = 0; i < SAMPLES; i++) {
    if (fabs(raw_samples[i] - mean) < threshold) {
      filtered_sum += raw_samples[i];
      filtered_count++;
    }
  }

  if (filtered_count < SAMPLES / 2) {
    ESP_LOGE(TAG, "Too many outliers rejected: %d/%d", SAMPLES - filtered_count,
             SAMPLES);
    return ESP_FAIL;
  }

  float final_zero = (float)(filtered_sum / filtered_count);
  int outliers = SAMPLES - filtered_count;

  ESP_LOGI(TAG, "Outliers rejected: %d (%.1f%%)", outliers,
           (outliers * 100.0f) / SAMPLES);

  // PHASE 5: Validation du bruit
  if (stddev > 50.0) {
    ESP_LOGW(TAG, "High noise level detected: stddev=%.2f", stddev);
  }

  // PHASE 6: Mise à jour de la structure de calibration
  ESP_LOGI(TAG, "Phase 5: Updating calibration...");

  calibration.zero_offset = final_zero;
  calibration.is_valid = 1;
  calibration.calibration_timestamp =
      (uint32_t)(esp_timer_get_time() / 1000000);
  calibration.noise_level_rms = (float)stddev;
  // vref_type et actual_vref_mv déjà mis à jour par adc_init_with_efuse()

  virtual_zero = final_zero; // Appliquer immédiatement

  // Sauvegarder les stats
  last_calib_stats.mean = (float)mean;
  last_calib_stats.stddev = (float)stddev;
  last_calib_stats.min = min_val;
  last_calib_stats.max = max_val;
  last_calib_stats.samples_count = SAMPLES;
  last_calib_stats.outliers_rejected = outliers;

  ESP_LOGI(TAG, "=== Auto-Calibration Complete ===");
  ESP_LOGI(TAG, "Final zero offset: %.1f ADC counts", final_zero);
  ESP_LOGI(TAG, "Noise level: %.2f ADC counts RMS", stddev);

  return ESP_OK;
}

/**
 * @brief Obtenir statistiques de la dernière calibration
 * @param stats Pointeur vers structure stats
 * @return ESP_OK on success
 */
esp_err_t adc_get_calibration_stats(adc_calib_stats_t *stats) {
  if (!stats) {
    return ESP_ERR_INVALID_ARG;
  }

  *stats = last_calib_stats;
  return ESP_OK;
}

/**
 * @brief Log détaillé de la calibration actuelle
 */
void adc_log_calibration_details(void) {
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "    ADC Calibration Report");
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "Zero offset:     %.1f ADC counts", calibration.zero_offset);
  ESP_LOGI(TAG, "Scale factor:    %.3f", calibration.scale_factor);
  ESP_LOGI(TAG, "Valid:           %s", calibration.is_valid ? "YES" : "NO");
  ESP_LOGI(TAG, "Vref type:       %d (0=eFuse, 1=TwoPoint, 2=Default)",
           calibration.vref_type);
  ESP_LOGI(TAG, "Actual Vref:     %u mV", calibration.actual_vref_mv);
  ESP_LOGI(TAG, "Noise RMS:       %.2f ADC counts",
           calibration.noise_level_rms);

  if (calibration.calibration_timestamp > 0) {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000000);
    uint32_t age_sec = now - calibration.calibration_timestamp;
    uint32_t age_hours = age_sec / 3600;
    ESP_LOGI(TAG, "Timestamp:       %u (age: %u hours)",
             calibration.calibration_timestamp, age_hours);
  } else {
    ESP_LOGI(TAG, "Timestamp:       N/A (never calibrated)");
  }

  ESP_LOGI(TAG, "----------------------------------------");
  ESP_LOGI(TAG, "Last Calibration Stats:");
  ESP_LOGI(TAG, "  Mean:          %.1f", last_calib_stats.mean);
  ESP_LOGI(TAG, "  Std Dev:       %.2f", last_calib_stats.stddev);
  ESP_LOGI(TAG, "  Min:           %d", last_calib_stats.min);
  ESP_LOGI(TAG, "  Max:           %d", last_calib_stats.max);
  ESP_LOGI(TAG, "  Samples:       %d", last_calib_stats.samples_count);
  ESP_LOGI(TAG, "  Outliers:      %d", last_calib_stats.outliers_rejected);
  ESP_LOGI(TAG, "========================================");
}
