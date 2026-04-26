#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

// Initialize and start the HTTP server
esp_err_t start_http_server(void);

// Register common handlers for both HTTP and HTTPS
void register_common_handlers(httpd_handle_t server);

// Helper for sending confirmation page
void send_conf_header(httpd_req_t *req, const char *title,
                      const char *redirect_url, int delay_sec);

#endif // HTTP_SERVER_H
