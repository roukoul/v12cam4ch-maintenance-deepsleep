#define _POSIX_C_SOURCE 200809L
/**
 * @file http_server.c
 * @brief HTTP Server Implementation for AepBill
 *
 * Ported from v7.1.1 Arduino WebServer to ESP-IDF esp_http_server
 */

#include "http_server.h"
#include "i18n.h"
#include "predictive_maintenance.h"
#include "power_manager.h"
#include "../drivers/adc_driver.h"
#include "alarm_manager.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "mdns.h"
#include "nvs.h"
#include "nvs_flash.h" // For NVS functions
#include <ctype.h>     // For isdigit
#include <stdlib.h>    // For setenv
#include <string.h>
#include <sys/time.h>
#include <time.h>

// OTA includes
#include "../logging.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

// cJSON for backup/restore
#include "cJSON.h"

// mbedtls for SHA256 validation
#include "mbedtls/sha256.h"

// External functions from drivers
extern int relay_get_state(int relay_index);
// extern float adc_read_current_amps(void); // REMOVED: Using header instead
extern void relay_set(int state);

// External NVS helpers from main.c
extern esp_err_t nvs_read_string(const char *key, char *value, size_t max_len);
extern esp_err_t nvs_write_string(const char *key, const char *value);

// Global Anomaly Code from main.c
extern volatile int g_system_anomaly_code;

static const char *TAG = "HTTP_SRV";
static httpd_handle_t server = NULL;
static bool is_authorized = false;
static volatile bool g_is_ota_active = false; // Safety flag for OTA

// Default password hash - same as v7.1.1 "admin"
#define DEFAULT_PASSWORD "admin"

// ========== CSS Style (Enhanced from v7.1.1) ==========
const char *HTML_STYLE =
    "<style>"
    "@import "
    "url('https://fonts.googleapis.com/"
    "css2?family=Cairo:wght@400;600;700&display=swap');"
    ":root{--primary-gradient:linear-gradient(135deg,#667eea "
    "0%,#764ba2 100%);"
    "--success-gradient:linear-gradient(135deg,#11998e "
    "0%,#38ef7d 100%);"
    "--danger-gradient:linear-gradient(135deg,#f093fb 0%,#f5576c 100%);"
    "--color-date: #10B981; --color-time: #3B82F6;"
    "--color-status-on: #22C55E; --color-status-off: #EF4444;"
    "--color-current-normal: #F59E0B; --color-current-low: #6B7280; "
    "--color-current-high: #DC2626;"
    "--color-alarm-none: #9CA3AF; --color-alarm-pending: #8B5CF6; "
    "--color-alarm-soon: #F97316;}"
    "@keyframes pulse-red { 0%,100% {opacity:1} 50% {opacity:0.6} }"
    "@keyframes pulse-orange { 0%,100% {opacity:1;transform:scale(1)} 50% "
    "{opacity:0.8;transform:scale(1.05)} }"
    "*{margin:0;padding:0;box-sizing:border-box;}"
    "body{background:var(--primary-gradient);min-height:100vh;padding:15px;}"
    "html[dir='rtl'] body{direction:rtl;font-family:'Cairo','Tajawal',sans-serif;text-align:right;}"
    "html[dir='ltr'] body{direction:ltr;font-family:system-ui,-apple-system,'Segoe UI',Roboto,sans-serif;text-align:left;}"
    ".lang-bar{display:flex;justify-content:center;gap:8px;margin-bottom:15px;background:rgba(255,255,255,0.15);padding:6px;border-radius:12px;backdrop-filter:blur(5px);}"
    ".lang-btn{background:rgba(255,255,255,0.85);border:none;border-radius:8px;padding:6px 14px;font-size:0.9rem;font-weight:600;cursor:pointer;color:#333;transition:all 0.2s ease;}"
    ".lang-btn:hover{background:#fff;transform:translateY(-1px);}"
    ".lang-btn.active{background:linear-gradient(135deg,#667eea,#764ba2);color:#fff;box-shadow:0 2px 8px rgba(0,0,0,0.2);}"
    ".container{max-width:800px;margin:0 "
    "auto;background:rgba(255,255,255,0.98);"
    "border-radius:20px;padding:25px;box-shadow:0 10px 30px "
    "rgba(0,0,0,0.1);}"
    "h1{background:var(--primary-gradient);-webkit-background-"
    "clip:text;"
    "-webkit-text-fill-color:transparent;font-size:2rem;text-"
    "align:center;"
    "margin-bottom:25px;}"
    "h2{color:#444;font-size:1.4rem;margin:20px 0 "
    "15px;font-weight:700;}"
    "label{display:block;margin-bottom:8px;font-weight:600;color:"
    "#333;font-"
    "size:1.1rem;}" /* Added Label Style */
    "input[type='password'],input[type='text'],input[type='time']"
    ",select{width:"
    "100%;"
    "padding:16px;margin-" /* Increased padding */
    "bottom:20px;"
    "border:2px solid "
    "#e0e0e0;border-radius:12px;font-size:1.2rem;}" /* Increased
                                                       font
                                                       to 1.2rem
                                                       (~19px)
                                                     */
    "input[type='submit'], button.tablink{padding:14px "
    "25px;border:none;border-radius:12px;"
    "background:var(--primary-gradient);color:white;font-size:"
    "1rem;cursor:"
    "pointer;width:100%;margin:5px 0;}"
    ".tablink.active{background:#4a5568;box-shadow:inset 0 2px "
    "4px "
    "rgba(0,0,0,0.1);}"
    ".tabcontent{display:none;padding:15px;border:1px solid "
    "#ddd;border-radius:10px;margin-top:10px;}"
    ".alarm-entry{background:linear-gradient(135deg,#fff "
    "0%,#f8f9fa 100%);"
    "padding:15px;margin-bottom:12px;border-radius:12px;border:"
    "2px solid "
    "#e9ecef;"
    "transition:all 0.3s ease;}"
    ".alarm-entry:hover{border-color:#667eea;transform:"
    "translateX(-3px);}"
    ".alarm-entry "
    "h3{color:#666;font-size:1.1rem;margin-bottom:10px;font-"
    "weight:600;}"
    ".grid{display:grid;grid-template-columns:repeat(auto-fill,"
    "minmax(280px,"
    "1fr));"
    "gap:15px;margin-top:15px;}"
    ".info-header{background:linear-gradient(135deg,#e3f2fd "
    "0%,#bbdefb 100%);"
    "color:#0d47a1;padding:18px;border-radius:15px;text-align:"
    "center;"
    "margin-bottom:25px;font-weight:700;font-size:1.1rem;}"
    ".status-box{padding:20px;border-radius:15px;margin-bottom:"
    "25px;text-align:"
    "center;"
    "color:white;font-weight:700;}"
    ".status-ok{background:var(--success-gradient);}"
    ".status-alert{background:var(--danger-gradient);}"
    ".dash-grid{display:grid;grid-template-columns:repeat(auto-"
    "fit,minmax("
    "140px,1fr));gap:15px;margin-bottom:25px;}"
    ".card{background:white;padding:15px;border-radius:15px;box-"
    "shadow:0 4px "
    "6px rgba(0,0,0,0.05);text-align:center;border:1px solid "
    "#eee;}"
    ".card-icon{font-size:2rem;margin-bottom:5px;display:block;}"
    ".card-title{font-size:0.9rem;color:#666;display:block;"
    "margin-bottom:5px;}"
    ".card-value{font-size:1.2rem;font-weight:bold;color:#333;}"
    ".status-ok-text{color:#27ae60;}.status-alert-text{color:#e74c3c;}"
    ".live-dot{display:inline-block;width:12px;height:12px;background:#2ecc71;"
    "border-radius:50%;margin-left:10px;box-shadow:0 0 8px #2ecc71;"
    "animation:pulse-live 2s infinite;vertical-align:middle;}"
    "@keyframes pulse-live{0%{transform:scale(0.95);box-shadow:0 0 0 0 "
    "rgba(46,204,113,0.7);}"
    "70%{transform:scale(1);box-shadow:0 0 0 10px rgba(46,204,113,0);}"
    "100%{transform:scale(0.95);box-shadow:0 0 0 0 rgba(46,204,113,0);}}"
    "#current-card, #anomaly-badge, .threshold-section { display: none "
    "!important; }"
    ".data-row{display:flex;justify-content:space-between;"
    "padding:12px;"
    "background:#f5f7fa;"
    "border-radius:10px;margin-bottom:10px;}"
    ".clock-green{color:#2ecc71;font-size:1.4rem;font-weight:700;text-shadow:0 "
    "2px 4px rgba(0,0,0,0.1);}"
    ".nav-menu "
    "a{display:block;background:var(--primary-gradient);color:"
    "white;padding:"
    "14px;"
    "text-align:center;text-decoration:none;border-radius:12px;"
    "margin:8px 0;}"
    ".nav-menu a.logout{background:var(--danger-gradient);}"
    "@media (max-width: 600px) {"
    "  .container { width: 95%; padding: 10px; margin: 10px auto; }"
    "  .card { max-width: 100% !important; margin: 0 0 20px 0; }"
    "  input, select, button { font-size: 16px !important; }"
    "  .nav-menu a { padding: 12px; }"
    "}"
    "</style>";

// ========== Confirmation Page Template (v8.18 - CSS Enhancement) ==========
// Reusable template for confirmation/redirect pages with consistent styling
// ========== Confirmation Page Template (v8.18 - CSS Enhancement) ==========
// Reusable CSS for confirmation pages
static const char *HTML_CONF_STYLE =
    "<style>"
    ".conf-box{text-align:center;padding:50px 30px;background:white;"
    "border-radius:20px;box-shadow:0 15px 50px rgba(0,0,0,0.15);"
    "max-width:550px;margin:100px auto;animation:fadeIn 0.5s;}"
    ".conf-icon{font-size:5rem;margin-bottom:25px;animation:bounce 1s "
    "ease-in-out;}"
    "@keyframes bounce{0%,100%{transform:scale(1)}50%{transform:scale(1.15)}}"
    "@keyframes "
    "fadeIn{from{opacity:0;transform:translateY(-20px)}to{opacity:1;transform:"
    "translateY(0)}}"
    ".conf-title{font-size:2rem;color:#2c3e50;margin:20px 0;font-weight:bold;}"
    ".conf-msg{color:#555;font-size:1.15rem;margin:20px 0;line-height:1.6;}"
    ".spinner{border:5px solid #f3f3f3;border-top:5px solid #667eea;"
    "border-radius:50%;width:50px;height:50px;animation:spin 1s linear "
    "infinite;"
    "margin:30px auto;}"
    "@keyframes spin{to{transform:rotate(360deg)}}"
    ".progress-bar{width:100%;height:10px;background:#eee;border-radius:5px;"
    "overflow:hidden;margin:20px 0;display:none;}"
    ".progress-fill{height:100%;background:linear-gradient(90deg,#667eea,#"
    "764ba2);"
    "width:0%;transition:width 0.1s linear;}"
    ".conf-btns{margin-top:25px;}"
    ".conf-btns a{display:inline-block;padding:12px 30px;margin:0 8px;"
    "background:linear-gradient(135deg,#667eea,#764ba2);"
    "color:white;text-decoration:none;border-radius:10px;font-weight:bold;}"
    ".conf-btns a:hover{transform:translateY(-2px);box-shadow:0 5px 15px "
    "rgba(0,0,0,0.2);}"
    "@media (max-width: 600px) {"
    "  .conf-box { margin: 20px auto; padding: 20px; width: 90%; }"
    "  .conf-icon { font-size: 3rem; }"
    "  .conf-title { font-size: 1.5rem; }"
    "  .conf-msg { font-size: 1rem; }"
    "}"
    "</style>";

// Helper to send confirmation page header
void send_conf_header(httpd_req_t *req, const char *title,
                      const char *redirect_url, int delay_sec) {
  app_lang_t lang = i18n_get_req_lang(req);
  char html_open[128];
  snprintf(html_open, sizeof(html_open), "<!DOCTYPE html><html lang='%s' dir='%s'><head>",
           i18n_code(lang), i18n_dir(lang));
  httpd_resp_sendstr_chunk(req, html_open);
  httpd_resp_sendstr_chunk(req, "<meta charset='UTF-8'>");
  httpd_resp_sendstr_chunk(
      req,
      "<meta name='viewport' content='width=device-width,initial-scale=1.0'>");

  if (delay_sec > 0) {
    char refresh[1024];
    snprintf(refresh, sizeof(refresh),
             "<meta http-equiv='refresh' content='%d;url=%s'>"
             "<script>"
             "window.onload = function() {"
             "  var bar = document.querySelector('.progress-bar');"
             "  var fill = document.querySelector('.progress-fill');"
             "  if(bar && fill) {"
             "    bar.style.display = 'block';"
             "    var start = Date.now();"
             "    var duration = %d * 1000;"
             "    var interval = setInterval(function() {"
             "      var elapsed = Date.now() - start;"
             "      var prog = (elapsed / duration) * 100;"
             "      if(prog >= 100) { prog = 100; clearInterval(interval); }"
             "      fill.style.width = prog + '%%';"
             "    }, 100);"
             "  }"
             "};"
             "</script>",
             delay_sec, redirect_url, delay_sec);
    httpd_resp_sendstr_chunk(req, refresh);
  } else if (delay_sec == 0) {
    char refresh[128];
    snprintf(refresh, sizeof(refresh),
             "<meta http-equiv='refresh' content='0;url=%s'>", redirect_url);
    httpd_resp_sendstr_chunk(req, refresh);
  }

  httpd_resp_sendstr_chunk(req, "<title>");
  httpd_resp_sendstr_chunk(req, title);
  httpd_resp_sendstr_chunk(req, "</title>");
  httpd_resp_sendstr_chunk(req, HTML_STYLE);
  httpd_resp_sendstr_chunk(req, HTML_CONF_STYLE);
  httpd_resp_sendstr_chunk(
      req, "</head><body><div class='container'><div class='conf-box'>");

  if (delay_sec > 0) {
    httpd_resp_sendstr_chunk(
        req,
        "<div class='progress-bar'><div class='progress-fill'></div></div>");
  }
}

#define HTML_CONF_FOOTER "</div></div></body></html>"

// ========== Login Page Handler ==========
static esp_err_t login_get_handler(httpd_req_t *req) {
  app_lang_t lang = i18n_get_req_lang(req);
  httpd_resp_set_type(req, "text/html; charset=UTF-8");

  char html_open[128];
  snprintf(html_open, sizeof(html_open), "<!DOCTYPE html><html lang='%s' dir='%s'><head>",
           i18n_code(lang), i18n_dir(lang));
  httpd_resp_sendstr_chunk(req, html_open);
  httpd_resp_sendstr_chunk(req,
                           "<meta charset='UTF-8'><meta name='viewport' "
                           "content='width=device-width,initial-scale=1.0'>");
  char title_buf[256];
  snprintf(title_buf, sizeof(title_buf), "<title>%s</title>", i18n_tr(STR_APP_TITLE, lang));
  httpd_resp_sendstr_chunk(req, title_buf);
  httpd_resp_sendstr_chunk(req, HTML_STYLE);
  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");
  i18n_send_lang_bar(req, lang);
  char h1_buf[256];
  snprintf(h1_buf, sizeof(h1_buf), "<h1>%s</h1>", i18n_tr(STR_APP_TITLE, lang));
  httpd_resp_sendstr_chunk(req, h1_buf);
  char subtitle_buf[512];
  snprintf(subtitle_buf, sizeof(subtitle_buf),
           "<p style='color:#e74c3c;font-weight:bold;font-size:0.9rem;text-align:center;margin-top:-10px;margin-bottom:20px;'>"
           "%s</p>", i18n_tr(STR_APP_SUBTITLE, lang));
  httpd_resp_sendstr_chunk(req, subtitle_buf);
  httpd_resp_sendstr_chunk(req, "<form action='/login' method='POST'>");
  char form_buf[512];
  snprintf(form_buf, sizeof(form_buf),
           "<input type='password' name='password' required placeholder='%s'>"
           "<input type='submit' value='%s'>",
           i18n_tr(STR_PASSWORD, lang), i18n_tr(STR_LOGIN_BTN, lang));
  httpd_resp_sendstr_chunk(req, form_buf);
  httpd_resp_sendstr_chunk(req, "</form></div></body></html>");

  return httpd_resp_sendstr_chunk(req, NULL);
}

// ========== Login POST Handler ==========
static esp_err_t login_post_handler(httpd_req_t *req) {
  char buf[100];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) {
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  // Parse password from form data
  char *password = strstr(buf, "password=");
  if (password) {
    password += 9; // Skip "password="
    char *end = strchr(password, '&');
    if (end)
      *end = '\0';

    // Phase 1.B: Read password from NVS (fallback to DEFAULT_PASSWORD if not
    // set)
    char stored_password[64] = "";
    esp_err_t err =
        nvs_read_string("password", stored_password, sizeof(stored_password));

    // If no password in NVS, use default
    if (err != ESP_OK) {
      strncpy(stored_password, DEFAULT_PASSWORD, sizeof(stored_password) - 1);
      ESP_LOGW(TAG, "No password in NVS, using default");
    }

    if (strcmp(password, stored_password) == 0) {
      is_authorized = true;
      httpd_resp_set_status(req, "302 Found");
      httpd_resp_set_hdr(req, "Location", "/");
      return httpd_resp_send(req, NULL, 0);
    }
  }

  // Login failed - Use the styled confirmation header
  app_lang_t lang = i18n_get_req_lang(req);
  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  send_conf_header(req, i18n_tr(STR_ERROR, lang), "/", 5);
  httpd_resp_sendstr_chunk(
      req, "<div class='conf-icon' style='color:#e74c3c'>❌</div>");
  char err_h1[256];
  snprintf(err_h1, sizeof(err_h1), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_WRONG_PASSWORD, lang));
  httpd_resp_sendstr_chunk(req, err_h1);
  char err_p[256];
  snprintf(err_p, sizeof(err_p), "<p class='conf-msg'>%s</p>", i18n_tr(STR_CHECK_PASSWORD, lang));
  httpd_resp_sendstr_chunk(req, err_p);
  char retry_btn[256];
  snprintf(retry_btn, sizeof(retry_btn), "<div class='conf-btns'><a href='/'>%s</a></div>", i18n_tr(STR_RETRY, lang));
  httpd_resp_sendstr_chunk(req, retry_btn);
  httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
  return httpd_resp_sendstr_chunk(req, NULL);
}

