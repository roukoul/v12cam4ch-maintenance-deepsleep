#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "mdns.h"
#include "nvs.h" // Required for NVS functions
#include "nvs_flash.h"
#include "esp_task_wdt.h" // Required for internal task watchdog

// Manual declarations to satisfy compiler if headers are filtered
#ifndef localtime_r
struct tm *localtime_r(const time_t *timep, struct tm *result);
#endif
#ifndef setenv
int setenv(const char *name, const char *value, int overwrite);
#endif

// Phase 5.2: Hardware Drivers
#include "adc_driver.h"
#include "buzzer_pattern.h" // Advanced buzzer with patterns
#include "logging.h"
// #include "dns_server.c" // Removed: .c files should not be included directly
#include "predictive_maintenance.h"
#include "power_manager.h"
#include "drivers/ds3231.h"
#include "drivers/gpio_driver.h"
#include "drivers/i2c_manager.h"
#include "drivers/lcd_i2c.h"

// Phase 5.3: Web Server
#include "http_server.h"
#include "https_server.h" // Added for HTTPS

static const char *TAG = "AEP_BILL";


// WiFi credentials fallback
#define WIFI_SSID "AepBill_Fallback"
#define WIFI_PASS "12345678"
#define WIFI_MAXIMUM_RETRY 5

// WiFi event group
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// Global Anomaly Code (0=None, 1=Overload, 2=Leak, 3=Underload, 4=NTP)
volatile int g_system_anomaly_code = 0;

static int s_retry_num = 0;
static bool s_app_started = false;
static TaskHandle_t s_reconnect_task_handle = NULL;

// Forward declaration
void start_ap_fallback(void);

static void wifi_reconnect_task(void *pvParameters) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(120000)); // Wait 2 minutes (120 seconds) before each try
    ESP_LOGI(TAG, "Background task: Attempting to reconnect to home WiFi...");
    esp_wifi_connect();
  }
}

static void trigger_ap_fallback_task(void *pvParameters) {
  (void)pvParameters;
  ESP_LOGW(TAG, "Task switching to AP Fallback Mode...");
  vTaskDelay(pdMS_TO_TICKS(1000)); // Ensure WiFi event handler has cleanly exited
  start_ap_fallback();

  if (s_reconnect_task_handle == NULL) {
      xTaskCreate(wifi_reconnect_task, "wifi_reconnect_task", 4096, NULL, 5, &s_reconnect_task_handle);
  }

  vTaskDelete(NULL);
}

// ========== AP BACKGROUND CLEANUP TASK ==========
// Runs every 2 seconds to check if clients have silently disconnected
// and aggressively clears blocked lwIP TCP sockets.
static void cleanup_ap_task(void *pvParameters) {
  int last_sta_count = -1;
  while (1) {
    wifi_sta_list_t wifi_sta_list;
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    
    // If we transition from having clients to having 0 clients, do a hard socket purge
    if (last_sta_count > 0 && wifi_sta_list.num == 0) {
        ESP_LOGW(TAG, "AP Empty (2s background check). Force cleaning HTTP sockets...");
        extern void http_server_stop(void);
        extern esp_err_t start_http_server(void);
        
        http_server_stop();
        vTaskDelay(pdMS_TO_TICKS(500)); // Time for lwIP to recycle
        start_http_server();
    }
    last_sta_count = wifi_sta_list.num;
    vTaskDelay(pdMS_TO_TICKS(2000)); // Check every 2 seconds
  }
}

// ========== WATCHDOG HEARTBEAT ==========
#define HEARTBEAT_PIN GPIO_NUM_13
volatile bool g_is_entering_sleep = false;

void heartbeat_feed(void) {
    static bool state = false;
    state = !state;
    gpio_set_level(HEARTBEAT_PIN, state);
    
    // Also feed the internal task watchdog just in case this task is monitored
    esp_task_wdt_reset();
}

