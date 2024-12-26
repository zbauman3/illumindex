
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "sdkconfig.h"
#include <string.h>

#define CONFIG_EXAMPLE_WIFI_CONN_MAX_RETRY 5
#define EXAMPLE_NETIF_DESC_STA "example_netif_sta"
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_LINK_LOCAL
esp_err_t example_wifi_sta_do_disconnect(void);

/**
 * @brief Checks the netif description if it contains specified prefix.
 * All netifs created withing common connect component are prefixed with the
 * module TAG, so it returns true if the specified netif is owned by this module
 */
bool example_is_our_netif(const char *prefix, esp_netif_t *netif) {
  return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

/* types of ipv6 addresses to be displayed on ipv6 events */
const char *example_ipv6_addr_types_to_str[6] = {
    "ESP_IP6_ADDR_IS_UNKNOWN",      "ESP_IP6_ADDR_IS_GLOBAL",
    "ESP_IP6_ADDR_IS_LINK_LOCAL",   "ESP_IP6_ADDR_IS_SITE_LOCAL",
    "ESP_IP6_ADDR_IS_UNIQUE_LOCAL", "ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6"};

static const char *TAG_WIF = "example_connect";
static esp_netif_t *s_example_sta_netif = NULL;
static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;
static SemaphoreHandle_t s_semph_get_ip6_addrs = NULL;

static int s_retry_num = 0;

static void example_handler_on_wifi_disconnect(void *arg,
                                               esp_event_base_t event_base,
                                               int32_t event_id,
                                               void *event_data) {
  s_retry_num++;
  if (s_retry_num > CONFIG_EXAMPLE_WIFI_CONN_MAX_RETRY) {
    ESP_LOGI(TAG_WIF, "WiFi Connect failed %d times, stop reconnect.",
             s_retry_num);
    /* let example_wifi_sta_do_connect() return */
    if (s_semph_get_ip_addrs) {
      xSemaphoreGive(s_semph_get_ip_addrs);
    }
    if (s_semph_get_ip6_addrs) {
      xSemaphoreGive(s_semph_get_ip6_addrs);
    }
    example_wifi_sta_do_disconnect();
    return;
  }
  ESP_LOGI(TAG_WIF, "Wi-Fi disconnected, trying to reconnect...");
  esp_err_t err = esp_wifi_connect();
  if (err == ESP_ERR_WIFI_NOT_STARTED) {
    return;
  }
  ESP_ERROR_CHECK(err);
}

static void example_handler_on_wifi_connect(void *esp_netif,
                                            esp_event_base_t event_base,
                                            int32_t event_id,
                                            void *event_data) {
  esp_netif_create_ip6_linklocal(esp_netif);
}

static void example_handler_on_sta_got_ip(void *arg,
                                          esp_event_base_t event_base,
                                          int32_t event_id, void *event_data) {
  s_retry_num = 0;
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  if (!example_is_our_netif(EXAMPLE_NETIF_DESC_STA, event->esp_netif)) {
    return;
  }
  ESP_LOGI(TAG_WIF, "Got IPv4 event: Interface \"%s\" address: " IPSTR,
           esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
  if (s_semph_get_ip_addrs) {
    xSemaphoreGive(s_semph_get_ip_addrs);
  } else {
    ESP_LOGI(TAG_WIF, "- IPv4 address: " IPSTR ",", IP2STR(&event->ip_info.ip));
  }
}

static void example_handler_on_sta_got_ipv6(void *arg,
                                            esp_event_base_t event_base,
                                            int32_t event_id,
                                            void *event_data) {
  ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
  if (!example_is_our_netif(EXAMPLE_NETIF_DESC_STA, event->esp_netif)) {
    return;
  }
  esp_ip6_addr_type_t ipv6_type =
      esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
  ESP_LOGI(TAG_WIF,
           "Got IPv6 event: Interface \"%s\" address: " IPV6STR ", type: %s",
           esp_netif_get_desc(event->esp_netif), IPV62STR(event->ip6_info.ip),
           example_ipv6_addr_types_to_str[ipv6_type]);

  if (ipv6_type == EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE) {
    if (s_semph_get_ip6_addrs) {
      xSemaphoreGive(s_semph_get_ip6_addrs);
    } else {
      ESP_LOGI(TAG_WIF, "- IPv6 address: " IPV6STR ", type: %s",
               IPV62STR(event->ip6_info.ip),
               example_ipv6_addr_types_to_str[ipv6_type]);
    }
  }
}

void example_wifi_start(void) {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_netif_inherent_config_t esp_netif_config =
      ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
  // Warning: the interface desc is used in tests to capture actual connection
  // details (IP, gw, mask)
  esp_netif_config.if_desc = EXAMPLE_NETIF_DESC_STA;
  esp_netif_config.route_prio = 128;
  s_example_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
  esp_wifi_set_default_wifi_sta_handlers();

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void example_wifi_stop(void) {
  esp_err_t err = esp_wifi_stop();
  if (err == ESP_ERR_WIFI_NOT_INIT) {
    return;
  }
  ESP_ERROR_CHECK(err);
  ESP_ERROR_CHECK(esp_wifi_deinit());
  ESP_ERROR_CHECK(
      esp_wifi_clear_default_wifi_driver_and_handlers(s_example_sta_netif));
  esp_netif_destroy(s_example_sta_netif);
  s_example_sta_netif = NULL;
}

esp_err_t example_wifi_sta_do_connect(wifi_config_t wifi_config, bool wait) {
  if (wait) {
    s_semph_get_ip_addrs = xSemaphoreCreateBinary();
    if (s_semph_get_ip_addrs == NULL) {
      return ESP_ERR_NO_MEM;
    }
    s_semph_get_ip6_addrs = xSemaphoreCreateBinary();
    if (s_semph_get_ip6_addrs == NULL) {
      vSemaphoreDelete(s_semph_get_ip_addrs);
      return ESP_ERR_NO_MEM;
    }
  }
  s_retry_num = 0;
  ESP_ERROR_CHECK(
      esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                 &example_handler_on_wifi_disconnect, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &example_handler_on_sta_got_ip, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(
      WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &example_handler_on_wifi_connect,
      s_example_sta_netif));
  ESP_ERROR_CHECK(esp_event_handler_register(
      IP_EVENT, IP_EVENT_GOT_IP6, &example_handler_on_sta_got_ipv6, NULL));

  ESP_LOGI(TAG_WIF, "Connecting to %s...", wifi_config.sta.ssid);
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  esp_err_t ret = esp_wifi_connect();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_WIF, "WiFi connect failed! ret:%x", ret);
    return ret;
  }
  if (wait) {
    ESP_LOGI(TAG_WIF, "Waiting for IP(s)");
    xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY);
    xSemaphoreTake(s_semph_get_ip6_addrs, portMAX_DELAY);
    if (s_retry_num > CONFIG_EXAMPLE_WIFI_CONN_MAX_RETRY) {
      return ESP_FAIL;
    }
  }
  return ESP_OK;
}