// ========== Dashboard Handler (Main Page) ==========
// ========== Dashboard Handler (Main Page) ==========
static esp_err_t root_get_handler(httpd_req_t *req) {
  if (!is_authorized) {
    return login_get_handler(req);
  }

  // Get current data
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  char time_str[32];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);

  // Basic Next Alarm Logic for Display
  char next_alarm[32];
  alarm_get_next_time_str_with_relay(0, -1, next_alarm, sizeof(next_alarm));
  if (strlen(next_alarm) == 0)
    strcpy(next_alarm, "None");

  int relay_state = relay_get_state(0);
  char buf[1024];

  app_lang_t lang = i18n_get_req_lang(req);
  httpd_resp_set_type(req, "text/html; charset=UTF-8");

  char html_open[128];
  snprintf(html_open, sizeof(html_open), "<!DOCTYPE html><html lang='%s' dir='%s'><head>",
           i18n_code(lang), i18n_dir(lang));
  httpd_resp_sendstr_chunk(req, html_open);
  httpd_resp_sendstr_chunk(req,
                           "<meta charset='UTF-8'><meta name='viewport' "
                           "content='width=device-width,initial-scale=1.0'>");
  char title_buf[256];
  snprintf(title_buf, sizeof(title_buf), "<title>%s</title>", i18n_tr(STR_APP_TITLE, lang));
  httpd_resp_sendstr_chunk(req, title_buf);
  httpd_resp_sendstr_chunk(req, HTML_STYLE);

  // AJAX live update script
  char js_status[1024];
  snprintf(js_status, sizeof(js_status),
      "<script>"
      "function updateStatus(){"
      "fetch('/status').then(r=>r.json()).then(d=>{"
      "document.getElementById('time-val').textContent=d.time;"
      "document.getElementById('alarm-val').textContent=d.next_alarm;"
      "const rval=document.getElementById('relay-val'); if(rval){"
      "rval.textContent=d.relay?'ON':'OFF'; rval.className=d.relay?'card-value status-ok-text':'card-value status-alert-text';}"
      "var sb=document.getElementById('status-box');"
      "if(sb){"
      "  sb.className=d.relay?'status-box status-ok':'status-box status-alert';"
      "  sb.textContent=d.relay?'%s':'%s';"
      "}"
      "}).catch(e=>console.error('Update failed',e));"
      "}"
      "setInterval(updateStatus,3000);"
      "setTimeout(updateStatus,500);"
      "</script>",
      i18n_tr(STR_SYS_RUNNING, lang), i18n_tr(STR_SYS_STOPPED, lang));
  httpd_resp_sendstr_chunk(req, js_status);

  // Power & Predictive Maintenance script
  httpd_resp_sendstr_chunk(req,
      "<script>\n"
      "function loadPowerSettings() {\n"
      "  fetch('/api/power').then(r=>r.json()).then(d=>{\n"
      "    document.getElementById('power-mode').value = d.mode;\n"
      "    document.getElementById('stealth-config').style.display = (d.mode == 2) ? 'block' : 'none';\n"
      "    document.getElementById('holiday-config').style.display = (d.mode == 3) ? 'block' : 'none';\n"
      "    document.getElementById('weekly-config').style.display = (d.mode == 4) ? 'block' : 'none';\n"
      "    const f=(n)=>(n<10?'0':'')+n;\n"
      "    document.getElementById('sleep-start').value = f(d.start_hour)+':'+f(d.start_min)+':'+f(d.start_sec);\n"
      "    document.getElementById('sleep-end').value = f(d.end_hour)+':'+f(d.end_min)+':'+f(d.end_sec);\n"
      "    const fd=(y,m,d_day)=>y+'-'+f(m)+'-'+f(d_day);\n"
      "    if(d.start_year > 2000) {\n"
      "       document.getElementById('holiday-start').value = fd(d.start_year,d.start_month,d.start_day)+'T'+f(d.start_hour)+':'+f(d.start_min);\n"
      "       document.getElementById('holiday-end').value = fd(d.end_year,d.end_month,d.end_day)+'T'+f(d.end_hour)+':'+f(d.end_min);\n"
      "    }\n"
      "    if(d.weekly_schedule) {\n"
      "      for(let i=0; i<7; i++) {\n"
      "        const de = document.getElementById('w-en-'+i);\n"
      "        if(de) de.checked = d.weekly_schedule[i].enabled;\n"
      "        const ds = document.getElementById('w-start-'+i);\n"
      "        if(ds) ds.value = f(d.weekly_schedule[i].start_hour)+':'+f(d.weekly_schedule[i].start_min)+':'+f(d.weekly_schedule[i].start_sec);\n"
      "        const de_e = document.getElementById('w-end-'+i);\n"
      "        if(de_e) de_e.value = f(d.weekly_schedule[i].end_hour)+':'+f(d.weekly_schedule[i].end_min)+':'+f(d.weekly_schedule[i].end_sec);\n"
      "      }\n"
      "    }\n"
      "  });\n"
      "}\n"
      "function updatePowerMode() {\n"
      "  const mode = document.getElementById('power-mode').value;\n"
      "  document.getElementById('stealth-config').style.display = (mode == 2) ? 'block' : 'none';\n"
      "  document.getElementById('holiday-config').style.display = (mode == 3) ? 'block' : 'none';\n"
      "  document.getElementById('weekly-config').style.display = (mode == 4) ? 'block' : 'none';\n"
      "  if(mode == 0 || mode == 1) savePowerSettings();\n"
      "}\n"
      "function savePowerSettings() {\n"
      "  const mode = parseInt(document.getElementById('power-mode').value);\n"
      "  const start = document.getElementById('sleep-start').value.split(':');\n"
      "  const end = document.getElementById('sleep-end').value.split(':');\n"
      "  let data = {\n"
      "    mode: mode,\n"
      "    start_hour: parseInt(start[0]||0),\n"
      "    start_min: parseInt(start[1]||0),\n"
      "    start_sec: parseInt(start[2]||0),\n"
      "    end_hour: parseInt(end[0]||0),\n"
      "    end_min: parseInt(end[1]||0),\n"
      "    end_sec: parseInt(end[2]||0)\n"
      "  };\n"
      "  if(mode == 3) {\n"
      "    const hs = new Date(document.getElementById('holiday-start').value);\n"
      "    const he = new Date(document.getElementById('holiday-end').value);\n"
      "    if(!isNaN(hs) && !isNaN(he)) {\n"
      "      data.start_year = hs.getFullYear(); data.start_month = hs.getMonth() + 1; data.start_day = hs.getDate();\n"
      "      data.start_hour = hs.getHours(); data.start_min = hs.getMinutes(); data.start_sec = 0;\n"
      "      data.end_year = he.getFullYear(); data.end_month = he.getMonth() + 1; data.end_day = he.getDate();\n"
      "      data.end_hour = he.getHours(); data.end_min = he.getMinutes(); data.end_sec = 0;\n"
      "    }\n"
      "  }\n"
      "  if(mode == 4) {\n"
      "    data.weekly_schedule = [];\n"
      "    for(let i=0; i<7; i++) {\n"
      "      const ds = document.getElementById('w-start-'+i);\n"
      "      const de = document.getElementById('w-end-'+i);\n"
      "      if(ds && de) {\n"
      "        const wstart = ds.value.split(':');\n"
      "        const wend = de.value.split(':');\n"
      "        data.weekly_schedule.push({\n"
      "          enabled: document.getElementById('w-en-'+i).checked,\n"
      "          start_hour: parseInt(wstart[0]||0), start_min: parseInt(wstart[1]||0), start_sec: parseInt(wstart[2]||0),\n"
      "          end_hour: parseInt(wend[0]||0), end_min: parseInt(wend[1]||0), end_sec: parseInt(wend[2]||0)\n"
      "        });\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "  fetch('/api/power', {\n"
      "    method: 'POST',\n"
      "    headers: {'Content-Type': 'application/json'},\n"
      "    body: JSON.stringify(data)\n"
      "  }).then(r=>r.json()).then(res=>{\n"
      "    if(res.status==='ok') {\n"
      "       const badge=document.getElementById('save-badge');\n"
      "       if(badge){badge.style.display='inline-block'; setTimeout(()=>badge.style.display='none',3000);}\n"
      "    }\n"
      "  });\n"
      "}\n"
      "setTimeout(loadPowerSettings,1500);\n"
      "function updateHealth(){\n"
      "fetch('/api/health').then(r=>r.json()).then(d=>{\n"
      "var hb=document.getElementById('health-bar');\n"
      "var hf=document.getElementById('health-fill');\n"
      "var ht=document.getElementById('health-text');\n"
      "if(hb && hf && ht){\n"
      "  hf.style.width=d.health+'%';\n"
      "  var daysStr = '';\n"
      "  if(d.days_remaining===undefined||d.days_remaining>=9999){\n"
      "    daysStr = \"<br><span style='color:#7f8c8d;font-size:0.85rem;'>\";");
  snprintf(buf, sizeof(buf), "daysStr += \"%s</span>\";", i18n_tr(STR_MAINT_CALIBRATING, lang));
  httpd_resp_sendstr_chunk(req, buf);
  httpd_resp_sendstr_chunk(req,
      "  } else if(d.days_remaining<=0){\n"
      "    daysStr = \"<br><span style='color:#e74c3c;'>\";");
  snprintf(buf, sizeof(buf), "daysStr += \"%s</span>\";", i18n_tr(STR_MAINT_CRITICAL_REPLACE, lang));
  httpd_resp_sendstr_chunk(req, buf);
  httpd_resp_sendstr_chunk(req,
      "  } else {\n"
      "    daysStr = \"<br><span style='color:#7f8c8d;font-size:0.9rem;'>\";");
  snprintf(buf, sizeof(buf), "daysStr += \"%s\" + d.days_remaining + \"%s</span>\";",
           i18n_tr(STR_MAINT_REMAINING_PREFIX, lang), i18n_tr(STR_MAINT_DAYS, lang));
  httpd_resp_sendstr_chunk(req, buf);
  httpd_resp_sendstr_chunk(req,
      "  }\n"
      "  ht.innerHTML = \"\";");
  snprintf(buf, sizeof(buf), "ht.innerHTML = \"%s\" + d.health.toFixed(1) + \"%%%% %s\" + d.temp.toFixed(1) + \"°C\" + daysStr;",
           i18n_tr(STR_MAINT_RELAY_HEALTH, lang), i18n_tr(STR_MAINT_TEMP, lang));
  httpd_resp_sendstr_chunk(req, buf);
  httpd_resp_sendstr_chunk(req,
      "  if(d.health>80) { hf.style.background='#2ecc71'; hb.style.boxShadow='none'; }\n"
      "  else if(d.health>20) { hf.style.background='#f39c12'; hb.style.boxShadow='none'; }\n"
      "  else { hf.style.background='#e74c3c'; hb.style.boxShadow='0 0 15px #e74c3c';");
  snprintf(buf, sizeof(buf), "ht.innerHTML += \" <br><span style='color:#e74c3c'>%s</span>\";", i18n_tr(STR_MAINT_DANGER_REPLACE, lang));
  httpd_resp_sendstr_chunk(req, buf);
  httpd_resp_sendstr_chunk(req,
      "  }\n"
      "}\n"
      "}).catch(e=>console.error('Health update failed',e));\n"
      "}\n"
      "setInterval(updateHealth,5000);\n"
      "setTimeout(updateHealth,1000);\n"
      "</script>");

  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");
  i18n_send_lang_bar(req, lang);

  char h1_buf[256];
  snprintf(h1_buf, sizeof(h1_buf),
           "<h1>%s <span class='live-dot' title='System Live'></span></h1>",
           i18n_tr(STR_APP_TITLE, lang));
  httpd_resp_sendstr_chunk(req, h1_buf);

  char sub_buf[512];
  snprintf(sub_buf, sizeof(sub_buf),
           "<p style='color:#e74c3c;font-weight:bold;font-size:0.9rem;text-align:center;margin-top:-10px;margin-bottom:20px;'>"
           "%s</p>",
           i18n_tr(STR_APP_SUBTITLE, lang));
  httpd_resp_sendstr_chunk(req, sub_buf);

  // Status box
  snprintf(buf, sizeof(buf),
           "<div class='status-box %s' id='status-box'>%s</div>",
           (relay_state) ? "status-ok" : "status-alert",
           (relay_state) ? i18n_tr(STR_SYS_RUNNING, lang) : i18n_tr(STR_SYS_STOPPED, lang));
  httpd_resp_sendstr_chunk(req, buf);

  // Predictive Maintenance Health Box
  snprintf(buf, sizeof(buf),
      "<div class='card' style='max-width:100%%; margin-bottom:20px; text-align:center; padding:20px; border-radius:15px; border-left:5px solid #667eea;'>"
      "<h3 style='margin-bottom:15px; color:#2c3e50;'>%s</h3>"
      "<div id='health-bar' style='width:100%%; height:25px; background:#e0e0e0; border-radius:12px; overflow:hidden; margin-bottom:15px;'>"
      "<div id='health-fill' style='height:100%%; width:100%%; background:#2ecc71; transition:all 1s ease;'></div>"
      "</div>"
      "<div id='health-text' style='font-size:1.1rem; font-weight:bold; color:#34495e;'>%s</div>"
      "</div>",
      i18n_tr(STR_MAINT_TITLE, lang),
      i18n_tr(STR_MAINT_CHECKING, lang));
  httpd_resp_sendstr_chunk(req, buf);

  // Anomaly & Alarm Badges
  httpd_resp_sendstr_chunk(
      req, "<div id='anomaly-badge' "
           "style='display:none;background:var(--danger-gradient);color:white;"
           "padding:15px;border-radius:10px;margin:15px "
           "0;text-align:center;font-size:1.2rem;font-weight:bold;box-shadow:0 "
           "5px 15px rgba(231,76,60,0.4);animation:bounce 1s infinite "
           "alternate'></div>");
  httpd_resp_sendstr_chunk(
      req, "<div id='alarm-badge' "
           "style='display:none;background:linear-gradient(135deg,#667eea,#"
           "764ba2);color:white;padding:10px;border-radius:8px;margin:10px "
           "0;text-align:center;font-weight:bold'>&#128276; Alarme "
           "Activ&eacute;e</div>");

  // Dashboard Grid
  httpd_resp_sendstr_chunk(req, "<div class='dash-grid'>");

  // 1. Time Card
  snprintf(buf, sizeof(buf),
           "<div class='card'><span class='card-icon'>🕒</span><span "
           "class='card-title'>%s</span><span class='clock-green' "
           "id='time-val'>%s</span></div>",
           i18n_tr(STR_CURRENT_TIME, lang),
           time_str);
  httpd_resp_sendstr_chunk(req, buf);

  // 2. Next Alarm Card
  snprintf(buf, sizeof(buf),
           "<div class='card'><span class='card-icon'>🔔</span><span "
           "class='card-title'>%s</span><span class='card-value' "
           "id='alarm-val'>%s</span></div>",
           i18n_tr(STR_NEXT_BELL, lang),
           next_alarm);
  httpd_resp_sendstr_chunk(req, buf);

  // 3. Relays
  const char *relay_names[] = {
      i18n_tr(STR_RELAY_BELL, lang),
      i18n_tr(STR_RELAY_DEV_A, lang),
      i18n_tr(STR_RELAY_DEV_B, lang),
      i18n_tr(STR_RELAY_DEV_C, lang)
  };
  for (int r = 0; r < 4; r++) {
    int state = relay_get_state(r);
    snprintf(buf, sizeof(buf),
             "<div class='card'><span class='card-icon'>🔌</span><span "
             "class='card-title'>%s</span><span class='card-value "
             "%s'>%s</span></div>",
             relay_names[r], state ? "status-ok-text" : "status-alert-text",
             state ? "ON" : "OFF");
    httpd_resp_sendstr_chunk(req, buf);
  }
  httpd_resp_sendstr_chunk(req, "</div>");

  // === ENERGY MANAGEMENT SECTION ===
  snprintf(buf, sizeof(buf),
      "<div class='card' style='max-width:100%%; margin-bottom:20px; text-align:%s; border-left:5px solid #16a085; direction:%s;'>"
      "<h3>%s</h3>"
      "<div class='data-row'>"
      "<span>%s</span>"
      "<select id='power-mode' onchange='updatePowerMode()' style='width:auto; padding:5px; margin:0;'>",
      (lang == LANG_AR) ? "right" : "left",
      i18n_dir(lang),
      i18n_tr(STR_POWER_TITLE, lang),
      i18n_tr(STR_POWER_MODE, lang));
  httpd_resp_sendstr_chunk(req, buf);

  snprintf(buf, sizeof(buf),
      "<option value='0'>%s</option>"
      "<option value='1'>%s</option>"
      "<option value='2'>%s</option>"
      "<option value='3'>%s</option>"
      "<option value='4'>%s</option>"
      "</select></div>",
      i18n_tr(STR_POWER_MODE_NORMAL, lang),
      i18n_tr(STR_POWER_MODE_MODEM, lang),
      i18n_tr(STR_POWER_MODE_DEEP, lang),
      i18n_tr(STR_POWER_MODE_HOLIDAY, lang),
      i18n_tr(STR_POWER_MODE_WEEKLY, lang));
  httpd_resp_sendstr_chunk(req, buf);

  // Stealth / Daily config
  snprintf(buf, sizeof(buf),
      "<div id='stealth-config' style='display:none; margin-top:10px; border-top:1px solid #eee; padding-top:10px;'>"
      "<p style='font-size:0.8rem; color:#666; margin-bottom:10px;'>%s</p>"
      "<div style='display:flex; gap:10px; justify-content:%s; align-items:center;'>"
      "<input type='time' id='sleep-start' step='1' style='width:auto; margin:0; padding:8px;'>"
      "<span>%s</span>"
      "<input type='time' id='sleep-end' step='1' style='width:auto; margin:0; padding:8px;'>"
      "</div></div>",
      i18n_tr(STR_POWER_DAILY_DESC, lang),
      (lang == LANG_AR) ? "flex-end" : "flex-start",
      i18n_tr(STR_POWER_TO, lang));
  httpd_resp_sendstr_chunk(req, buf);

  // Holiday config
  snprintf(buf, sizeof(buf),
      "<div id='holiday-config' style='display:none; margin-top:10px; border-top:1px solid #eee; padding-top:10px;'>"
      "<p style='font-size:0.8rem; color:#666; margin-bottom:10px;'>%s</p>"
      "<div style='display:flex; flex-direction:column; gap:10px; padding:0 20px;'>"
      "<label style='display:flex; justify-content:space-between; align-items:center;'>%s <input type='datetime-local' id='holiday-start' style='padding:8px;'></label>"
      "<label style='display:flex; justify-content:space-between; align-items:center;'>%s <input type='datetime-local' id='holiday-end' style='padding:8px;'></label>"
      "</div></div>",
      i18n_tr(STR_POWER_HOLIDAY_DESC, lang),
      i18n_tr(STR_POWER_FROM, lang),
      i18n_tr(STR_POWER_TO, lang));
  httpd_resp_sendstr_chunk(req, buf);

  // Weekly config
  snprintf(buf, sizeof(buf),
      "<div id='weekly-config' style='display:none; margin-top:10px; border-top:1px solid #eee; padding-top:10px;'>"
      "<p style='font-size:0.8rem; color:#666; margin-bottom:10px;'>%s</p>"
      "<div style='display:flex; flex-direction:column; gap:10px;'>",
      i18n_tr(STR_POWER_WEEKLY_DESC, lang));
  httpd_resp_sendstr_chunk(req, buf);

  const app_str_t day_keys[] = {
      STR_DAY_SUN_SHORT, STR_DAY_MON_SHORT, STR_DAY_TUE_SHORT,
      STR_DAY_WED_SHORT, STR_DAY_THU_SHORT, STR_DAY_FRI_SHORT, STR_DAY_SAT_SHORT
  };
  for (int d_idx = 0; d_idx < 7; d_idx++) {
      snprintf(buf, sizeof(buf),
          "<div style='display:flex; justify-content:space-between; align-items:center; font-size:0.8rem;'>"
          "<label><input type='checkbox' id='w-en-%d'> %s</label> "
          "<div><input type='time' id='w-start-%d' step='1'> - <input type='time' id='w-end-%d' step='1'></div>"
          "</div>",
          d_idx, i18n_tr(day_keys[d_idx], lang), d_idx, d_idx);
      httpd_resp_sendstr_chunk(req, buf);
  }

  // Save button & badge
  snprintf(buf, sizeof(buf),
      "</div></div>"
      "<div style='margin-top:15px; text-align:%s;'>"
      "<span id='save-badge' style='display:none; color:#2ecc71; margin-%s:15px; font-weight:bold; font-size:1.1rem; vertical-align:middle;'>%s</span>"
      "<button onclick='savePowerSettings()' style='background:#16a085; color:white; border:none; padding:8px 15px; border-radius:8px; cursor:pointer; vertical-align:middle;'>%s</button>"
      "</div></div>",
      (lang == LANG_AR) ? "left" : "right",
      (lang == LANG_AR) ? "right" : "left",
      i18n_tr(STR_POWER_SAVED_OK, lang),
      i18n_tr(STR_POWER_SAVE_BTN, lang));
  httpd_resp_sendstr_chunk(req, buf);

  // === CAMERA SECTION ===
  char cam_ip_display[32] = "";
  char stored_ip[32];
  if (nvs_read_string("cam_ip", stored_ip, sizeof(stored_ip)) == ESP_OK &&
      strlen(stored_ip) > 0) {
    strcpy(cam_ip_display, stored_ip);
  }

  const char *cam_target =
      (strlen(cam_ip_display) > 0) ? cam_ip_display : "esp32-cam.local";

  // 1. Camera Feed Card Header and Image
  snprintf(buf, sizeof(buf),
           "<div class='card' style='max-width:100%%; overflow:hidden; margin-bottom: 20px;'>"
           "<h3>%s</h3>"
           "<div style='width:100%%; max-width:640px; border-radius:15px; overflow:hidden; box-shadow:0 4px 15px rgba(0,0,0,0.2); background:#000; margin:auto;'>"
           "<img id='cam-stream' style='width:100%%; height:auto; display:block;' src='http://%s/stream' "
           "onerror=\"this.src='https://placehold.co/640x480/000000/ffffff.png?text=Camera+Offline'; setTimeout(()=>this.src='http://%s/stream?t='+Date.now(), 5000);\">"
           "</div>",
           i18n_tr(STR_CAM_FEED, lang), cam_target, cam_target);
  httpd_resp_sendstr_chunk(req, buf);

  // 2. Control Button Link
  snprintf(
      buf, sizeof(buf),
      "<div style='margin-top:20px; display:flex; justify-content:center;'>"
      "<a href='http://%s/' id='cam-link' "
      "style='background:linear-gradient(45deg,#3498db,#2980b9); color:white; "
      "padding:12px 25px; text-decoration:none; border-radius:10px; font-weight:bold; "
      "box-shadow:0 4px 15px rgba(52,152,219,0.3); transition:all 0.3s;'>%s</a>"
      "</div></div>",
      cam_target, i18n_tr(STR_CAM_DASHBOARD, lang));
  httpd_resp_sendstr_chunk(req, buf);

  char js_cam[512];
  snprintf(js_cam, sizeof(js_cam),
      "<script>"
      "const cl=document.getElementById('cam-link');"
      "cl.onclick=(e)=>{"
      "  e.preventDefault();"
      "  const img=document.getElementById('cam-stream');"
      "  if(img){ img.src=''; }"
      "  setTimeout(()=>{"
      "    const base=cl.href.split('/')[2];"
      "    if(!base || base==='esp32-cam.local'){"
      "      const ip=prompt(\"%s\");"
      "      if(ip){ location.href='http://'+ip+'/'; }"
      "    } else { location.href=cl.href; }"
      "  }, 300);"
      "};"
      "</script>",
      i18n_tr(STR_CAM_ENTER_IP, lang));
  httpd_resp_sendstr_chunk(req, js_cam);

  // RTC Manual & Auto-Device Sync Form
  snprintf(buf, sizeof(buf),
      "<div class='card' style='margin:20px 0;text-align:%s'>"
      "<h3>%s</h3>"
      "<button type='button' id='syncDevBtn' "
      "style='background:linear-gradient(135deg,#2980b9,#3498db);color:white;padding:12px 16px;"
      "border:none;border-radius:8px;font-weight:bold;cursor:pointer;width:100%%;margin-bottom:15px;"
      "font-size:1rem;display:flex;align-items:center;justify-content:center;gap:8px;box-shadow:0 3px 6px rgba(0,0,0,0.15)'>"
      "%s"
      "</button>",
      (lang == LANG_AR) ? "right" : "left",
      i18n_tr(STR_SET_TIME_TITLE, lang),
      i18n_tr(STR_SYNC_DEV_TIME, lang));
  httpd_resp_sendstr_chunk(req, buf);

  snprintf(buf, sizeof(buf),
      "<form id='timeForm' method='POST' action='/setDateTime' style='display:grid;gap:10px;direction:%s'>"
      "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;'>"
      "<div style='text-align:center'><label style='font-size:0.8rem;display:block'>%s</label><input type='number' id='year' min='2000' max='2099' value='2026' style='padding:8px;width:100%%'></div>"
      "<div style='text-align:center'><label style='font-size:0.8rem;display:block'>%s</label><input type='number' id='month' min='1' max='12' style='padding:8px;width:100%%'></div>"
      "<div style='text-align:center'><label style='font-size:0.8rem;display:block'>%s</label><input type='number' id='day' min='1' max='31' style='padding:8px;width:100%%'></div>"
      "</div>",
      i18n_dir(lang),
      i18n_tr(STR_YEAR, lang),
      i18n_tr(STR_MONTH, lang),
      i18n_tr(STR_DAY, lang));
  httpd_resp_sendstr_chunk(req, buf);

  snprintf(buf, sizeof(buf),
      "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;'>"
      "<div style='text-align:center'><label style='font-size:0.8rem;display:block'>%s</label><input type='number' id='hour' min='0' max='23' style='padding:8px;width:100%%'></div>"
      "<div style='text-align:center'><label style='font-size:0.8rem;display:block'>%s</label><input type='number' id='minute' min='0' max='59' style='padding:8px;width:100%%'></div>"
      "<div style='display:flex;align-items:flex-end'><button type='submit' style='background:var(--success-gradient);color:white;padding:12px;border:none;border-radius:8px;font-weight:bold;cursor:pointer;width:100%%'>%s</button></div>"
      "</div></form><div id='timeResult' style='margin-top:10px;font-size:0.95rem;text-align:center'></div></div>",
      i18n_tr(STR_HOUR, lang),
      i18n_tr(STR_MINUTE, lang),
      i18n_tr(STR_SAVE_TIME, lang));
  httpd_resp_sendstr_chunk(req, buf);

  const char *sync_wait = (lang == LANG_AR) ? "جاري المزامنة الفورية مع جهازك..." :
                          (lang == LANG_FR) ? "Synchronisation en cours..." :
                          "Syncing with device...";
  const char *save_wait = (lang == LANG_AR) ? "جاري حفظ التعديل اليدوي..." :
                          (lang == LANG_FR) ? "Enregistrement en cours..." :
                          "Saving manual time...";
  const char *sync_ok = (lang == LANG_AR) ? "✅ تم ضبط الوقت وتحديث الساعة بنجاح!" :
                        (lang == LANG_FR) ? "✅ Heure mise à jour avec succès !" :
                        "✅ Time updated successfully!";
  const char *sync_err = (lang == LANG_AR) ? "❌ خطأ: " :
                         (lang == LANG_FR) ? "❌ Erreur : " :
                         "❌ Error: ";

  httpd_resp_sendstr_chunk(req,
      "<script>"
      "function sendDateTimeData(d, waitMsg){"
      "var res=document.getElementById('timeResult');"
      "res.innerHTML='⏳ '+waitMsg;"
      "fetch('/setDateTime',{method:'POST',headers:{'Content-Type':'application/json'},"
      "body:JSON.stringify(d)}).then(function(r){"
      "if(!r.ok) throw new Error('Status '+r.status); return r.json();"
      "}).then(function(data){");

  snprintf(buf, sizeof(buf),
      "res.innerHTML='<span style=\"color:#27ae60;font-weight:bold;\">%s</span>';"
      "setTimeout(function(){location.reload();},1200);"
      "}).catch(function(err){"
      "res.innerHTML='<span style=\"color:#e74c3c;font-weight:bold;\">%s'+err.message+'</span>';"
      "});"
      "}",
      sync_ok, sync_err);
  httpd_resp_sendstr_chunk(req, buf);

  snprintf(buf, sizeof(buf),
      "document.getElementById('syncDevBtn').onclick=function(){"
      "var now=new Date();"
      "var d={year:now.getFullYear(),month:now.getMonth()+1,day:now.getDate(),"
      "hour:now.getHours(),minute:now.getMinutes(),second:now.getSeconds(),"
      "tz_offset:now.getTimezoneOffset()};"
      "sendDateTimeData(d,'%s');"
      "};",
      sync_wait);
  httpd_resp_sendstr_chunk(req, buf);

  snprintf(buf, sizeof(buf),
      "document.getElementById('timeForm').onsubmit=function(e){"
      "e.preventDefault();"
      "var now=new Date();"
      "var d={year:parseInt(document.getElementById('year').value),"
      "month:parseInt(document.getElementById('month').value),"
      "day:parseInt(document.getElementById('day').value),"
      "hour:parseInt(document.getElementById('hour').value),"
      "minute:parseInt(document.getElementById('minute').value),"
      "second:now.getSeconds(),"
      "tz_offset:now.getTimezoneOffset()};"
      "sendDateTimeData(d,'%s');"
      "return false;"
      "};",
      save_wait);
  httpd_resp_sendstr_chunk(req, buf);

  httpd_resp_sendstr_chunk(req,
      "(function(){var n=new Date();"
      "document.getElementById('year').value=n.getFullYear();"
      "document.getElementById('month').value=n.getMonth()+1;"
      "document.getElementById('day').value=n.getDate();"
      "document.getElementById('hour').value=n.getHours();"
      "document.getElementById('minute').value=n.getMinutes();"
      "})();"
      "</script>");

  // Navigation Menu
  httpd_resp_sendstr_chunk(req, "<div class='nav-menu'>");
  char nav_item[256];
  snprintf(nav_item, sizeof(nav_item), "<a href='/settings'>%s</a>", i18n_tr(STR_NAV_SETTINGS, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  snprintf(nav_item, sizeof(nav_item), "<a href='/wifiConfig'>%s</a>", i18n_tr(STR_NAV_WIFI, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  snprintf(nav_item, sizeof(nav_item), "<a href='/timezoneConfig'>%s</a>", i18n_tr(STR_NAV_TIMEZONE, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  snprintf(nav_item, sizeof(nav_item), "<a href='/changePassword'>%s</a>", i18n_tr(STR_NAV_PASSWORD, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  snprintf(nav_item, sizeof(nav_item), "<a href='/about'>%s</a>", i18n_tr(STR_NAV_ABOUT, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  snprintf(nav_item, sizeof(nav_item), "<a href='/backup' style='background:#16a085'>%s</a>", i18n_tr(STR_NAV_BACKUP, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  snprintf(nav_item, sizeof(nav_item), "<a href='/update' style='background:#9b59b6'>%s</a>", i18n_tr(STR_NAV_OTA, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  snprintf(nav_item, sizeof(nav_item), "<a href='/logs' style='background:#27ae60' download>%s</a>", i18n_tr(STR_NAV_LOGS, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  snprintf(nav_item, sizeof(nav_item), "<a href='/restart' style='background:#f39c12'>%s</a>", i18n_tr(STR_NAV_RESTART, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  snprintf(nav_item, sizeof(nav_item), "<a href='/factoryReset' style='background:#c0392b'>%s</a>", i18n_tr(STR_NAV_FACTORY_RESET, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  snprintf(nav_item, sizeof(nav_item), "<a href='/logout' class='logout'>%s</a>", i18n_tr(STR_NAV_LOGOUT, lang));
  httpd_resp_sendstr_chunk(req, nav_item);
  httpd_resp_sendstr_chunk(req, "</div></div></body></html>");

  return httpd_resp_sendstr_chunk(req, NULL);
}

// ========== Settings Handler (Complete v7.1.1 reproduction) ==========
static esp_err_t settings_get_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  // Get current day for auto-selection
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  int currentDayIndex = (timeinfo.tm_wday + 6) % 7; // 0=Monday

  app_lang_t lang = i18n_get_req_lang(req);
  httpd_resp_set_type(req, "text/html; charset=UTF-8");

  char html_open[128];
  snprintf(html_open, sizeof(html_open), "<!DOCTYPE html><html lang='%s' dir='%s'><head>",
           i18n_code(lang), i18n_dir(lang));
  httpd_resp_sendstr_chunk(req, html_open);
  httpd_resp_sendstr_chunk(req,
                           "<meta charset='UTF-8'><meta name='viewport' "
                           "content='width=device-width,initial-scale=1.0'>");
  char title_buf[256];
  snprintf(title_buf, sizeof(title_buf), "<title>%s</title>", i18n_tr(STR_SCHEDULE_TITLE, lang));
  httpd_resp_sendstr_chunk(req, title_buf);
  httpd_resp_sendstr_chunk(req, HTML_STYLE);

  // Mobile responsive styles (settings page specific)
  httpd_resp_sendstr_chunk(
      req, "<style>"
           "@media (max-width: 600px) {"
           "  .info-header { padding: 8px !important; font-size: 0.9rem "
           "!important; }"
           "  .tab-container { gap: 4px !important; padding: 6px !important; }"
           "  .tablink { padding: 8px 6px !important; font-size: 0.8rem "
           "!important; min-width: 70px; }"
           "  .grid { grid-template-columns: 1fr !important; }"
           "  input[type='time'], select { font-size: 16px !important; "
           "padding: 12px !important; }"
           "  .alarm-entry { padding: 10px !important; }"
           "  .alarm-entry h3 { font-size: 0.95rem !important; }"
           "}"
           "@media (max-width: 400px) {"
           "  .tablink { font-size: 0.7rem !important; padding: 6px 4px "
           "!important; min-width: 50px; }"
           "}"
           "</style>");

  // JavaScript: Tabs + Validation + Date/Time Update
  char js_part1[2048];
  snprintf(js_part1, sizeof(js_part1),
      "<script>"
      "function openTab(evt, dayName) {"
      " var i, tabcontent, tablinks;"
      " tabcontent = document.getElementsByClassName('tabcontent');"
      " for (i = 0; i < tabcontent.length; i++) { tabcontent[i].style.display = 'none'; }"
      " tablinks = document.getElementsByClassName('tablink');"
      " for (i = 0; i < tablinks.length; i++) { tablinks[i].className = tablinks[i].className.replace(' active', ''); }"
      " document.getElementById(dayName).style.display = 'block';"
      " if(evt) evt.currentTarget.className += ' active';"
      "}"
      "function validateForm(evt) {"
      "  const days = 7; const alarms = 20;"
      "  for (let i = 0; i < days; i++) {"
      "    for (let j = 0; j < alarms; j++) {"
      "      let s_input = document.getElementsByName('start' + i + '_' + j)[0];"
      "      let e_input = document.getElementsByName('end' + i + '_' + j)[0];"
      "      let endday_input = document.getElementsByName('endday' + i + '_' + j)[0];"
      "      if (s_input && e_input && endday_input) {"
      "        let s = s_input.value; let e_val = e_input.value; let endDay = endday_input.value;"
      "        if (s !== '' && e_val !== '') {"
      "          let sParts = s.split(':'); let eParts = e_val.split(':');"
      "          let sSec = parseInt(sParts[0])*3600 + parseInt(sParts[1])*60 + (sParts[2] ? parseInt(sParts[2]) : 0);"
      "          let eSec = parseInt(eParts[0])*3600 + parseInt(eParts[1])*60 + (eParts[2] ? parseInt(eParts[2]) : 0);"
      "          if (endDay == '0') {"
      "            if (eSec <= sSec) {"
      "              alert(\"%s\");"
      "              evt.preventDefault(); return false;"
      "            }"
      "          }"
      "        }"
      "      }"
      "    }"
      "  }"
      "  return true;"
      "}"
      "document.addEventListener('DOMContentLoaded', function() {",
      i18n_tr(STR_ALERT_END_AFTER_START, lang));

  httpd_resp_sendstr_chunk(req, js_part1);

  char buf[512];
  snprintf(
      buf, sizeof(buf),
      " try { openTab(null, 'day%d');"
      " const activeButton = document.querySelector('.tablink:nth-child(%d)');"
      " if (activeButton) { activeButton.classList.add('active'); } } catch(e) { console.error('Tab init error', e); }"
      "});",
      currentDayIndex, currentDayIndex + 1);
  httpd_resp_sendstr_chunk(req, buf);

  // Time update script
  httpd_resp_sendstr_chunk(
      req,
      "setInterval(() => fetch('/time').then(res => res.text()).then(data => {"
      " const elem = document.getElementById('dateTime');"
      " if (elem) elem.innerText = data;"
      "}).catch(err => console.log('Time update failed')), 1000);"
      "</script>");

  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");
  i18n_send_lang_bar(req, lang);

  // Card Settings Start
  char card_start[256];
  snprintf(card_start, sizeof(card_start),
           "<div class='card' style='max-width:900px;margin:0 auto;text-align:%s'>",
           (lang == LANG_AR) ? "right" : "left");
  httpd_resp_sendstr_chunk(req, card_start);

  // Header with Icon
  char sched_h1[256];
  snprintf(sched_h1, sizeof(sched_h1),
           "<div style='text-align:center'><span class='card-icon' style='font-size:3rem'>⏰</span><h1>%s</h1></div>",
           i18n_tr(STR_SCHEDULE_TITLE, lang));
  httpd_resp_sendstr_chunk(req, sched_h1);

  // Date/Time header (STICKY)
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
  snprintf(buf, sizeof(buf),
           "<div class='info-header' "
           "style='position:sticky;top:0;background:white;padding:15px;margin-"
           "bottom:20px;box-shadow:0 2px 8px "
           "rgba(0,0,0,0.1);z-index:100;border-radius:8px'>%s <span "
           "id='dateTime'>%s</span></div>",
           i18n_tr(STR_DATE_AND_TIME, lang),
           time_str);
  httpd_resp_sendstr_chunk(req, buf);

  // Form with validation
  httpd_resp_sendstr_chunk(
      req, "<form id='settingsForm' action='/save' method='POST' "
           "onsubmit='return validateForm(event)'>");

  // Day tabs
  httpd_resp_sendstr_chunk(
      req, "<div class='tab-container' style='display:flex;flex-wrap:wrap;"
           "gap:8px;justify-content:center;margin-bottom:15px;"
           "position:sticky;top:70px;background:white;padding:10px;"
           "z-index:99;box-shadow:0 2px 5px rgba(0,0,0,0.1)'>");
  for (int i = 0; i < 7; i++) {
    snprintf(buf, sizeof(buf),
             "<button type='button' class='tablink%s' style='width:auto' "
             "onclick=\"openTab(event, 'day%d')\">%s</button>",
             (i == currentDayIndex) ? " active" : "", i, i18n_day_name(i, lang));
    httpd_resp_sendstr_chunk(req, buf);
  }
  httpd_resp_sendstr_chunk(req, "</div>");

  // Day content with Grid layout
  for (int i = 0; i < 7; i++) {
    snprintf(buf, sizeof(buf), "<div id='day%d' class='tabcontent'%s>", i, (i == currentDayIndex) ? " style='display:block;'" : "");
    httpd_resp_sendstr_chunk(req, buf);

    snprintf(buf, sizeof(buf), "<h2>%s</h2>", i18n_day_name(i, lang));
    httpd_resp_sendstr_chunk(req, buf);

    // Enabled checkbox
    snprintf(
        buf, sizeof(buf),
        "<label>%s <input type='checkbox' name='enabled_%d'%s></label><hr>",
        i18n_tr(STR_ENABLE_DAY, lang),
        i, alarmsEnabled[0][i] ? " checked" : "");
    httpd_resp_sendstr_chunk(req, buf);

    // Grid container for alarms
    httpd_resp_sendstr_chunk(req, "<div class='grid'>");

    int emptySlotsShown = 0;
    for (int j = 0; j < MAX_ALARMS_PER_DAY; j++) {
      bool isConfigured = (schedule[0][i].startHour[j] != -1);

      // Optimization: Only show configured alarms OR at most 10 empty slots
      if (!isConfigured) {
        if (emptySlotsShown >= 10)
          continue;
        emptySlotsShown++;
      }

      char startVal[16] = "";
      char endVal[16] = "";
      int endDayOffset = schedule[0][i].endDayOffset[j];

      if (isConfigured) {
        snprintf(startVal, sizeof(startVal), "%02d:%02d:%02d",
                 schedule[0][i].startHour[j], schedule[0][i].startMinute[j],
                 schedule[0][i].startSecond[j]);
      }
      if (schedule[0][i].endHour[j] != -1) {
        snprintf(endVal, sizeof(endVal), "%02d:%02d:%02d",
                 schedule[0][i].endHour[j], schedule[0][i].endMinute[j],
                 schedule[0][i].endSecond[j]);
      }

      httpd_resp_sendstr_chunk(req, "<div class='alarm-entry'>");

      snprintf(buf, sizeof(buf), "<h3>%s %d</h3>", i18n_tr(STR_ALARM_NUM, lang), j + 1);
      httpd_resp_sendstr_chunk(req, buf);

      // Start time
      if (strlen(startVal) > 0) {
        snprintf(buf, sizeof(buf),
                 "%s <input type='time' step='1' name='start%d_%d' "
                 "value='%s'><br>",
                 i18n_tr(STR_START_TIME, lang), i, j, startVal);
      } else {
        snprintf(buf, sizeof(buf),
                 "%s <input type='time' step='1' name='start%d_%d'><br>",
                 i18n_tr(STR_START_TIME, lang), i, j);
      }
      httpd_resp_sendstr_chunk(req, buf);

      // End time
      if (strlen(endVal) > 0) {
        snprintf(buf, sizeof(buf),
                 "%s <input type='time' step='1' name='end%d_%d' "
                 "value='%s'><br>",
                 i18n_tr(STR_END_TIME, lang), i, j, endVal);
      } else {
        snprintf(buf, sizeof(buf),
                 "%s <input type='time' step='1' name='end%d_%d'><br>",
                 i18n_tr(STR_END_TIME, lang), i, j);
      }
      httpd_resp_sendstr_chunk(req, buf);

      // End day selector
      snprintf(buf, sizeof(buf),
               "%s <select name='endday%d_%d'>"
               "<option value='0'%s>%s</option>"
               "<option value='1'%s>%s</option>"
               "</select><br>",
               i18n_tr(STR_END_DAY, lang), i, j,
               (endDayOffset == 0) ? " selected" : "", i18n_tr(STR_SAME_DAY, lang),
               (endDayOffset == 1) ? " selected" : "", i18n_tr(STR_NEXT_DAY, lang));
      httpd_resp_sendstr_chunk(req, buf);

      // Relay selector
      int rIdx = schedule[0][i].relayIdx[j];
      if (rIdx < 0 || rIdx > 3)
        rIdx = 0;

      snprintf(buf, sizeof(buf),
               "%s <select name='relay%d_%d'>"
               "<option value='0'%s>%s</option>"
               "<option value='1'%s>%s</option>"
               "<option value='2'%s>%s</option>"
               "<option value='3'%s>%s</option>"
               "</select><br>",
               i18n_tr(STR_SELECT_OUTPUT, lang), i, j,
               (rIdx == 0) ? " selected" : "", i18n_tr(STR_RELAY_BELL, lang),
               (rIdx == 1) ? " selected" : "", i18n_tr(STR_RELAY_DEV_A, lang),
               (rIdx == 2) ? " selected" : "", i18n_tr(STR_RELAY_DEV_B, lang),
               (rIdx == 3) ? " selected" : "", i18n_tr(STR_RELAY_DEV_C, lang));
      httpd_resp_sendstr_chunk(req, buf);

      httpd_resp_sendstr_chunk(req, "</div>");
    }

    httpd_resp_sendstr_chunk(req, "</div></div>"); // Close grid and tabcontent
  }

  // Card End
  httpd_resp_sendstr_chunk(req, "</div>"); // Close Card

  // Close form tag BEFORE Import/Export section
  httpd_resp_sendstr_chunk(req, "</form>");

  // Import/Export Section
  char imp_buf[768];
  snprintf(imp_buf, sizeof(imp_buf),
      "<div style='margin:20px auto;padding:20px;background:#ecf0f1;border-radius:10px;max-width:600px'>"
      "<h3 style='text-align:center;margin-bottom:15px'>%s</h3>"
      "<form action='/uploadSchedule' method='POST' enctype='multipart/form-data' style='margin-bottom:15px;text-align:center'>"
      "<label style='display:block;margin-bottom:10px;font-weight:bold'>%s</label>"
      "<input type='file' name='schedule' accept='.json' style='margin-bottom:10px;display:block;margin:10px auto'>"
      "<button type='submit' style='background:#27ae60;color:white;padding:12px 24px;border:none;border-radius:8px;cursor:pointer;font-size:1rem;font-weight:bold'>%s</button>"
      "</form>"
      "<hr style='margin:20px 0;border:none;border-top:1px solid #bdc3c7'>",
      i18n_tr(STR_IMPORT_EXPORT, lang),
      i18n_tr(STR_IMPORT_JSON, lang),
      i18n_tr(STR_UPLOAD_SCHEDULE, lang));
  httpd_resp_sendstr_chunk(req, imp_buf);

  char dl_buf[512];
  snprintf(dl_buf, sizeof(dl_buf),
      "<div style='text-align:center'>"
      "<a href='/downloadSchedule' download='alarmes.json' style='display:inline-block;background:#3498db;color:white;padding:12px 24px;text-decoration:none;border-radius:8px;font-size:1rem;font-weight:bold'>%s</a>"
      "</div></div>",
      i18n_tr(STR_DOWNLOAD_SCHEDULE, lang));
  httpd_resp_sendstr_chunk(req, dl_buf);

  // Single Sticky Container for both buttons (no overlap)
  char sticky_btn[1024];
  snprintf(sticky_btn, sizeof(sticky_btn),
      "<div style='position:sticky;bottom:0;background:white;padding:15px;box-shadow:0 -4px 10px rgba(0,0,0,0.15);z-index:100'>"
      "<button id='saveBtn' onclick='showSaving()' style='width:100%%;margin-bottom:10px;font-size:1.1rem;font-weight:bold;padding:12px;background:#667eea;color:white;border:none;border-radius:8px;cursor:pointer'>%s</button>"
      "<a href='/' style='display:block;text-align:center;padding:12px;background:#95a5a6;color:white;text-decoration:none;border-radius:8px'>%s</a>"
      "</div>",
      i18n_tr(STR_SAVE_SETTINGS, lang),
      i18n_tr(STR_BACK_TO_DASHBOARD, lang));
  httpd_resp_sendstr_chunk(req, sticky_btn);

  char sticky_script[1024];
  snprintf(sticky_script, sizeof(sticky_script),
      "<script>"
      "function showSaving(){"
      "  var btn = document.getElementById('saveBtn');"
      "  btn.innerHTML = '%s <div class=\"spinner\" style=\"display:inline-block;width:20px;height:20px;vertical-align:middle;border:3px solid #eee;border-top:3px solid #667eea;margin-left:10px;border-radius:50%%;animation:spin 1s linear infinite\"></div>';"
      "  btn.disabled = true;"
      "  btn.style.opacity = '0.7';"
      "  document.getElementById('settingsForm').submit();"
      "}"
      "</script>"
      "</div></body></html>",
      i18n_tr(STR_SAVING, lang));
  httpd_resp_sendstr_chunk(req, sticky_script);

  return httpd_resp_sendstr_chunk(req, NULL);
}

// ========== Download Schedule Handler (JSON Export) ==========
static esp_err_t download_schedule_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  // Set response headers for chunked encoding download
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Content-Disposition",
                     "attachment; filename=\"alarmes.json\"");

  // Start sending JSON stream in chunks, avoiding massive malloc()
  httpd_resp_sendstr_chunk(req, "{\"version\":\"1.0\",\"schedule\":{");

  const char *daysEn[] = {"monday", "tuesday",  "wednesday", "thursday",
                          "friday", "saturday", "sunday"};

  char chunk[256];

  for (int i = 0; i < 7; i++) {
    snprintf(chunk, sizeof(chunk), "\"%s\":[", daysEn[i]);
    httpd_resp_sendstr_chunk(req, chunk);

    bool first = true;
    for (int j = 0; j < MAX_ALARMS_PER_DAY; j++) {
      if (schedule[0][i].startHour[j] != -1 &&
          schedule[0][i].endHour[j] != -1) {

        snprintf(chunk, sizeof(chunk),
                 "%s{\"start\":\"%02d:%02d:%02d\",\"end\":\"%02d:%02d:%02d\","
                 "\"endDay\":%d,\"relay\":%d}",
                 first ? "" : ",", schedule[0][i].startHour[j],
                 schedule[0][i].startMinute[j], schedule[0][i].startSecond[j],
                 schedule[0][i].endHour[j], schedule[0][i].endMinute[j],
                 schedule[0][i].endSecond[j], schedule[0][i].endDayOffset[j],
                 schedule[0][i].relayIdx[j]);

        // Send this single alarm as a chunk
        httpd_resp_sendstr_chunk(req, chunk);
        first = false;
      }
    }

    if (i < 6) {
      httpd_resp_sendstr_chunk(req, "],");
    } else {
      httpd_resp_sendstr_chunk(req, "]");
    }
  }

  httpd_resp_sendstr_chunk(req, "}}");

  // Send NULL chunk to signify end of chunked response
  esp_err_t ret = httpd_resp_sendstr_chunk(req, NULL);

  ESP_LOGI(TAG, "Schedule downloaded successfully using secure streaming");
  return ret;
}

// ========== Upload Schedule Handler (JSON Import) ==========
static esp_err_t upload_schedule_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  size_t total_size = req->content_len;
  if (total_size == 0 ||
      total_size > 65536) { // Allow up to 64KB max for large complet files
    ESP_LOGE(TAG, "Invalid upload size: %d", total_size);
    return ESP_FAIL;
  }

  // Dynamically allocate EXACTLY what we need instead of a massive 96KB block
  char *buf = malloc(total_size + 1);
  if (!buf) {
    ESP_LOGE(TAG, "Failed to allocate exactly %d bytes for upload", total_size);
    return ESP_FAIL;
  }

  size_t received = 0;
  while (received < total_size) {
    int ret = httpd_req_recv(req, buf + received, total_size - received);
    if (ret <= 0) {
      if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        continue;
      free(buf);
      return ESP_FAIL;
    }
    received += ret;
  }
  buf[received] = '\0';

  ESP_LOGI(TAG, "Upload: received %d bytes", (int)received);

  // Find JSON start in multipart/form-data
  char *json_start = strstr(buf, "\r\n\r\n");
  if (!json_start) {
    json_start = strstr(buf, "\n\n");
    if (json_start) {
      json_start += 2;
    }
  } else {
    json_start += 4;
  }

  if (!json_start) {
    ESP_LOGE(TAG, "Invalid multipart format - no JSON start found");
    free(buf);
    return ESP_FAIL;
  }

  // Find JSON end
  char *json_end = strstr(json_start, "\r\n--");
  if (!json_end)
    json_end = strstr(json_start, "\n--");
  if (json_end) {
    *json_end = '\0';
  }

  // Reset schedule
  alarm_lock();
  for (int i = 0; i < 7; i++) {
    alarmsEnabled[0][i] = false;
    for (int j = 0; j < MAX_ALARMS_PER_DAY; j++) {
      schedule[0][i].startHour[j] = -1;
      schedule[0][i].endHour[j] = -1;
      schedule[0][i].endDayOffset[j] = 0;
      schedule[0][i].relayIdx[j] = 0;
    }
  }

  // Parse JSON EXACTLY as the original string-matching logic
  const char *daysEn[] = {"monday", "tuesday",  "wednesday", "thursday",
                          "friday", "saturday", "sunday"};

  for (int i = 0; i < 7; i++) {
    char dayKey[32];
    snprintf(dayKey, sizeof(dayKey), "\"%s\"", daysEn[i]);
    char *dayStart = strstr(json_start, dayKey);
    if (!dayStart)
      continue;

    char *arrayStart = strchr(dayStart, '[');
    if (!arrayStart)
      continue;

    char *arrayEnd = strchr(arrayStart, ']');
    if (!arrayEnd)
      continue;

    int alarmIdx = 0;
    char *p = arrayStart;

    while (p < arrayEnd && alarmIdx < MAX_ALARMS_PER_DAY) {
      char *objStart = strstr(p, "{");
      if (!objStart || objStart > arrayEnd)
        break;
      char *objEnd = strstr(objStart, "}");
      if (!objEnd || objEnd > arrayEnd)
        break;

      char *s_key = strstr(objStart, "\"start\"");
      char *e_key = strstr(objStart, "\"end\"");
      char *r_key = strstr(objStart, "\"relay\"");
      char *d_key = strstr(objStart, "\"endDay\"");

      if (s_key && e_key && s_key < objEnd && e_key < objEnd) {
        int sh = -1, sm = -1, ss = 0, eh = -1, em = -1, es = 0; // Default seconds to 0
        int rIdx = 0, endDay = 0;

        char *s_val = strchr(s_key, ':');
        char *e_val = strchr(e_key, ':');

        if (s_val && e_val) {
          s_val = strchr(s_val, '\"');
          e_val = strchr(e_val, '\"');
          if (s_val && e_val &&
              sscanf(s_val + 1, "%d:%d:%d", &sh, &sm, &ss) >= 2 &&
              sscanf(e_val + 1, "%d:%d:%d", &eh, &em, &es) >= 2) {

            if (r_key && r_key < objEnd) {
              char *r_val = strchr(r_key, ':');
              if (r_val)
                sscanf(r_val + 1, "%d", &rIdx);
            }
            if (d_key && d_key < objEnd) {
              char *d_val = strchr(d_key, ':');
              if (d_val)
                sscanf(d_val + 1, "%d", &endDay);
            }

            if (sh >= 0 && sh < 24 && sm >= 0 && sm < 60 && eh >= 0 &&
                eh < 24 && em >= 0 && em < 60) {
              schedule[0][i].startHour[alarmIdx] = sh;
              schedule[0][i].startMinute[alarmIdx] = sm;
              schedule[0][i].startSecond[alarmIdx] = ss;
              schedule[0][i].endHour[alarmIdx] = eh;
              schedule[0][i].endMinute[alarmIdx] = em;
              schedule[0][i].endSecond[alarmIdx] = es;
              schedule[0][i].endDayOffset[alarmIdx] = endDay;
              schedule[0][i].relayIdx[alarmIdx] =
                  (rIdx >= 0 && rIdx <= 3) ? rIdx : 0;
              alarmsEnabled[0][i] = true;
              alarmIdx++;
            }
          }
        }
      }
      p = objEnd + 1;
    }
  }
  alarm_unlock();

  free(buf);

  // Save to NVS
  alarm_manager_save();

  ESP_LOGI(TAG, "Schedule imported successfully from JSON");

  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location", "/");
  return httpd_resp_send(req, NULL, 0);
}

// ========== Save Handler (POST) ==========
// Helper to decode URL-encoded body safely
static void url_decode_n(char *dst, const char *src, size_t max_len) {
  char a, b;
  size_t count = 0;
  while (*src && count < max_len - 1) {
    if ((*src == '%') && (src[1] != '\0') && (src[2] != '\0') &&
        (isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2]))) {
      a = src[1];
      b = src[2];
      if (a >= 'a')
        a -= 'a' - 'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a' - 'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16 * a + b;
      src += 3;
    } else if (*src == '+') {
      *dst++ = ' ';
      src++;
    } else {
      *dst++ = *src++;
    }
    count++;
  }
  *dst = '\0';
}

static esp_err_t save_post_handler(httpd_req_t *req) {
  if (!is_authorized)
    return ESP_FAIL;

  size_t total_size = req->content_len;
  if (total_size >= 49152) { // Increased to 48KB for 60 alarms/day
    ESP_LOGE(TAG, "POST body too large: %d", total_size);
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                               "Body too large (Max 48KB)");
  }

  char *buf = malloc(total_size + 1);
  if (!buf) {
    ESP_LOGE(TAG, "Failed to allocate %d bytes", total_size + 1);
    return ESP_FAIL;
  }

  size_t received = 0;
  while (received < total_size) {
    int ret = httpd_req_recv(req, buf + received, total_size - received);
    if (ret <= 0) {
      if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        continue;
      free(buf);
      return ESP_FAIL;
    }
    received += ret;
  }
  buf[total_size] = '\0';
  ESP_LOGI(TAG, "Received %d bytes of POST data", received);

  // Reset current schedule first
  alarm_lock();
  for (int i = 0; i < 7; i++) {
    alarmsEnabled[0][i] =
        false; // Default to false, enabled by presence of checkbox
    for (int j = 0; j < MAX_ALARMS_PER_DAY; j++) {
      schedule[0][i].startHour[j] = -1;
      schedule[0][i].endHour[j] = -1;
    }
  }
  alarm_unlock();

  // Naive form parsing (strtok based)
  // Format: start0_0=12%3A00%3A00&end0_0=...
  char *pair = strtok(buf, "&");
  while (pair) {
    char *eq = strchr(pair, '=');
    if (eq) {
      *eq = '\0';
      char *key = pair;
      char *val = eq + 1;

      // Decoded value
      char decodedVal[32];
      url_decode_n(decodedVal, val, sizeof(decodedVal));

      alarm_lock();

      // Check keys
      // enabled_X
      if (strncmp(key, "enabled_", 8) == 0) {
        int day = atoi(key + 8);
        if (day >= 0 && day < 7)
          alarmsEnabled[0][day] = true;
      }
      // enddayX_Y
      else if (strncmp(key, "endday", 6) == 0) {
        int day, idx;
        char *underscore = strchr(key, '_');
        if (underscore) {
          day = atoi(key + 6);
          idx = atoi(underscore + 1);
          if (day >= 0 && day < 7 && idx >= 0 && idx < MAX_ALARMS_PER_DAY) {
            schedule[0][day].endDayOffset[idx] = atoi(decodedVal);
          }
        }
      }
      // relayX_Y (NEW)
      else if (strncmp(key, "relay", 5) == 0) {
        int day, idx;
        char *underscore = strchr(key, '_');
        if (underscore) {
          day = atoi(key + 5);
          idx = atoi(underscore + 1);
          if (day >= 0 && day < 7 && idx >= 0 && idx < MAX_ALARMS_PER_DAY) {
            schedule[0][day].relayIdx[idx] = atoi(decodedVal);
          }
        }
      }
      // startX_Y or endX_Y
      else if (strncmp(key, "start", 5) == 0 || strncmp(key, "end", 3) == 0) {
        int day, idx;
        bool isStart = (strncmp(key, "start", 5) == 0);
        char *underscore = strchr(key, '_');

        if (underscore && strlen(decodedVal) >= 5) { // At least HH:MM
          day = atoi(key + (isStart ? 5 : 3));
          idx = atoi(underscore + 1);

          if (day >= 0 && day < 7 && idx >= 0 && idx < MAX_ALARMS_PER_DAY) {
            // Parse HH:MM:SS
            int h, m, s = 0;
            sscanf(decodedVal, "%d:%d:%d", &h, &m, &s);

            if (isStart) {
              schedule[0][day].startHour[idx] = h;
              schedule[0][day].startMinute[idx] = m;
              schedule[0][day].startSecond[idx] = s;
            } else {
              schedule[0][day].endHour[idx] = h;
              schedule[0][day].endMinute[idx] = m;
              schedule[0][day].endSecond[idx] = s;
            }
          }
        }
      }
    }
    alarm_unlock();
    pair = strtok(NULL, "&");
  }
  free(buf);

  // SERVER-SIDE VALIDATION (v7.1.1): Reject invalid alarms
  // If endDay=0 (same day) and end <= start, mark as invalid
  int validCount = 0;
  int rejectedCount = 0;

  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < MAX_ALARMS_PER_DAY; j++) {
      if (schedule[0][i].startHour[j] != -1 &&
          schedule[0][i].endHour[j] != -1) {
        int startSec = schedule[0][i].startHour[j] * 3600 +
                       schedule[0][i].startMinute[j] * 60 +
                       schedule[0][i].startSecond[j];
        int endSec = schedule[0][i].endHour[j] * 3600 +
                     schedule[0][i].endMinute[j] * 60 +
                     schedule[0][i].endSecond[j];

        // Reject if same day AND end <= start
        if (schedule[0][i].endDayOffset[j] == 0 && endSec <= startSec) {
          ESP_LOGW(TAG,
                   "Rejected invalid alarm: Day %d, Alarm %d (end <= start on "
                   "same day)",
                   i, j);
          schedule[0][i].startHour[j] = -1;
          schedule[0][i].endHour[j] = -1;
          schedule[0][i].endDayOffset[j] = 0;
          rejectedCount++;
        } else {
          validCount++;
        }
      }
    }
  }

  ESP_LOGI(TAG, "Form processed: %d valid alarms, %d rejected", validCount,
           rejectedCount);

  // Save to NVS
  alarm_manager_save();

  // If some alarms were rejected, show warning page
  if (rejectedCount > 0) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");

    char msgBuf[1200];
    snprintf(
        msgBuf, sizeof(msgBuf),
        "<html><head><meta charset='utf-8'>"
        "<meta http-equiv='refresh' content='5;url=/settings'>"
        "<style>"
        "body{font-family:Arial;margin:0;padding:50px;text-align:center;"
        "background:linear-gradient(135deg,#667eea 0%%,#764ba2 100%%)}"
        ".card{background:white;padding:40px;border-radius:15px;"
        "box-shadow:0 10px 30px rgba(0,0,0,0.3);max-width:500px;margin:0 auto}"
        "h1{color:#e74c3c;margin-bottom:20px}"
        "p{color:#34495e;font-size:1.1rem;line-height:1.6}"
        ".count{font-size:2rem;font-weight:bold;color:#e74c3c}"
        "a{color:#3498db;text-decoration:none;font-weight:bold}"
        "</style></head>"
        "<body><div class='card'>"
        "<h1>⚠️ تحذير</h1>"
        "<p>تم حفظ <span class='count'>%d</span> منبه بنجاح</p>"
        "<p>تم رفض <span class='count'>%d</span> منبه غير صالح</p>"
        "<p style='font-size:0.9rem;color:#7f8c8d'>"
        "السبب: وقت النهاية يجب أن يكون بعد وقت البداية<br>"
        "(إذا كان في نفس اليوم)</p>"
        "<p style='margin-top:30px'>سيتم توجيهك إلى الإعدادات خلال 5 "
        "ثوانٍ...</p>"
        "<p><a href='/settings'>العودة الآن →</a></p>"
        "</div></body></html>",
        validCount, rejectedCount);

    return httpd_resp_sendstr(req, msgBuf);
  }

  // All alarms valid - redirect to dashboard
  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location", "/");
  return httpd_resp_send(req, NULL, 0);
}

// ========== Logout Handler ==========
static esp_err_t logout_handler(httpd_req_t *req) {
  is_authorized = false;
  ESP_LOGI(TAG, "User logged out");

  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location", "/");
  return httpd_resp_send(req, NULL, 0);
}

// ========== Status API (JSON) ==========
static esp_err_t status_api_handler(httpd_req_t *req) {
  // Current sensor data (Non-blocking)
  float current_amps = adc_get_last_current();

  // Get all relay states
  int r1_state = relay_get_state(0);
  int r2_state = relay_get_state(1);
  int r3_state = relay_get_state(2);
  int r4_state = relay_get_state(3);

  // Time data
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char time_str[32];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);

  // Next alarm
  char next_alarm[32];
  alarm_get_next_time_str_with_relay(0, -1, next_alarm, sizeof(next_alarm));
  if (strlen(next_alarm) == 0)
    strcpy(next_alarm, "None");

  // Alarm active status
  bool alarm_active = alarm_is_active(0, 0) || alarm_is_active(0, 1) ||
                      alarm_is_active(0, 2) || alarm_is_active(0, 3);

  char json[512]; // Increased size for more relays
  snprintf(json, sizeof(json),
           "{\"relay\":%d,\"relay2\":%d,\"relay3\":%d,\"relay4\":%d,"
           "\"current\":%.3f,\"time\":\"%s\",\"next_alarm\":\"%s\","
           "\"alarm_active\":%s,\"anomaly_code\":%d,\"heap\":%lu,"
           "\"ota_mode\":%s}",
           r1_state, r2_state, r3_state, r4_state, current_amps, time_str,
           next_alarm, alarm_active ? "true" : "false", g_system_anomaly_code,
           (unsigned long)esp_get_free_heap_size(),
           g_is_ota_active ? "true" : "false");

  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, json, strlen(json));
}

// ========== Time API ==========
static esp_err_t time_api_handler(httpd_req_t *req) {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);

  httpd_resp_set_type(req, "text/plain; charset=UTF-8");
  return httpd_resp_send(req, time_str, strlen(time_str));
}

// ========== About Handler ==========
static esp_err_t about_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  app_lang_t lang = i18n_get_req_lang(req);
  httpd_resp_set_type(req, "text/html; charset=UTF-8");

  char html_open[128];
  snprintf(html_open, sizeof(html_open), "<!DOCTYPE html><html lang='%s' dir='%s'><head>",
           i18n_code(lang), i18n_dir(lang));
  httpd_resp_sendstr_chunk(req, html_open);
  httpd_resp_sendstr_chunk(req,
                           "<meta charset='UTF-8'><meta name='viewport' "
                           "content='width=device-width,initial-scale=1.0'>");
  char title_buf[256];
  snprintf(title_buf, sizeof(title_buf), "<title>%s</title>", i18n_tr(STR_NAV_ABOUT, lang));
  httpd_resp_sendstr_chunk(req, title_buf);
  httpd_resp_sendstr_chunk(req, HTML_STYLE);
  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");
  i18n_send_lang_bar(req, lang);

  char h1_buf[256];
  snprintf(h1_buf, sizeof(h1_buf), "<h1>%s</h1>", i18n_tr(STR_ABOUT_TITLE, lang));
  httpd_resp_sendstr_chunk(req, h1_buf);

  // About Grid
  httpd_resp_sendstr_chunk(req, "<div class='dash-grid'>");

  // Device Card
  char card_buf[512];
  snprintf(card_buf, sizeof(card_buf),
           "<div class='card'><span class='card-icon'>🏫</span><span "
           "class='card-title'>%s</span><span class='card-value'>Aep</span></div>",
           i18n_tr(STR_ABOUT_SYSTEM, lang));
  httpd_resp_sendstr_chunk(req, card_buf);

  // Version Card
  snprintf(card_buf, sizeof(card_buf),
           "<div class='card'><span class='card-icon'>🏷️</span><span "
           "class='card-title'>%s</span><span class='card-value' style='color:#667eea'>v12.0</span></div>",
           i18n_tr(STR_ABOUT_VERSION, lang));
  httpd_resp_sendstr_chunk(req, card_buf);

  // Developer Card
  snprintf(card_buf, sizeof(card_buf),
           "<div class='card'><span class='card-icon'>👨‍🏫</span><span "
           "class='card-title'>%s</span><span class='card-value' style='font-size:1rem'>%s</span></div>",
           i18n_tr(STR_ABOUT_DEV, lang),
           i18n_tr(STR_ABOUT_DEV_NAME, lang));
  httpd_resp_sendstr_chunk(req, card_buf);

  // Location Card
  snprintf(card_buf, sizeof(card_buf),
           "<div class='card'><span class='card-icon'>📍</span><span "
           "class='card-title'>%s</span><span class='card-value' style='font-size:0.9rem'>%s</span></div>",
           i18n_tr(STR_ABOUT_INSTITUTION, lang),
           i18n_tr(STR_ABOUT_INSTITUTION_NAME, lang));
  httpd_resp_sendstr_chunk(req, card_buf);

  // Copyright Card
  snprintf(card_buf, sizeof(card_buf),
           "<div class='card'><span class='card-icon'>©</span><span "
           "class='card-title'>%s</span><span class='card-value' style='font-size:0.9rem'>%s</span></div>",
           i18n_tr(STR_ABOUT_RIGHTS, lang),
           i18n_tr(STR_ABOUT_RIGHTS_TEXT, lang));
  httpd_resp_sendstr_chunk(req, card_buf);

  httpd_resp_sendstr_chunk(req, "</div>");
  char back_buf[256];
  snprintf(back_buf, sizeof(back_buf),
           "<div class='nav-menu'><a href='/'>%s</a></div>",
           i18n_tr(STR_BACK_TO_DASHBOARD, lang));
  httpd_resp_sendstr_chunk(req, back_buf);
  httpd_resp_sendstr_chunk(req, "</div></body></html>");

  return httpd_resp_sendstr_chunk(req, NULL);
}

// ========== Restart Handler ==========
static esp_err_t restart_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  app_lang_t lang = i18n_get_req_lang(req);
  char query[32];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
    if (strstr(query, "confirm=yes")) {
      httpd_resp_set_type(req, "text/html; charset=UTF-8");
      send_conf_header(req, i18n_tr(STR_NAV_RESTART, lang), "/", 12);
      httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#128267;</div>");
      char h1_buf[256];
      snprintf(h1_buf, sizeof(h1_buf), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_REBOOTING, lang));
      httpd_resp_sendstr_chunk(req, h1_buf);
      char p_buf[256];
      snprintf(p_buf, sizeof(p_buf), "<p class='conf-msg'>%s</p>", i18n_tr(STR_PLEASE_WAIT, lang));
      httpd_resp_sendstr_chunk(req, p_buf);
      httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
      httpd_resp_sendstr_chunk(req, NULL);
      vTaskDelay(pdMS_TO_TICKS(1500));
      esp_restart();
      return ESP_OK;
    }
  }

  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  char html_open[128];
  snprintf(html_open, sizeof(html_open), "<!DOCTYPE html><html lang='%s' dir='%s'><head>",
           i18n_code(lang), i18n_dir(lang));
  httpd_resp_sendstr_chunk(req, html_open);
  httpd_resp_sendstr_chunk(req, "<meta charset='UTF-8'>");
  httpd_resp_sendstr_chunk(req, HTML_STYLE);
  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");

  // Card centered
  httpd_resp_sendstr_chunk(req,
                           "<div class='card' style='max-width:400px;margin:0 "
                           "auto;border-color:#f39c12'>");
  httpd_resp_sendstr_chunk(
      req,
      "<span class='card-icon' style='color:#f39c12;font-size:4rem'>🔄</span>");
  char title_h2[256];
  snprintf(title_h2, sizeof(title_h2), "<h2>%s</h2>", i18n_tr(STR_NAV_RESTART, lang));
  httpd_resp_sendstr_chunk(req, title_h2);
  char confirm_p[256];
  snprintf(confirm_p, sizeof(confirm_p),
           "<p style='color:#7f8c8d;margin-bottom:20px'>%s</p>",
           i18n_tr(STR_REBOOT_CONFIRM, lang));
  httpd_resp_sendstr_chunk(req, confirm_p);

  httpd_resp_sendstr_chunk(
      req, "<div style='display:grid;grid-template-columns:1fr 1fr;gap:10px'>");
  char btns[512];
  snprintf(btns, sizeof(btns),
           "<a href='/restart?confirm=yes' class='tablink' "
           "style='background:#f39c12;text-align:center;text-decoration:none;display:block'>%s</a>"
           "<a href='/' class='tablink' "
           "style='background:#95a5a6;text-align:center;text-decoration:none;display:block'>%s</a>",
           i18n_tr(STR_YES, lang), i18n_tr(STR_CANCEL, lang));
  httpd_resp_sendstr_chunk(req, btns);
  httpd_resp_sendstr_chunk(req, "</div></div></div></body></html>");

  return httpd_resp_sendstr_chunk(req, NULL);
}

// ========== Factory Reset Handler ==========
static esp_err_t factory_reset_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  app_lang_t lang = i18n_get_req_lang(req);
  char query[32];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
    if (strstr(query, "confirm=yes")) {
      httpd_resp_set_type(req, "text/html; charset=UTF-8");
      send_conf_header(req, i18n_tr(STR_NAV_FACTORY_RESET, lang), "/", 0);
      httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#9888;</div>");
      char h1_buf[256];
      snprintf(h1_buf, sizeof(h1_buf), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_FACTORY_RESET_TITLE, lang));
      httpd_resp_sendstr_chunk(req, h1_buf);
      char p_buf[256];
      snprintf(p_buf, sizeof(p_buf), "<p class='conf-msg'>%s</p>", i18n_tr(STR_PLEASE_WAIT, lang));
      httpd_resp_sendstr_chunk(req, p_buf);
      httpd_resp_sendstr_chunk(req, "<div class='spinner'></div>");
      httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
      httpd_resp_sendstr_chunk(req, NULL);
      alarm_manager_factory_reset();
      ESP_LOGI(TAG, "Factory reset triggered by user");
      vTaskDelay(pdMS_TO_TICKS(2000));
      esp_restart();
      return ESP_OK;
    }
  }

  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  char html_open[128];
  snprintf(html_open, sizeof(html_open), "<!DOCTYPE html><html lang='%s' dir='%s'><head>",
           i18n_code(lang), i18n_dir(lang));
  httpd_resp_sendstr_chunk(req, html_open);
  httpd_resp_sendstr_chunk(req, "<meta charset='UTF-8'>");
  httpd_resp_sendstr_chunk(req, HTML_STYLE);
  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");
  // Card centered red
  httpd_resp_sendstr_chunk(req,
                           "<div class='card' style='max-width:400px;margin:0 "
                           "auto;border-color:#e74c3c'>");
  httpd_resp_sendstr_chunk(
      req,
      "<span class='card-icon' style='color:#e74c3c;font-size:4rem'>⚠️</span>");
  char f_h2[256];
  snprintf(f_h2, sizeof(f_h2), "<h2>%s</h2>", i18n_tr(STR_FACTORY_RESET_TITLE, lang));
  httpd_resp_sendstr_chunk(req, f_h2);

  char warn_box[1024];
  snprintf(warn_box, sizeof(warn_box),
      "<div style='background:#fff3cd;padding:15px;border-radius:10px;margin:20px 0;color:#856404;text-align:%s'>"
      "%s</div>",
      (lang == LANG_AR) ? "right" : "left",
      i18n_tr(STR_FACTORY_RESET_WARN, lang));
  httpd_resp_sendstr_chunk(req, warn_box);

  httpd_resp_sendstr_chunk(
      req, "<div style='display:grid;grid-template-columns:1fr 1fr;gap:10px'>");
  char f_btns[512];
  snprintf(f_btns, sizeof(f_btns),
      "<a href='/factoryReset?confirm=yes' class='tablink' "
      "style='background:#e74c3c;text-align:center;text-decoration:none;display:block'>%s</a>"
      "<a href='/' class='tablink' "
      "style='background:#95a5a6;text-align:center;text-decoration:none;display:block'>%s</a>",
      i18n_tr(STR_FACTORY_RESET_CONFIRM_BTN, lang),
      i18n_tr(STR_CANCEL, lang));
  httpd_resp_sendstr_chunk(req, f_btns);
  httpd_resp_sendstr_chunk(req, "</div></div></div></body></html>");

  return httpd_resp_sendstr_chunk(req, NULL);
}

// ========== Change Password Handlers ==========
static esp_err_t change_password_get_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  app_lang_t lang = i18n_get_req_lang(req);
  httpd_resp_set_type(req, "text/html; charset=UTF-8");

  char html_open[128];
  snprintf(html_open, sizeof(html_open), "<!DOCTYPE html><html lang='%s' dir='%s'><head>",
           i18n_code(lang), i18n_dir(lang));
  httpd_resp_sendstr_chunk(req, html_open);
  httpd_resp_sendstr_chunk(req, "<meta charset='UTF-8'>");
  char title_buf[256];
  snprintf(title_buf, sizeof(title_buf), "<title>%s</title>", i18n_tr(STR_PWD_TITLE, lang));
  httpd_resp_sendstr_chunk(req, title_buf);
  httpd_resp_sendstr_chunk(req, HTML_STYLE);
  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");
  i18n_send_lang_bar(req, lang);

  // Card Start
  httpd_resp_sendstr_chunk(
      req, "<div class='card' style='max-width:500px;margin:0 auto'>");
  httpd_resp_sendstr_chunk(req, "<span class='card-icon'>🔑</span>");
  char h2_buf[256];
  snprintf(h2_buf, sizeof(h2_buf), "<h2>%s</h2>", i18n_tr(STR_PWD_TITLE, lang));
  httpd_resp_sendstr_chunk(req, h2_buf);

  char form_buf[1024];
  snprintf(form_buf, sizeof(form_buf),
      "<form action='/changePassword' method='POST' style='text-align:%s'>"
      "<label>%s</label>"
      "<input type='password' name='oldpass' required><br>"
      "<label>%s</label>"
      "<input type='password' name='newpass' required><br>"
      "<label>%s</label>"
      "<input type='password' name='confirmpass' required><br>"
      "<input type='submit' value='%s'>"
      "</form>",
      (lang == LANG_AR) ? "right" : "left",
      i18n_tr(STR_OLD_PWD, lang),
      i18n_tr(STR_NEW_PWD, lang),
      i18n_tr(STR_CONFIRM_PWD, lang),
      i18n_tr(STR_SAVE, lang));
  httpd_resp_sendstr_chunk(req, form_buf);

  char back_buf[256];
  snprintf(back_buf, sizeof(back_buf),
           "<div class='nav-menu' style='margin-top:20px'><a href='/'>%s</a></div>",
           i18n_tr(STR_BACK_TO_DASHBOARD, lang));
  httpd_resp_sendstr_chunk(req, back_buf);
  httpd_resp_sendstr_chunk(req, "</div></div></body></html>");

  return httpd_resp_sendstr_chunk(req, NULL);
}

