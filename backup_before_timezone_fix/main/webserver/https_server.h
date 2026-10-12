#ifndef HTTPS_SERVER_H
#define HTTPS_SERVER_H

#include "esp_err.h"

esp_err_t start_https_server(void);
void stop_https_server(void);

#endif // HTTPS_SERVER_H
