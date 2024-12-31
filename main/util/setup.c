#include "esp_err.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "network/wifi.h"

esp_err_t app_init(void) {
  esp_err_t init_ret;

  init_ret = nvs_flash_init();
  if (init_ret != ESP_OK) {
    ESP_ERROR_CHECK(nvs_flash_erase());

    init_ret = nvs_flash_init();
    if (init_ret != ESP_OK) {
      return init_ret;
    }
  }

  init_ret = esp_event_loop_create_default();
  if (init_ret != ESP_OK) {
    return init_ret;
  }

  return wifi_init();
}