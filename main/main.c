#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "drivers/matrix.h"
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

  init_ret = matrixInit(&matrix, &matrixConfig);
  ESP_RETURN_VOID_ON_ERROR(init_ret, TAG, "Unable to init the matrix");

  static uint16_t topRow[64] = {
      BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,
      BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,
      BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,
      BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,
      BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,
      BV_565_RED_5, // 21 red
      BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5,
      BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5,
      BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5,
      BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5,
      BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5,
      BV_565_GREEN_5,
      BV_565_GREEN_5, // 22 green
      BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,
      BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,
      BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,
      BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,
      BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,
      BV_565_BLUE_5 // 21 blue
  };

  static uint16_t bottomRow[64] = {
      BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,
      BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,
      BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,
      BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,
      BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,  BV_565_BLUE_5,
      BV_565_BLUE_5, // 21 blue
      BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5,
      BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5,
      BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5,
      BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5,
      BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5, BV_565_GREEN_5,
      BV_565_GREEN_5,
      BV_565_GREEN_5, // 22 green
      BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,
      BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,
      BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,
      BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,
      BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,   BV_565_RED_5,
      BV_565_RED_5 // 21 red
  };

  // generate an image
  static uint16_t buffer[64 * 32];
  for (uint8_t row = 0; row < 16; row++) {
    for (uint8_t col = 0; col < 64; col++) {
      buffer[(row * 64) + col] = topRow[col];
      buffer[1024 + (row * 64) + col] = bottomRow[col];
    }
  }

  ESP_LOGI(TAG, "Starting");

  showFrame(matrix, buffer);

  while (true) {
    ESP_LOGI(TAG, "LOOP!");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
