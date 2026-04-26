/*
 * ESP-CAM FIRMWARE - GUARDIAN V1 (PRODUCTION)
 * Features:
 * 1. WiFi Streaming (MJPEG)
 * 2. Motion Detection (Pixel Diff)
 * 3. SD Recording (AVI/MJPEG)
 * 4. External Watchdog (Heartbeat Monitor)
 */

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <time.h>

// ===================================
// PINS & CONFIG
// ===================================
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27
#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

// WATCHDOG CONFIG
#define PIN_HEARTBEAT_IN GPIO_NUM_12 // Input from Primary ESP
#define PIN_RESET_OUT GPIO_NUM_33    // Output to Primary ESP (Active LOW)
#define WATCHDOG_TIMEOUT_MS 15000    // 15s timeout

// WIFI CONFIG (Hardcoded)
#define ESP_WIFI_SSID "HUAWEI-2.4G"
#define ESP_WIFI_PASS "ARIMAF04arimaf2"

// MOTION & RECORDING
#define MOTION_THRESHOLD 40    // Sensitivity (0-255)
#define MIN_PIXELS_CHANGED 500 // How many pixels must change
#define RECORD_TIME_SEC 10     // Duration to record after motion

static const char *TAG = "ESP_CAM_PRO";
static int64_t last_heartbeat_time = 0;
static httpd_handle_t stream_httpd = NULL;
static bool s_sd_card_mounted = false;
static bool s_is_recording = false;

// LED CONTROL
#define LED_PIN GPIO_NUM_4
// Pour images sombres: JPEG plus petit (moins de détails)
// VGA (640x480) lumineux ≈ 15-30KB, sombre ≈ 5-12KB
#define JPEG_SIZE_DARK_THRESHOLD 13000 // octets
#define LED_TIMEOUT_MS 15000           // 15s après dernier mouvement
static bool led_auto_mode = true;      // true = auto, false = manual
static bool led_manual_state = false;  // État si mode manuel
static int64_t last_motion_time = 0;   // Timestamp dernier mouvement

// ===================================
// AVI VIDEO WRAPPER STRUCTURES
// ===================================
typedef struct {
  uint32_t fcc;
  uint32_t cb;
  uint32_t fccType;
  uint32_t fccHandler;
  uint32_t dwFlags;
  uint32_t dwCaps;
  uint16_t wPriority;
  uint16_t wLanguage;
  uint32_t dwScale;
  uint32_t dwRate;
  uint32_t dwStart;
  uint32_t dwLength;
  uint32_t dwInitialFrames;
  uint32_t dwSuggestedBufferSize;
  uint32_t dwQuality;
  uint32_t dwSampleSize;
  struct {
    uint16_t left;
    uint16_t top;
    uint16_t right;
    uint16_t bottom;
  } rcFrame;
} avi_stream_header_t;

typedef struct {
  uint32_t fcc;
  uint32_t cb;
  uint32_t dwMicroSecPerFrame;
  uint32_t dwMaxBytesPerSec;
  uint32_t dwPaddingGranularity;
  uint32_t dwFlags;
  uint32_t dwTotalFrames;
  uint32_t dwInitialFrames;
  uint32_t dwStreams;
  uint32_t dwSuggestedBufferSize;
  uint32_t dwWidth;
  uint32_t dwHeight;
  uint32_t dwReserved[4];
} avi_main_header_t;

typedef struct {
  uint32_t biSize;
  uint32_t biWidth;
  uint32_t biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  uint32_t biXPelsPerMeter;
  uint32_t biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
} bitmap_info_header_t;

#define AVIF_HASINDEX 0x00000010
#define AVIF_MUSTUSEINDEX 0x00000020
#define AVIF_ISINTERLEAVED 0x00000100
#define AVIF_TRUSTCKTYPE 0x00000800
#define AVIF_WASCAPTUREFILE 0x00010000
#define AVIF_COPYRIGHTED 0x00020000
#define AVIIF_KEYFRAME 0x00000010

typedef struct {
  uint32_t fcc;
  uint32_t dwFlags;
  uint32_t dwOffset; // Relative to 'movi' list content
  uint32_t dwSize;
} avi_idx1_entry_t;

#define AVI_CHUNK_ID(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

// ===================================
// TIME & SNTP
// ===================================
void init_sntp() {
  ESP_LOGI(TAG, "Initializing SNTP");
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_init();

  // Set timezone to CET (Central European Time) - Adjust as needed
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
}

// ===================================
// WATCHDOG (HEARTBEAT)
// ===================================
static void IRAM_ATTR heartbeat_isr_handler(void *arg) {
  last_heartbeat_time = esp_timer_get_time() / 1000;
}

