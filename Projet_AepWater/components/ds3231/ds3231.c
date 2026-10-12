#include "ds3231.h"
#include "esp_log.h"

static const char *TAG = "DS3231";

// Fonctions utilitaires de conversion BCD (Binary-Coded Decimal) vers Décimal et inversement
static uint8_t bcd2dec(uint8_t val) { return (val >> 4) * 10 + (val & 0x0f); }
static uint8_t dec2bcd(uint8_t val) { return ((val / 10) << 4) + (val % 10); }

esp_err_t ds3231_init(i2c_port_t i2c_num, int sda_io_num, int scl_io_num) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_io_num,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl_io_num,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, // 100kHz standard pour le RTC
    };
    
    esp_err_t err = i2c_param_config(i2c_num, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erreur de configuration I2C: %s", esp_err_to_name(err));
        return err;
    }
    
    // Installation du driver I2C
    err = i2c_driver_install(i2c_num, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erreur d'installation du driver I2C: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Driver I2C installé avec succès pour le DS3231");
    }
    return err;
}

esp_err_t ds3231_get_time(i2c_port_t i2c_num, struct tm *timeinfo) {
    uint8_t data[7];
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Registre de départ (Secondes)
    
    i2c_master_start(cmd); // Repeated start pour la lecture
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_READ, true);
    
    i2c_master_read(cmd, data, 6, I2C_MASTER_ACK); // Lire 6 octets avec ACK
    i2c_master_read_byte(cmd, &data[6], I2C_MASTER_NACK); // Le dernier octet avec NACK
    i2c_master_stop(cmd);
    
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (err == ESP_OK) {
        timeinfo->tm_sec = bcd2dec(data[0]);
        timeinfo->tm_min = bcd2dec(data[1]);
        timeinfo->tm_hour = bcd2dec(data[2] & 0x3F); // Format 24h
        timeinfo->tm_wday = bcd2dec(data[3]) - 1;   // Le DS3231 utilise 1-7, struct tm utilise 0-6
        timeinfo->tm_mday = bcd2dec(data[4]);
        timeinfo->tm_mon = bcd2dec(data[5] & 0x1F) - 1; // struct tm utilise 0-11
        timeinfo->tm_year = bcd2dec(data[6]) + 100;     // struct tm compte les années depuis 1900
    } else {
        ESP_LOGE(TAG, "Erreur de lecture de l'heure: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t ds3231_set_time(i2c_port_t i2c_num, const struct tm *timeinfo) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Registre de départ
    
    i2c_master_write_byte(cmd, dec2bcd(timeinfo->tm_sec), true);
    i2c_master_write_byte(cmd, dec2bcd(timeinfo->tm_min), true);
    i2c_master_write_byte(cmd, dec2bcd(timeinfo->tm_hour), true);
    i2c_master_write_byte(cmd, dec2bcd(timeinfo->tm_wday + 1), true); // struct tm 0-6 vers DS3231 1-7
    i2c_master_write_byte(cmd, dec2bcd(timeinfo->tm_mday), true);
    i2c_master_write_byte(cmd, dec2bcd(timeinfo->tm_mon + 1), true); // struct tm 0-11 vers DS3231 1-12
    i2c_master_write_byte(cmd, dec2bcd(timeinfo->tm_year - 100), true);
    
    i2c_master_stop(cmd);
    
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erreur lors du réglage de l'heure: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Heure du RTC mise à jour avec succès");
    }
    
    return err;
}
