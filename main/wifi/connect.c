#include "sdkconfig.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>

#include "lwip/err.h"
#include "lwip/sys.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_MAXIMUM_RETRY 3

static const char *TAG = "WIFI_STATION";

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == IP_EVENT) {
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP: {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGI(TAG, "Got IP V4:" IPSTR, IP2STR(&event->ip_info.ip));
      s_retry_num = 0;
      xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
      break;
    }
    case IP_EVENT_GOT_IP6: {
      ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
      ESP_LOGI(TAG, "Got IP V6:" IPV6STR, IPV62STR(event->ip6_info.ip));
      s_retry_num = 0;
      xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
      break;
    }
    case IP_EVENT_STA_LOST_IP: {
      ESP_LOGI(TAG, "Lost IP");
      xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
      break;
    }
    }
    return;
  }

  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_WIFI_READY: {
      ESP_LOGI(TAG, "WIFI_EVENT_WIFI_READY");
      break;
    }
    case WIFI_EVENT_SCAN_DONE: {
      ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE");
      break;
    }
    case WIFI_EVENT_STA_START: {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
      esp_wifi_connect();
      break;
    }
    case WIFI_EVENT_STA_STOP: {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
      break;
    }
    case WIFI_EVENT_STA_CONNECTED: {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
      xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
      break;
    }
    case WIFI_EVENT_STA_DISCONNECTED: {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");

      if (s_retry_num < WIFI_MAXIMUM_RETRY) {
        esp_wifi_connect();
        s_retry_num++;

        ESP_LOGI(TAG, "retry to connect to the AP");
      } else {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        ESP_LOGI(TAG, "connect to the AP fail");
      }

      break;
    }
    case WIFI_EVENT_STA_BEACON_TIMEOUT: {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_BEACON_TIMEOUT");
      break;
    }
    case WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START: {
      ESP_LOGI(TAG, "WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START");
      break;
    }
    }

    return;
  }
}

esp_err_t wifi_init_sta(void) {
  ESP_ERROR_CHECK(esp_netif_init());

  s_wifi_event_group = xEventGroupCreate();

  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

  wifi_config_t wifi_config = {
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
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we
   * can test which event actually happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID:%s", CONFIG_WIFI_SSID);
    return ESP_OK;
  }

  if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s", CONFIG_WIFI_SSID);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED CONNECTION STATUS");
  }
  return WIFI_REASON_CONNECTION_FAIL;
}