void init_watchdog() {
  gpio_config_t conf = {.intr_type = GPIO_INTR_ANYEDGE,
                        .mode = GPIO_MODE_INPUT,
                        .pin_bit_mask = (1ULL << PIN_HEARTBEAT_IN),
                        .pull_up_en = 1};
  gpio_config(&conf);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(PIN_HEARTBEAT_IN, heartbeat_isr_handler, NULL);

  // Reset Pin (Connected to EN of Primary ESP)
  gpio_config_t rst_conf = {.mode = GPIO_MODE_OUTPUT,
                            .pin_bit_mask = (1ULL << PIN_RESET_OUT),
                            .pull_down_en = 0,
                            .pull_up_en = 0};
  gpio_config(&rst_conf);
  gpio_set_level(PIN_RESET_OUT, 1); // Default HIGH (Don't Reset)

  last_heartbeat_time = esp_timer_get_time() / 1000;
}

void watchdog_task(void *pv) {
  ESP_LOGI(TAG, "Watchdog Task Started");
  while (1) {
    int64_t now = esp_timer_get_time() / 1000;
    if (now - last_heartbeat_time > WATCHDOG_TIMEOUT_MS) {
      ESP_LOGE(TAG, "Watchdog Timeout! Resetting Primary ESP...");

      // Pulse LOW to Reset
      gpio_set_level(PIN_RESET_OUT, 0);
      vTaskDelay(pdMS_TO_TICKS(500));
      gpio_set_level(PIN_RESET_OUT, 1);

      // Wait for system to reboot
      last_heartbeat_time = (esp_timer_get_time() / 1000) + 30000;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ===================================
// SD RECORDING HELPER
// ===================================
// Nettoyage automatique: Supprime les fichiers les plus anciens si l'espace <
// 50Mo
void check_and_cleanup_sd() {
  uint64_t total_bytes = 0;
  uint64_t free_bytes = 0;
  if (esp_vfs_fat_info("/sdcard", &total_bytes, &free_bytes) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get SD card status (esp_vfs_fat_info)");
    return;
  }

  uint64_t threshold = 50ULL * 1024 * 1024; // 50 Mo

  if (free_bytes < threshold) {
    ESP_LOGW(TAG, "Low SD space: %llu MB. Cleaning up...",
             free_bytes / (1024 * 1024));

    DIR *dir = opendir("/sdcard");
    if (!dir)
      return;

    // Approche simple: on liste et on supprime le premier fichier .mjpeg ou
    // .avi trouvé Pour faire mieux, il faudrait trier par date, mais readdir
    // n'est pas trié. Ici on supprime par blocs de 5 fichiers max à chaque
    // appel pour pas bloquer trop longtemps.
    struct dirent *entry;
    int deleted = 0;
    while ((entry = readdir(dir)) != NULL && deleted < 5) {
      if (entry->d_type == DT_REG &&
          (strstr(entry->d_name, ".mjpeg") || strstr(entry->d_name, ".avi"))) {
        char path[300];
        snprintf(path, sizeof(path), "/sdcard/%s", entry->d_name);
        if (unlink(path) == 0) {
          ESP_LOGI(TAG, "Deleted old record: %s", entry->d_name);
          deleted++;
        }
      }
    }
    closedir(dir);

    if (deleted == 0) {
      ESP_LOGE(TAG, "SD Full but no recordings found to delete!");
    }
  } else {
    ESP_LOGI(TAG, "SD Space OK: %llu MB free", free_bytes / (1024 * 1024));
  }
}

void write_avi_header(FILE *f, int frames, int width, int height) {
  uint32_t fcc_riff = AVI_CHUNK_ID('R', 'I', 'F', 'F');
  uint32_t fcc_avi = AVI_CHUNK_ID('A', 'V', 'I', ' ');
  uint32_t fcc_list = AVI_CHUNK_ID('L', 'I', 'S', 'T');
  uint32_t fcc_hdrl = AVI_CHUNK_ID('h', 'd', 'r', 'l');
  uint32_t fcc_avih = AVI_CHUNK_ID('a', 'v', 'i', 'h');
  uint32_t fcc_strl = AVI_CHUNK_ID('s', 't', 'r', 'l');
  uint32_t fcc_strh = AVI_CHUNK_ID('s', 't', 'r', 'h');
  uint32_t fcc_vids = AVI_CHUNK_ID('v', 'i', 'd', 's');
  uint32_t fcc_mjpg = AVI_CHUNK_ID('M', 'J', 'P', 'G');
  uint32_t fcc_strf = AVI_CHUNK_ID('s', 't', 'r', 'f');
  uint32_t fcc_movi = AVI_CHUNK_ID('m', 'o', 'v', 'i');

  avi_main_header_t avih = {.fcc = fcc_avih,
                            .cb = sizeof(avi_main_header_t) - 8,
                            .dwMicroSecPerFrame = 100000,
                            .dwMaxBytesPerSec = 0,
                            .dwPaddingGranularity = 0,
                            .dwFlags = AVIF_HASINDEX | AVIF_TRUSTCKTYPE,
                            .dwTotalFrames = frames,
                            .dwInitialFrames = 0,
                            .dwStreams = 1,
                            .dwSuggestedBufferSize = 64 * 1024,
                            .dwWidth = width,
                            .dwHeight = height,
                            .dwReserved = {0}};

  avi_stream_header_t strh = {
      .fcc = fcc_strh,
      .cb = sizeof(avi_stream_header_t) - 8,
      .fccType = fcc_vids,
      .fccHandler = fcc_mjpg,
      .dwFlags = 0,
      .dwCaps = 0,
      .wPriority = 0,
      .wLanguage = 0,
      .dwScale = 1,
      .dwRate = 10,
      .dwStart = 0,
      .dwLength = frames,
      .dwInitialFrames = 0,
      .dwSuggestedBufferSize = 64 * 1024,
      .dwQuality = 10000,
      .dwSampleSize = 0,
      .rcFrame = {0, 0, (uint16_t)width, (uint16_t)height}};

  bitmap_info_header_t strf = {.biSize = sizeof(bitmap_info_header_t),
                               .biWidth = width,
                               .biHeight = height,
                               .biPlanes = 1,
                               .biBitCount = 24,
                               .biCompression = fcc_mjpg,
                               .biSizeImage = width * height * 3,
                               .biXPelsPerMeter = 0,
                               .biYPelsPerMeter = 0,
                               .biClrUsed = 0,
                               .biClrImportant = 0};

  fseek(f, 0, SEEK_SET);
  fwrite(&fcc_riff, 4, 1, f);
  uint32_t riff_placeholder = 0;
  fwrite(&riff_placeholder, 4, 1, f);
  fwrite(&fcc_avi, 4, 1, f);

  fwrite(&fcc_list, 4, 1, f);
  uint32_t hdrl_size = sizeof(avi_main_header_t) + sizeof(avi_stream_header_t) +
                       sizeof(bitmap_info_header_t) + 36;
  fwrite(&hdrl_size, 4, 1, f);
  fwrite(&fcc_hdrl, 4, 1, f);

  fwrite(&avih, sizeof(avih), 1, f);

  fwrite(&fcc_list, 4, 1, f);
  uint32_t strl_size =
      sizeof(avi_stream_header_t) + sizeof(bitmap_info_header_t) + 12;
  fwrite(&strl_size, 4, 1, f);
  fwrite(&fcc_strl, 4, 1, f);

  fwrite(&strh, sizeof(strh), 1, f);

  fwrite(&fcc_strf, 4, 1, f);
  uint32_t strf_size = sizeof(strf);
  fwrite(&strf_size, 4, 1, f);
  fwrite(&strf, sizeof(strf), 1, f);

  fwrite(&fcc_list, 4, 1, f);
  uint32_t movi_placeholder = 0;
  fwrite(&movi_placeholder, 4, 1, f);
  fwrite(&fcc_movi, 4, 1, f);
}

void finalize_avi_file(FILE *f, int frames, avi_idx1_entry_t *idx,
                       int idx_count) {
  long movi_end = ftell(f);

  // Write Index
  uint32_t fcc_idx1 = AVI_CHUNK_ID('i', 'd', 'x', '1');
  uint32_t idx1_size = idx_count * sizeof(avi_idx1_entry_t);
  fwrite(&fcc_idx1, 4, 1, f);
  fwrite(&idx1_size, 4, 1, f);
  fwrite(idx, sizeof(avi_idx1_entry_t), idx_count, f);

  long file_size = ftell(f);

  // Update RIFF size
  uint32_t riff_size = (uint32_t)file_size - 8;
  fseek(f, 4, SEEK_SET);
  fwrite(&riff_size, 4, 1, f);

  // Update movi LIST size
  // movi LIST starts at offset 212 (8 bytes for LIST + size, 4 bytes for 'movi'
  // tag) The size in the LIST chunk is the size of everything following the
  // size field until movi_end
  uint32_t movi_list_size = (uint32_t)movi_end - 212 - 8;
  fseek(f, 216, SEEK_SET);
  fwrite(&movi_list_size, 4, 1, f);

  // Update total frames in headers
  fseek(f, 48, SEEK_SET); // dwTotalFrames in avih
  fwrite(&frames, 4, 1, f);
  fseek(f, 140, SEEK_SET); // dwLength in strh
  fwrite(&frames, 4, 1, f);
}

void save_video_segment() {
  if (!s_sd_card_mounted) {
    ESP_LOGW(TAG, "Motion detected but No SD Card!");
    return;
  }

  check_and_cleanup_sd();
  s_is_recording = true;

  // Generate Filename based on date
  char fname[128];
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  if (timeinfo.tm_year < (2020 - 1900)) {
    // Time not synced yet, fallback to ms
    snprintf(fname, sizeof(fname), "/sdcard/rec_%lld.avi",
             esp_timer_get_time() / 1000);
  } else {
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%Y%m%d_%H%M%S", &timeinfo);
    snprintf(fname, sizeof(fname), "/sdcard/rec_%s.avi", strftime_buf);
  }

  FILE *f = fopen(fname, "wb");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open %s for writing: %s (errno %d)", fname,
             strerror(errno), errno);
    s_is_recording = false;
    return;
  }

  ESP_LOGI(TAG, "Start Recording AVI: %s", fname);

  // Reserve space for AVI header (approx 240 bytes)
  uint8_t dummy[240] = {0};
  fwrite(dummy, 1, 240, f);

  int64_t start_time = esp_timer_get_time() / 1000;
  int frames = 0;
  uint32_t fcc_00dc = AVI_CHUNK_ID('0', '0', 'd', 'c');

  // Index array to track frames
  int max_frames = RECORD_TIME_SEC * 15; // Buffer for up to 15fps
  avi_idx1_entry_t *index = malloc(max_frames * sizeof(avi_idx1_entry_t));
  if (!index) {
    ESP_LOGE(TAG, "Failed to allocate AVI index");
    fclose(f);
    s_is_recording = false;
    return;
  }

  while ((esp_timer_get_time() / 1000) - start_time <
             (RECORD_TIME_SEC * 1000) &&
         frames < max_frames) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    long current_pos = ftell(f);
    // dwOffset is relative to 'movi' tag (which starts at 220)
    index[frames].fcc = fcc_00dc;
    index[frames].dwFlags = AVIIF_KEYFRAME;
    index[frames].dwOffset = (uint32_t)current_pos - 220;
    index[frames].dwSize = fb->len;

    // Write AVI Chunk Header
    fwrite(&fcc_00dc, 4, 1, f);
    uint32_t chunk_size = fb->len;
    fwrite(&chunk_size, 4, 1, f);

    // Write JPEG Data
    fwrite(fb->buf, 1, fb->len, f);

    // Pad to even byte count if needed
    if (chunk_size % 2 != 0) {
      uint8_t zero = 0;
      fwrite(&zero, 1, 1, f);
    }

    esp_camera_fb_return(fb);
    frames++;
    vTaskDelay(pdMS_TO_TICKS(100)); // ~10fps
  }

  // Go back and write the AVI header
  write_avi_header(f, frames, 640, 480);
  // Finalize with index
  finalize_avi_file(f, frames, index, frames);

  free(index);
  fclose(f);
  ESP_LOGI(TAG, "AVI Recording Saved: %s (%d frames)", fname, frames);
  s_is_recording = false;
}