static esp_err_t change_password_post_handler(httpd_req_t *req) {
  if (!is_authorized)
    return ESP_FAIL;

  app_lang_t lang = i18n_get_req_lang(req);
  char buf[200];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0)
    return ESP_FAIL;
  buf[ret] = '\0';

  char oldpass[50] = "", newpass[50] = "", confirmpass[50] = "";

  // Parse form data
  char *token = strtok(buf, "&");
  while (token) {
    if (strncmp(token, "oldpass=", 8) == 0) {
      url_decode_n(oldpass, token + 8, sizeof(oldpass));
    } else if (strncmp(token, "newpass=", 8) == 0) {
      url_decode_n(newpass, token + 8, sizeof(newpass));
    } else if (strncmp(token, "confirmpass=", 12) == 0) {
      url_decode_n(confirmpass, token + 12, sizeof(confirmpass));
      char *newline = strpbrk(confirmpass, "\r\n");
      if (newline) *newline = '\0';
    }
    char *newline1 = strpbrk(oldpass, "\r\n"); if (newline1) *newline1 = '\0';
    char *newline2 = strpbrk(newpass, "\r\n"); if (newline2) *newline2 = '\0';
    token = strtok(NULL, "&");
  }

  char stored_password[64] = {0};
  if (nvs_read_string("password", stored_password, sizeof(stored_password)) != ESP_OK ||
      strlen(stored_password) == 0) {
    strncpy(stored_password, DEFAULT_PASSWORD, sizeof(stored_password) - 1);
  }

  if (strcmp(oldpass, stored_password) != 0) {
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    send_conf_header(req, i18n_tr(STR_ERROR, lang), "/changePassword", 5);
    httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#9888;</div>");
    char h1_buf[256];
    snprintf(h1_buf, sizeof(h1_buf), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_ERROR, lang));
    httpd_resp_sendstr_chunk(req, h1_buf);
    char p_buf[256];
    snprintf(p_buf, sizeof(p_buf), "<p class='conf-msg'>%s</p>", i18n_tr(STR_PWD_WRONG_OLD, lang));
    httpd_resp_sendstr_chunk(req, p_buf);
    char btn_buf[256];
    snprintf(btn_buf, sizeof(btn_buf), "<div class='conf-btns'><a href='/changePassword'>%s</a></div>", i18n_tr(STR_RETRY, lang));
    httpd_resp_sendstr_chunk(req, btn_buf);
    httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
    return httpd_resp_sendstr_chunk(req, NULL);
  }

  if (strcmp(newpass, confirmpass) != 0) {
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    send_conf_header(req, i18n_tr(STR_ERROR, lang), "/changePassword", 5);
    httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#9888;</div>");
    char h1_buf[256];
    snprintf(h1_buf, sizeof(h1_buf), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_ERROR, lang));
    httpd_resp_sendstr_chunk(req, h1_buf);
    char p_buf[256];
    snprintf(p_buf, sizeof(p_buf), "<p class='conf-msg'>%s</p>", i18n_tr(STR_PWD_MISMATCH, lang));
    httpd_resp_sendstr_chunk(req, p_buf);
    char btn_buf[256];
    snprintf(btn_buf, sizeof(btn_buf), "<div class='conf-btns'><a href='/changePassword'>%s</a></div>", i18n_tr(STR_RETRY, lang));
    httpd_resp_sendstr_chunk(req, btn_buf);
    httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
    return httpd_resp_sendstr_chunk(req, NULL);
  }

  esp_err_t err = nvs_write_string("password", newpass);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to save password to NVS: %s", esp_err_to_name(err));
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    send_conf_header(req, i18n_tr(STR_ERROR, lang), "/changePassword", 5);
    httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#10060;</div>");
    char h1_buf[256];
    snprintf(h1_buf, sizeof(h1_buf), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_ERROR, lang));
    httpd_resp_sendstr_chunk(req, h1_buf);
    char btn_buf[256];
    snprintf(btn_buf, sizeof(btn_buf), "<div class='conf-btns'><a href='/changePassword'>%s</a></div>", i18n_tr(STR_RETRY, lang));
    httpd_resp_sendstr_chunk(req, btn_buf);
    httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
    return httpd_resp_sendstr_chunk(req, NULL);
  }

  ESP_LOGI(TAG, "Password changed and saved to NVS successfully");

  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  send_conf_header(req, i18n_tr(STR_SUCCESS, lang), "/", 3);
  httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#9989;</div>");
  char ok_h1[256];
  snprintf(ok_h1, sizeof(ok_h1), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_SUCCESS, lang));
  httpd_resp_sendstr_chunk(req, ok_h1);
  char ok_p[256];
  snprintf(ok_p, sizeof(ok_p), "<p class='conf-msg'>%s</p>", i18n_tr(STR_PWD_CHANGED_OK, lang));
  httpd_resp_sendstr_chunk(req, ok_p);
  httpd_resp_sendstr_chunk(req, "<div class='spinner'></div>");
  char ret_p[256];
  snprintf(ret_p, sizeof(ret_p), "<p style='color:#999;font-size:0.95rem'>%s</p>", i18n_tr(STR_RETURNING, lang));
  httpd_resp_sendstr_chunk(req, ret_p);
  httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
  return httpd_resp_sendstr_chunk(req, NULL);
}

