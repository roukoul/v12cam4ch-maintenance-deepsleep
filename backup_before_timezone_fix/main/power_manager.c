#define _POSIX_C_SOURCE 200809L
#include "power_manager.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "drivers/ds3231.h"
#include "drivers/gpio_driver.h"
#include "driver/rtc_io.h"
#include "esp_timer.h"
#include "drivers/lcd_i2c.h"
#include <time.h>
#include <sys/time.h>

static const char *TAG = "POWER_MGR";

static power_mode_t s_mode = POWER_MODE_NORMAL;
static sleep_window_t s_window = {22, 0, 0, 6, 0, 0, 1, 1, 2026, 1, 1, 2026}; // Default 10:00:00pm - 6:00:00am

static weekly_sleep_schedule_t s_weekly = {
    .days = {
        {false, 22, 0, 0, 6, 0, 0}, // 0 = Sunday
        {false, 22, 0, 0, 6, 0, 0}, // 1 = Monday
        {false, 22, 0, 0, 6, 0, 0}, // 2 = Tuesday
        {false, 22, 0, 0, 6, 0, 0}, // 3 = Wednesday
        {false, 22, 0, 0, 6, 0, 0}, // 4 = Thursday
        {false, 22, 0, 0, 6, 0, 0}, // 5 = Friday
        {false, 22, 0, 0, 6, 0, 0}  // 6 = Saturday
    }
};

void power_manager_init(void) {
    nvs_handle_t handle;
    esp_reset_reason_t reason = esp_reset_reason();
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    
    ESP_LOGI(TAG, "Init: Reason=%d, WakeupCause=%d", (int)reason, (int)wakeup_cause);
    
    // SAFETY: We only allow Stealth mode to continue if it was an AUTOMATIC wakeup (EXT0 = RTC)
    // Any other reason (Power-on, Hard reset, or Button wakeup via EXT1) forces NORMAL mode.
    bool was_auto_wakeup = (reason == ESP_RST_DEEPSLEEP && wakeup_cause == ESP_SLEEP_WAKEUP_EXT0);
    
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        uint8_t mode = 0;
        if (nvs_get_u8(handle, "pwr_mode", &mode) == ESP_OK) {
            ESP_LOGI(TAG, "Loaded mode from NVS: %d", (int)mode);
            s_mode = (power_mode_t)mode;
        }
        
        // If it was NOT an automatic RTC wakeup, force NORMAL mode for safety
        if (!was_auto_wakeup) {
            if (s_mode != POWER_MODE_NORMAL) {
                ESP_LOGW(TAG, "Manual restart/button detected! PERSISTENTLY forcing NORMAL mode.");
                s_mode = POWER_MODE_NORMAL;
                // Persistent wipe: ensure it stays Normal even after another reboot
                nvs_set_u8(handle, "pwr_mode", (uint8_t)POWER_MODE_NORMAL);
                nvs_commit(handle);
            }
        }

        nvs_get_i32(handle, "pwr_s_h", &s_window.start_hour);
        nvs_get_i32(handle, "pwr_s_m", &s_window.start_min);
        nvs_get_i32(handle, "pwr_s_s", &s_window.start_sec);
        nvs_get_i32(handle, "pwr_e_h", &s_window.end_hour);
        nvs_get_i32(handle, "pwr_e_m", &s_window.end_min);
        nvs_get_i32(handle, "pwr_e_s", &s_window.end_sec);
        // Dates
        nvs_get_i32(handle, "pwr_s_dy", &s_window.start_day);
        nvs_get_i32(handle, "pwr_s_mo", &s_window.start_month);
        nvs_get_i32(handle, "pwr_s_yr", &s_window.start_year);
        nvs_get_i32(handle, "pwr_e_dy", &s_window.end_day);
        nvs_get_i32(handle, "pwr_e_mo", &s_window.end_month);
        nvs_get_i32(handle, "pwr_e_yr", &s_window.end_year);

        size_t weekly_size = sizeof(weekly_sleep_schedule_t);
        if (nvs_get_blob(handle, "pwr_weekly", &s_weekly, &weekly_size) != ESP_OK || weekly_size != sizeof(weekly_sleep_schedule_t)) {
            ESP_LOGW(TAG, "No valid weekly schedule found in NVS, using defaults");
        }

        nvs_close(handle);
    }
    
    // Check RTC Power Loss
    if (ds3231_check_power_lost()) {
        ESP_LOGW(TAG, "RTC Power Loss detected! Reverting to NORMAL mode for safety.");
        s_mode = POWER_MODE_NORMAL;
    }

    power_manager_apply();
}