void heartbeat_task(void *pvParameters) {
  ESP_LOGI(TAG, "Starting Heartbeat Task on GPIO %d", HEARTBEAT_PIN);
  gpio_reset_pin(HEARTBEAT_PIN);
  gpio_set_direction(HEARTBEAT_PIN, GPIO_MODE_OUTPUT);

  while (1) {
    heartbeat_feed();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Periodic check for power saving window
    power_manager_check();
  }
}

// ========== NVS Helper Functions ==========
esp_err_t nvs_read_string(const char *key, char *value, size_t max_len) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
  if (err != ESP_OK) {
    if (err != ESP_ERR_NVS_NOT_FOUND) {
      ESP_LOGD("NVS", "Read: NVS handle error for %s: %s", key,
               esp_err_to_name(err));
    }
    return err;
  }

  size_t required_size = max_len;
  err = nvs_get_str(handle, key, value, &required_size);
  if (err == ESP_OK) {
    ESP_LOGI("NVS", "Read: %s = %s", key, value);
  } else if (err != ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGE("NVS", "Read: Failed to get string for %s: %s", key,
             esp_err_to_name(err));
  }
  nvs_close(handle);
  return err;
}

esp_err_t nvs_write_string(const char *key, const char *value) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE("NVS", "Error opening NVS for write: %s", esp_err_to_name(err));
    return err;
  }

  err = nvs_set_str(handle, key, value);
  if (err == ESP_OK) {
    err = nvs_commit(handle);
    if (err == ESP_OK) {
      ESP_LOGI("NVS", "Successfully saved %s = %s", key, value);
    } else {
      ESP_LOGE("NVS", "NVS Commit FAILED for %s: %s", key,
               esp_err_to_name(err));
    }
  } else {
    ESP_LOGE("NVS", "NVS SetStr FAILED for %s: %s", key, esp_err_to_name(err));
  }
  nvs_close(handle);
  return err;
}

// ========== WiFi Event Handler ==========
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    ESP_LOGI(TAG, "WiFi STA started, attempting connection...");
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    wifi_event_sta_disconnected_t *event =
        (wifi_event_sta_disconnected_t *)event_data;
    ESP_LOGW(TAG, "WiFi disconnected, reason: %d", event->reason);
    if (s_retry_num < WIFI_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "Retry WiFi connection (%d/%d)", s_retry_num,
               WIFI_MAXIMUM_RETRY);
    } else {
      if (s_retry_num == WIFI_MAXIMUM_RETRY) {
          ESP_LOGE(TAG, "WiFi connection FAILED after %d retries!", WIFI_MAXIMUM_RETRY);
          xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);

          if (s_app_started) {
            ESP_LOGW(TAG, "WiFi connection lost during runtime. Triggering AP fallback...");
            s_retry_num = WIFI_MAXIMUM_RETRY + 1; // Prevent future STA_DISCONNECTED events from looping
            xTaskCreate(trigger_ap_fallback_task, "ap_fallback_task", 4096, NULL, 5, NULL);
          }
      }
    }
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGW(TAG, "Station %02x:%02x:%02x:%02x:%02x:%02x left our AP. Restarting HTTP server to clear dead sockets.",
             event->mac[0], event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5]);
    // Forcefully restart the HTTP server to clear any lingering CLOSE_WAIT or FIN_WAIT sockets.
    extern void http_server_stop(void);
    extern esp_err_t start_http_server(void);

    http_server_stop();
    vTaskDelay(pdMS_TO_TICKS(500)); // Give lwIP time to clean up TCP sockets
    start_http_server();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

    // Log l'IP
    ESP_LOGI(TAG, "WiFi connected! Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

    if (s_reconnect_task_handle != NULL) {
        ESP_LOGI(TAG, "Background reconnection SUCCESS! We are back online.");
        vTaskDelete(s_reconnect_task_handle);
        s_reconnect_task_handle = NULL;
        // Stop the AP network since we are connected to the home router again
        esp_wifi_set_mode(WIFI_MODE_STA);
    }

    // === NOUVEAU: Afficher IP sur LCD pendant 10 secondes ===
    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));
    lcd_show_ip_address(ip_str, false); // false = WiFi Station mode

    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