// ========== WiFi Config Handlers ==========
static esp_err_t wifi_config_get_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  app_lang_t lang = i18n_get_req_lang(req);
  httpd_resp_set_type(req, "text/html; charset=UTF-8");

  char html_open[128];
  snprintf(html_open, sizeof(html_open), "<!DOCTYPE html><html lang='%s' dir='%s'><head>",
           i18n_code(lang), i18n_dir(lang));
  httpd_resp_sendstr_chunk(req, html_open);
  httpd_resp_sendstr_chunk(req,
                           "<meta charset='UTF-8'><meta name='viewport' "
                           "content='width=device-width,initial-scale=1.0'>");
  char title_buf[256];
  snprintf(title_buf, sizeof(title_buf), "<title>%s</title>", i18n_tr(STR_WIFI_TITLE, lang));
  httpd_resp_sendstr_chunk(req, title_buf);
  httpd_resp_sendstr_chunk(req, HTML_STYLE);
  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");
  i18n_send_lang_bar(req, lang);

  // Card Start
  httpd_resp_sendstr_chunk(
      req, "<div class='card' style='max-width:500px;margin:0 auto'>");
  httpd_resp_sendstr_chunk(
      req, "<span class='card-icon' style='color:#3498db'>📶</span>");
  char h2_buf[256];
  snprintf(h2_buf, sizeof(h2_buf), "<h2>%s</h2>", i18n_tr(STR_WIFI_TITLE, lang));
  httpd_resp_sendstr_chunk(req, h2_buf);

  char current_ssid[33] = {0};
  nvs_read_string("wifi_ssid", current_ssid, sizeof(current_ssid));
  if (strlen(current_ssid) == 0) strcpy(current_ssid, "--");

  char buf[512];
  snprintf(
      buf, sizeof(buf),
      "<div style='background:#f0f8ff;padding:10px;border-radius:8px;margin-bottom:15px;text-align:center'>"
      "<strong>%s</strong> %s</div>",
      i18n_tr(STR_CURRENT_SSID, lang), current_ssid);
  httpd_resp_sendstr_chunk(req, buf);

  char form_buf[1024];
  snprintf(form_buf, sizeof(form_buf),
      "<form action='/wifiConfig' method='POST' style='text-align:%s'>"
      "<label>%s</label>"
      "<input type='text' name='ssid' placeholder='SSID' required><br>"
      "<label>%s</label>"
      "<input type='password' name='password' required><br>"
      "<input type='submit' value='%s'>"
      "</form>",
      (lang == LANG_AR) ? "right" : "left",
      i18n_tr(STR_NETWORK_NAME, lang),
      i18n_tr(STR_PASSWORD, lang),
      i18n_tr(STR_SAVE, lang));
  httpd_resp_sendstr_chunk(req, form_buf);

  char warn_p[512];
  snprintf(warn_p, sizeof(warn_p),
      "<p style='color:#856404;font-size:0.9rem;text-align:center;margin-top:10px'>%s</p>",
      i18n_tr(STR_WIFI_WARN_RESTART, lang));
  httpd_resp_sendstr_chunk(req, warn_p);

  char back_buf[256];
  snprintf(back_buf, sizeof(back_buf),
      "<div class='nav-menu' style='margin-top:20px'><a href='/'>%s</a></div>",
      i18n_tr(STR_BACK_TO_DASHBOARD, lang));
  httpd_resp_sendstr_chunk(req, back_buf);
  httpd_resp_sendstr_chunk(req, "</div></div></body></html>");

  return httpd_resp_sendstr_chunk(req, NULL);
}

