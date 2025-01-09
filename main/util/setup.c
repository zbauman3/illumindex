#include "esp_err.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "network/wifi.h"
#include "util/error_helpers.h"

esp_err_t app_init(void) {
  esp_err_t init_ret = nvs_flash_init();
  if (init_ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      init_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_BUBBLE(nvs_flash_erase());
    ESP_ERROR_BUBBLE(nvs_flash_init());
  }

  // ESP_ERROR_BUBBLE(esp_event_loop_create_default());

  // return wifi_init();
  return esp_event_loop_create_default();
}