/*
 * Projet : Système de Gestion Intelligente de l'Eau (Anti-Gaspillage) - AepWater
 * Description : Contrôle d'électrovanne via RTC et détection de fuite (capteur de débit)
 * Framework : ESP-IDF (C)
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ds3231.h"

static const char *TAG = "AepWater";

// Définition des broches
#define VALVE_RELAY_PIN 4
#define FLOW_SENSOR_PIN 5

// Broches I2C pour le module RTC DS3231
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0

#define ESP_INTR_FLAG_DEFAULT 0

// Variables globales pour le capteur de débit
volatile int pulse_count = 0;
float flow_rate = 0.0;
int64_t continuous_flow_start_time = 0;
bool is_leaking = false;

// Service d'interruption (ISR) pour le capteur de débit
static void IRAM_ATTR flow_sensor_isr_handler(void* arg) {
    pulse_count++;
}

// Tâche FreeRTOS pour surveiller le débit d'eau
void flow_monitoring_task(void *pvParameter) {
    int64_t last_time = esp_timer_get_time(); // Temps en microsecondes
    
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Attendre 1 seconde
        
        int64_t current_time = esp_timer_get_time();
        int64_t time_diff_us = current_time - last_time;
        
        // Désactiver temporairement l'interruption pour éviter les accès concurrents
        gpio_intr_disable(FLOW_SENSOR_PIN);
        int current_pulses = pulse_count;
        pulse_count = 0;
        gpio_intr_enable(FLOW_SENSOR_PIN);
        
        last_time = current_time;
        
        // Calcul du débit (Formule YF-S201: F = 7.5 * Q en L/min)
        flow_rate = ((float)current_pulses / 7.5f) * (1000000.0f / (float)time_diff_us);
        
        // Détection de fuite (flux continu pendant > 30 minutes)
        if (flow_rate > 0) {
            if (continuous_flow_start_time == 0) {
                continuous_flow_start_time = esp_timer_get_time();
            } else if ((esp_timer_get_time() - continuous_flow_start_time) > (30LL * 60LL * 1000000LL)) { // 30 mins en µs
                if (!is_leaking) {
                    ESP_LOGE(TAG, "ALERTE : Fuite détectée ou robinet oublié ! Coupure de l'eau.");
                    gpio_set_level(VALVE_RELAY_PIN, 0); // Fermer l'électrovanne
                    is_leaking = true;
                }
            }
        } else {
            continuous_flow_start_time = 0; // Réinitialiser si le flux s'arrête (robinet bien fermé)
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Démarrage du système AepWater sous ESP-IDF...");

    // 1. Initialisation I2C et RTC DS3231
    ESP_LOGI(TAG, "Initialisation du module RTC DS3231...");
    if (ds3231_init(I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO) == ESP_OK) {
        struct tm rtc_time;
        if (ds3231_get_time(I2C_MASTER_NUM, &rtc_time) == ESP_OK) {
            ESP_LOGI(TAG, "Heure actuelle RTC : %02d/%02d/%04d %02d:%02d:%02d",
                     rtc_time.tm_mday, rtc_time.tm_mon + 1, rtc_time.tm_year + 1900,
                     rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec);
        }
    } else {
        ESP_LOGE(TAG, "Échec de l'initialisation du RTC.");
    }

    // 2. Configuration du relais (Électrovanne)
    gpio_config_t io_conf_relay = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << VALVE_RELAY_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf_relay);
    gpio_set_level(VALVE_RELAY_PIN, 0); // Par sécurité, la valve est fermée au démarrage

    // 3. Configuration du capteur de débit
    gpio_config_t io_conf_sensor = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << FLOW_SENSOR_PIN),
        .pull_down_en = 0,
        .pull_up_en = 1
    };
    gpio_config(&io_conf_sensor);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(FLOW_SENSOR_PIN, flow_sensor_isr_handler, (void*) FLOW_SENSOR_PIN);

    // 4. Lancement de la tâche FreeRTOS de monitoring
    xTaskCreate(&flow_monitoring_task, "flow_monitoring_task", 4096, NULL, 5, NULL);

    // Boucle principale (ou tâche de fond)
    while(1) {
        struct tm current_time;
        if (ds3231_get_time(I2C_MASTER_NUM, &current_time) == ESP_OK) {
            // Exemple basique: L'école est ouverte de 07h30 à 18h30 (Heure: 7 à 18)
            // L'eau est coupée le dimanche (tm_wday == 0)
            bool is_open_hours = (current_time.tm_hour >= 7 && current_time.tm_hour < 18);
            bool is_sunday = (current_time.tm_wday == 0);
            
            if (is_open_hours && !is_sunday && !is_leaking) {
                gpio_set_level(VALVE_RELAY_PIN, 1); // Ouvrir
            } else {
                gpio_set_level(VALVE_RELAY_PIN, 0); // Fermer
            }
        }
        
        vTaskDelay(10000 / portTICK_PERIOD_MS); // Vérifier l'heure toutes les 10 secondes
    }
}