// ===================================
// MOTION DETECTION
// ===================================
// Simple Downsampled Comparison
bool check_motion(camera_fb_t *fb, camera_fb_t *prev_fb) {
  if (!fb || !prev_fb)
    return false;
  if (fb->len != prev_fb->len)
    return false; // Resolution changed?

  int pixels_changed = 0;

  // Check 1 in every 10 bytes to speed up
  for (size_t i = 0; i < fb->len; i += 10) {
    int diff = abs(fb->buf[i] - prev_fb->buf[i]);
    if (diff > MOTION_THRESHOLD) {
      pixels_changed++;
    }
  }

  return (pixels_changed > MIN_PIXELS_CHANGED);
}

void motion_task(void *pv) {
  ESP_LOGI(TAG, "Motion Detection Task Started");

  // Allocate buffer for previous frame
  // We can't keep full resolution in IRAM usually, but VRAM might work.
  // However, simplest way with ESP-CAM constraints:
  // Just capture frames slowly and compare.

  camera_fb_t *prev_fb = NULL;

  while (1) {
    // If recording, skip detection (camera is busy)
    if (s_is_recording) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    // AUTO LED CONTROL: Allumer lors mouvement en obscurité + timeout 15s
    // Images sombres = moins de détails = meilleure compression = fichier plus
    // petit
    if (led_auto_mode) {
      size_t jpeg_size = fb->len;
      bool is_dark = (jpeg_size < JPEG_SIZE_DARK_THRESHOLD);

      // LED ON si: obscurité ET mouvement récent (< 15s)
      int64_t now_ms = esp_timer_get_time() / 1000;
      int64_t time_since_motion = now_ms - last_motion_time;
      bool motion_recent = (time_since_motion < LED_TIMEOUT_MS);
      bool should_led_on = (is_dark && motion_recent);
      gpio_set_level(LED_PIN, should_led_on ? 1 : 0);

      // Debug log occasionnel
      static int log_counter = 0;
      if (++log_counter % 20 == 0) {
        ESP_LOGI(TAG, "LED Auto: JPEG=%zu, Dark=%d, LastMotion=%lldms, LED=%s",
                 jpeg_size, is_dark, time_since_motion,
                 should_led_on ? "ON" : "OFF");
      }
    } else {
      // Mode manuel: utiliser led_manual_state
      gpio_set_level(LED_PIN, led_manual_state ? 1 : 0);
    }

    // We need a persistent copy of the previous frame.
    // esp_camera_fb_get() returns a pointer to the DMA buffer.
    // We cannot just hold it indefinitely if we want new frames?
    // Actually, we can hold one if fb_count > 1.

    if (prev_fb) {
      if (check_motion(fb, prev_fb)) {
        ESP_LOGI(TAG, "MOTION DETECTED!");

        // Mettre à jour timestamp pour LED
        last_motion_time = esp_timer_get_time() / 1000;

        // Release both before recording (to free buffers)
        esp_camera_fb_return(prev_fb);
        prev_fb = NULL;
        esp_camera_fb_return(fb);

        // Record!
        save_video_segment();
        continue;
      }
      esp_camera_fb_return(prev_fb); // Done with old one
    }

    // Keep current frame as previous
    // WARNING: To keep a frame, we must NOT return it yet.
    // But we need to verify we aren't starving the driver.
    // If fb_count=2, we can hold 1.
    prev_fb = fb;

    // Sampling Rate
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ===================================
// STREAMING
// ===================================
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  char *part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK)
    return res;

  while (true) {
    if (s_is_recording) {
      // If recording, stream is unavailable (or implement complex shared
      // access)
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    fb = esp_camera_fb_get();
    if (!fb) {
      res = ESP_FAIL;
    } else {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY,
                                  strlen(_STREAM_BOUNDARY));
      if (res == ESP_OK)
        res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
      if (res == ESP_OK)
        res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
    if (res != ESP_OK)
      break;
    vTaskDelay(pdMS_TO_TICKS(50)); // Cap stream fps
  }
  return res;
}

// ===================================
// SD CARD FILE MANAGEMENT HANDLERS
// ===================================

// Handler: Liste tous les fichiers .avi sur SD
esp_err_t sd_list_handler(httpd_req_t *req) {
  if (!s_sd_card_mounted) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "SD card not mounted");
    return ESP_FAIL;
  }

  DIR *dir = opendir("/sdcard");
  if (!dir) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed to open SD directory");
    return ESP_FAIL;
  }

  // Construire JSON avec liste fichiers
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr_chunk(req, "{\"files\":[");

  struct dirent *entry;
  int file_count = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG &&
        (strstr(entry->d_name, ".avi") || strstr(entry->d_name, ".mjpeg"))) {
      if (file_count > 0) {
        httpd_resp_sendstr_chunk(req, ",");
      }

      // Obtenir la taille du fichier
      char filepath[300];
      snprintf(filepath, sizeof(filepath), "/sdcard/%s", entry->d_name);
      struct stat st;
      long size = 0;
      if (stat(filepath, &st) == 0) {
        size = st.st_size;
      }

      char json_entry[300];
      snprintf(json_entry, sizeof(json_entry), "{\"name\":\"%s\",\"size\":%ld}",
               entry->d_name, size);
      httpd_resp_sendstr_chunk(req, json_entry);
      file_count++;
    }
  }
  closedir(dir);

  char json_footer[64];
  snprintf(json_footer, sizeof(json_footer), "],\"count\":%d}", file_count);
  httpd_resp_sendstr_chunk(req, json_footer);
  httpd_resp_sendstr_chunk(req, NULL); // Fin
  return ESP_OK;
}

