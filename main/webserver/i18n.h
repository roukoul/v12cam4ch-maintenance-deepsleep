#ifndef AEP_I18N_H
#define AEP_I18N_H

#include "esp_http_server.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LANG_AR = 0,
    LANG_FR = 1,
    LANG_EN = 2,
    LANG_MAX = 3
} app_lang_t;

typedef enum {
    // General
    STR_APP_TITLE,
    STR_APP_SUBTITLE,
    STR_BELL_SYSTEM,
    STR_YES,
    STR_CANCEL,
    STR_CONFIRM,
    STR_SAVE,
    STR_SUCCESS,
    STR_ERROR,
    STR_SAVED_SUCCESS,
    STR_PLEASE_WAIT,
    STR_BACK_TO_DASHBOARD,
    STR_RETURNING,

    // Login
    STR_LOGIN_TITLE,
    STR_PASSWORD,
    STR_LOGIN_BTN,
    STR_WRONG_PASSWORD,
    STR_CHECK_PASSWORD,
    STR_RETRY,

    // Dashboard
    STR_SYS_NORMAL,
    STR_SYS_RUNNING,
    STR_SYS_STOPPED,
    STR_CURRENT_TIME,
    STR_NEXT_BELL,
    STR_NONE,
    STR_RELAY_BELL,
    STR_RELAY_DEV_A,
    STR_RELAY_DEV_B,
    STR_RELAY_DEV_C,
    STR_CAM_FEED,
    STR_CAM_DASHBOARD,
    STR_CAM_ENTER_IP,
    STR_SET_TIME_TITLE,
    STR_SYNC_DEV_TIME,
    STR_YEAR,
    STR_MONTH,
    STR_DAY,
    STR_HOUR,
    STR_MINUTE,
    STR_SAVE_TIME,

    // v12 Predictive Maintenance
    STR_MAINT_TITLE,
    STR_MAINT_CHECKING,
    STR_MAINT_RELAY_HEALTH,
    STR_MAINT_TEMP,
    STR_MAINT_CALIBRATING,
    STR_MAINT_CRITICAL_REPLACE,
    STR_MAINT_REMAINING_PREFIX,
    STR_MAINT_DAYS,
    STR_MAINT_DANGER_REPLACE,

    // v12 Energy & Sleep Management
    STR_POWER_TITLE,
    STR_POWER_MODE,
    STR_POWER_MODE_NORMAL,
    STR_POWER_MODE_MODEM,
    STR_POWER_MODE_DEEP,
    STR_POWER_MODE_HOLIDAY,
    STR_POWER_MODE_WEEKLY,
    STR_POWER_DAILY_DESC,
    STR_POWER_TO,
    STR_POWER_FROM,
    STR_POWER_HOLIDAY_DESC,
    STR_POWER_WEEKLY_DESC,
    STR_POWER_SAVED_OK,
    STR_POWER_SAVE_BTN,
    STR_DAY_SUN_SHORT,
    STR_DAY_MON_SHORT,
    STR_DAY_TUE_SHORT,
    STR_DAY_WED_SHORT,
    STR_DAY_THU_SHORT,
    STR_DAY_FRI_SHORT,
    STR_DAY_SAT_SHORT,

    // Navigation Menu
    STR_NAV_SETTINGS,
    STR_NAV_WIFI,
    STR_NAV_TIMEZONE,
    STR_NAV_PASSWORD,
    STR_NAV_ABOUT,
    STR_NAV_BACKUP,
    STR_NAV_OTA,
    STR_NAV_LOGS,
    STR_NAV_RESTART,
    STR_NAV_FACTORY_RESET,
    STR_NAV_LOGOUT,

    // Settings / Schedule
    STR_SCHEDULE_TITLE,
    STR_DATE_AND_TIME,
    STR_ENABLE_DAY,
    STR_ALARM_NUM,
    STR_START_TIME,
    STR_END_TIME,
    STR_END_DAY,
    STR_SAME_DAY,
    STR_NEXT_DAY,
    STR_SELECT_OUTPUT,
    STR_IMPORT_EXPORT,
    STR_IMPORT_JSON,
    STR_UPLOAD_SCHEDULE,
    STR_DOWNLOAD_SCHEDULE,
    STR_SAVE_SETTINGS,
    STR_SAVING,
    STR_ALERT_END_AFTER_START,

    // Days of Week
    STR_DAY_MONDAY,
    STR_DAY_TUESDAY,
    STR_DAY_WEDNESDAY,
    STR_DAY_THURSDAY,
    STR_DAY_FRIDAY,
    STR_DAY_SATURDAY,
    STR_DAY_SUNDAY,

    // WiFi
    STR_WIFI_TITLE,
    STR_CURRENT_SSID,
    STR_NETWORK_NAME,
    STR_WIFI_WARN_RESTART,
    STR_WIFI_INVALID_INPUT,
    STR_WIFI_SAVE_FAILED,
    STR_WIFI_SAVED,
    STR_WIFI_RESTART_PROMPT,
    STR_RESTART_NOW,
    STR_LATER,

    // Timezone
    STR_TZ_TITLE,
    STR_SELECT_TZ,
    STR_TZ_SAVED_TITLE,
    STR_TZ_APPLIED_IMMEDIATELY,

    // Current Config
    STR_CURR_TITLE,
    STR_CURR_CURRENT_STATUS,
    STR_CURR_ZERO_POINT,
    STR_CURR_SCALE_FACTOR,
    STR_CURR_CALIB_SAVED,
    STR_CURR_CALIB_DEFAULT,
    STR_CURR_THRESHOLDS,
    STR_CURR_MIN_LOAD,
    STR_CURR_ANOMALY,
    STR_CURR_MAX_CRITICAL,

    // Password change
    STR_PWD_TITLE,
    STR_OLD_PWD,
    STR_NEW_PWD,
    STR_CONFIRM_PWD,
    STR_PWD_WRONG_OLD,
    STR_PWD_MISMATCH,
    STR_PWD_CHANGED_OK,

    // About
    STR_ABOUT_TITLE,
    STR_ABOUT_SYSTEM,
    STR_ABOUT_VERSION,
    STR_ABOUT_DEV,
    STR_ABOUT_DEV_NAME,
    STR_ABOUT_INSTITUTION,
    STR_ABOUT_INSTITUTION_NAME,
    STR_ABOUT_RIGHTS,
    STR_ABOUT_RIGHTS_TEXT,

    // Backup & Restore
    STR_BACKUP_TITLE,
    STR_BACKUP_DOWNLOAD_TITLE,
    STR_BACKUP_DOWNLOAD_DESC,
    STR_BACKUP_DOWNLOAD_BTN,
    STR_RESTORE_TITLE,
    STR_RESTORE_DESC,
    STR_RESTORE_BTN,

    // OTA
    STR_OTA_TITLE,
    STR_OTA_HASH_LABEL,
    STR_OTA_HASH_HINT,
    STR_OTA_FILE_LABEL,
    STR_OTA_SUBMIT,

    // System actions
    STR_REBOOT_CONFIRM,
    STR_REBOOTING,
    STR_FACTORY_RESET_TITLE,
    STR_FACTORY_RESET_WARN,
    STR_FACTORY_RESET_CONFIRM_BTN,

    STR_TOTAL_COUNT
} app_str_t;

// Get the language of the current HTTP request (Cookie -> Query Param -> NVS -> Default AR)
app_lang_t i18n_get_req_lang(httpd_req_t *req);

// Set default language in NVS
void i18n_set_default_lang(app_lang_t lang);

// Translate a key
const char* i18n_tr(app_str_t str_id, app_lang_t lang);

// Text direction: "rtl" for AR, "ltr" for FR/EN
const char* i18n_dir(app_lang_t lang);

// Language ISO code: "ar", "fr", "en"
const char* i18n_code(app_lang_t lang);

// Font-family CSS string
const char* i18n_font(app_lang_t lang);

// Get day name by index (0 = Monday, 6 = Sunday)
const char* i18n_day_name(int day_idx, app_lang_t lang);

// Helper to send the language switcher bar HTML
void i18n_send_lang_bar(httpd_req_t *req, app_lang_t lang);

#ifdef __cplusplus
}
#endif

#endif // AEP_I18N_H
