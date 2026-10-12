#include "esp_log.h"
#include "http_server.h"
#include <esp_https_server.h>

static const char *TAG = "HTTPS_SRV";

static httpd_handle_t https_server = NULL;

// Certificates embedded via CMake EMBED_TXTFILES
extern const unsigned char
    server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const unsigned char
    server_cert_pem_end[] asm("_binary_server_cert_pem_end");
extern const unsigned char
    server_key_pem_start[] asm("_binary_server_key_pem_start");
extern const unsigned char
    server_key_pem_end[] asm("_binary_server_key_pem_end");

esp_err_t start_https_server(void) {
  httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();

  // Load certificates from embedded data (null-terminated PEM strings)
  config.cacert_pem = NULL; // No CA cert for self-signed
  config.cacert_len = 0;
  config.servercert = server_cert_pem_start;
  config.servercert_len = server_cert_pem_end - server_cert_pem_start;
  config.prvtkey_pem = server_key_pem_start;
  config.prvtkey_len = server_key_pem_end - server_key_pem_start;

  // Port 443
  config.httpd.server_port = 443;
  config.httpd.max_uri_handlers = 60;
  config.httpd.stack_size = 12288; // Increase stack to 12KB for HTTPS handlers

  ESP_LOGI(TAG, "Starting HTTPS server on port 443");

  esp_err_t ret = httpd_ssl_start(&https_server, &config);
  if (ret == ESP_OK) {
    // Register the SAME handlers as HTTP server
    register_common_handlers(https_server);
    ESP_LOGI(TAG, "HTTPS server started successfully");
  } else {
    ESP_LOGE(TAG, "Error starting HTTPS server: %s", esp_err_to_name(ret));
  }

  return ret;
}

void stop_https_server(void) {
  if (https_server) {
    httpd_ssl_stop(https_server);
    https_server = NULL;
  }
}