// Handler: Télécharger un fichier
esp_err_t sd_download_handler(httpd_req_t *req) {
  if (!s_sd_card_mounted) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "SD card not mounted");
    return ESP_FAIL;
  }

  // Extraire paramètre ?file=xxx.avi
  char filename[64] = {0};
  if (httpd_req_get_url_query_str(req, filename, sizeof(filename)) == ESP_OK) {
    char param[64] = {0};
    if (httpd_query_key_value(filename, "file", param, sizeof(param)) ==
        ESP_OK) {
      // Construire chemin complet
      char filepath[300];
      snprintf(filepath, sizeof(filepath), "/sdcard/%s", param);

      FILE *file = fopen(filepath, "rb");
      if (!file) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
      }

      // Headers HTTP
      httpd_resp_set_type(req, "video/x-msvideo");
      char disposition[128];
      snprintf(disposition, sizeof(disposition), "attachment; filename=\"%s\"",
               param);
      httpd_resp_set_hdr(req, "Content-Disposition", disposition);

      // Stream le fichier par chunks
      char buffer[1024];
      size_t read_bytes;
      while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, read_bytes) != ESP_OK) {
          fclose(file);
          return ESP_FAIL;
        }
      }
      fclose(file);
      httpd_resp_send_chunk(req, NULL, 0); // Fin
      return ESP_OK;
    }
  }

  httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'file' parameter");
  return ESP_FAIL;
}