static esp_err_t wifi_config_post_handler(httpd_req_t *req) {
  if (!is_authorized)
    return ESP_FAIL;

  app_lang_t lang = i18n_get_req_lang(req);
  char buf[300];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0)
    return ESP_FAIL;
  buf[ret] = '\0';

  char ssid[50] = "", password[70] = "";

  // Parse form data
  char *token = strtok(buf, "&");
  while (token) {
    if (strncmp(token, "ssid=", 5) == 0) {
      url_decode_n(ssid, token + 5, sizeof(ssid));
    } else if (strncmp(token, "password=", 9) == 0) {
      url_decode_n(password, token + 9, sizeof(password));
    }
    token = strtok(NULL, "&");
  }

  // Validate
  if (strlen(ssid) == 0 || strlen(password) == 0) {
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    send_conf_header(req, i18n_tr(STR_ERROR, lang), "/wifiConfig", 5);
    httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#9888;</div>");
    char h1_buf[256];
    snprintf(h1_buf, sizeof(h1_buf), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_ERROR, lang));
    httpd_resp_sendstr_chunk(req, h1_buf);
    char p_buf[256];
    snprintf(p_buf, sizeof(p_buf), "<p class='conf-msg'>%s</p>", i18n_tr(STR_WIFI_INVALID_INPUT, lang));
    httpd_resp_sendstr_chunk(req, p_buf);
    char btn_buf[256];
    snprintf(btn_buf, sizeof(btn_buf), "<div class='conf-btns'><a href='/wifiConfig'>%s</a></div>", i18n_tr(STR_RETRY, lang));
    httpd_resp_sendstr_chunk(req, btn_buf);
    httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
    return httpd_resp_sendstr_chunk(req, NULL);
  }

  // Save to NVS
  esp_err_t err = nvs_write_string("wifi_ssid", ssid);
  if (err == ESP_OK) {
    err = nvs_write_string("wifi_pass", password);
  }

  if (err != ESP_OK) {
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    send_conf_header(req, i18n_tr(STR_ERROR, lang), "/wifiConfig", 5);
    httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#10060;</div>");
    char h1_buf[256];
    snprintf(h1_buf, sizeof(h1_buf), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_ERROR, lang));
    httpd_resp_sendstr_chunk(req, h1_buf);
    char p_buf[256];
    snprintf(p_buf, sizeof(p_buf), "<p class='conf-msg'>%s</p>", i18n_tr(STR_WIFI_SAVE_FAILED, lang));
    httpd_resp_sendstr_chunk(req, p_buf);
    char btn_buf[256];
    snprintf(btn_buf, sizeof(btn_buf), "<div class='conf-btns'><a href='/wifiConfig'>%s</a></div>", i18n_tr(STR_RETRY, lang));
    httpd_resp_sendstr_chunk(req, btn_buf);
    httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
    return httpd_resp_sendstr_chunk(req, NULL);
  }

  ESP_LOGI(TAG, "WiFi credentials saved to NVS (SSID: %s)", ssid);

  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  send_conf_header(req, i18n_tr(STR_SUCCESS, lang), "/", 5);
  httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#9989;</div>");
  char ok_h1[256];
  snprintf(ok_h1, sizeof(ok_h1), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_WIFI_SAVED, lang));
  httpd_resp_sendstr_chunk(req, ok_h1);
  char ok_p[256];
  snprintf(ok_p, sizeof(ok_p), "<p class='conf-msg'>%s</p>", i18n_tr(STR_WIFI_RESTART_PROMPT, lang));
  httpd_resp_sendstr_chunk(req, ok_p);
  char btns_buf[512];
  snprintf(btns_buf, sizeof(btns_buf),
      "<div class='conf-btns'><a href='/restart'>%s</a><a href='/'>%s</a></div>",
      i18n_tr(STR_RESTART_NOW, lang),
      i18n_tr(STR_LATER, lang));
  httpd_resp_sendstr_chunk(req, btns_buf);
  httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
  return httpd_resp_sendstr_chunk(req, NULL);
}

// ========== Timezone Config Handlers ==========
static esp_err_t timezone_config_get_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  app_lang_t lang = i18n_get_req_lang(req);

  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  char html_head[256];
  snprintf(html_head, sizeof(html_head),
           "<!DOCTYPE html><html lang='%s' dir='%s'><head>"
           "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>"
           "<title>%s</title>",
           i18n_code(lang), i18n_dir(lang), i18n_tr(STR_TZ_TITLE, lang));
  httpd_resp_sendstr_chunk(req, html_head);
  httpd_resp_sendstr_chunk(req, HTML_STYLE);
  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");
  i18n_send_lang_bar(req, lang);

  // Card Start
  httpd_resp_sendstr_chunk(
      req, "<div class='card' style='max-width:500px;margin:0 auto'>");
  httpd_resp_sendstr_chunk(req, "<span class='card-icon'>🌍</span>");
  char h2_buf[256];
  snprintf(h2_buf, sizeof(h2_buf), "<h2>%s</h2>", i18n_tr(STR_TZ_TITLE, lang));
  httpd_resp_sendstr_chunk(req, h2_buf);

  httpd_resp_sendstr_chunk(
      req,
      "<form action='/timezoneConfig' method='POST'>");
  char lbl_buf[256];
  snprintf(lbl_buf, sizeof(lbl_buf), "<label>%s</label>", i18n_tr(STR_SELECT_TZ, lang));
  httpd_resp_sendstr_chunk(req, lbl_buf);

  char current_tz[64] = {0};
  nvs_read_string("timezone", current_tz, sizeof(current_tz));
  if (strlen(current_tz) == 0 || strstr(current_tz, "WET-0WEST") != NULL) {
    strcpy(current_tz, "GMT0");
  }

  httpd_resp_sendstr_chunk(req, "<select name='timezone' style='padding:10px;width:100%;font-size:1rem;border-radius:8px;border:1px solid #ccc;margin:10px 0;'>");
  
  char buf[700];
  if (lang == LANG_FR) {
    snprintf(buf, sizeof(buf), 
             "<option value='GMT0' %s>🇲🇦 Maroc / Heure légale GMT (GMT+0)</option>"
             "<option value='CET-1' %s>🇲🇦 Maroc / Heure d'été (GMT+1)</option>"
             "<option value='GMT0' %s>🌍 GMT Universel (GMT+0)</option>"
             "<option value='CET-1' %s>🌍 Europe Centrale (GMT+1)</option>"
             "<option value='EET-2' %s>🌍 Europe de l'Est / Le Caire (GMT+2)</option>"
             "<option value='AST-3' %s>🕋 La Mecque / Arabie Saoudite (GMT+3)</option>",
             (strcmp(current_tz, "GMT0") == 0) ? "selected" : "",
             (strcmp(current_tz, "CET-1") == 0) ? "selected" : "",
             "",
             "",
             (strcmp(current_tz, "EET-2") == 0) ? "selected" : "",
             (strcmp(current_tz, "AST-3") == 0) ? "selected" : "");
  } else if (lang == LANG_EN) {
    snprintf(buf, sizeof(buf), 
             "<option value='GMT0' %s>🇲🇦 Morocco / Official Standard Time (GMT+0)</option>"
             "<option value='CET-1' %s>🇲🇦 Morocco / Daylight Saving Time (GMT+1)</option>"
             "<option value='GMT0' %s>🌍 Universal GMT (GMT+0)</option>"
             "<option value='CET-1' %s>🌍 Central Europe (GMT+1)</option>"
             "<option value='EET-2' %s>🌍 Eastern Europe / Cairo (GMT+2)</option>"
             "<option value='AST-3' %s>🕋 Makkah / Saudi Arabia (GMT+3)</option>",
             (strcmp(current_tz, "GMT0") == 0) ? "selected" : "",
             (strcmp(current_tz, "CET-1") == 0) ? "selected" : "",
             "",
             "",
             (strcmp(current_tz, "EET-2") == 0) ? "selected" : "",
             (strcmp(current_tz, "AST-3") == 0) ? "selected" : "");
  } else {
    snprintf(buf, sizeof(buf), 
             "<option value='GMT0' %s>🇲🇦 المغرب / توقيت غرينتش الرسمي (GMT+0)</option>"
             "<option value='CET-1' %s>🇲🇦 المغرب / التوقيت الصيفي (GMT+1)</option>"
             "<option value='GMT0' %s>🌍 توقيت غرينتش العالمي (GMT+0)</option>"
             "<option value='CET-1' %s>🌍 وسط أوروبا (GMT+1)</option>"
             "<option value='EET-2' %s>🌍 شرق أوروبا / القاهرة (GMT+2)</option>"
             "<option value='AST-3' %s>🕋 مكة المكرمة / السعودية (GMT+3)</option>",
             (strcmp(current_tz, "GMT0") == 0) ? "selected" : "",
             (strcmp(current_tz, "CET-1") == 0) ? "selected" : "",
             "",
             "",
             (strcmp(current_tz, "EET-2") == 0) ? "selected" : "",
             (strcmp(current_tz, "AST-3") == 0) ? "selected" : "");
  }
  httpd_resp_sendstr_chunk(req, buf);
  httpd_resp_sendstr_chunk(req, "</select><br>");

  char submit_btn[128];
  snprintf(submit_btn, sizeof(submit_btn), "<input type='submit' value='%s'>", i18n_tr(STR_SAVE, lang));
  httpd_resp_sendstr_chunk(req, submit_btn);
  httpd_resp_sendstr_chunk(req, "</form>");

  char nav_back[256];
  snprintf(nav_back, sizeof(nav_back),
           "<div class='nav-menu' style='margin-top:20px'><a href='/'>%s</a></div>",
           i18n_tr(STR_BACK_TO_DASHBOARD, lang));
  httpd_resp_sendstr_chunk(req, nav_back);
  httpd_resp_sendstr_chunk(req, "</div></div></body></html>");

  return httpd_resp_sendstr_chunk(req, NULL);
}