// ========== SoftAP Fallback ==========
extern void start_dns_server(uint8_t ip_last_octet);

void start_ap_fallback(void) {
  ESP_LOGW(TAG, "Starting Access Point (AP) Fallback Mode...");

  // Get MAC Address to generate unique SSID and IP
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);

  // Generate unique IP octet (avoid 0, 1, 255)
  uint8_t ip_last = mac[5];
  if (ip_last == 0 || ip_last == 1 || ip_last == 255) {
    ip_last = 100 + (mac[4] % 100); // Fallback to safe range
  }

  // Generate unique SSID
  char unique_ssid[32];
  snprintf(unique_ssid, sizeof(unique_ssid), "Control_System_v12_%02X%02X", mac[4], mac[5]);

  // Stop previous STA attempt to perform clean AP start
  esp_wifi_stop();

  static esp_netif_t *ap_netif = NULL;
  if (ap_netif == NULL) {
      ap_netif = esp_netif_create_default_wifi_ap();
  }

  // Set static IP for AP
  esp_netif_ip_info_t ip_info;
  IP4_ADDR(&ip_info.ip, 192, 168, 4, ip_last);
  IP4_ADDR(&ip_info.gw, 192, 168, 4, ip_last);
  IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
  esp_netif_dhcps_stop(ap_netif);
  esp_netif_set_ip_info(ap_netif, &ip_info);
  esp_netif_dhcps_start(ap_netif);

  wifi_config_t wifi_ap_config = {
      .ap =
          {
              .channel = 1,
              .password = "12345678",
              .max_connection = 4,
              .authmode = WIFI_AUTH_WPA2_PSK,
              .pmf_cfg =
                  {
                      .required = false,
                  },
          },
  };
  
  // Copy generated SSID to config
  strncpy((char *)wifi_ap_config.ap.ssid, unique_ssid, sizeof(wifi_ap_config.ap.ssid));
  wifi_ap_config.ap.ssid_len = strlen(unique_ssid);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  char ip_str[16];
  snprintf(ip_str, sizeof(ip_str), "192.168.4.%d", ip_last);

  ESP_LOGI(TAG, "====================================");
  ESP_LOGI(TAG, "AP Mode Started Successfully!");
  ESP_LOGI(TAG, "SSID: '%s'", unique_ssid);
  ESP_LOGI(TAG, "Password: '12345678'");
  ESP_LOGI(TAG, "IP Address: %s", ip_str);
  ESP_LOGI(TAG, "Access http://%s to configure", ip_str);
  ESP_LOGI(TAG, "====================================");

  // === NOUVEAU: Afficher IP AP sur LCD pendant 10 secondes ===
  lcd_show_ip_address(ip_str, true); // true = Access Point mode

  // === NOUVEAU: Démarrer DNS Captif pour redirection automatique avec l'IP unique ===
  start_dns_server(ip_last);

  // START AP BACKGROUND CLEANUP
  static TaskHandle_t s_cleanup_task_handle = NULL;
  if (s_cleanup_task_handle == NULL) {
    xTaskCreate(cleanup_ap_task, "cleanup_ap_task", 4096, NULL, 5, &s_cleanup_task_handle);
  }

  ESP_LOGI(TAG, "AP Started. Connect to SSID: '%s' Password: '12345678' to configure.", unique_ssid);
}