void power_manager_apply(void) {
    if (s_mode == POWER_MODE_ECO) {
        ESP_LOGI(TAG, "Applying ECO Mode (Modem Sleep)");
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    } else {
        ESP_LOGI(TAG, "Applying NORMAL Mode");
        esp_wifi_set_ps(WIFI_PS_NONE);
    }
}

void power_manager_set_mode(power_mode_t mode) {
    s_mode = mode;
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_u8(handle, "pwr_mode", (uint8_t)s_mode);
        nvs_commit(handle);
        nvs_close(handle);
    }
    power_manager_apply();
}

power_mode_t power_manager_get_mode(void) { return s_mode; }

void power_manager_set_window(sleep_window_t window) {
    s_window = window;
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_i32(handle, "pwr_s_h", s_window.start_hour);
        nvs_set_i32(handle, "pwr_s_m", s_window.start_min);
        nvs_set_i32(handle, "pwr_s_s", s_window.start_sec);
        nvs_set_i32(handle, "pwr_e_h", s_window.end_hour);
        nvs_set_i32(handle, "pwr_e_m", s_window.end_min);
        nvs_set_i32(handle, "pwr_e_s", s_window.end_sec);
        // Dates
        nvs_set_i32(handle, "pwr_s_dy", s_window.start_day);
        nvs_set_i32(handle, "pwr_s_mo", s_window.start_month);
        nvs_set_i32(handle, "pwr_s_yr", s_window.start_year);
        nvs_set_i32(handle, "pwr_e_dy", s_window.end_day);
        nvs_set_i32(handle, "pwr_e_mo", s_window.end_month);
        nvs_set_i32(handle, "pwr_e_yr", s_window.end_year);
        nvs_commit(handle);
        nvs_close(handle);
    }
}

sleep_window_t power_manager_get_window(void) { return s_window; }

void power_manager_set_weekly(weekly_sleep_schedule_t weekly) {
    s_weekly = weekly;
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_blob(handle, "pwr_weekly", &s_weekly, sizeof(weekly_sleep_schedule_t));
        nvs_commit(handle);
        nvs_close(handle);
    }
}

weekly_sleep_schedule_t power_manager_get_weekly(void) { return s_weekly; }

