#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "network/request.h"
#include "util/setup.h"

#define RED_LED GPIO_NUM_22
#define GREEN_LED GPIO_NUM_5
#define BTN_1 GPIO_NUM_21
#define BTN_2 GPIO_NUM_19

static const char *TAG = "APP_MAIN";

void initGPOI() {
  ESP_ERROR_CHECK(gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level(RED_LED, 0));
  ESP_ERROR_CHECK(gpio_set_level(GREEN_LED, 0));

  ESP_ERROR_CHECK(gpio_set_direction(BTN_1, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_set_direction(BTN_2, GPIO_MODE_INPUT));

  ESP_ERROR_CHECK(gpio_set_pull_mode(BTN_1, GPIO_PULLUP_ONLY));
  ESP_ERROR_CHECK(gpio_set_pull_mode(BTN_2, GPIO_PULLUP_ONLY));
}

void buttonTask(void *pvParameters) {
  for (;;) {
    if (gpio_get_level(BTN_1) != 1) {
      ESP_ERROR_CHECK(gpio_set_level(RED_LED, 1));
      ESP_ERROR_CHECK(gpio_set_level(GREEN_LED, 1));

      request_ctx_t ctx =
          requestCtxCreate("https://api-tests-ten.vercel.app/esp/red");

      getRequest(&ctx);

      ESP_LOGI(TAG, "BTN_1 GOT DATA: \"%s\"", ctx.data);

      requestCtxCleanup(ctx);

      ESP_ERROR_CHECK(gpio_set_level(RED_LED, 1));
      ESP_ERROR_CHECK(gpio_set_level(GREEN_LED, 0));

      vTaskDelay(1000 / portTICK_PERIOD_MS);

      ESP_ERROR_CHECK(gpio_set_level(RED_LED, 0));
      ESP_ERROR_CHECK(gpio_set_level(GREEN_LED, 0));
    } else if (gpio_get_level(BTN_2) != 1) {
      ESP_ERROR_CHECK(gpio_set_level(RED_LED, 1));
      ESP_ERROR_CHECK(gpio_set_level(GREEN_LED, 1));

      request_ctx_t ctx =
          requestCtxCreate("https://api-tests-ten.vercel.app/esp/green");

      getRequest(&ctx);

      ESP_LOGI(TAG, "BTN_2 GOT DATA: \"%s\"", ctx.data);

      requestCtxCleanup(ctx);

      ESP_ERROR_CHECK(gpio_set_level(RED_LED, 0));
      ESP_ERROR_CHECK(gpio_set_level(GREEN_LED, 1));

      vTaskDelay(1000 / portTICK_PERIOD_MS);

      ESP_ERROR_CHECK(gpio_set_level(RED_LED, 0));
      ESP_ERROR_CHECK(gpio_set_level(GREEN_LED, 0));
    } else {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}

// void ledTask(void *pvParameters) {
//   for (;;) {
//     gpio_set_level(RED_LED, 1);
//     gpio_set_level(GREEN_LED, 0);
//     vTaskDelay(500 / portTICK_PERIOD_MS);

//     gpio_set_level(RED_LED, 0);
//     gpio_set_level(GREEN_LED, 1);
//     vTaskDelay(500 / portTICK_PERIOD_MS);
//   }
// }

void app_main(void) {
  initGPOI();

  esp_err_t init_ret = app_init();
  if (init_ret != ESP_OK) {
    // if there's an error initiating the app, delay then restart
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP_LOGW(TAG, "Failed to initiate the application - restarting");
    esp_restart();
    return;
  }

  xTaskCreate(buttonTask, "BUTTONS", 4096, NULL, 1, NULL);
  // xTaskCreate(ledTask, "LEDs", 4096, NULL, 1, NULL);
}
