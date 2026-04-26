/**
 * @file Manual RTC Set Handler
 * @brief Add to http_server.c - Endpoint pour régler manuellement le RTC
 * 
 * INSTRUCTIONS:
 * 1. Ajouter cette fonction dans http_server.c
 * 2. Enregistrer l'endpoint dans start_http_server()
 * 3. Ajouter formulaire HTML dans la page principale
 */

// ========== ADD TO http_server.c (after other handlers) ==========

/**
 * @brief Handler POST /setDateTime - Set RTC manually
 * 
 * Expected POST body:
 * {
 *   "year": 2025,
 *   "month": 12,
 *   "day": 17,
 *   "hour": 22,
 *   "minute": 30,
 *   "second": 0
 * }
 */
static esp_err_t set_datetime_handler(httpd_req_t *req) {
    char content[256];
    int received = httpd_req_recv(req, content, sizeof(content) - 1);
    
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data received");
        return ESP_FAIL;
    }
    
    content[received] = '\0';
    
    // Parse JSON simple (chercher les valeurs)
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    
    // Simple parsing (pas de cJSON pour économiser RAM)
    if (sscanf(content, "{\"year\":%d,\"month\":%d,\"day\":%d,\"hour\":%d,\"minute\":%d,\"second\":%d}",
               &year, &month, &day, &hour, &minute, &second) != 6) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        return ESP_FAIL;
    }
    
    // Validation
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || 
        day < 1 || day > 31 || hour < 0 || hour > 23 || 
        minute < 0 || minute > 59 || second < 0 || second > 59) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid date/time values");
        return ESP_FAIL;
    }
    
    // Set system time first
    struct tm timeinfo = {
        .tm_year = year - 1900,
        .tm_mon = month - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min = minute,
        .tm_sec = second,
        .tm_isdst = -1
    };
    
    time_t t = mktime(&timeinfo);
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    
    ESP_LOGI("HTTP_SRV", "System time set manually: %04d-%02d-%02d %02d:%02d:%02d",
             year, month, day, hour, minute, second);
    
    // Sync to RTC
    extern void ds3231_sync_from_system(void);
    ds3231_sync_from_system();
    
    ESP_LOGI("HTTP_SRV", "RTC synchronized with manual time");
    
    // Mark time as synced
    extern bool g_is_time_synced;
    g_is_time_synced = true;
    
    // Send response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\",\"message\":\"Time set successfully\"}");
    
    return ESP_OK;
}

// ========== ADD TO URI REGISTRATION ==========
static const httpd_uri_t uri_set_datetime = {
    .uri = "/setDateTime",
    .method = HTTP_POST,
    .handler = set_datetime_handler
};

// Dans start_http_server(), ajouter:
httpd_register_uri_handler(server_handle, &uri_set_datetime);


// ========== HTML FORM TO ADD IN DASHBOARD ==========
/*
<div class="card" style="margin:20px 0;">
  <h3>⏰ Réglage Manuel Heure/Date</h3>
  <form id="timeForm" style="display:grid;gap:10px;">
    <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;">
      <input type="number" id="year" placeholder="Année" min="2000" max="2099" value="2025" required>
      <input type="number" id="month" placeholder="Mois" min="1" max="12" required>
      <input type="number" id="day" placeholder="Jour" min="1" max="31" required>
    </div>
    <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;">
      <input type="number" id="hour" placeholder="Heure" min="0" max="23" required>
      <input type="number" id="minute" placeholder="Minute" min="0" max="59" required>
      <input type="number" id="second" placeholder="Seconde" min="0" max="59" value="0" required>
    </div>
    <button type="submit" style="background:#10B981;color:white;padding:12px;border:none;border-radius:8px;font-weight:600;cursor:pointer;">
      Définir l'heure
    </button>
  </form>
  <div id="timeResult" style="margin-top:10px;"></div>
</div>

<script>
document.getElementById('timeForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  
  const data = {
    year: parseInt(document.getElementById('year').value),
    month: parseInt(document.getElementById('month').value),
    day: parseInt(document.getElementById('day').value),
    hour: parseInt(document.getElementById('hour').value),
    minute: parseInt(document.getElementById('minute').value),
    second: parseInt(document.getElementById('second').value)
  };
  
  try {
    const response = await fetch('/setDateTime', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(data)
    });
    
    const result = await response.json();
    
    const resultDiv = document.getElementById('timeResult');
    if (response.ok) {
      resultDiv.innerHTML = '<div style="background:#10B981;color:white;padding:10px;border-radius:4px;">✅ ' + result.message + '</div>';
      setTimeout(() => location.reload(), 2000);
    } else {
      resultDiv.innerHTML = '<div style="background:#EF4444;color:white;padding:10px;border-radius:4px;">❌ Error: ' + result.message + '</div>';
    }
  } catch (err) {
    document.getElementById('timeResult').innerHTML = '<div style="background:#EF4444;color:white;padding:10px;border-radius:4px;">❌ Network error</div>';
  }
});

// Auto-fill with current browser time
const now = new Date();
document.getElementById('year').value = now.getFullYear();
document.getElementById('month').value = now.getMonth() + 1;
document.getElementById('day').value = now.getDate();
document.getElementById('hour').value = now.getHours();
document.getElementById('minute').value = now.getMinutes();
document.getElementById('second').value = now.getSeconds();
</script>
*/