// Handler: Supprimer un fichier
esp_err_t sd_delete_handler(httpd_req_t *req) {
  if (!s_sd_card_mounted) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "SD card not mounted");
    return ESP_FAIL;
  }

  char filename[64] = {0};
  if (httpd_req_get_url_query_str(req, filename, sizeof(filename)) == ESP_OK) {
    char param[64] = {0};
    if (httpd_query_key_value(filename, "file", param, sizeof(param)) ==
        ESP_OK) {
      char filepath[300];
      snprintf(filepath, sizeof(filepath), "/sdcard/%s", param);

      if (unlink(filepath) == 0) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req,
                           "{\"success\":true,\"message\":\"File deleted\"}");
        return ESP_OK;
      } else {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND,
                            "File not found or delete failed");
        return ESP_FAIL;
      }
    }
  }

  httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'file' parameter");
  return ESP_FAIL;
}

// ===================================
// MODERN WEB PORTAL
// ===================================
// Handler: LED ON
esp_err_t led_on_handler(httpd_req_t *req) {
  led_auto_mode = false;
  led_manual_state = true;
  gpio_set_level(LED_PIN, 1);
  ESP_LOGI(TAG, "LED Manual: ON");
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(
      req, "{\"status\":\"success\",\"mode\":\"manual\",\"state\":\"on\"}");
  return ESP_OK;
}

