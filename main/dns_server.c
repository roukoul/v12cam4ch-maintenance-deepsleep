#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

static const char *TAG = "DNS_SERVER";

#define DNS_PORT 53

// Helper to find substring in binary data
static char *memstr(char *haystack, size_t haystack_len, const char *needle) {
  size_t needle_len = strlen(needle);
  if (needle_len == 0)
    return haystack;
  if (haystack_len < needle_len)
    return NULL;
  for (size_t i = 0; i <= haystack_len - needle_len; i++) {
    if (memcmp(&haystack[i], needle, needle_len) == 0)
      return &haystack[i];
  }
  return NULL;
}

void dns_server_task(void *pvParameters) {
  uint8_t ip_last_octet = (uint8_t)(uintptr_t)pvParameters;
  uint8_t data[512];
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(DNS_PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
    close(sock);
    vTaskDelete(NULL);
    return;
  }

  ESP_LOGI(TAG, "DNS Captive Portal Server started on port %d", DNS_PORT);

  while (1) {
    int len = recvfrom(sock, data, sizeof(data), 0,
                       (struct sockaddr *)&client_addr, &client_addr_len);
    if (len < 0) {
      ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
      break;
    }

    if (len > 12) { // Minimum DNS query length
      // Basic check for captive portal domains
      bool should_redirect = false;
      const char *redirect_queries[] = {
          "google",       "gstatic", "apple",       "msft",
          "connectivity", "captive", "detectportal"};
      for (int i = 0; i < 7; i++) {
        if (memstr((char *)data + 12, len - 12, redirect_queries[i])) {
          should_redirect = true;
          break;
        }
      }

      if (should_redirect) {
        // Modification du header pour en faire une réponse
        data[2] |= 0x80; // QR = 1 (Response)
        data[3] |= 0x84; // RA = 1, Auth
        data[7] = 1;     // ANCOUNT = 1 (One Answer)

        // Construction de la réponse simplifiée (Réponse universelle :
        // 192.168.4.2)
        int response_len = len;
        data[response_len++] = 0xc0; // Name pointer
        data[response_len++] = 0x0c;
        data[response_len++] = 0x00; // Type A
        data[response_len++] = 0x01;
        data[response_len++] = 0x00; // Class IN
        data[response_len++] = 0x01;
        data[response_len++] = 0x00; // TTL
        data[response_len++] = 0x00;
        data[response_len++] = 0x00;
        data[response_len++] = 0x0a; // 10 seconds
        data[response_len++] = 0x00; // Data length (4 bytes for IP)
        data[response_len++] = 0x04;
        data[response_len++] = 192; // IP Address: 192.168.4.X
        data[response_len++] = 168;
        data[response_len++] = 4;
        data[response_len++] = ip_last_octet;

        sendto(sock, data, response_len, 0, (struct sockaddr *)&client_addr,
               client_addr_len);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  close(sock);
  vTaskDelete(NULL);
}

void start_dns_server(uint8_t ip_last_octet) {
  static bool is_dns_running = false;
  if (!is_dns_running) {
    xTaskCreate(dns_server_task, "dns_server", 4096, (void *)(uintptr_t)ip_last_octet, 5, NULL);
    is_dns_running = true;
  }
}
