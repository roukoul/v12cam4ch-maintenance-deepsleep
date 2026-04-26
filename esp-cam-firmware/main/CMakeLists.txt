idf_component_register(SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES mdns esp_http_server nvs_flash esp_wifi esp_timer esp_netif fatfs driver sdmmc)