// Handler: LED OFF
esp_err_t led_off_handler(httpd_req_t *req) {
  led_auto_mode = false;
  led_manual_state = false;
  gpio_set_level(LED_PIN, 0);
  ESP_LOGI(TAG, "LED Manual: OFF");
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(
      req, "{\"status\":\"success\",\"mode\":\"manual\",\"state\":\"off\"}");
  return ESP_OK;
}

// Handler: LED AUTO
esp_err_t led_auto_handler(httpd_req_t *req) {
  led_auto_mode = true;
  ESP_LOGI(TAG, "LED Mode: AUTO");
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"status\":\"success\",\"mode\":\"auto\"}");
  return ESP_OK;
}

// Handler: LED STATUS
esp_err_t led_status_handler(httpd_req_t *req) {
  char json[128];
  snprintf(json, sizeof(json), "{\"mode\":\"%s\",\"state\":\"%s\"}",
           led_auto_mode ? "auto" : "manual",
           (led_auto_mode ? (gpio_get_level(LED_PIN) ? "on" : "off")
                          : (led_manual_state ? "on" : "off")));
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, json);
  return ESP_OK;
}

esp_err_t portal_get_handler(httpd_req_t *req) {
  const char *HTML_BODY =
      "<!DOCTYPE html><html><head>"
      "<meta charset='utf-8'><meta name='viewport' "
      "content='width=device-width,initial-scale=1'>"
      "<title>ESP32-CAM Portal</title>"
      "<style>"
      "body{font-family:'Segoe "
      "UI',Roboto,sans-serif;background:#0f0c29;background:linear-gradient("
      "135deg,#0f0c29,#302b63,#24243e);color:#fff;min-height:100vh;margin:0;"
      "display:flex;flex-direction:column;align-items:center;padding:20px;}"
      ".glass{background:rgba(255,255,255,0.1);backdrop-filter:blur(10px);"
      "border-radius:20px;border:1px solid rgba(255,255,255,0.2);box-shadow:0 "
      "8px 32px "
      "rgba(0,0,0,0.3);padding:30px;width:100%;max-width:500px;margin-bottom:"
      "20px;}"
      "h1{margin:0 0 20px "
      "0;font-weight:300;letter-spacing:2px;text-align:center;text-transform:"
      "uppercase;font-size:1.5rem;}"
      ".btn-grid{display:grid;grid-template-columns:1fr 1fr "
      "1fr;gap:10px;margin-bottom:20px;}"
      "button{padding:12px;border:none;border-radius:12px;cursor:pointer;font-"
      "weight:bold;transition:all "
      "0.3s;font-size:0.9rem;text-transform:uppercase;}"
      ".btn-on{background:linear-gradient(45deg,#f1c40f,#f39c12);color:#000;}"
      ".btn-off{background:rgba(255,255,255,0.15);color:#fff;}"
      ".btn-auto{background:linear-gradient(45deg,#3498db,#2980b9);color:#fff;}"
      "button:hover{transform:translateY(-2px);filter:brightness(1.2);box-"
      "shadow:0 5px 15px rgba(0,0,0,0.3);}"
      "#status{text-align:center;font-size:0.9rem;margin-top:10px;font-weight:"
      "bold;min-height:1.2em;color:rgba(255,255,255,0.7);}"
      ".sd-list{background:rgba(0,0,0,0.2);border-radius:15px;max-height:300px;"
      "overflow-y:auto;margin-top:20px;}"
      ".sd-item{display:flex;justify-content:space-between;align-items:center;"
      "padding:12px 15px;border-bottom:1px solid rgba(255,255,255,0.05);}"
      ".sd-item:last-child{border-bottom:none;}"
      "a{color:#3498db;text-decoration:none;font-size:1.2rem;}"
      ".del-btn{color:#e74c3c;background:none;border:none;font-size:1.2rem;"
      "cursor:pointer;padding:5px;}"
      ".back-link{margin-top:30px;color:rgba(255,255,255,0.5);font-size:0.9rem;"
      "text-decoration:none;border-bottom:1px solid "
      "rgba(255,255,255,0.2);padding-bottom:2px;transition:all 0.3s;}"
      ".back-link:hover{color:#fff;border-color:#fff;}"
      "</style></head><body>"
      "<div class='glass'><h1>💡 LED Control</h1>"
      "<div class='btn-grid'>"
      "<button class='btn-on' onclick=\"cmd('on')\">ON</button>"
      "<button class='btn-off' onclick=\"cmd('off')\">OFF</button>"
      "<button class='btn-auto' onclick=\"cmd('auto')\">AUTO</button>"
      "</div><div id='status'>Ready</div></div>"
      "<div class='glass'><h1>📂 Recordings</h1>"
      "<div id='sd-list' class='sd-list'><div "
      "style='padding:20px;text-align:center;opacity:0.5'>Loading...</div></"
      "div>"
      "</div><a href='#' id='back-btn' class='back-link'>← "
      "Retour au Dashboard AepBill</a>"
      "<script>"
      "const bb=document.getElementById('back-btn');"
      "if(document.referrer){bb.href=document.referrer;}"
      "else{bb.href='http://aepbill.local/';}"
      "function msg(t,c){const "
      "s=document.getElementById('status');s.textContent=t;s.style.color=c||'"
      "rgba(255,255,255,0.7)'}"
      "function "
      "cmd(c){msg('Sending...','');fetch('/led/"
      "'+c).then(r=>r.json()).then(d=>msg('✅ MODE: "
      "'+(d.mode||c).toUpperCase(),'#2ecc71')).catch(e=>msg('❌ "
      "Error','#e74c3c'))}"
      "function loadSD(){fetch('/sd/list').then(r=>r.json()).then(d=>{const "
      "l=document.getElementById('sd-list');if(!d.files||d.files.length===0){l."
      "innerHTML='<"
      "div style=\"padding:20px;text-align:center;opacity:0.5\">No files "
      "found</div>';return;}let h='';d.files.forEach(f=>{h+=`<div "
      "class='sd-item'><div><div style='font-weight:600'>${f.name}</div><small "
      "style='opacity:0.6'>${(f.size/1024).toFixed(1)} KB</small></div><div><a "
      "href='/sd/download?file=${encodeURIComponent(f.name)}' "
      "download>⬇️</a><button class='del-btn' "
      "onclick=\"delFile('${f.name}')\">🗑️</button></div></"
      "div>`});l.innerHTML=h}).catch(e=>{document.getElementById('sd-list')."
      "innerHTML='<div "
      "style=\"padding:20px;text-align:center;color:#e74c3c\">Error loading SD "
      "card</div>'})}"
      "function delFile(n){if(!confirm('Delete "
      "'+n+'?'))return;fetch('/sd/"
      "delete?file='+encodeURIComponent(n),{method:'POST'}).then(r=>r.json())."
      "then(()=>loadSD()).catch(e=>alert('Error: '+e))}"
      "loadSD();setInterval(loadSD,10000);"
      "</script></body></html>";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, HTML_BODY);
  return ESP_OK;
}

