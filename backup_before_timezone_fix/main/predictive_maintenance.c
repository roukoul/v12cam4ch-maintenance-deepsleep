#include "predictive_maintenance.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_idf_version.h"
#include "alarm_manager.h"

// Temperature Sensor includes

#include "sdkconfig.h"
#include "ds3231.h"

#if defined(CONFIG_IDF_TARGET_ESP32) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  // ESPRESSIF REMOVED hardware temperature sensor for ESP32 Classic in v5.0+
  // We must mock it to prevent linker "undefined reference" errors.
  #define USE_MOCK_TSENS 1
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #include "driver/temperature_sensor.h"
  static temperature_sensor_handle_t temp_handle = NULL;
  #define USE_LEGACY_TSENS 0
  #define USE_MOCK_TSENS 0
#else
  #include "driver/temp_sensor.h"
  #define USE_LEGACY_TSENS 1
  #define USE_MOCK_TSENS 0
#endif

static const char *TAG = "PM_AI";
static float s_relay_health = 100.0f;

void pm_init(void) {
    ESP_LOGI(TAG, "Initializing Predictive Maintenance AI...");

    // 1. Initialize temperature sensor
#if defined(USE_MOCK_TSENS) && USE_MOCK_TSENS == 1
    ESP_LOGW(TAG, "Hardware Temp Sensor is unsupported on ESP32 in IDF v5+. Using fixed 40C mock.");
#elif USE_LEGACY_TSENS == 0
    temperature_sensor_config_t config_tsens = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    if (temperature_sensor_install(&config_tsens, &temp_handle) == ESP_OK) {
        temperature_sensor_enable(temp_handle);
        ESP_LOGI(TAG, "Temperature sensor initialized (IDF v5+)");
    } else {
        ESP_LOGE(TAG, "Failed to initialize temperature sensor");
    }
#else
    temp_sensor_config_t config_tsens = TSENS_CONFIG_DEFAULT();
    temp_sensor_get_config(&config_tsens);
    if (temp_sensor_set_config(config_tsens) == ESP_OK && temp_sensor_start() == ESP_OK) {
        ESP_LOGI(TAG, "Temperature sensor initialized (Legacy API)");
    } else {
        ESP_LOGE(TAG, "Failed to initialize temperature sensor");
    }
#endif

    // 2. Load health from NVS
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        uint32_t int_health = 10000; // 100.00%
        if (nvs_get_u32(handle, "pm_health", &int_health) == ESP_OK) {
            s_relay_health = int_health / 100.0f;
            ESP_LOGI(TAG, "Relay Health Loaded: %.2f%%", s_relay_health);
        } else {
            ESP_LOGI(TAG, "First boot detected. Initializing Health to 100.00%%");
            nvs_set_u32(handle, "pm_health", 10000);
            nvs_commit(handle);
        }
        nvs_close(handle);
    }
}

float pm_get_temp(void) {
    // 1. Try DS3231 RTC Temperature first (More accurate Ambient/Board temp)
    float ds_temp = ds3231_get_temp();
    if (ds_temp > 0.01f || ds_temp < -0.01f) {
        return ds_temp;
    }

    // 2. Fallback to ESP32 Internal / Mock
    float tsens_out = 40.0f; // Typical normal operating temp
#if defined(USE_MOCK_TSENS) && USE_MOCK_TSENS == 1
    // Return mock 40.0f
#elif USE_LEGACY_TSENS == 0
    if (temp_handle) {
        temperature_sensor_get_celsius(temp_handle, &tsens_out);
    }
#else
    temp_sensor_read_celsius(&tsens_out);
#endif
    return tsens_out;
}

float pm_get_health(void) {
    return s_relay_health;
}

void pm_record_ring(uint32_t duration_ms) {
    float temp = pm_get_temp();
    
    // Algorithm: Base wear per second
    // Assuming relay dies after 100,000 seconds (approx 27 hours) of CONTINUOUS ringing in perfect conditions.
    // 100,000s = 100% -> 1s = 0.001% wear.
    float base_wear_per_sec = 0.001f; 
    
    // Fuzzy logic multipliers based on temperature
    float temp_multiplier = 1.0f;
    if (temp > 60.0f) {
        temp_multiplier = 5.0f; // Critical heat: 5x damage
    } else if (temp > 45.0f) {
        temp_multiplier = 2.5f; // High heat: 2.5x damage
    } else if (temp > 35.0f) {
        temp_multiplier = 1.2f; // Slight heat: 1.2x damage
    }
    
    float wear = base_wear_per_sec * temp_multiplier * (duration_ms / 1000.0f);
    
    s_relay_health -= wear;
    if (s_relay_health < 0.0f) s_relay_health = 0.0f;
    
    ESP_LOGI(TAG, "Ring %lu ms at %.1fC -> Wear: -%.4f%%. New Health: %.2f%%", duration_ms, temp, wear, s_relay_health);
    
    // Avoid writing to flash too often. Write only if integer percentage drops to save NVS.
    // But since this is a prototype, we'll write it every time to ensure accuracy.
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_u32(handle, "pm_health", (uint32_t)(s_relay_health * 100.0f));
        nvs_commit(handle);
        nvs_close(handle);
    }
}

int pm_get_days_remaining(void) {
    if (s_relay_health <= 20.0f) return 0; // Already critical

    uint32_t weekly_seconds = alarm_calculate_weekly_ring_seconds();
    
    if (weekly_seconds == 0) return 9999; // Infinite / No active alarms

    float temp = pm_get_temp();
    
    // Using the same algorithm logic as pm_record_ring
    float base_wear_per_sec = 0.001f; 
    
    float temp_multiplier = 1.0f;
    if (temp > 60.0f) {
        temp_multiplier = 5.0f;
    } else if (temp > 45.0f) {
        temp_multiplier = 2.5f;
    } else if (temp > 35.0f) {
        temp_multiplier = 1.2f;
    }

    float wear_per_week = base_wear_per_sec * temp_multiplier * weekly_seconds;
    
    if (wear_per_week <= 0.0001f) return 9999;
    
    float remaining_health = s_relay_health - 20.0f;
    float weeks_remaining = remaining_health / wear_per_week;
    
    return (int)(weeks_remaining * 7.0f);
}
