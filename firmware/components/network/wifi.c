#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include <string.h>

#include "helper_utils.h"

#include "network/events.h"

#define WIFI_MAXIMUM_RETRY 3

static const char *TAG = "NETWORK:WIFI";
static uint8_t connect_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == IP_EVENT) {
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP: {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGD(TAG, "EVENT - IP_EVENT_STA_GOT_IP");
      ESP_LOGD(TAG, "IPV4 is: " IPSTR, IP2STR(&event->ip_info.ip));
      connect_retry_num = 0;
      xEventGroupSetBits(wifi_event_group, WIFI_EVENT_CONNECTED_BIT |
                                               WIFI_EVENT_RECREATE_SOCKETS_BIT);
      break;
    }
    case IP_EVENT_GOT_IP6: {
      ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
      ESP_LOGD(TAG, "EVENT - IP_EVENT_GOT_IP6");
      ESP_LOGD(TAG, "IPV6 is: " IPV6STR, IPV62STR(event->ip6_info.ip));
      connect_retry_num = 0;
      xEventGroupSetBits(wifi_event_group, WIFI_EVENT_CONNECTED_BIT |
                                               WIFI_EVENT_RECREATE_SOCKETS_BIT);
      break;
    }
    case IP_EVENT_STA_LOST_IP: {
      ESP_LOGD(TAG, "EVENT - IP_EVENT_STA_LOST_IP");
      xEventGroupClearBits(wifi_event_group,
                           WIFI_EVENT_CONNECTED_BIT |
                               WIFI_EVENT_RECREATE_SOCKETS_BIT);
      break;
    }
    }
    return;
  }

  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START: {
      ESP_LOGD(TAG, "EVENT - WIFI_EVENT_STA_START");
      esp_wifi_connect();
      break;
    }
    case WIFI_EVENT_STA_CONNECTED: {
      ESP_LOGD(TAG, "EVENT - WIFI_EVENT_STA_CONNECTED");
      xEventGroupClearBits(wifi_event_group, WIFI_EVENT_FAIL_BIT);
      break;
    }
    case WIFI_EVENT_STA_DISCONNECTED: {
      ESP_LOGD(TAG, "EVENT - WIFI_EVENT_STA_DISCONNECTED");

      if (connect_retry_num < WIFI_MAXIMUM_RETRY) {
        ESP_LOGD(TAG, "Trying to connect again");
        esp_wifi_connect();
        connect_retry_num++;
      } else {
        ESP_LOGD(TAG, "Failed to connect after %u tries", connect_retry_num);
        xEventGroupSetBits(wifi_event_group, WIFI_EVENT_FAIL_BIT);
      }

      break;
    }
    default: {
      ESP_LOGD(TAG, "EVENT - %ld", event_id);
      break;
    }
    }

    return;
  }
}

// Using overview from:
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/wifi.html#esp32-wi-fi-station-general-scenario
esp_err_t wifi_init(void) {
  ESP_LOGD(TAG, "Starting WiFi connection to \"%s\"", CONFIG_WIFI_SSID);

  wifi_event_group = xEventGroupCreate();

  ESP_ERROR_BUBBLE(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                              &event_handler, NULL));
  ESP_ERROR_BUBBLE(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                              &event_handler, NULL));

  ESP_ERROR_BUBBLE(esp_netif_init());

  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_BUBBLE(esp_wifi_init(&cfg));

  // get the current mode from NVS. If not correct, set it
  wifi_mode_t wifi_nvs_mode;
  esp_err_t get_mode_ret = esp_wifi_get_mode(&wifi_nvs_mode);
  if (get_mode_ret != ESP_OK || wifi_nvs_mode != WIFI_MODE_STA) {
    ESP_LOGW(TAG, "WiFi NVS \"mode\" not correct - setting");
    ESP_ERROR_BUBBLE(esp_wifi_set_mode(WIFI_MODE_STA));
  }

  // get the current config from NVS. If not correct, set it
  wifi_config_t wifi_nvs_config;
  esp_err_t get_config_ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_nvs_config);
  if (get_config_ret != ESP_OK ||
      strcmp((char *)wifi_nvs_config.sta.ssid, CONFIG_WIFI_SSID) != 0 ||
      strcmp((char *)wifi_nvs_config.sta.password, CONFIG_WIFI_PWD) != 0) {
    ESP_LOGW(TAG, "WiFi NVS \"config\" not correct - setting");
    wifi_config_t wifi_config_new = {
        .sta =
            {
                .ssid = CONFIG_WIFI_SSID,
                .password = CONFIG_WIFI_PWD,
                .scan_method = WIFI_ALL_CHANNEL_SCAN,
                .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                .threshold =
                    {
                        .rssi = 0,
                        .authmode = 0,
                    },
            },
    };

    ESP_ERROR_BUBBLE(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_new));
  }

  ESP_ERROR_BUBBLE(esp_wifi_start());

  // Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
  // connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
  EventBits_t bits = xEventGroupWaitBits(
      wifi_event_group, WIFI_EVENT_CONNECTED_BIT | WIFI_EVENT_FAIL_BIT, pdFALSE,
      pdFALSE, portMAX_DELAY);

  if (bits & WIFI_EVENT_CONNECTED_BIT) {
    ESP_LOGD(TAG, "Connected to \"%s\"", CONFIG_WIFI_SSID);
    return ESP_OK;
  }

  if (bits & WIFI_EVENT_FAIL_BIT) {
    ESP_LOGE(TAG, "Failed to connect to \"%s\"", CONFIG_WIFI_SSID);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED CONNECTION STATUS");
  }

  return ESP_ERR_WIFI_NOT_CONNECT;
}