static esp_err_t timezone_config_post_handler(httpd_req_t *req) {
  if (!is_authorized)
    return ESP_FAIL;

  app_lang_t lang = i18n_get_req_lang(req);

  // Phase 1.B: Parse, save and apply timezone
  char buf[200];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0)
    return ESP_FAIL;
  buf[ret] = '\0';

  char timezone[80] = "";

  // Parse form data
  char *token = strtok(buf, "&");
  while (token) {
    if (strncmp(token, "timezone=", 9) == 0) {
      url_decode_n(timezone, token + 9, sizeof(timezone));
      // Nettoyage des caractères de fin de ligne invisibles (\r ou \n)
      char *newline = strpbrk(timezone, "\r\n");
      if (newline) {
          *newline = '\0';
      }
      break;
    }
    token = strtok(NULL, "&");
  }

  // Validate
  if (strlen(timezone) == 0) {
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    send_conf_header(req, i18n_tr(STR_ERROR, lang), "/timezoneConfig", 5);
    httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#10060;</div>");
    char h1_err[256];
    snprintf(h1_err, sizeof(h1_err), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_ERROR, lang));
    httpd_resp_sendstr_chunk(req, h1_err);
    char p_err[256];
    snprintf(p_err, sizeof(p_err), "<p class='conf-msg'>%s</p>",
             (lang == LANG_FR) ? "Le fuseau horaire est requis" :
             (lang == LANG_EN) ? "Time zone is required" :
             "المنطقة الزمنية مطلوبة");
    httpd_resp_sendstr_chunk(req, p_err);
    char btn_err[256];
    snprintf(btn_err, sizeof(btn_err), "<div class='conf-btns'><a href='/timezoneConfig'>%s</a></div>", i18n_tr(STR_RETRY, lang));
    httpd_resp_sendstr_chunk(req, btn_err);
    httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
    return httpd_resp_sendstr_chunk(req, NULL);
  }

  // Save to NVS
  esp_err_t err = nvs_write_string("timezone", timezone);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to save timezone to NVS: %s", esp_err_to_name(err));
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    send_conf_header(req, i18n_tr(STR_ERROR, lang), "/timezoneConfig", 5);
    httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#10060;</div>");
    char h1_err[256];
    snprintf(h1_err, sizeof(h1_err), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_ERROR, lang));
    httpd_resp_sendstr_chunk(req, h1_err);
    char p_err[256];
    snprintf(p_err, sizeof(p_err), "<p class='conf-msg'>%s</p>",
             (lang == LANG_FR) ? "Échec de l'enregistrement du fuseau horaire" :
             (lang == LANG_EN) ? "Failed to save time zone" :
             "فشل حفظ المنطقة الزمنية");
    httpd_resp_sendstr_chunk(req, p_err);
    char btn_err[256];
    snprintf(btn_err, sizeof(btn_err), "<div class='conf-btns'><a href='/timezoneConfig'>%s</a></div>", i18n_tr(STR_RETRY, lang));
    httpd_resp_sendstr_chunk(req, btn_err);
    httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
    return httpd_resp_sendstr_chunk(req, NULL);
  }

  // Apply timezone immediately using setenv (heap copy in libc, stack-safe)
  setenv("TZ", timezone, 1);
  tzset();

  // Mettre à jour l'heure locale et le DS3231 avec la nouvelle timezone
  extern void ds3231_sync_from_system(void);
  ds3231_sync_from_system();

  ESP_LOGI(TAG, "Timezone saved to NVS and applied: %s", timezone);

  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  send_conf_header(req, i18n_tr(STR_SUCCESS, lang), "/", 3);
  httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#9989;</div>");
  char ok_h1[256];
  snprintf(ok_h1, sizeof(ok_h1), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_TZ_SAVED_TITLE, lang));
  httpd_resp_sendstr_chunk(req, ok_h1);
  char ok_p[256];
  snprintf(ok_p, sizeof(ok_p), "<p class='conf-msg'>%s</p>", i18n_tr(STR_TZ_APPLIED_IMMEDIATELY, lang));
  httpd_resp_sendstr_chunk(req, ok_p);
  httpd_resp_sendstr_chunk(req, "<div class='spinner'></div>");
  char ret_p[256];
  snprintf(ret_p, sizeof(ret_p), "<p style='color:#999;font-size:0.95rem'>%s</p>", i18n_tr(STR_RETURNING, lang));
  httpd_resp_sendstr_chunk(req, ret_p);
  httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
  return httpd_resp_sendstr_chunk(req, NULL);
}
// ========== Current Config Handlers (Phase 1.F - NVS Calibration) ==========
static esp_err_t current_config_get_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }
  app_lang_t lang = i18n_get_req_lang(req);
  // Get current calibration and thresholds
  const adc_calibration_t *calib = adc_get_calibration();
  const current_thresholds_t *thresh = adc_get_thresholds();
  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  char html_head[256];
  snprintf(html_head, sizeof(html_head),
           "<!DOCTYPE html><html lang='%s' dir='%s'><head>"
           "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>"
           "<title>%s</title>",
           i18n_code(lang), i18n_dir(lang), i18n_tr(STR_CURR_TITLE, lang));
  httpd_resp_sendstr_chunk(req, html_head);
  httpd_resp_sendstr_chunk(req, HTML_STYLE);
  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");
  i18n_send_lang_bar(req, lang);

  // Card Start
  httpd_resp_sendstr_chunk(
      req, "<div class='card' style='max-width:600px;margin:0 auto'>");
  httpd_resp_sendstr_chunk(
      req, "<span class='card-icon' style='color:#3498db'>⚙️</span>");
  char h2_buf[256];
  snprintf(h2_buf, sizeof(h2_buf), "<h2>%s</h2>", i18n_tr(STR_CURR_TITLE, lang));
  httpd_resp_sendstr_chunk(req, h2_buf);

  // Info box
  char buf[1024];
  snprintf(
      buf, sizeof(buf),
      "<div style='background:#e8f4f8;padding:12px;border-radius:8px;margin:15px 0'>"
      "<p style='margin:0;color:#2980b9'><strong>%s</strong></p>"
      "<p style='margin:5px 0'>%s <strong>%.1f</strong> (expected ~2048)</p>"
      "<p style='margin:5px 0'>%s <strong>%.3f</strong></p>"
      "<p style='margin:0'>Status: %s</p></div>",
      i18n_tr(STR_CURR_CURRENT_STATUS, lang),
      i18n_tr(STR_CURR_ZERO_POINT, lang), calib->zero_offset,
      i18n_tr(STR_CURR_SCALE_FACTOR, lang), calib->scale_factor,
      (calib->is_valid == 1) ? i18n_tr(STR_CURR_CALIB_SAVED, lang) : i18n_tr(STR_CURR_CALIB_DEFAULT, lang));
  httpd_resp_sendstr_chunk(req, buf);

  // Form Start
  httpd_resp_sendstr_chunk(
      req,
      "<div class='threshold-section'>"
      "<form action='/currentConfig' method='POST'>");
  // Scale Factor Input
  snprintf(buf, sizeof(buf),
           "<label style='display:block;margin:15px 0 5px'>%s</label>"
           "<input type='number' name='scale' step='0.001' min='0.5' max='1.5' "
           "value='%.3f' style='width:100%%' required>",
           i18n_tr(STR_CURR_SCALE_FACTOR, lang),
           calib->scale_factor);
  httpd_resp_sendstr_chunk(req, buf);

  // Thresholds Section
  char th_hdr[256];
  snprintf(th_hdr, sizeof(th_hdr), "<h3 style='margin-top:25px;color:#2c3e50'>%s</h3>", i18n_tr(STR_CURR_THRESHOLDS, lang));
  httpd_resp_sendstr_chunk(req, th_hdr);

  // Min Load
  snprintf(
      buf, sizeof(buf),
      "<label style='display:block;margin:15px 0 5px'>%s</label>"
      "<input type='number' name='min_load' step='0.01' min='0.01' max='10' "
      "value='%.2f' style='width:100%%' required>",
      i18n_tr(STR_CURR_MIN_LOAD, lang),
      thresh->min_load);
  httpd_resp_sendstr_chunk(req, buf);

  // Anomaly Threshold
  snprintf(
      buf, sizeof(buf),
      "<label style='display:block;margin:15px 0 5px'>%s</label>"
      "<input type='number' name='anomaly' step='0.01' min='0.01' max='10' "
      "value='%.2f' style='width:100%%' required>",
      i18n_tr(STR_CURR_ANOMALY, lang),
      thresh->anomaly_threshold);
  httpd_resp_sendstr_chunk(req, buf);

  // Max Critical
  snprintf(buf, sizeof(buf),
           "<label style='display:block;margin:15px 0 5px'>%s</label>"
           "<input type='number' name='max_critical' step='0.01' min='0.01' "
           "max='10' "
           "value='%.2f' style='width:100%%' required>",
           i18n_tr(STR_CURR_MAX_CRITICAL, lang),
           thresh->max_critical);
  httpd_resp_sendstr_chunk(req, buf);

  // Save Button
  char save_btn[256];
  snprintf(save_btn, sizeof(save_btn),
           "<input type='submit' value='%s' "
           "style='margin-top:25px;width:100%%;font-size:1.1rem;font-weight:bold'>",
           i18n_tr(STR_SAVE_SETTINGS, lang));
  httpd_resp_sendstr_chunk(req, save_btn);
  httpd_resp_sendstr_chunk(req, "</form>");

  // Recalibrate Zero Button
  httpd_resp_sendstr_chunk(
      req,
      "<form action='/recalibrateZero' method='POST' style='margin-top:15px'>");
  httpd_resp_sendstr_chunk(
      req, "<input type='submit' value='🔄 Zero Calibration' "
           "style='width:100%;background:#f39c12;font-weight:bold'>");
  httpd_resp_sendstr_chunk(req, "</form></div>");

  // Warning Note
  httpd_resp_sendstr_chunk(req, "<p style='color:#856404;font-size:0.85rem;text-align:center;margin-top:15px'>"
                                "⚠️ Disconnect load before recalibrating</p>");
  httpd_resp_sendstr_chunk(req, "</div>"); // Close Card

  // Navigation
  char nav_menu[256];
  snprintf(nav_menu, sizeof(nav_menu),
           "<div class='nav-menu' style='margin-top:20px'><a href='/'>%s</a></div>",
           i18n_tr(STR_BACK_TO_DASHBOARD, lang));
  httpd_resp_sendstr_chunk(req, nav_menu);
  httpd_resp_sendstr_chunk(req, "</div></body></html>");
  return httpd_resp_sendstr_chunk(req, NULL);
}
static esp_err_t current_config_post_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }
  char buf[512];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) {
    return ESP_FAIL;
  }
  buf[ret] = '\0';
  // Parse form data
  float scale = 1.0f, min_load = 0.08f, anomaly = 0.60f, max_critical = 4.0f;
  char temp[32];
  if (httpd_query_key_value(buf, "scale", temp, sizeof(temp)) == ESP_OK) {
    scale = atof(temp);
  }
  if (httpd_query_key_value(buf, "min_load", temp, sizeof(temp)) == ESP_OK) {
    min_load = atof(temp);
  }
  if (httpd_query_key_value(buf, "anomaly", temp, sizeof(temp)) == ESP_OK) {
    anomaly = atof(temp);
  }
  if (httpd_query_key_value(buf, "max_critical", temp, sizeof(temp)) ==
      ESP_OK) {
    max_critical = atof(temp);
  }
  // Validate thresholds
  if (min_load <= 0 || min_load >= anomaly || anomaly >= max_critical ||
      max_critical > 10) {
    const char *error_html =
        "<!DOCTYPE html><html><body><h1>خطأ</h1>"
        "<p>القيم غير صحيحة. يجب أن يكون: min < anomaly < max <= 10</p>"
        "<a href='/currentConfig'>عودة</a></body></html>";
    return httpd_resp_send(req, error_html, strlen(error_html));
  }
  // Update calibration
  const adc_calibration_t *current_calib = adc_get_calibration();
  adc_calibration_t new_calib = *current_calib;
  new_calib.scale_factor = scale;
  new_calib.is_valid = 1;
  adc_set_calibration(&new_calib);
  adc_save_calibration();
  // Update thresholds
  current_thresholds_t new_thresh = {.min_load = min_load,
                                     .anomaly_threshold = anomaly,
                                     .max_critical = max_critical};
  adc_set_thresholds(&new_thresh);
  adc_save_thresholds();
  // Success redirect
  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location", "/");
  return httpd_resp_send(req, NULL, 0);
}
static esp_err_t recalibrate_zero_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }
  app_lang_t lang = i18n_get_req_lang(req);
  float new_zero = 0; // adc_calibrate_zero();
  adc_save_calibration();

  // Use modern confirmation template
  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  send_conf_header(req, i18n_tr(STR_CURR_TITLE, lang), "/", 3);
  httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>&#128276;</div>");
  char ok_h1[256];
  snprintf(ok_h1, sizeof(ok_h1), "<h1 class='conf-title'>%s</h1>", i18n_tr(STR_SUCCESS, lang));
  httpd_resp_sendstr_chunk(req, ok_h1);

  char msg[256];
  snprintf(msg, sizeof(msg),
           "<p class='conf-msg'>%s: <strong style='color:#667eea'>%.1f</strong><br>%s</p>",
           i18n_tr(STR_CURR_ZERO_POINT, lang),
           new_zero,
           i18n_tr(STR_SAVED_SUCCESS, lang));
  httpd_resp_sendstr_chunk(req, msg);

  httpd_resp_sendstr_chunk(req, "<div class='spinner'></div>");
  char ret_p[256];
  snprintf(ret_p, sizeof(ret_p), "<p style='color:#999;font-size:0.95rem'>%s</p>", i18n_tr(STR_RETURNING, lang));
  httpd_resp_sendstr_chunk(req, ret_p);
  httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);

  return httpd_resp_sendstr_chunk(req, NULL);
}

// ========== Settings API (JSON) ==========
static esp_err_t settings_api_get_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
    return ESP_FAIL;
  }

  cJSON *root = cJSON_CreateObject();

  // WiFi SSID
  char ssid[33] = {0};
  nvs_read_string("wifi_ssid", ssid, sizeof(ssid));
  cJSON_AddStringToObject(root, "wifi_ssid", ssid);

  // Timezone
  char timezone[64] = {0};
  nvs_read_string("timezone", timezone, sizeof(timezone));
  if (strlen(timezone) == 0 || strstr(timezone, "WET-0WEST") != NULL)
    strcpy(timezone, "GMT0"); // Default
  cJSON_AddStringToObject(root, "timezone", timezone);

  // Thresholds
  const current_thresholds_t *thresh = adc_get_thresholds();
  cJSON *th = cJSON_CreateObject();
  cJSON_AddNumberToObject(th, "min_load", thresh->min_load);
  cJSON_AddNumberToObject(th, "anomaly", thresh->anomaly_threshold);
  cJSON_AddNumberToObject(th, "max_critical", thresh->max_critical);
  cJSON_AddItemToObject(root, "thresholds", th);

  // Calibration Factor
  const adc_calibration_t *calib = adc_get_calibration();
  cJSON_AddNumberToObject(root, "scale_factor", calib->scale_factor);

  char *json = cJSON_PrintUnformatted(root);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, json, strlen(json));

  cJSON_Delete(root);
  free(json);
  return ESP_OK;
}

static esp_err_t settings_api_post_handler(httpd_req_t *req) {
  if (!is_authorized)
    return ESP_FAIL;

  // 1. Read JSON body
  char *buf = malloc(req->content_len + 1);
  if (!buf)
    return ESP_FAIL;

  int ret = 0;
  int remaining = req->content_len;
  while (remaining > 0) {
    int r = httpd_req_recv(req, buf + ret, remaining);
    if (r <= 0) {
      if (r == HTTPD_SOCK_ERR_TIMEOUT)
        continue;
      free(buf);
      return ESP_FAIL;
    }
    ret += r;
    remaining -= r;
  }
  buf[ret] = '\0';

  cJSON *root = cJSON_Parse(buf);
  free(buf);
  if (!root)
    return ESP_FAIL;

  // 2. Parse & Save Settings

  // WiFi
  cJSON *wifi_ssid = cJSON_GetObjectItem(root, "wifi_ssid");
  cJSON *wifi_pass = cJSON_GetObjectItem(root, "wifi_pass");
  bool wifi_changed = false;

  if (wifi_ssid && cJSON_IsString(wifi_ssid)) {
    nvs_write_string("wifi_ssid", wifi_ssid->valuestring);
    wifi_changed = true;
  }
  if (wifi_pass && cJSON_IsString(wifi_pass)) {
    nvs_write_string("wifi_pass", wifi_pass->valuestring);
    wifi_changed = true;
  }

  // Timezone
  cJSON *tz = cJSON_GetObjectItem(root, "timezone");
  if (tz && cJSON_IsString(tz)) {
    nvs_write_string("timezone", tz->valuestring);
    setenv("TZ", tz->valuestring, 1);
    tzset();
    extern void ds3231_sync_from_system(void);
    ds3231_sync_from_system();
  }

  // Thresholds
  cJSON *th_obj = cJSON_GetObjectItem(root, "thresholds");
  if (th_obj) {
    float min = -1, anom = -1, max_c = -1;
    cJSON *c_min = cJSON_GetObjectItem(th_obj, "min_load");
    cJSON *c_anom = cJSON_GetObjectItem(th_obj, "anomaly");
    cJSON *c_max = cJSON_GetObjectItem(th_obj, "max_critical");

    const current_thresholds_t *curr = adc_get_thresholds();
    if (c_min)
      min = (float)c_min->valuedouble;
    else
      min = curr->min_load;
    if (c_anom)
      anom = (float)c_anom->valuedouble;
    else
      anom = curr->anomaly_threshold;
    if (c_max)
      max_c = (float)c_max->valuedouble;
    else
      max_c = curr->max_critical;

    // Basic validation
    if (min > 0 && anom > min && max_c > anom) {
      current_thresholds_t new_t = {min, anom, max_c};
      adc_set_thresholds(&new_t);
      adc_save_thresholds();
    }
  }

  // Calibration Scale
  cJSON *scale = cJSON_GetObjectItem(root, "scale_factor");
  if (scale && cJSON_IsNumber(scale)) {
    const adc_calibration_t *curr_c = adc_get_calibration();
    adc_calibration_t new_c = *curr_c;
    new_c.scale_factor = (float)scale->valuedouble;
    adc_set_calibration(&new_c);
    adc_save_calibration();
  }

  cJSON_Delete(root);

  // Response
  httpd_resp_set_type(req, "application/json");
  const char *resp = "{\"status\":\"ok\", \"wifi_restart_required\": %s}";
  char resp_buf[64];
  snprintf(resp_buf, sizeof(resp_buf), resp, wifi_changed ? "true" : "false");
  httpd_resp_send(req, resp_buf, strlen(resp_buf));

  return ESP_OK;
}

// ========== OTA Update Handlers ==========
static esp_err_t update_get_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  app_lang_t lang = i18n_get_req_lang(req);

  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  char html_head[256];
  snprintf(html_head, sizeof(html_head),
           "<!DOCTYPE html><html lang='%s' dir='%s'><head>"
           "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>"
           "<title>%s</title>",
           i18n_code(lang), i18n_dir(lang), i18n_tr(STR_OTA_TITLE, lang));
  httpd_resp_sendstr_chunk(req, html_head);
  httpd_resp_sendstr_chunk(req, HTML_STYLE);
  httpd_resp_sendstr_chunk(req, "</head><body><div class='container'>");
  i18n_send_lang_bar(req, lang);

  httpd_resp_sendstr_chunk(
      req, "<div class='card' style='max-width:500px;margin:0 auto'>");
  httpd_resp_sendstr_chunk(req, "<span class='card-icon'>🔄</span>");
  char h2_buf[256];
  snprintf(h2_buf, sizeof(h2_buf), "<h2>%s</h2>", i18n_tr(STR_OTA_TITLE, lang));
  httpd_resp_sendstr_chunk(req, h2_buf);

  httpd_resp_sendstr_chunk(
      req, "<div style='background:#fff3cd;border:1px solid #ffc107;padding:15px;border-radius:8px;margin:15px 0;'>");
  char warn_buf[256];
  snprintf(warn_buf, sizeof(warn_buf),
           "<p style='margin:0;color:#856404;'>⚠️ <strong>%s</strong></p>",
           (lang == LANG_FR) ? "Attention : Le système redémarrera après la mise à jour" :
           (lang == LANG_EN) ? "Warning: The system will restart after firmware update" :
           "تحذير: سيتم إعادة تشغيل النظام بعد التحديث");
  httpd_resp_sendstr_chunk(req, warn_buf);
  httpd_resp_sendstr_chunk(req, "</div>");

  httpd_resp_sendstr_chunk(req, "<form method='POST' action='/update' enctype='multipart/form-data'>");

  // SHA256 FIRST - so it arrives before firmware in multipart!
  char sha_lbl[256];
  snprintf(sha_lbl, sizeof(sha_lbl),
           "<label style='margin-top:15px'>🔒 %s</label>",
           i18n_tr(STR_OTA_HASH_LABEL, lang));
  httpd_resp_sendstr_chunk(req, sha_lbl);
  httpd_resp_sendstr_chunk(req,
                           "<input type='text' name='sha256' "
                           "pattern='[a-fA-F0-9]{64}' "
                           "maxlength='64' required "
                           "placeholder='64-char SHA256' "
                           "style='font-family:monospace;font-size:13px'>");
  char hint_buf[256];
  snprintf(hint_buf, sizeof(hint_buf),
           "<small style='color:#666;font-size:0.85rem'>%s</small>",
           i18n_tr(STR_OTA_HASH_HINT, lang));
  httpd_resp_sendstr_chunk(req, hint_buf);

  // Firmware file SECOND
  char file_lbl[256];
  snprintf(file_lbl, sizeof(file_lbl),
           "<label>%s</label>",
           i18n_tr(STR_OTA_FILE_LABEL, lang));
  httpd_resp_sendstr_chunk(req, file_lbl);
  httpd_resp_sendstr_chunk(
      req, "<input type='file' name='firmware' accept='.bin' required>");

  char submit_btn[256];
  snprintf(submit_btn, sizeof(submit_btn),
           "<button type='submit' class='btn-submit'>%s</button>",
           i18n_tr(STR_OTA_SUBMIT, lang));
  httpd_resp_sendstr_chunk(req, submit_btn);
  httpd_resp_sendstr_chunk(req, "</form>");

  char cancel_btn[256];
  snprintf(cancel_btn, sizeof(cancel_btn),
           "<a href='/' class='btn-cancel'>%s</a>",
           i18n_tr(STR_CANCEL, lang));
  httpd_resp_sendstr_chunk(req, cancel_btn);
  httpd_resp_sendstr_chunk(req, "</div></div></body></html>");

  return httpd_resp_sendstr_chunk(req, NULL);
}

static esp_err_t update_post_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  esp_err_t err;
  esp_ota_handle_t ota_handle = 0;
  const esp_partition_t *update_partition = NULL;
  char buffer[1024];
  int received;
  size_t total_written = 0;
  bool binary_started = false;
  // char *boundary_end = "\r\n\r\n"; // End of multipart headers (Unused)

  // SHA256 validation variables
  char expected_sha256[65] = {0};
  bool sha256_extracted = false;
  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts(&sha_ctx, 0); // 0 = SHA256 (not SHA224)

  // Header accumulation buffer for multipart parsing
  static char header_accumulator[1536] = {0};
  size_t header_pos = 0;

  // Multipart boundary handling
  char boundary[128] = {0};
  char boundary_search[132] = {0}; // \r\n--boundary
  size_t content_type_len = httpd_req_get_hdr_value_len(req, "Content-Type");
  if (content_type_len > 0) {
    char *content_type = malloc(content_type_len + 1);
    if (content_type) {
      httpd_req_get_hdr_value_str(req, "Content-Type", content_type,
                                  content_type_len + 1);
      char *bound_ptr = strstr(content_type, "boundary=");
      if (bound_ptr) {
        bound_ptr += 9; // Skip "boundary="
        strncpy(boundary, bound_ptr, sizeof(boundary) - 1);

        // Trim any trailing whitespace or newlines from boundary
        char *end = boundary + strlen(boundary) - 1;
        while (end > boundary &&
               (*end == '\r' || *end == '\n' || *end == ' ' || *end == ';')) {
          *end = '\0';
          end--;
        }

        snprintf(boundary_search, sizeof(boundary_search), "\r\n--%s",
                 boundary);
        ESP_LOGI(TAG, "Multipart boundary: '%s'", boundary);
      }
      free(content_type);
    }
  }

  ESP_LOGI(TAG, "Starting OTA update...");

  // Set Safety Flag prevents status polling interference
  g_is_ota_active = true;

  // Get next OTA partition
  update_partition = esp_ota_get_next_update_partition(NULL);
  if (update_partition == NULL) {
    ESP_LOGE(TAG, "OTA partition not found");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "OTA partition not found");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lx",
           update_partition->subtype, update_partition->address);

  // Begin OTA
  err =
      esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "OTA begin failed");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "OTA started successfully");

  // Receive and write firmware data
  // Pending buffer for delayed write (to handle split boundaries)
  char pending[256];
  int pending_len = 0;
  // Keep enough back to cover spanning boundary (boundary is ~40-70 chars)
  const int KEEP_BACK = 128;
  bool update_finished = false;

// Ensure we revert flag on error
#define OTA_FAIL_CLEANUP()                                                     \
  {                                                                            \
    g_is_ota_active = false;                                                   \
  }

  // Receive and write firmware data
  while ((received = httpd_req_recv(req, buffer, sizeof(buffer))) > 0) {
    char *data_start = buffer;
    int data_len = received;

    // Accumulate headers before binary starts
    if (!binary_started && header_pos < sizeof(header_accumulator) - 1) {
      size_t to_copy =
          (received < (sizeof(header_accumulator) - header_pos - 1))
              ? received
              : (sizeof(header_accumulator) - header_pos - 1);
      memcpy(header_accumulator + header_pos, buffer, to_copy);
      header_pos += to_copy;
      header_accumulator[header_pos] = '\0';

      // Try to extract SHA256 from accumulated headers
      if (!sha256_extracted) {
        char *sha_field = strstr(header_accumulator, "name=\"sha256\"");
        if (sha_field) {
          char *sha_value = strstr(sha_field, "\r\n\r\n");
          if (sha_value) {
            sha_value += 4; // Skip "\r\n\r\n"
            while (*sha_value == ' ' || *sha_value == '\t') sha_value++;
            char *sha_end = strstr(sha_value, "\r\n");
            if (sha_end) {
              while (sha_end > sha_value && (*(sha_end - 1) == ' ' || *(sha_end - 1) == '\t' || *(sha_end - 1) == '\r')) sha_end--;
              int sha_len = sha_end - sha_value;
              if (sha_len == 64) {
                memcpy(expected_sha256, sha_value, 64);
                expected_sha256[64] = '\0';
                sha256_extracted = true;
                ESP_LOGI(TAG, "✓ SHA256 extracted: %.16s...", expected_sha256);
              }
            }
          }
        }
      }
    }

    // Look for firmware binary start in current buffer
    if (!binary_started) {
      char *firmware_field = strstr(buffer, "name=\"firmware\"");
      if (firmware_field) {
        char *header_end = strstr(firmware_field, "\r\n\r\n");
        if (header_end) {
          data_start = header_end + 4; // Skip "\r\n\r\n"
          data_len = received - (data_start - buffer);
          binary_started = true;
          ESP_LOGI(TAG, "Found binary start, skipping %d header bytes",
                   (int)(data_start - buffer));
        } else {
          continue;
        }
      } else {
        continue;
      }
    }

    // Delayed Writethrough Logic
    if (binary_started && data_len > 0 && !update_finished) {
      // We use a working buffer to combine pending data + new data
      // to handle boundaries that might be split across chunks.
      char work_buf[1024 + 256];

      if (pending_len + data_len > sizeof(work_buf)) {
        ESP_LOGE(TAG, "OTA work buffer overflow");
        return ESP_FAIL;
      }

      // Construct continuous stream: [pending] + [new_data]
      memcpy(work_buf, pending, pending_len);
      memcpy(work_buf + pending_len, data_start, data_len);
      int work_len = pending_len + data_len;

      // Search for boundary in the combined buffer
      int boundary_idx = -1;
      if (boundary_search[0] != 0) {
        int b_len = strlen(boundary_search);
        // Search can be optimized, but manual loop is safe
        for (int i = 0; i <= work_len - b_len; i++) {
          if (memcmp(work_buf + i, boundary_search, b_len) == 0) {
            boundary_idx = i;
            break;
          }
        }
      }

      int bytes_to_write = 0;

      if (boundary_idx >= 0) {
        // Boundary found! Write everything up to it.
        bytes_to_write = boundary_idx;
        // We are done with the file
        update_finished = true;
        pending_len = 0; // Clear pending
        ESP_LOGI(TAG, "Boundary found at offset %d. Finalizing write.",
                 boundary_idx);
      } else {
        // No boundary found.
        // We must keep back KEEP_BACK bytes to ensure we don't write a split
        // boundary.
        if (work_len > KEEP_BACK) {
          bytes_to_write = work_len - KEEP_BACK;
          // Save the tail for next time
          pending_len = KEEP_BACK;
          memcpy(pending, work_buf + bytes_to_write, KEEP_BACK);
        } else {
          // Not enough data to write properly yet, verify in next chunk
          bytes_to_write = 0;
          pending_len = work_len;
          memcpy(pending, work_buf, work_len);
        }
      }

      // PERFOM WRITE
      if (bytes_to_write > 0) {
        // DEBUG: Log start of binary to verify magic byte
        if (total_written == 0) {
          ESP_LOGI(TAG, "First 16 bytes written:");
          ESP_LOG_BUFFER_HEXDUMP(TAG, work_buf,
                                 (bytes_to_write > 16) ? 16 : bytes_to_write,
                                 ESP_LOG_INFO);
        }

        mbedtls_sha256_update(&sha_ctx, (const unsigned char *)work_buf,
                              bytes_to_write);
        err = esp_ota_write(ota_handle, work_buf, bytes_to_write);
        if (err != ESP_OK) {
          esp_ota_abort(ota_handle);
          ESP_LOGE(TAG, "OTA write failed");
          return ESP_FAIL;
        }
        total_written += bytes_to_write;

        // DEBUG: Log end of this chunk layout
        if (boundary_idx >= 0) { // This is the final write before boundary
          ESP_LOGI(TAG, "Last 32 bytes written (at total %d):", total_written);
          if (bytes_to_write >= 32)
            ESP_LOG_BUFFER_HEXDUMP(TAG, work_buf + bytes_to_write - 32, 32,
                                   ESP_LOG_INFO);
          else
            ESP_LOG_BUFFER_HEXDUMP(TAG, work_buf, bytes_to_write, ESP_LOG_INFO);
        }
      }
    }
  }

  if (received < 0) {
    ESP_LOGE(TAG, "Error receiving OTA data");
    esp_ota_abort(ota_handle);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "OTA receive failed");
    OTA_FAIL_CLEANUP();
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Total written: %d bytes", total_written);

  // Finalize and verify SHA256
  if (sha256_extracted) {
    unsigned char calculated_hash[32];
    mbedtls_sha256_finish(&sha_ctx, calculated_hash);
    mbedtls_sha256_free(&sha_ctx);

    // Convert to hex string
    char calculated_sha256[65];
    for (int i = 0; i < 32; i++) {
      sprintf(calculated_sha256 + (i * 2), "%02x", calculated_hash[i]);
    }
    calculated_sha256[64] = '\0';

    ESP_LOGI(TAG, "Calculated SHA256: %.16s...", calculated_sha256);

    // Compare hashes (case-insensitive)
    if (strcasecmp(calculated_sha256, expected_sha256) != 0) {
      ESP_LOGE(TAG, "SHA256 MISMATCH!");
      ESP_LOGE(TAG, "Expected:    %s", expected_sha256);
      ESP_LOGE(TAG, "Calculated:  %s", calculated_sha256);
      esp_ota_abort(ota_handle);
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                          "SHA256 validation failed - firmware rejected");
      OTA_FAIL_CLEANUP();
      return ESP_FAIL;
    }
    ESP_LOGI(TAG, "✓ SHA256 validation PASSED");
  } else {
    // STRICT MODE: Reject uploads without SHA256
    ESP_LOGE(TAG, "SHA256 not provided - upload REJECTED (security policy)");
    ESP_LOGE(TAG, "For security, all OTA updates MUST include SHA256 hash");
    esp_ota_abort(ota_handle);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                        "SHA256 hash required for OTA update");
    OTA_FAIL_CLEANUP();
    return ESP_FAIL;
  }

  // End OTA
  err = esp_ota_end(ota_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA end failed");
    OTA_FAIL_CLEANUP();
    return ESP_FAIL;
  }

  // Set boot partition
  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)",
             esp_err_to_name(err));
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Set boot partition failed");
    OTA_FAIL_CLEANUP();
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "OTA update successful. Restarting...");

  // Send success response
  app_lang_t lang = i18n_get_req_lang(req);
  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  send_conf_header(req, i18n_tr(STR_SUCCESS, lang), "/", 10);
  httpd_resp_sendstr_chunk(req, "<div class='conf-icon'>✅</div>");
  char ok_h1[256];
  snprintf(ok_h1, sizeof(ok_h1), "<h1 class='conf-title'>%s</h1>",
           (lang == LANG_FR) ? "Mise à jour réussie !" :
           (lang == LANG_EN) ? "Update Successful!" :
           "تم التحديث بنجاح");
  httpd_resp_sendstr_chunk(req, ok_h1);
  char ok_p[512];
  snprintf(ok_p, sizeof(ok_p),
           "<p class='conf-msg'>%s</p>"
           "<p style='margin-top:15px;font-size:1.05rem;color:#555;'>%s <span id='cd' style='font-weight:bold;color:#27ae60;'>10</span>s</p>"
           "<script>let s=10;setInterval(()=>{s--;let e=document.getElementById('cd');if(e)e.innerText=s;if(s<=0)location.href='/';},1000);</script>",
           (lang == LANG_FR) ? "Le firmware a été flashé avec succès !" :
           (lang == LANG_EN) ? "Firmware flashed successfully!" :
           "تم تثبيت النظام بنجاح!",
           (lang == LANG_FR) ? "Redémarrage et retour à la page principale dans" :
           (lang == LANG_EN) ? "Restarting and returning to dashboard in" :
           "إعادة التشغيل والعودة للوحة التحكم خلال");
  httpd_resp_sendstr_chunk(req, ok_p);
  httpd_resp_sendstr_chunk(req, HTML_CONF_FOOTER);
  httpd_resp_sendstr_chunk(req, NULL);

  // Generous delay to ensure TCP buffers flush and browser renders confirmation page
  vTaskDelay(pdMS_TO_TICKS(3500));
  esp_restart();

  return ESP_OK;
}

