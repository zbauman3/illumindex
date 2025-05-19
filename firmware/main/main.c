#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "lib/display.h"
#include "lib/state.h"
#include "network/wifi.h"
#include "util/error_helpers.h"

static const char *TAG = "APP_MAIN";
static DisplayHandle display;

esp_err_t appInit() {
  esp_err_t init_ret = nvs_flash_init();
  if (init_ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      init_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_BUBBLE(nvs_flash_erase());
    ESP_ERROR_BUBBLE(nvs_flash_init());
  }

  ESP_ERROR_BUBBLE(esp_event_loop_create_default());

  MatrixInitConfig matrixConfig = {
      .width = 64,
      .height = 64,
      .pins =
          {
              .a0 = GPIO_NUM_10,    // 10
              .a1 = GPIO_NUM_11,    // 11
              .a2 = GPIO_NUM_9,     // 9
              .a3 = GPIO_NUM_12,    // 12
              .a4 = GPIO_NUM_6,     // 6
              .latch = GPIO_NUM_5,  // 5
              .clock = GPIO_NUM_36, // SCK
              .oe = GPIO_NUM_35,    // MOSI
              .g1 = GPIO_NUM_18,    // A0
              .g2 = GPIO_NUM_17,    // A1
              // Blue and red are swapped, as compared to the physical
              // connector. This is due to the pin mapping on the ICN2037
              .b1 = GPIO_NUM_16, // A2
              .b2 = GPIO_NUM_15, // A3
              .r1 = GPIO_NUM_14, // A4
              .r2 = GPIO_NUM_8,  // A5
          },
  };

  ESP_ERROR_BUBBLE(displayInit(&display, &matrixConfig));

  ESP_ERROR_BUBBLE(wifi_init());

  displaySetup(display);

  return ESP_OK;
}

void app_main(void) {
  if (appInit() != ESP_OK) {
    displayEnd(display);

    ESP_LOGE(TAG, "Failed to initiate the application - restarting");
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    esp_restart();
    return;
  }

  while (true) {
    displayLoop(display);
  }
}
