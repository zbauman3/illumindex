#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "drivers/matrix.h"
#include "util/setup.h"

static const char *TAG = "APP_MAIN";

void app_main(void) {
  esp_err_t init_ret = app_init();
  if (init_ret != ESP_OK) {
    // if there's an error initiating the app, delay then restart
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP_LOGW(TAG, "Failed to initiate the application - restarting");
    esp_restart();
    return;
  }

  matrix_init();

  static TaskHandle_t xHandle = NULL;

  xTaskCreate(displayTest,   /* Function that implements the task. */
              "displayTest", /* Text name for the task. */
              (1024 * 4),    /* Stack size in words, not bytes. */
              NULL,          /* Parameter passed into the task. */
              1,             /* Priority at which the task is created. */
              &xHandle);
}