// ========== Logs Download Handler ==========
static esp_err_t logs_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  // Generate system report
  char report[2048];
  int offset = 0;

  // Header
  // NOTE: Using UTF-8 Arabic for report header
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "=== تقرير نظام التحكم الاسلكي الاتوماتيكي ===\n");

  // Timestamp
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "Generated: %s\n\n", time_str);

  // System Info
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "--- System Information ---\n");
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "Free Heap: %lu bytes\n",
                     (unsigned long)esp_get_free_heap_size());
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "IDF Version: %s\n", esp_get_idf_version());

  // WiFi status (simplified)
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "\n--- Network Status ---\n");
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "WiFi: Connected\n"); // Simplified

  // Alarms summary
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "\n--- Alarm Status ---\n");
  char next_alarm[32];
  alarm_get_next_time_str_with_relay(0, -1, next_alarm, sizeof(next_alarm));
  offset +=
      snprintf(report + offset, sizeof(report) - offset, "Next Alarm: %s\n",
               strlen(next_alarm) > 0 ? next_alarm : "None");

  // Current sensor - DISABLED in Classic
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "\n--- Current Sensor ---\n");
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "Current: DISABLED (Classic)\n");

  // Relay state
  offset += snprintf(report + offset, sizeof(report) - offset, "Relays: ");
  for (int r = 0; r < 4; r++) {
    offset += snprintf(report + offset, sizeof(report) - offset, "R%d:%s ",
                       r + 1, relay_get_state(r) ? "ON" : "OFF");
  }
  offset += snprintf(report + offset, sizeof(report) - offset, "\n");

  // Calibration
  const adc_calibration_t *calib = adc_get_calibration();
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "Zero Offset: %.1f\n", calib->zero_offset);

  // Recent logs from buffer (if available)
  size_t log_len = 0;
  log_lock();
  const char *logs = log_buffer_get(&log_len);
  if (log_len > 0 && logs != NULL) {
    offset += snprintf(report + offset, sizeof(report) - offset,
                       "\n--- Recent Logs ---\n");
    // Add last portion of logs (max 1000 bytes)
    size_t copy_len = (log_len > 1000) ? 1000 : log_len;
    if (offset + copy_len < sizeof(report)) {
      memcpy(report + offset, logs + (log_len - copy_len), copy_len);
      offset += copy_len;
    }
  }
  log_unlock();

  // Footer
  offset += snprintf(report + offset, sizeof(report) - offset,
                     "\n=== End of Report ===\n");

  // Set headers for download
  httpd_resp_set_type(req, "text/plain; charset=utf-8");
  httpd_resp_set_hdr(req, "Content-Disposition",
                     "attachment; filename=\"system_report.txt\"");

  // Send the report
  httpd_resp_send(req, report, strlen(report));

  ESP_LOGI(TAG, "System report downloaded (%d bytes)", strlen(report));
  return ESP_OK;
}

// ========== Backup/Restore Handlers ==========
static esp_err_t backup_download_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  cJSON *root = cJSON_CreateObject();
  if (root == NULL) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed to create JSON");
    return ESP_FAIL;
  }

  // Version and timestamp
  cJSON_AddStringToObject(root, "version", "1.0");
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timestamp[32];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  cJSON_AddStringToObject(root, "timestamp", timestamp);

  // WiFi configuration (SSID only, no password for security)
  cJSON *wifi = cJSON_AddObjectToObject(root, "wifi");
  if (wifi != NULL) {
    char ssid[33] = {0};
    if (nvs_read_string("wifi_ssid", ssid, sizeof(ssid)) == ESP_OK) {
      cJSON_AddStringToObject(wifi, "ssid", ssid);
    }
  }

  // Timezone
  char timezone[64] = {0};
  if (nvs_read_string("timezone", timezone, sizeof(timezone)) == ESP_OK) {
    cJSON_AddStringToObject(root, "timezone", timezone);
  }

  // ADC Calibration
  cJSON *calibration = cJSON_AddObjectToObject(root, "calibration");
  if (calibration != NULL) {
    const adc_calibration_t *calib = adc_get_calibration();
    cJSON_AddNumberToObject(calibration, "zero_offset", calib->zero_offset);
    cJSON_AddNumberToObject(calibration, "scale_factor", calib->scale_factor);

    const current_thresholds_t *thresh = adc_get_thresholds();
    cJSON_AddNumberToObject(calibration, "min_threshold", thresh->min_load);
    cJSON_AddNumberToObject(calibration, "anomaly_threshold",
                            thresh->anomaly_threshold);
    cJSON_AddNumberToObject(calibration, "max_threshold", thresh->max_critical);
  }

  // Alarms (simplified - we'll export the entire schedule)
  // Note: This is a simplified version. Full implementation would iterate
  // through all days
  cJSON *alarms_note = cJSON_AddObjectToObject(root, "alarms_note");
  cJSON_AddStringToObject(
      alarms_note, "message",
      "Alarm data is stored in NVS - manual backup recommended");

  // Generate JSON string
  char *json_string = cJSON_Print(root);
  cJSON_Delete(root);

  if (json_string == NULL) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed to serialize JSON");
    return ESP_FAIL;
  }

  // Set headers for download
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Content-Disposition",
                     "attachment; filename=\"system_config.json\"");

  // Send JSON
  httpd_resp_send(req, json_string, strlen(json_string));

  // Free memory
  cJSON_free(json_string);

  ESP_LOGI(TAG, "Configuration backup downloaded");
  return ESP_OK;
}

static esp_err_t backup_restore_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  // Read content length
  if (req->content_len == 0 || req->content_len > 4096) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid backup file size");
    return ESP_FAIL;
  }

  // Allocate buffer for JSON
  char *json_buffer = malloc(req->content_len + 1);
  if (json_buffer == NULL) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
    return ESP_FAIL;
  }

  // Read the JSON data
  int remaining = req->content_len;
  int received = 0;
  while (remaining > 0) {
    int ret = httpd_req_recv(req, json_buffer + received, remaining);
    if (ret <= 0) {
      free(json_buffer);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Failed to receive data");
      return ESP_FAIL;
    }
    received += ret;
    remaining -= ret;
  }
  json_buffer[received] = '\0';

  // Parse JSON
  cJSON *root = cJSON_Parse(json_buffer);
  free(json_buffer);

  if (root == NULL) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
    return ESP_FAIL;
  }

  // Validate version
  cJSON *version = cJSON_GetObjectItem(root, "version");
  if (version == NULL || !cJSON_IsString(version)) {
    cJSON_Delete(root);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                        "Missing or invalid version");
    return ESP_FAIL;
  }

  // Restore timezone
  cJSON *timezone = cJSON_GetObjectItem(root, "timezone");
  if (timezone != NULL && cJSON_IsString(timezone)) {
    nvs_write_string("timezone", timezone->valuestring);
    ESP_LOGI(TAG, "Restored timezone: %s", timezone->valuestring);
  }

  // Restore calibration
  cJSON *calibration = cJSON_GetObjectItem(root, "calibration");
  if (calibration != NULL && cJSON_IsObject(calibration)) {
    adc_calibration_t calib;
    cJSON *item;

    item = cJSON_GetObjectItem(calibration, "zero_offset");
    if (item && cJSON_IsNumber(item)) {
      calib.zero_offset = item->valuedouble;
    }

    item = cJSON_GetObjectItem(calibration, "scale_factor");
    if (item && cJSON_IsNumber(item)) {
      calib.scale_factor = item->valuedouble;
    }

    // Save calibration - must set first, then save
    adc_set_calibration(&calib);
    adc_save_calibration();

    // Note: Thresholds are separate, we'll restore them via NVS directly
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
      item = cJSON_GetObjectItem(calibration, "min_threshold");
      if (item && cJSON_IsNumber(item)) {
        float val = item->valuedouble;
        nvs_set_blob(handle, "adc_thresh", &val, sizeof(float));
      }

      item = cJSON_GetObjectItem(calibration, "anomaly_threshold");
      if (item && cJSON_IsNumber(item)) {
        float val = item->valuedouble;
        nvs_set_blob(handle, "adc_anom", &val, sizeof(float));
      }

      item = cJSON_GetObjectItem(calibration, "max_threshold");
      if (item && cJSON_IsNumber(item)) {
        float val = item->valuedouble;
        nvs_set_blob(handle, "adc_max", &val, sizeof(float));
      }

      nvs_commit(handle);
      nvs_close(handle);
    }
    ESP_LOGI(TAG, "Restored ADC calibration and thresholds");
  }

  cJSON_Delete(root);

  // Send success response
  const char *response =
      "Configuration restaurée avec succès ! Le système va redémarrer...";
  httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);

  // Schedule restart after response sent - give time for response
  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_restart();

  return ESP_OK;
}


// ========== Predictive Maintenance API Handler ==========
#include "predictive_maintenance.h"
#include "power_manager.h"
#include <cJSON.h>
static esp_err_t api_health_get_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "401 Unauthorized");
    return httpd_resp_send(req, "{\"error\":\"Unauthorized\"}", -1);
  }

  float health = pm_get_health();
  float temp = pm_get_temp();
  int days = pm_get_days_remaining();
  const char *status = (health > 20.0f) ? "ok" : "critical";

  char buf[192];
  if (days >= 9999) {
    snprintf(buf, sizeof(buf),
      "{\"health\": %.2f, \"temp\": %.2f, \"status\": \"%s\", \"days_remaining\": 9999}",
      health, temp, status);
  } else {
    snprintf(buf, sizeof(buf),
      "{\"health\": %.2f, \"temp\": %.2f, \"status\": \"%s\", \"days_remaining\": %d}",
      health, temp, status, days);
  }

  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, buf, -1);
}

static const httpd_uri_t uri_api_health = {
    .uri = "/api/health",
    .method = HTTP_GET,
    .handler = api_health_get_handler,
    .user_ctx = NULL
};

// ========== POWER MANAGEMENT API ==========

static esp_err_t api_power_get_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "401 Unauthorized");
    return httpd_resp_send(req, "{\"error\":\"Unauthorized\"}", -1);
  }

  power_mode_t mode = power_manager_get_mode();
  sleep_window_t window = power_manager_get_window();
  
  cJSON *root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "mode", (int)mode);
  cJSON_AddNumberToObject(root, "start_hour", window.start_hour);
  cJSON_AddNumberToObject(root, "start_min", window.start_min);
  cJSON_AddNumberToObject(root, "end_hour", window.end_hour);
  cJSON_AddNumberToObject(root, "end_min", window.end_min);
  cJSON_AddNumberToObject(root, "start_sec", window.start_sec);
  cJSON_AddNumberToObject(root, "end_sec", window.end_sec);
  
  // Dates
  cJSON_AddNumberToObject(root, "start_day", window.start_day);
  cJSON_AddNumberToObject(root, "start_month", window.start_month);
  cJSON_AddNumberToObject(root, "start_year", window.start_year);
  cJSON_AddNumberToObject(root, "end_day", window.end_day);
  cJSON_AddNumberToObject(root, "end_month", window.end_month);
  cJSON_AddNumberToObject(root, "end_year", window.end_year);
  // Weekly
  weekly_sleep_schedule_t weekly = power_manager_get_weekly();
  cJSON *w_arr = cJSON_AddArrayToObject(root, "weekly_schedule");
  for (int i = 0; i < 7; i++) {
    cJSON *day = cJSON_CreateObject();
    cJSON_AddBoolToObject(day, "enabled", weekly.days[i].enabled);
    cJSON_AddNumberToObject(day, "start_hour", weekly.days[i].start_hour);
    cJSON_AddNumberToObject(day, "start_min", weekly.days[i].start_min);
    cJSON_AddNumberToObject(day, "start_sec", weekly.days[i].start_sec);
    cJSON_AddNumberToObject(day, "end_hour", weekly.days[i].end_hour);
    cJSON_AddNumberToObject(day, "end_min", weekly.days[i].end_min);
    cJSON_AddNumberToObject(day, "end_sec", weekly.days[i].end_sec);
    cJSON_AddItemToArray(w_arr, day);
  }
  
  char *json = cJSON_PrintUnformatted(root);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, json, strlen(json));
  
  cJSON_Delete(root);
  free(json);
  return ESP_OK;
}

static esp_err_t api_power_post_handler(httpd_req_t *req) {
  if (!is_authorized) return ESP_FAIL;

  int total_len = req->content_len;
  if (total_len <= 0 || total_len >= 2048) {
      return ESP_FAIL;
  }

  char *buf = malloc(total_len + 1);
  if (!buf) {
      return ESP_FAIL;
  }

  int received = 0;
  while (received < total_len) {
      int ret = httpd_req_recv(req, buf + received, total_len - received);
      if (ret <= 0) {
          if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
          free(buf);
          return ESP_FAIL;
      }
      received += ret;
  }
  buf[total_len] = '\0';

  cJSON *root = cJSON_Parse(buf);
  free(buf);
  if (!root) return ESP_FAIL;

  cJSON *m = cJSON_GetObjectItem(root, "mode");
  if (m) {
    power_manager_set_mode((power_mode_t)m->valueint);
  }
  
  sleep_window_t window = power_manager_get_window();
  cJSON *sh = cJSON_GetObjectItem(root, "start_hour");
  if (sh) window.start_hour = sh->valueint;
  cJSON *sm = cJSON_GetObjectItem(root, "start_min");
  if (sm) window.start_min = sm->valueint;
  cJSON *eh = cJSON_GetObjectItem(root, "end_hour");
  if (eh) window.end_hour = eh->valueint;
  cJSON *em = cJSON_GetObjectItem(root, "end_min");
  if (em) window.end_min = em->valueint;
  cJSON *ss = cJSON_GetObjectItem(root, "start_sec");
  if (ss) window.start_sec = ss->valueint;
  cJSON *es = cJSON_GetObjectItem(root, "end_sec");
  if (es) window.end_sec = es->valueint;
  
  // Dates
  cJSON *sdy = cJSON_GetObjectItem(root, "start_day");
  if (sdy) window.start_day = sdy->valueint;
  cJSON *smo = cJSON_GetObjectItem(root, "start_month");
  if (smo) window.start_month = smo->valueint;
  cJSON *syr = cJSON_GetObjectItem(root, "start_year");
  if (syr) window.start_year = syr->valueint;

  cJSON *edy = cJSON_GetObjectItem(root, "end_day");
  if (edy) window.end_day = edy->valueint;
  cJSON *emo = cJSON_GetObjectItem(root, "end_month");
  if (emo) window.end_month = emo->valueint;
  cJSON *eyr = cJSON_GetObjectItem(root, "end_year");
  if (eyr) window.end_year = eyr->valueint;

  power_manager_set_window(window);

  cJSON *w_arr = cJSON_GetObjectItem(root, "weekly_schedule");
  if (w_arr && cJSON_IsArray(w_arr)) {
    weekly_sleep_schedule_t weekly = power_manager_get_weekly();
    int arr_sz = cJSON_GetArraySize(w_arr);
    for (int i = 0; i < arr_sz && i < 7; i++) {
        cJSON *day = cJSON_GetArrayItem(w_arr, i);
        if (!day) continue;
        
        cJSON *en = cJSON_GetObjectItem(day, "enabled");
        if (en) weekly.days[i].enabled = cJSON_IsTrue(en);
        
        cJSON *sh = cJSON_GetObjectItem(day, "start_hour");
        if (sh) weekly.days[i].start_hour = sh->valueint;
        cJSON *sm = cJSON_GetObjectItem(day, "start_min");
        if (sm) weekly.days[i].start_min = sm->valueint;
        cJSON *ss = cJSON_GetObjectItem(day, "start_sec");
        if (ss) weekly.days[i].start_sec = ss->valueint;
        
        cJSON *eh = cJSON_GetObjectItem(day, "end_hour");
        if (eh) weekly.days[i].end_hour = eh->valueint;
        cJSON *em = cJSON_GetObjectItem(day, "end_min");
        if (em) weekly.days[i].end_min = em->valueint;
        cJSON *es = cJSON_GetObjectItem(day, "end_sec");
        if (es) weekly.days[i].end_sec = es->valueint;
    }
    power_manager_set_weekly(weekly);
  }

  cJSON_Delete(root);
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, "{\"status\":\"ok\"}", -1);
}

static const httpd_uri_t uri_power_get = {
    .uri = "/api/power", .method = HTTP_GET, .handler = api_power_get_handler};

static const httpd_uri_t uri_power_post = {
    .uri = "/api/power", .method = HTTP_POST, .handler = api_power_post_handler};

// ========== Language Switcher API ==========
static esp_err_t set_lang_handler(httpd_req_t *req) {
  char query[64] = {0};
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
    char val[16] = {0};
    if (httpd_query_key_value(query, "lang", val, sizeof(val)) == ESP_OK) {
      if (strcmp(val, "fr") == 0) {
        i18n_set_default_lang(LANG_FR);
      } else if (strcmp(val, "en") == 0) {
        i18n_set_default_lang(LANG_EN);
      } else if (strcmp(val, "ar") == 0) {
        i18n_set_default_lang(LANG_AR);
      }
      char cookie_hdr[64];
      snprintf(cookie_hdr, sizeof(cookie_hdr), "lang=%s; Path=/; Max-Age=31536000", val);
      httpd_resp_set_hdr(req, "Set-Cookie", cookie_hdr);
    }
  }
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t backup_page_handler(httpd_req_t *req) {
  if (!is_authorized) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
  }

  app_lang_t lang = i18n_get_req_lang(req);

  httpd_resp_set_type(req, "text/html; charset=utf-8");

  send_conf_header(req, i18n_tr(STR_BACKUP_TITLE, lang), "/", -1);
  i18n_send_lang_bar(req, lang);

  // Navigation menu
  char nav_buf[512];
  snprintf(nav_buf, sizeof(nav_buf),
           "<nav><a href='/'>🏠 %s</a><a href='/settings'>⚙️ %s</a><a href='/logout'>🚪 %s</a></nav>",
           i18n_tr(STR_BACK_TO_DASHBOARD, lang),
           i18n_tr(STR_NAV_SETTINGS, lang),
           i18n_tr(STR_NAV_LOGOUT, lang));
  httpd_resp_sendstr_chunk(req, nav_buf);

  // Main content
  char main_h2[256];
  snprintf(main_h2, sizeof(main_h2), "<main><h2>%s</h2>", i18n_tr(STR_BACKUP_TITLE, lang));
  httpd_resp_sendstr_chunk(req, main_h2);

  // Download section
  httpd_resp_sendstr_chunk(req, "<div class='card'>");
  char dl_h3[256];
  snprintf(dl_h3, sizeof(dl_h3), "<h3>%s</h3>", i18n_tr(STR_BACKUP_DOWNLOAD_TITLE, lang));
  httpd_resp_sendstr_chunk(req, dl_h3);
  char dl_desc[256];
  snprintf(dl_desc, sizeof(dl_desc), "<p>%s</p>", i18n_tr(STR_BACKUP_DOWNLOAD_DESC, lang));
  httpd_resp_sendstr_chunk(req, dl_desc);
  char dl_btn[256];
  snprintf(dl_btn, sizeof(dl_btn),
           "<a href='/backup/download' class='button' download style='background:#27ae60'>%s</a>",
           i18n_tr(STR_BACKUP_DOWNLOAD_BTN, lang));
  httpd_resp_sendstr_chunk(req, dl_btn);
  httpd_resp_sendstr_chunk(req, "</div>");

  // Upload section
  httpd_resp_sendstr_chunk(req, "<div class='card'>");
  char rest_h3[256];
  snprintf(rest_h3, sizeof(rest_h3), "<h3>%s</h3>", i18n_tr(STR_RESTORE_TITLE, lang));
  httpd_resp_sendstr_chunk(req, rest_h3);
  char rest_desc[256];
  snprintf(rest_desc, sizeof(rest_desc), "<p>%s</p>", i18n_tr(STR_RESTORE_DESC, lang));
  httpd_resp_sendstr_chunk(req, rest_desc);
  httpd_resp_sendstr_chunk(req, "<form method='POST' action='/backup/restore' enctype='multipart/form-data'>");
  httpd_resp_sendstr_chunk(req, "<input type='file' name='config' accept='.json' required>");
  char rest_btn[256];
  snprintf(rest_btn, sizeof(rest_btn),
           "<button type='submit' style='background:#e67e22'>%s</button>",
           i18n_tr(STR_RESTORE_BTN, lang));
  httpd_resp_sendstr_chunk(req, rest_btn);
  httpd_resp_sendstr_chunk(req, "</form></div>");

  httpd_resp_sendstr_chunk(req, "</main></body></html>");
  httpd_resp_sendstr_chunk(req, NULL);

  return ESP_OK;
}