// ========== WiFi Initialization ==========
void wifi_init_sta(void) {
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,
      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL,
      &instance_got_ip));

  wifi_config_t wifi_config = {0};

  // Try to load from NVS
  char ssid_buf[33] = {0};
  char pass_buf[65] = {0};

  esp_err_t err_ssid = nvs_read_string("wifi_ssid", ssid_buf, sizeof(ssid_buf));
  esp_err_t err_pass = nvs_read_string("wifi_pass", pass_buf, sizeof(pass_buf));

  if (err_ssid == ESP_OK && err_pass == ESP_OK && strlen(ssid_buf) > 0) {
    ESP_LOGI(TAG, "Loaded WiFi credentials from NVS. SSID: %s", ssid_buf);
    strncpy((char *)wifi_config.sta.ssid, ssid_buf,
            sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, pass_buf,
            sizeof(wifi_config.sta.password));
  } else {
    ESP_LOGW(
        TAG,
        "WiFi credentials not found in NVS, using default/hardcoded. SSID: %s",
        WIFI_SSID);
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID,
            sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS,
            sizeof(wifi_config.sta.password));
  }

  // WPA2/WPA3 Transition Mode with PMF
  wifi_config.sta.threshold.authmode =
      WIFI_AUTH_WPA2_PSK; // Min security WPA2 for transition
  wifi_config.sta.sae_pwe_h2e =
      WPA3_SAE_PWE_BOTH; // Support both H2E and Hunt-and-Peck
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false; // false for transition mode

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi Security: WPA2/WPA3 Transition Mode with PMF");
  ESP_LOGI(TAG, "WiFi initialization finished. Connecting to SSID:%s",
           wifi_config.sta.ssid);

  // Wait for connection
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "WiFi connected successfully");
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGE(TAG, "WiFi connection failed. Switching to AP Mode...");
    start_ap_fallback();
  }

  s_app_started = true;
}

// ========== SNTP Time Sync ==========
volatile bool g_is_time_synced = false;

static void time_sync_notification_cb(struct timeval *tv) {
  (void)tv;
  ESP_LOGI("AEP_BILL", "Time synchronized with NTP server");
  g_is_time_synced = true;
  ds3231_sync_from_system(); // Mettre à jour le DS3231 avec l'heure exacte d'Internet
}

void initialize_sntp(void) {
  ESP_LOGI(TAG, "Initializing SNTP");
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_set_time_sync_notification_cb((void *)time_sync_notification_cb);
  esp_sntp_init();

  // Wait for time to be set
  time_t now = 0;
  struct tm timeinfo = {0};
  int retry = 0;
  const int retry_count = 15;

  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET &&
         ++retry < retry_count) {
    ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry,
             retry_count);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }

  time(&now);
  localtime_r(&now, &timeinfo);

  if (timeinfo.tm_year < (2016 - 1900)) {
    ESP_LOGW(TAG, "Time not set, using fallback");
  } else {
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Current time: %s", strftime_buf);
  }

}
#include "alarm_manager.h"

void initialise_mdns(void) {
  // Initialiser le service mDNS
  esp_err_t err = mdns_init();
  if (err) {
    ESP_LOGE("MDNS", "mDNS Init failed: %d", err);
    return;
  }
  // Définir le nom de l'hôte : http://aepbill.local
  mdns_hostname_set("aepbill");
  // Définir le nom de l'instance
  mdns_instance_name_set("نظام التحكم اللاسلكي الاتوماتيكي");

  // Ajouter les services pour la découverte réseau
  mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
  mdns_service_add(NULL, "_https", "_tcp", 443, NULL, 0);

  ESP_LOGI("MDNS", "mDNS started. Access via: http://aepbill.local");
}