void start_camera_server() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.stack_size = 10240;    // CRITICAL for large HTML portal
  config.max_uri_handlers = 20; // CRITICAL for 9+ URIs
  config.lru_purge_enable = true;
  config.max_open_sockets = 7;

  httpd_uri_t root_uri = {
      .uri = "/", .method = HTTP_GET, .handler = portal_get_handler};
  httpd_uri_t stream_uri = {.uri = "/stream",
                            .method = HTTP_GET,
                            .handler = stream_handler,
                            .user_ctx = NULL};

  httpd_uri_t sd_list_uri = {.uri = "/sd/list",
                             .method = HTTP_GET,
                             .handler = sd_list_handler,
                             .user_ctx = NULL};

  httpd_uri_t sd_download_uri = {.uri = "/sd/download",
                                 .method = HTTP_GET,
                                 .handler = sd_download_handler,
                                 .user_ctx = NULL};

  httpd_uri_t sd_delete_uri = {.uri = "/sd/delete",
                               .method = HTTP_POST,
                               .handler = sd_delete_handler,
                               .user_ctx = NULL};

  httpd_uri_t led_on_uri = {.uri = "/led/on",
                            .method = HTTP_GET,
                            .handler = led_on_handler,
                            .user_ctx = NULL};

  httpd_uri_t led_off_uri = {.uri = "/led/off",
                             .method = HTTP_GET,
                             .handler = led_off_handler,
                             .user_ctx = NULL};

  httpd_uri_t led_auto_uri = {.uri = "/led/auto",
                              .method = HTTP_GET,
                              .handler = led_auto_handler,
                              .user_ctx = NULL};

  httpd_uri_t led_status_uri = {.uri = "/led/status",
                                .method = HTTP_GET,
                                .handler = led_status_handler,
                                .user_ctx = NULL};

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &root_uri);
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    httpd_register_uri_handler(stream_httpd, &sd_list_uri);
    httpd_register_uri_handler(stream_httpd, &sd_download_uri);
    httpd_register_uri_handler(stream_httpd, &sd_delete_uri);
    httpd_register_uri_handler(stream_httpd, &led_on_uri);
    httpd_register_uri_handler(stream_httpd, &led_off_uri);
    httpd_register_uri_handler(stream_httpd, &led_auto_uri);
    httpd_register_uri_handler(stream_httpd, &led_status_uri);
    ESP_LOGI(TAG,
             "HTTP Server started with Modern Portal + SD + LED management");
  }
}