void power_manager_enter_stealth(void) {
    ESP_LOGW(TAG, "!!!! ENTERING STEALTH MODE (DEEP SLEEP) !!!!");
    
    extern volatile bool g_is_entering_sleep;
    extern void heartbeat_feed(void);
    g_is_entering_sleep = true;

    time_t now;
    struct tm t;
    time(&now);
    localtime_r(&now, &t);

    // Calculate next wakeup time based on window end
    struct tm wakeup_t = t;
    wakeup_t.tm_hour = s_window.end_hour;
    wakeup_t.tm_min = s_window.end_min;
    wakeup_t.tm_sec = s_window.end_sec;

    time_t wakeup_time;
    if (s_mode == POWER_MODE_HOLIDAY) {
        wakeup_t.tm_year = s_window.end_year - 1900;
        wakeup_t.tm_mon  = s_window.end_month - 1;
        wakeup_t.tm_mday = s_window.end_day;
        wakeup_time = mktime(&wakeup_t);
    } else if (s_mode == POWER_MODE_WEEKLY) {
        int today = t.tm_wday;
        int yesterday = (today + 6) % 7;
        int active_day = today;
        int now_s = (t.tm_hour * 3600) + (t.tm_min * 60) + t.tm_sec;
        bool from_yesterday = false;

        if (s_weekly.days[yesterday].enabled) {
            int start_s = (s_weekly.days[yesterday].start_hour * 3600) + (s_weekly.days[yesterday].start_min * 60) + s_weekly.days[yesterday].start_sec;
            int end_s = (s_weekly.days[yesterday].end_hour * 3600) + (s_weekly.days[yesterday].end_min * 60) + s_weekly.days[yesterday].end_sec;
            if (start_s >= end_s && now_s < end_s) {
                active_day = yesterday;
                from_yesterday = true;
            }
        }

        wakeup_t.tm_hour = s_weekly.days[active_day].end_hour;
        wakeup_t.tm_min = s_weekly.days[active_day].end_min;
        wakeup_t.tm_sec = s_weekly.days[active_day].end_sec;
        wakeup_time = mktime(&wakeup_t);
        
        if (wakeup_time <= now && !from_yesterday) {
            wakeup_time += 86400; // Tomorrow
        }
    } else {
        wakeup_time = mktime(&wakeup_t);
        if (wakeup_time <= now) {
            wakeup_time += 86400; // Tomorrow
        }
    }

    // --- ALARM SAFETY ---
    extern time_t alarm_get_next_timestamp(void);
    time_t next_alarm = alarm_get_next_timestamp();
    if (next_alarm > now && next_alarm < wakeup_time) {
        ESP_LOGI(TAG, "Alarm detected during sleep window. Adjusting wakeup to 2 mins before.");
        wakeup_time = next_alarm - 120; // 2 minutes buffer
    }

    uint64_t sleep_duration_us = (uint64_t)(wakeup_time - now) * 1000000ULL;
    ESP_LOGI(TAG, "Stealth Mode: Sleeping for %llds", (long long)(wakeup_time - now));
    
    // --- LCD SLEEP COUNTDOWN ---
    char line1[17];
    char line2[17];
    struct tm info_t;
    localtime_r(&wakeup_time, &info_t);
    snprintf(line2, sizeof(line2), "REVEIL @ %02d:%02d", info_t.tm_hour, info_t.tm_min);

    for (int i = 12; i > 0; i--) {
        lcd_set_cursor(0, 0);
        snprintf(line1, sizeof(line1), "VEILLE DANS %2ds", i);
        lcd_print(line1);
        lcd_set_cursor(1, 0);
        lcd_print(line2);
        
        if (i % 10 == 0) ESP_LOGI(TAG, "Deep sleep countdown: %ds left", i);
        
        heartbeat_feed();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // --- PHASE TRANSITION DOUCE ---
    heartbeat_feed();

    // 1. Prepare DS3231 Alarm (While I2C is stable)
    ds3231_set_alarm(info_t.tm_hour, info_t.tm_min);
    ds3231_clear_alarm(); // CRITICAL: Clear previous alarm flags so INT pin goes HIGH!
    
    // 2. Clear LCD and Turn Off Backlight
    lcd_clear();
    lcd_set_cursor(0, 0);
    snprintf(line1, sizeof(line1), "DODO-> %02d:%02d:%02d", info_t.tm_hour, info_t.tm_min, info_t.tm_sec);
    lcd_print(line1);
    heartbeat_feed();
    vTaskDelay(pdMS_TO_TICKS(500)); // Short visible feedback
    lcd_backlight(false);
    heartbeat_feed();

    // 3. Disable WiFi (Heavy operation)
    ESP_LOGI(TAG, "Stopping WiFi...");
    esp_wifi_stop();
    heartbeat_feed();

    // 4. Configure Wakeup Sources
    rtc_gpio_pullup_en(RTC_INTERRUPT_PIN);
    rtc_gpio_pulldown_dis(RTC_INTERRUPT_PIN);
    esp_sleep_enable_ext0_wakeup(RTC_INTERRUPT_PIN, 0); // 0 = LOW
    
    rtc_gpio_pullup_en(RESTART_BTN_PIN);
    rtc_gpio_pulldown_dis(RESTART_BTN_PIN);
    esp_sleep_enable_ext1_wakeup(1ULL << RESTART_BTN_PIN, ESP_EXT1_WAKEUP_ALL_LOW);
    
    esp_sleep_enable_timer_wakeup(sleep_duration_us);
    
    // Keep RTC peripherals ON during deep sleep so the internal pullups don't drop to 0V
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    
    heartbeat_feed();
    ESP_LOGW(TAG, "Good night!");
    esp_deep_sleep_start();
    
    // Safety only
    g_is_entering_sleep = false;
}

void power_manager_check(void) {
    // Log state periodically
    static uint32_t last_log = 0;
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    if (now_ms - last_log > 10000) {
        ESP_LOGD(TAG, "Periodic check: current mode=%d", (int)s_mode);
        last_log = now_ms;
    }

    if (s_mode != POWER_MODE_STEALTH && s_mode != POWER_MODE_HOLIDAY && s_mode != POWER_MODE_WEEKLY) return;

    time_t now;
    struct tm t;
    time(&now);
    localtime_r(&now, &t);

    bool in_window = false;

    if (s_mode == POWER_MODE_STEALTH) {
        int now_s = (t.tm_hour * 3600) + (t.tm_min * 60) + t.tm_sec;
        int start_s = (s_window.start_hour * 3600) + (s_window.start_min * 60) + s_window.start_sec;
        int end_s = (s_window.end_hour * 3600) + (s_window.end_min * 60) + s_window.end_sec;

        if (start_s < end_s) {
            if (now_s >= start_s && now_s < end_s) in_window = true;
        } else {
            // Window crosses midnight (e.g. 23:00 to 06:00)
            if (now_s >= start_s || now_s < end_s) in_window = true;
        }
    } else if (s_mode == POWER_MODE_WEEKLY) {
        int today = t.tm_wday; // 0=Sunday
        int now_s = (t.tm_hour * 3600) + (t.tm_min * 60) + t.tm_sec;

        if (s_weekly.days[today].enabled) {
            int start_s = (s_weekly.days[today].start_hour * 3600) + (s_weekly.days[today].start_min * 60) + s_weekly.days[today].start_sec;
            int end_s = (s_weekly.days[today].end_hour * 3600) + (s_weekly.days[today].end_min * 60) + s_weekly.days[today].end_sec;

            if (start_s < end_s) {
                if (now_s >= start_s && now_s < end_s) in_window = true;
            } else {
                if (now_s >= start_s || now_s < end_s) in_window = true;
            }
        }
        
        // Check if yesterday's window crossed midnight into today
        int yesterday = (today + 6) % 7;
        if (!in_window && s_weekly.days[yesterday].enabled) {
            int start_s = (s_weekly.days[yesterday].start_hour * 3600) + (s_weekly.days[yesterday].start_min * 60) + s_weekly.days[yesterday].start_sec;
            int end_s = (s_weekly.days[yesterday].end_hour * 3600) + (s_weekly.days[yesterday].end_min * 60) + s_weekly.days[yesterday].end_sec;
            if (start_s >= end_s && now_s < end_s) {
                in_window = true;
            }
        }
    } else if (s_mode == POWER_MODE_HOLIDAY) {
        struct tm start_t = t;
        start_t.tm_year = s_window.start_year - 1900;
        start_t.tm_mon  = s_window.start_month - 1;
        start_t.tm_mday = s_window.start_day;
        start_t.tm_hour = s_window.start_hour;
        start_t.tm_min  = s_window.start_min;
        start_t.tm_sec  = s_window.start_sec;
        
        struct tm end_t = t;
        end_t.tm_year = s_window.end_year - 1900;
        end_t.tm_mon  = s_window.end_month - 1;
        end_t.tm_mday = s_window.end_day;
        end_t.tm_hour = s_window.end_hour;
        end_t.tm_min  = s_window.end_min;
        end_t.tm_sec  = s_window.end_sec;

        time_t start_epoch = mktime(&start_t);
        time_t end_epoch = mktime(&end_t);
        
        if (now >= start_epoch && now < end_epoch) {
            in_window = true;
        }
    }

    if (in_window) {
        // --- SAFETY 1: BOOT DELAY ---
        // Prevenir le sommeil profond pendant les 3 premieres minutes (180s)
        if (esp_timer_get_time() < 180000000ULL) {
            ESP_LOGD(TAG, "Postponing sleep: boot grace period (uptime < %llds)", (long long)(esp_timer_get_time()/1000000ULL));
            return;
        }

        // --- SAFETY 2: AP MODE & CONNECTIVITY LOCKOUT ---
        // On ne bloque le sommeil QUE si on est en mode STA et non connecté (recherche WiFi)
        // En mode AP/APSTA, on autorise le sommeil (le délai de 3 min d'uptime protège l'accès)
        wifi_mode_t w_mode;
        if (esp_wifi_get_mode(&w_mode) == ESP_OK) {
            if (w_mode == WIFI_MODE_STA) {
                wifi_ap_record_t ap_info;
                if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
                    ESP_LOGI(TAG, "Postponing sleep: STA mode but WiFi not connected");
                    return;
                }
            }
        }

        // Don't sleep if an alarm is ringing or about to ring (within 5 mins)
        extern time_t alarm_get_next_timestamp(void);
        time_t next_alarm = alarm_get_next_timestamp();
        if (next_alarm > 0 && (next_alarm - now) < 300) {
            ESP_LOGI(TAG, "Postponing sleep: alarm in %ld seconds", (long)(next_alarm - now));
            return;
        }
        
        ESP_LOGW(TAG, "ALL PROTECTIONS PASSED. Entering stealth now!");
        power_manager_enter_stealth();
    }
}