void app_main(void) {
  printf("\n\n");
  printf("  \033[1;36m_                ____  _ _ _ \033[0m\n");
  printf(" \033[1;36m/ \\   ___ _ __ | __ )(_) | |\033[0m\n");
  printf("\033[1;36m/ _ \\ / _ \\ '_ \\|  _ \\| | | |\033[0m\n");
  printf("\033[1;36m/ ___ \\  __/ |_) | |_) | | | |\033[0m\n");
  printf("\033[1;36m/_/   \\_\\___| .__/|____/|_|_|_|\033[0m\n");
  printf("            \033[1;33m|_| v12.0 FINAL\033[0m\n\n");

  ESP_LOGI(TAG, "================================================");
  ESP_LOGI(TAG, "    نظام التحكم v12.0 - Accès mDNS Activé");
  ESP_LOGI(TAG, "================================================");

  // 1. Initialize NVS (replaces EEPROM)
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_LOGI(TAG, "[1/7] NVS initialized");

  // NOUVEAU: Initialiser la Zone Horaire IMMEDIATEMENT après le NVS
  // C'est vital de le faire avant d'appeler ds3231_sync_to_system() pour 
  // que l'heure locale stockée dans le RTC génère la bonne heure UTC interne.
  char timezone[64] = {0};
  if (nvs_read_string("timezone", timezone, sizeof(timezone)) == ESP_OK && strlen(timezone) > 0) {
    ESP_LOGI(TAG, "Setting timezone from NVS: %s", timezone);
    setenv("TZ", timezone, 1);
  } else {
    ESP_LOGW(TAG, "Timezone not set in NVS, using default CET-1");
    setenv("TZ", "CET-1", 1);
  }
  tzset();
  ESP_LOGI(TAG, "[1b/7] Timezone applied globally");

  // --- NOUVEAU: Initialisation PRIORITAIRE de l'horloge I2C ---
  i2c_manager_init();
  vTaskDelay(pdMS_TO_TICKS(100));
  i2c_manager_scan_bus();

  // --- NOUVEAU: Initialisation de l'energie APRES I2C ---
  // On le fait apres I2C pour que ds3231_check_power_lost() fonctionne
  extern void power_manager_init(void);
  power_manager_init();

  // 2a. Initialize RTC (Passive)
  if (ds3231_init() == ESP_OK) {
    // Check flags or just log
    ds3231_sync_to_system(); // Force sync from RTC at startup for AP mode
                             // support
  }

  // 2b. Initialize LCD (Passive) - AVANT WiFi pour afficher IP
  if (lcd_init() == ESP_OK) {
    lcd_print("System v12.0");
    lcd_set_cursor(1, 0);
    lcd_print("Connecting WiFi");
  }

  // 3. Initialize WiFi Station (va afficher IP sur LCD pendant 10s)
  wifi_init_sta();

  // 3b. Initialize mDNS
  initialise_mdns();

  // Check if WiFi connected
  EventBits_t wifi_bits = xEventGroupGetBits(s_wifi_event_group);
  if (wifi_bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "[2/7] WiFi initialized & CONNECTED");
  } else {
    ESP_LOGE(TAG, "[2/7] WiFi initialized but NOT CONNECTED!");
  }

  // NOTE: L'affichage IP a déjà eu lieu dans le handler WiFi (10s)
  // Le LCD a été clear automatiquement après 10s

  // 4. Initialize SNTP (time sync)
  initialize_sntp();
  ESP_LOGI(TAG, "[3/7] Time sync initialized");

  // 5. Initialize GPIO (Relay, Buzzer)
  gpio_driver_init();
  pm_init();
  // power_manager_init(); // DEPLACÉ AU DÉBUT D'APP_MAIN

  ESP_LOGI(TAG, "System initialized successfully (Classic v12.0)");

  // 5a. Initialize Advanced Buzzer (PWM Patterns)
  buzzer_pattern_init();
  ESP_LOGI(TAG, "[4a/7] Buzzer patterns initialized (non-blocking)");

  // 6. Initialize ADC (Current Sensor) - DISABLED in Classic
  // adc_driver_init();
  ESP_LOGI(TAG, "[5/7] ADC (Current Sensor) DISABLED in Classic");

  // 7. Initialize Alarm Manager
  alarm_manager_init();
  ESP_LOGI(TAG, "[7/9] Alarm Manager initialized");

  // 8. Start HTTP Web Server
  start_http_server(); // Corrected function name from previous context if it
                       // was http_server_start
  ESP_LOGI(TAG, "[7/7] HTTP Server started");

  // 9. Start Heartbeat Task (Watchdog Feed)
  xTaskCreate(heartbeat_task, "heartbeat_task", 8192, NULL, 5, NULL);
  ESP_LOGI(TAG, "[9/9] Heartbeat Task Started (Feeding the Guardian)");

