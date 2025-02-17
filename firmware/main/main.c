#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "drivers/matrix.h"
#include "gfx/displayBuffer.h"
#include "util/565_color.h"
#include "util/config.h"
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

  MatrixInitConfig matrixConfig = {
      .pins =
          {
              .a0 = MATRIX_ADDR_A,
              .a1 = MATRIX_ADDR_B,
              .a2 = MATRIX_ADDR_C,
              .a3 = MATRIX_ADDR_D,
              .latch = MATRIX_LATCH,
              .clock = MATRIX_CLOCK,
              .oe = MATRIX_OE,
              .r1 = MATRIX_RED_1,
              .b1 = MATRIX_BLUE_1,
              .g1 = MATRIX_GREEN_1,
              .r2 = MATRIX_RED_2,
              .b2 = MATRIX_BLUE_2,
              .g2 = MATRIX_GREEN_2,
          },
  };
  MatrixHandle matrix;

  ESP_RETURN_VOID_ON_ERROR(matrixInit(&matrix, &matrixConfig), TAG,
                           "Unable to init the matrix");

  DisplayBufferHandle displayBuffer;
  displayBufferInit(&displayBuffer);

  ESP_LOGI(TAG, "Starting");

  matrixStart(matrix);

  ESP_LOGI(TAG, "Running test");
  displayBufferTest(displayBuffer);

  matrixShow(matrix, displayBuffer->buffer);

  while (true) {
    ESP_LOGI(TAG, "LOOP!");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
