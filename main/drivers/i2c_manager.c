#include "i2c_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "I2C_MANAGER";
static bool s_initialized = false;
static SemaphoreHandle_t s_i2c_mutex = NULL;

esp_err_t i2c_manager_init(void) {
  if (s_initialized) {
    ESP_LOGW(TAG, "I2C Manager already initialized!");
    return ESP_OK;
  }

  s_i2c_mutex = xSemaphoreCreateMutex();
  if (s_i2c_mutex == NULL) {
    ESP_LOGE(TAG, "Failed to create I2C Mutex!");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Initializing Shared I2C Bus (SDA=%d, SCL=%d)...",
           I2C_MANAGER_SDA_PIN, I2C_MANAGER_SCL_PIN);

  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = I2C_MANAGER_SDA_PIN,
      .scl_io_num = I2C_MANAGER_SCL_PIN,
      .sda_pullup_en = GPIO_PULLUP_ENABLE, // Internal pullups (Weak)
      .scl_pullup_en = GPIO_PULLUP_ENABLE, // Internal pullups (Weak)
      .master.clk_speed = I2C_MANAGER_FREQ_HZ,
  };

  esp_err_t err = i2c_param_config(I2C_MANAGER_PORT, &conf);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2C Param Config Failed: %s", esp_err_to_name(err));
    return err;
  }

  err = i2c_driver_install(I2C_MANAGER_PORT, conf.mode, 0, 0, 0);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2C Driver Install Failed: %s", esp_err_to_name(err));
    return err;
  }

  // Increase timeout to avoid spurious errors on slow slaves
  i2c_set_timeout(I2C_MANAGER_PORT,
                  80000); // ~1s at 80MHz APB (Default is ~300us?) Actually unit
                          // is APB cycles.

  s_initialized = true;
  ESP_LOGI(TAG, "Shared I2C Bus Initialized Successfully.");
  return ESP_OK;
}

bool i2c_manager_check_device(uint8_t address) {
  if (!s_initialized)
    return false;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
  i2c_master_stop(cmd);

  xSemaphoreTake(s_i2c_mutex, portMAX_DELAY);
  esp_err_t ret =
      i2c_master_cmd_begin(I2C_MANAGER_PORT, cmd, pdMS_TO_TICKS(50));
  xSemaphoreGive(s_i2c_mutex);

  i2c_cmd_link_delete(cmd);

  return (ret == ESP_OK);
}

void i2c_manager_scan_bus(void) {
  if (!s_initialized) {
    ESP_LOGE(TAG, "Cannot scan: Bus not initialized!");
    return;
  }

  ESP_LOGI(TAG, "--- Starting I2C Bus Scan ---");
  int devices_found = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    if (i2c_manager_check_device(addr)) {
      ESP_LOGI(TAG, "✅ Device Found at Address: 0x%02X", addr);
      if (addr == I2C_ADDR_LCD)
        ESP_LOGI(TAG, "   -> Likely LCD 16x2");
      if (addr == I2C_ADDR_DS3231)
        ESP_LOGI(TAG, "   -> Likely DS3231 RTC");
      devices_found++;
      // Small delay between successful probes
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }

  if (devices_found == 0) {
    ESP_LOGW(TAG,
             "❌ NO I2C DEVICES FOUND! Check wiring (SDA/SCL) and Pullups.");
  } else {
    ESP_LOGI(TAG, "--- Scan Complete: %d Devices Found ---", devices_found);
  }
}

// --- Helper Wrappers ---

esp_err_t i2c_manager_write(uint8_t addr, const uint8_t *data, size_t len) {
  if (!s_initialized)
    return ESP_FAIL;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write(cmd, data, len, true);
  i2c_master_stop(cmd);

  xSemaphoreTake(s_i2c_mutex, portMAX_DELAY);
  esp_err_t ret =
      i2c_master_cmd_begin(I2C_MANAGER_PORT, cmd, pdMS_TO_TICKS(100));
  xSemaphoreGive(s_i2c_mutex);
  i2c_cmd_link_delete(cmd);
  return ret;
}

esp_err_t i2c_manager_read(uint8_t addr, uint8_t *data, size_t len) {
  if (!s_initialized)
    return ESP_FAIL;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
  if (len > 1) {
    i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
  i2c_master_stop(cmd);

  xSemaphoreTake(s_i2c_mutex, portMAX_DELAY);
  esp_err_t ret =
      i2c_master_cmd_begin(I2C_MANAGER_PORT, cmd, pdMS_TO_TICKS(100));
  xSemaphoreGive(s_i2c_mutex);
  i2c_cmd_link_delete(cmd);
  return ret;
}

esp_err_t i2c_manager_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data,
                                size_t len) {
  if (!s_initialized)
    return ESP_FAIL;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg, true);
  if (len > 0) {
    i2c_master_write(cmd, data, len, true);
  }
  i2c_master_stop(cmd);

  xSemaphoreTake(s_i2c_mutex, portMAX_DELAY);
  esp_err_t ret =
      i2c_master_cmd_begin(I2C_MANAGER_PORT, cmd, pdMS_TO_TICKS(100));
  xSemaphoreGive(s_i2c_mutex);
  i2c_cmd_link_delete(cmd);
  return ret;
}

esp_err_t i2c_manager_read_reg(uint8_t addr, uint8_t reg, uint8_t *data,
                               size_t len) {
  if (!s_initialized)
    return ESP_FAIL;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  // Write Register Address
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg, true);

  // Restart for Read
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
  if (len > 1) {
    i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
  }
  if (len > 0) {
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
  }
  i2c_master_stop(cmd);

  xSemaphoreTake(s_i2c_mutex, portMAX_DELAY);
  esp_err_t ret =
      i2c_master_cmd_begin(I2C_MANAGER_PORT, cmd, pdMS_TO_TICKS(100));
  xSemaphoreGive(s_i2c_mutex);
  i2c_cmd_link_delete(cmd);
  return ret;
}