esp_err_t example_wifi_sta_do_disconnect(void) {
  ESP_ERROR_CHECK(
      esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                   &example_handler_on_wifi_disconnect));
  ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &example_handler_on_sta_got_ip));
  ESP_ERROR_CHECK(esp_event_handler_unregister(
      WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &example_handler_on_wifi_connect));
  ESP_ERROR_CHECK(esp_event_handler_unregister(
      IP_EVENT, IP_EVENT_GOT_IP6, &example_handler_on_sta_got_ipv6));
  if (s_semph_get_ip_addrs) {
    vSemaphoreDelete(s_semph_get_ip_addrs);
  }
  if (s_semph_get_ip6_addrs) {
    vSemaphoreDelete(s_semph_get_ip6_addrs);
  }
  return esp_wifi_disconnect();
}

void example_wifi_shutdown(void) {
  example_wifi_sta_do_disconnect();
  example_wifi_stop();
}

esp_err_t example_wifi_connect(void) {
  ESP_LOGI(TAG_WIF, "Start example_connect.");
  example_wifi_start();

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = "",
              .password = "",
              .scan_method = WIFI_ALL_CHANNEL_SCAN,
              .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
              .threshold.rssi = -127,
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
          },
  };

  return example_wifi_sta_do_connect(wifi_config, true);
}