// ===================================
// WIFI & INIT
// ===================================
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    esp_wifi_connect();
    ESP_LOGW(TAG, "WiFi Disconnected. Retrying...");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Check out the stream at: http://" IPSTR "/stream",
             IP2STR(&event->ip_info.ip));

    // Initialize mDNS
    esp_err_t err = mdns_init();
    if (err == ESP_OK) {
      mdns_hostname_set("esp32-cam");
      mdns_instance_name_set("ESP32-CAM Guardian");
      ESP_LOGI(TAG, "mDNS started: http://esp32-cam.local/stream");
    }

    // Start SNTP sync
    init_sntp();

    start_camera_server();
  }
}

void app_main() {
  // 1. Storage & NVS
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  // 2. Watchdog (High Priority)
  init_watchdog();
  xTaskCreatePinnedToCore(watchdog_task, "watchdog", 3072, NULL, 5, NULL, 1);

  // 2b. LED Init (GPIO 4)
  gpio_reset_pin(LED_PIN);
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_PIN, 0); // OFF par défaut

  // 3. Camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAM_PIN_D0;
  config.pin_d1 = CAM_PIN_D1;
  config.pin_d2 = CAM_PIN_D2;
  config.pin_d3 = CAM_PIN_D3;
  config.pin_d4 = CAM_PIN_D4;
  config.pin_d5 = CAM_PIN_D5;
  config.pin_d6 = CAM_PIN_D6;
  config.pin_d7 = CAM_PIN_D7;
  config.pin_xclk = CAM_PIN_XCLK;
  config.pin_pclk = CAM_PIN_PCLK;
  config.pin_vsync = CAM_PIN_VSYNC;
  config.pin_href = CAM_PIN_HREF;
  config.pin_sccb_sda = CAM_PIN_SIOD;
  config.pin_sccb_scl = CAM_PIN_SIOC;
  config.pin_pwdn = CAM_PIN_PWDN;
  config.pin_reset = CAM_PIN_RESET;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA; // Good balance
  config.jpeg_quality = 12;
  config.fb_count = 2; // Need 2 for comparison

  if (esp_camera_init(&config) != ESP_OK) {
    ESP_LOGE(TAG, "Camera Failed");
    return;
  }

  // 4. SD Card (1-Bit Mode)
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.flags = SDMMC_HOST_FLAG_1BIT;
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = true,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};
  sdmmc_card_t *card;
  if (esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config,
                              &card) == ESP_OK) {
    s_sd_card_mounted = true;
    ESP_LOGI(TAG, "SD Card Ready");
  } else {
    ESP_LOGE(TAG, "SD Card Failed - Recording Disabled");
  }

  // 5. WiFi
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &wifi_event_handler, NULL, NULL);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &wifi_event_handler, NULL, NULL);
  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = ESP_WIFI_SSID,
              .password = ESP_WIFI_PASS,
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
          },
  };
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();

  // 6. Start Motion Detection
  xTaskCreatePinnedToCore(motion_task, "motion", 4096, NULL, 5, NULL, 1);
}