// ========== URI Handlers Registration ==========
static const httpd_uri_t uri_root = {
    .uri = "/", .method = HTTP_GET, .handler = root_get_handler};

static const httpd_uri_t uri_login_post = {
    .uri = "/login", .method = HTTP_POST, .handler = login_post_handler};

static const httpd_uri_t uri_logout = {
    .uri = "/logout", .method = HTTP_GET, .handler = logout_handler};

static const httpd_uri_t uri_settings = {
    .uri = "/settings", .method = HTTP_GET, .handler = settings_get_handler};

static const httpd_uri_t uri_save = {
    .uri = "/save", .method = HTTP_POST, .handler = save_post_handler};

static const httpd_uri_t uri_download_schedule = {
    .uri = "/downloadSchedule",
    .method = HTTP_GET,
    .handler = download_schedule_handler};

static const httpd_uri_t uri_upload_schedule = {.uri = "/uploadSchedule",
                                                .method = HTTP_POST,
                                                .handler =
                                                    upload_schedule_handler};

static const httpd_uri_t uri_status = {
    .uri = "/status", .method = HTTP_GET, .handler = status_api_handler};

static const httpd_uri_t uri_time = {
    .uri = "/time", .method = HTTP_GET, .handler = time_api_handler};

// Phase 1 - New URI handlers
static const httpd_uri_t uri_about = {
    .uri = "/about", .method = HTTP_GET, .handler = about_handler};

static const httpd_uri_t uri_restart = {
    .uri = "/restart", .method = HTTP_GET, .handler = restart_handler};

static const httpd_uri_t uri_factory_reset = {.uri = "/factoryReset",
                                              .method = HTTP_GET,
                                              .handler = factory_reset_handler};

static const httpd_uri_t uri_change_password_get = {
    .uri = "/changePassword",
    .method = HTTP_GET,
    .handler = change_password_get_handler};

static const httpd_uri_t uri_change_password_post = {
    .uri = "/changePassword",
    .method = HTTP_POST,
    .handler = change_password_post_handler};

static const httpd_uri_t uri_wifi_config_get = {.uri = "/wifiConfig",
                                                .method = HTTP_GET,
                                                .handler =
                                                    wifi_config_get_handler};

static const httpd_uri_t uri_wifi_config_post = {.uri = "/wifiConfig",
                                                 .method = HTTP_POST,
                                                 .handler =
                                                     wifi_config_post_handler};

static const httpd_uri_t uri_timezone_config_get = {
    .uri = "/timezoneConfig",
    .method = HTTP_GET,
    .handler = timezone_config_get_handler};

static const httpd_uri_t uri_timezone_config_post = {
    .uri = "/timezoneConfig",
    .method = HTTP_POST,
    .handler = timezone_config_post_handler};
static const httpd_uri_t uri_current_config_get = {
    .uri = "/currentConfig",
    .method = HTTP_GET,
    .handler = current_config_get_handler};
static const httpd_uri_t uri_current_config_post = {
    .uri = "/currentConfig",
    .method = HTTP_POST,
    .handler = current_config_post_handler};
static const httpd_uri_t uri_recalibrate_zero = {.uri = "/recalibrateZero",
                                                 .method = HTTP_POST,
                                                 .handler =
                                                     recalibrate_zero_handler};

static const httpd_uri_t uri_update_get = {
    .uri = "/update", .method = HTTP_GET, .handler = update_get_handler};

static const httpd_uri_t uri_update_post = {
    .uri = "/update", .method = HTTP_POST, .handler = update_post_handler};

static const httpd_uri_t uri_logs = {
    .uri = "/logs", .method = HTTP_GET, .handler = logs_handler};

static const httpd_uri_t uri_backup_page = {
    .uri = "/backup", .method = HTTP_GET, .handler = backup_page_handler};

static const httpd_uri_t uri_set_lang = {
    .uri = "/setLang", .method = HTTP_GET, .handler = set_lang_handler};

static const httpd_uri_t uri_backup_download = {.uri = "/backup/download",
                                                .method = HTTP_GET,
                                                .handler =
                                                    backup_download_handler};

static const httpd_uri_t uri_backup_restore = {.uri = "/backup/restore",
                                               .method = HTTP_POST,
                                               .handler =
                                                   backup_restore_handler};

static const httpd_uri_t uri_settings_api_get = {.uri = "/api/settings",
                                                 .method = HTTP_GET,
                                                 .handler =
                                                     settings_api_get_handler};

static const httpd_uri_t uri_settings_api_post = {
    .uri = "/api/settings",
    .method = HTTP_POST,
    .handler = settings_api_post_handler};

// Schedule API - Returns JSON schedule for Android app
static esp_err_t schedule_api_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "Schedule API: Streaming JSON schedule to client...");

  // Send response as JSON API with chunked encoding
  httpd_resp_set_type(req, "application/json");

  // Stream start
  httpd_resp_sendstr_chunk(req, "{\"schedule\":{");

  const char *daysEn[] = {"monday", "tuesday",  "wednesday", "thursday",
                          "friday", "saturday", "sunday"};
  char chunk[256];

  alarm_lock();
  for (int i = 0; i < 7; i++) {
    snprintf(chunk, sizeof(chunk), "\"%s\":[", daysEn[i]);
    httpd_resp_sendstr_chunk(req, chunk);

    bool first = true;
    for (int j = 0; j < MAX_ALARMS_PER_DAY; j++) {
      if (schedule[0][i].startHour[j] != -1 &&
          schedule[0][i].endHour[j] != -1) {
        
        snprintf(chunk, sizeof(chunk),
                 "%s{\"start\":\"%02d:%02d:%02d\",\"end\":\"%02d:%02d:%02d\","
                 "\"endDay\":%d,\"relay\":%d}",
                 first ? "" : ",",
                 schedule[0][i].startHour[j], schedule[0][i].startMinute[j],
                 schedule[0][i].startSecond[j], schedule[0][i].endHour[j],
                 schedule[0][i].endMinute[j], schedule[0][i].endSecond[j],
                 schedule[0][i].endDayOffset[j], schedule[0][i].relayIdx[j]);
                 
        httpd_resp_sendstr_chunk(req, chunk);
        first = false;
      }
    }

    snprintf(chunk, sizeof(chunk), "]%s", (i < 6) ? "," : "");
    httpd_resp_sendstr_chunk(req, chunk);
  }
  alarm_unlock();

  // Stream end
  httpd_resp_sendstr_chunk(req, "}}");

  // Finalize chunked response
  httpd_resp_sendstr_chunk(req, NULL);

  ESP_LOGI(TAG, "Schedule API: JSON stream completed successfully");
  return ESP_OK;
}

// ========== Manual RTC Set Handler ==========
static esp_err_t set_datetime_handler(httpd_req_t *req) {
  char content[256];
  int received = httpd_req_recv(req, content, sizeof(content) - 1);
  if (received <= 0)
    return ESP_FAIL;
  content[received] = '\0';

  cJSON *root = cJSON_Parse(content);
  cJSON *item;
  int year =
      ((item = cJSON_GetObjectItem(root, "year")) && cJSON_IsNumber(item))
          ? item->valueint
          : 2026;
  int month =
      ((item = cJSON_GetObjectItem(root, "month")) && cJSON_IsNumber(item))
          ? item->valueint
          : 1;
  int day = ((item = cJSON_GetObjectItem(root, "day")) && cJSON_IsNumber(item))
                ? item->valueint
                : 1;
  int hour =
      ((item = cJSON_GetObjectItem(root, "hour")) && cJSON_IsNumber(item))
          ? item->valueint
          : 0;
  int minute =
      ((item = cJSON_GetObjectItem(root, "minute")) && cJSON_IsNumber(item))
          ? item->valueint
          : 0;
  int second =
      ((item = cJSON_GetObjectItem(root, "second")) && cJSON_IsNumber(item))
          ? item->valueint
          : 0;

  // Si le client transmet le décalage horaire de son appareil (new Date().getTimezoneOffset())
  cJSON *tz_item = cJSON_GetObjectItem(root, "tz_offset");
  if (tz_item && cJSON_IsNumber(tz_item)) {
    int offset = tz_item->valueint; // 0 = GMT0, -60 = GMT+1
    const char *target_tz = NULL;
    if (offset == 0) {
      target_tz = "GMT0";
    } else if (offset == -60) {
      target_tz = "CET-1";
    } else if (offset == -120) {
      target_tz = "EET-2";
    }
    if (target_tz) {
      nvs_write_string("timezone", target_tz);
      setenv("TZ", target_tz, 1);
      tzset();
      ESP_LOGI(TAG, "Timezone synced with client device (offset %d min -> %s)", offset, target_tz);
    }
  }

  cJSON_Delete(root);

  struct tm tm = {0};
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;
  tm.tm_isdst = -1; // Auto-determine DST base on selected timezone

  time_t t = mktime(&tm);
  struct timeval tv = {.tv_sec = t, .tv_usec = 0};
  settimeofday(&tv, NULL);

  extern void ds3231_sync_from_system(void);
  ds3231_sync_from_system();

  extern bool g_is_time_synced;
  g_is_time_synced = true;

  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
  ESP_LOGI("HTTP_SRV", "Manual time set: %04d-%02d-%02d %02d:%02d:%02d", year, month,
           day, hour, minute, second);
  return ESP_OK;
}

// Handler for WiFi Scan
static esp_err_t wifi_scan_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "Starting WiFi Scan via API...");
  uint16_t number = 15;
  wifi_ap_record_t ap_info[15];
  uint16_t ap_count = 0;
  memset(ap_info, 0, sizeof(ap_info));

  esp_err_t err = esp_wifi_scan_start(NULL, true);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "WiFi Scan failed: %s", esp_err_to_name(err));
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Scan failed");
    return ESP_FAIL;
  }

  esp_wifi_scan_get_ap_records(&number, ap_info);
  esp_wifi_scan_get_ap_num(&ap_count);
  ESP_LOGI(TAG, "Scan detected %d networks", ap_count);

  cJSON *root = cJSON_CreateObject();
  cJSON *networks = cJSON_CreateArray();
  cJSON_AddItemToObject(root, "networks", networks);

  for (int i = 0; i < number; i++) {
    if (strlen((char *)ap_info[i].ssid) == 0)
      continue;
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "ssid", (char *)ap_info[i].ssid);
    cJSON_AddNumberToObject(item, "rssi", ap_info[i].rssi);
    cJSON_AddNumberToObject(item, "authmode", ap_info[i].authmode);
    cJSON_AddItemToArray(networks, item);
  }

  const char *json_response = cJSON_Print(root);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, json_response);

  free((void *)json_response);
  cJSON_Delete(root);

  return ESP_OK;
}

static const httpd_uri_t uri_wifi_scan = {
    .uri = "/wifi_scan", .method = HTTP_GET, .handler = wifi_scan_handler};

static const httpd_uri_t uri_set_datetime = {.uri = "/setDateTime",
                                             .method = HTTP_POST,
                                             .handler = set_datetime_handler};

static const httpd_uri_t uri_schedule_api = {.uri = "/api/schedule",
                                             .method = HTTP_GET,
                                             .handler = schedule_api_handler};

// ===================================
// ESP-CAM PROXY HANDLERS
// ===================================
static char last_proxy_error[256] = "Aucune erreur";

esp_err_t check_proxy_status_handler(httpd_req_t *req) {
  char resp[300];
  snprintf(resp, sizeof(resp), "{\"last_error\":\"%s\"}", last_proxy_error);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, resp);
  return ESP_OK;
}

esp_err_t save_cam_ip_handler(httpd_req_t *req) {
  char buf[100];
  int ret, remaining = req->content_len;
  if (remaining >= sizeof(buf)) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  if ((ret = httpd_req_recv(req, buf, remaining)) <= 0) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  cJSON *json = cJSON_Parse(buf);
  if (json != NULL) {
    cJSON *ip = cJSON_GetObjectItem(json, "ip");
    if (ip && ip->valuestring) {
      nvs_write_string("cam_ip", ip->valuestring);
      httpd_resp_set_type(req, "application/json");
      httpd_resp_sendstr(req, "{\"success\":true}");
      cJSON_Delete(json);
      return ESP_OK;
    }
    cJSON_Delete(json);
  }
  httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
  return ESP_FAIL;
}

esp_err_t debug_cam_handler(httpd_req_t *req) {
  esp_ip4_addr_t addr;
  addr.addr = 0;
  char response[300];
  char test_ip[20] = {0};
  bool ip_found = false;
  char source[10] = "";

  ESP_LOGI(TAG, "Debug: Resolving esp32-cam via mDNS...");
  esp_err_t err = mdns_query_a("esp32-cam", 1500, &addr);
  if (err != ESP_OK) {
    err = mdns_query_a("esp32-cam.local", 1500, &addr);
  }

  if (err == ESP_OK) {
    snprintf(test_ip, sizeof(test_ip), IPSTR, IP2STR(&addr));
    strcpy(source, "mDNS");
    ip_found = true;
    nvs_write_string("cam_ip", test_ip);
    ESP_LOGI(TAG, "Debug: Camera found at %s. Saved to NVS.", test_ip);
  } else {
    esp_err_t nvs_err = nvs_read_string("cam_ip", test_ip, sizeof(test_ip));
    if (nvs_err == ESP_OK && strlen(test_ip) > 0) {
      strcpy(source, "NVS");
      ip_found = true;
    }
  }

  if (ip_found) {
    char test_url[64];
    snprintf(test_url, sizeof(test_url), "http://%s/led/status", test_ip);
    esp_http_client_config_t config = {
        .url = test_url,
        .timeout_ms = 2000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t http_err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (http_err == ESP_OK) {
      snprintf(
          response, sizeof(response),
          "{\"success\":true,\"ip\":\"%s\",\"msg\":\"Camera en ligne via %s\"}",
          test_ip, source);
    } else {
      snprintf(response, sizeof(response),
               "{\"success\":false,\"msg\":\"IP %s trouvée (%s) mais camera "
               "injoignable (Hors ligne?)\"}",
               test_ip, source);
    }
  } else {
    snprintf(response, sizeof(response),
             "{\"success\":false,\"msg\":\"Camera introuvable. mDNS failed + "
             "pas d'IP configuree.\"}");
  }

  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, response);
  return ESP_OK;
}

esp_err_t cam_proxy_handler(httpd_req_t *req) {
  esp_ip4_addr_t addr;
  addr.addr = 0;
  bool ip_found = false;
  char target_ip[20] = {0};

  esp_err_t err = mdns_query_a("esp32-cam", 1500, &addr);
  if (err != ESP_OK) {
    err = mdns_query_a("esp32-cam.local", 1500, &addr);
  }

  if (err == ESP_OK) {
    snprintf(target_ip, sizeof(target_ip), IPSTR, IP2STR(&addr));
    ip_found = true;
  } else {
    err = nvs_read_string("cam_ip", target_ip, sizeof(target_ip));
    if (err == ESP_OK && strlen(target_ip) > 0) {
      ip_found = true;
    }
  }

  if (!ip_found) {
    ESP_LOGE(TAG, "Proxy: No IP available (mDNS + NVS failed)");
    snprintf(last_proxy_error, sizeof(last_proxy_error),
             "Err: Camera introuvable (mDNS+NVS failed)");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, "502 Bad Gateway");
    httpd_resp_sendstr(req,
                       "{\"error\":\"Camera introuvable. Cliquez Test 🔍.\"}");
    return ESP_OK;
  }

  const char *subpath = req->uri + strlen("/cam_proxy");
  char target_url[128];
  snprintf(target_url, sizeof(target_url), "http://%s%s", target_ip, subpath);
  ESP_LOGI(TAG, "Proxying request to: %s", target_url);

  esp_http_client_config_t config = {
      .url = target_url,
      .timeout_ms = 4000,
      .buffer_size_tx = 1024,
      .user_data = NULL // Useful for perform if needed
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);

  if (req->method == HTTP_POST) {
    esp_http_client_set_method(client, HTTP_METHOD_POST);
  } else {
    esp_http_client_set_method(client, HTTP_METHOD_GET);
  }

  if (req->method == HTTP_GET && strstr(subpath, "/sd/download") != NULL) {
    // Chunked mode for potential large files
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Proxy: Failed to open connect for download: %s",
               esp_err_to_name(err));
      httpd_resp_set_status(req, "502 Bad Gateway");
      httpd_resp_sendstr(req, "Camera unreachable for file");
      esp_http_client_cleanup(client);
      return ESP_OK;
    }
    esp_http_client_fetch_headers(client);
    char *content_dis = NULL;
    esp_http_client_get_header(client, "Content-Disposition", &content_dis);
    if (content_dis != NULL) {
      httpd_resp_set_hdr(req, "Content-Disposition", content_dis);
    }
    httpd_resp_set_type(req, "application/octet-stream");
    char stream_buf[1024];
    int read_len;
    while ((read_len = esp_http_client_read(client, stream_buf,
                                            sizeof(stream_buf))) > 0) {
      httpd_resp_send_chunk(req, stream_buf, read_len);
    }
    httpd_resp_send_chunk(req, NULL, 0);
  } else {
    // API call mode: Use perform for robustness
    char api_resp_buf[1024] = {0};
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
      int status_code = esp_http_client_get_status_code(client);
      int actual_read = esp_http_client_read_response(client, api_resp_buf,
                                                      sizeof(api_resp_buf) - 1);

      if (actual_read >= 0) {
        api_resp_buf[actual_read] = '\0';
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, api_resp_buf);
        snprintf(last_proxy_error, sizeof(last_proxy_error), "Succès (%d)",
                 status_code);
      } else {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\":\"Empty response from camera\"}");
      }
    } else {
      ESP_LOGE(TAG, "Proxy: esp_http_client_perform failed: %s",
               esp_err_to_name(err));
      snprintf(last_proxy_error, sizeof(last_proxy_error), "Perform error: %s",
               esp_err_to_name(err));
      httpd_resp_set_type(req, "application/json");
      httpd_resp_set_status(req, "502 Bad Gateway");
      httpd_resp_sendstr(req, "{\"error\":\"Camera Communication Error\"}");
    }
  }

  esp_http_client_cleanup(client);
  return ESP_OK;
}

static const httpd_uri_t uri_cam_proxy = {
    .uri = "/cam_proxy/*", .method = HTTP_GET, .handler = cam_proxy_handler};
static const httpd_uri_t uri_cam_proxy_post = {
    .uri = "/cam_proxy/*", .method = HTTP_POST, .handler = cam_proxy_handler};
static const httpd_uri_t uri_proxy_status = {.uri = "/api/proxy_status",
                                             .method = HTTP_GET,
                                             .handler =
                                                 check_proxy_status_handler};
static const httpd_uri_t uri_save_cam_ip = {
    .uri = "/api/cam_ip", .method = HTTP_POST, .handler = save_cam_ip_handler};
static const httpd_uri_t uri_debug_cam = {
    .uri = "/api/debug_cam", .method = HTTP_GET, .handler = debug_cam_handler};

// ========== Server Start/Stop ==========

// Register common handlers for both HTTP and HTTPS
void register_common_handlers(httpd_handle_t server_handle) {
  // Common handlers
  httpd_register_uri_handler(server_handle, &uri_root);
  httpd_register_uri_handler(server_handle, &uri_login_post);
  httpd_register_uri_handler(server_handle, &uri_logout);
  httpd_register_uri_handler(server_handle, &uri_settings);
  httpd_register_uri_handler(server_handle, &uri_save);
  httpd_register_uri_handler(server_handle, &uri_download_schedule);
  httpd_register_uri_handler(server_handle, &uri_upload_schedule);
  httpd_register_uri_handler(server_handle, &uri_status);
  httpd_register_uri_handler(server_handle, &uri_time);

  // Phase 1 - New handlers
  httpd_register_uri_handler(server_handle, &uri_about);
  httpd_register_uri_handler(server_handle, &uri_restart);
  httpd_register_uri_handler(server_handle, &uri_factory_reset);
  httpd_register_uri_handler(server_handle, &uri_change_password_get);
  httpd_register_uri_handler(server_handle, &uri_change_password_post);
  httpd_register_uri_handler(server_handle, &uri_wifi_config_get);
  httpd_register_uri_handler(server_handle, &uri_wifi_config_post);
  httpd_register_uri_handler(server_handle, &uri_timezone_config_get);
  httpd_register_uri_handler(server_handle, &uri_timezone_config_post);
  httpd_register_uri_handler(server_handle, &uri_current_config_get);

  // API Endpoints for Android App
  httpd_register_uri_handler(server_handle, &uri_schedule_api);
  httpd_register_uri_handler(server_handle, &uri_current_config_post);
  httpd_register_uri_handler(server_handle, &uri_recalibrate_zero);

  // OTA Update handlers
  httpd_register_uri_handler(server_handle, &uri_update_get);
  httpd_register_uri_handler(server_handle, &uri_update_post);

  // Logs download handler
  httpd_register_uri_handler(server_handle, &uri_logs);

  // Backup/restore handlers
  httpd_register_uri_handler(server_handle, &uri_backup_page);
  httpd_register_uri_handler(server_handle, &uri_backup_download);
  httpd_register_uri_handler(server_handle, &uri_backup_restore);
  httpd_register_uri_handler(server_handle, &uri_set_lang);
  httpd_register_uri_handler(server_handle, &uri_api_health);
  httpd_register_uri_handler(server_handle, &uri_power_get);
  httpd_register_uri_handler(server_handle, &uri_power_post);

  // Settings API
  httpd_register_uri_handler(server_handle, &uri_settings_api_get);
  httpd_register_uri_handler(server_handle, &uri_settings_api_post);

  // Manual RTC
  httpd_register_uri_handler(server_handle, &uri_set_datetime);

  // WiFi Scan API
  httpd_register_uri_handler(server_handle, &uri_wifi_scan);

  // Camera Proxy API
  httpd_register_uri_handler(server_handle, &uri_cam_proxy);
  httpd_register_uri_handler(server_handle, &uri_cam_proxy_post);
  httpd_register_uri_handler(server_handle, &uri_proxy_status);
  httpd_register_uri_handler(server_handle, &uri_save_cam_ip);
  httpd_register_uri_handler(server_handle, &uri_debug_cam);
}

esp_err_t start_http_server(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;
  config.max_uri_handlers = 60;   // Increased further to avoid "no slots left"
  config.lru_purge_enable = true; // Crucial to avoid 'accept 23' error
  config.stack_size = 10240;      // More stack for large form processing
  config.uri_match_fn =
      httpd_uri_match_wildcard; // REQUIRED for /cam_proxy/* matching

  // NEW AGGRESSIVE SOCKET CLEANUP SETTINGS FOR AP MODE (Restored from previous fix)
  config.keep_alive_enable = true;         // Keep-Alive must be TRUE otherwise ESP gets stuck in TIME_WAIT
  // Keep regular timeouts to prevent rapid connections drops
  // config.recv_wait_timeout = 5; 
  // config.send_wait_timeout = 5;
  config.max_open_sockets = 10;            // Increase capacity to handle browsers (max allowed usually 10-12)

  ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);

  esp_err_t ret = httpd_start(&server, &config);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return ret;
  }

  // Register URI handlers
  register_common_handlers(server);

  ESP_LOGI(TAG, "HTTP server started successfully");
  return ESP_OK;
}

void http_server_stop(void) {
  if (server) {
    httpd_stop(server);
    server = NULL;
    ESP_LOGI(TAG, "HTTP server stopped");
  }
}

bool http_is_authorized(void) { return is_authorized; }

void http_set_authorized(bool authorized) { is_authorized = authorized; }