#ifdef CONFIG_ESP_HTTPS_SERVER_ENABLE
  // 8. Start HTTPS Web Server
  esp_err_t https_ret = start_https_server();
  if (https_ret == ESP_OK) {
    ESP_LOGI(TAG, "[8/8] HTTPS Server started");
  } else {
    ESP_LOGE(TAG, "[8/8] HTTPS Server FAILED to start");
  }
#endif

  ESP_LOGI(TAG, "================================================");
  ESP_LOGI(TAG, "    Phase 5.4 Complete - Alarm Logic Active");

  // Display actual IP address
  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (netif) {
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
      ESP_LOGI(TAG, "    Access http://" IPSTR " to Configure",
               IP2STR(&ip_info.ip));
    }
  }
  ESP_LOGI(TAG, "================================================");

  // === NOUVEAU: Check Anomalie au Startup === DISABLED in Classic
  ESP_LOGI(TAG, "Startup anomaly check DISABLED in Classic");
  /*
    ESP_LOGI(TAG, "Performing startup anomaly check...");
    vTaskDelay(pdMS_TO_TICKS(500)); // Attendre stabilisation ADC

    float startup_current = adc_read_current_amps();
    const current_thresholds_t *startup_thresh = adc_get_thresholds();

    // Relais devrait être OFF au démarrage
    int startup_relay_state = relay_get_state(); // Pas d'argument!

    if (startup_relay_state == RELAY_OFF &&
        startup_current > startup_thresh->anomaly_threshold) {
      ESP_LOGW(TAG, "⚠️ STARTUP ANOMALY DETECTED!");
      ESP_LOGW(TAG, "Relay OFF but current = %.2f A (threshold: %.2f A)",
               startup_current, startup_thresh->anomaly_threshold);

      // === ALARME STARTUP: 5 secondes continu ===
      ESP_LOGW(TAG, "🚨 ACTIVATING STARTUP ALARM (5s continuous)");
      buzzer_start(BUZZER_PATTERN_CONTINUOUS, 5000); // 5s continu

      // Attendre 5 secondes (non-bloquant, géré par task)
      vTaskDelay(pdMS_TO_TICKS(5000));
  */

  /*
    // Puis passer en mode intermittent
    ESP_LOGW(TAG, "Switching to intermittent alarm pattern");
    buzzer_start(BUZZER_PATTERN_SLOW_BEEP, 300000); // 5 min intermittent
  } else {
    ESP_LOGI(TAG, "✅ No startup anomaly detected (Current: %.2f A)",
             startup_current);
  }
  */

  // Main loop - displays system status and checks alarms
  while (1) {
    // === CHECK EXTERNAL RESTART BUTTON (GPIO 27) ===
    if (is_restart_button_pressed()) {
      ESP_LOGW(TAG, "🔄 RESTART BUTTON PRESSED (GPIO 27)");

      // Debounce - attendre relâchement
      vTaskDelay(pdMS_TO_TICKS(50));

      // Si toujours appuyé après 50ms, c'est un vrai appui
      if (is_restart_button_pressed()) {
        // Feedback: 1 bip court
        buzzer_start(BUZZER_PATTERN_FAST_BEEP, 100);
        vTaskDelay(pdMS_TO_TICKS(150));
        buzzer_stop();

        ESP_LOGI(TAG, "Rebooting...");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
      }
    }

    // === CHECK FACTORY RESET BUTTON (GPIO 23, >5s press) ===
    if (is_factory_reset_requested()) {
      ESP_LOGW(TAG, "🏭 FACTORY RESET REQUESTED (button RESET held >5s)");
      ESP_LOGW(TAG,
               "This will erase ALL settings (WiFi, alarms, calibrations)!");

      // Feedback sonore: 3 bips rapides
      ESP_LOGI(TAG, "Confirmation beeps...");
      for (int i = 0; i < 3; i++) {
        buzzer_start(BUZZER_PATTERN_FAST_BEEP, 150);
        vTaskDelay(pdMS_TO_TICKS(250));
        buzzer_stop();
        vTaskDelay(pdMS_TO_TICKS(100));
      }

      ESP_LOGW(TAG, "Erasing NVS flash...");
      esp_err_t err = nvs_flash_erase();
      if (err == ESP_OK) {
        ESP_LOGI(TAG, "✅ NVS erased successfully - all settings cleared");
      } else {
        ESP_LOGE(TAG, "❌ NVS erase failed: %s", esp_err_to_name(err));
      }

      // Attendre 1 seconde
      vTaskDelay(pdMS_TO_TICKS(1000));

      ESP_LOGW(TAG, "🔄 Rebooting to factory defaults...");
      vTaskDelay(pdMS_TO_TICKS(1000));

      // Reboot
      esp_restart();
    }

    // Check Alarms (logic handles its own rate limiting)
    alarm_manager_check();

    /* ========== ANOMALY DETECTION REMOVED IN V12 ========== */
    // Code d'anomalie supprimé conformément aux corrections QA

    // Log status and update LCD every 1 second
    static unsigned long last_log = 0;
    static unsigned long last_lcd = 0;
    unsigned long now_ms = (unsigned long)(esp_timer_get_time() / 1000ULL);

    // Log status every 5 seconds
    if (now_ms - last_log > 5000) {
      time_t now;
      struct tm timeinfo;
      time(&now);
      localtime_r(&now, &timeinfo);

      char strftime_buf[64];
      strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S",
               &timeinfo);

      ESP_LOGI(TAG, "Time: %s | R1:%d R2:%d R3:%d R4:%d | Heap: %lu",
               strftime_buf, relay_get_state(0), relay_get_state(1),
               relay_get_state(2), relay_get_state(3),
               esp_get_free_heap_size());

      last_log = now_ms;
    }

    if (now_ms - last_lcd > 1000 && !g_is_entering_sleep) {
      struct tm timeinfo;
      time_t now;
      time(&now);
      localtime_r(&now, &timeinfo);

      // LCD Carousel Logic
      static int carousel_relay_idx = 0;
      static unsigned long last_carousel_switch = 0;

      // Switch relay every 3 seconds
      if (now_ms - last_carousel_switch > 3000) {
        carousel_relay_idx = (carousel_relay_idx + 1) % 4;
        last_carousel_switch = now_ms;
      }

      int r_state = relay_get_state(carousel_relay_idx);
      char next_alarm[16];
      // Get next alarm for THIS specific relay
      alarm_get_next_time_str(0, carousel_relay_idx, next_alarm,
                              sizeof(next_alarm));

      // Format: "R1:ON  A:12:00"
      // Format: "R1:ON  A:12:00"
      // Relay ID is carousel_relay_idx + 1
      char status_line[32];
      char full_alarm[9] = "--:--:--";

      if (strlen(next_alarm) >= 8) {
        strncpy(full_alarm, next_alarm, 8);
        full_alarm[8] = '\0';
      } else if (strlen(next_alarm) >= 5) {
        // Fallback for HH:MM format if HH:MM:SS is missing
        strncpy(full_alarm, next_alarm, 5);
        full_alarm[5] = '\0';
      }

      // Relay Mapping: 0=S, 1=A, 2=B, 3=C
      const char *relay_labels[] = {"S", "A", "B", "C"};
      const char *relay_label = relay_labels[carousel_relay_idx];

      snprintf(status_line, sizeof(status_line), "%s:%s A:%s", relay_label,
               r_state ? "ON " : "OFF", full_alarm);

      lcd_update_display(timeinfo.tm_mday, timeinfo.tm_mon + 1,
                         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                         status_line, g_is_time_synced);


      last_lcd = now_ms;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Run loop at ~10Hz
  }
}
