#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "drivers/matrix.h"
#include "gfx/displayBuffer.h"
#include "gfx/fonts.h"
#include "network/request.h"
#include "network/wifi.h"
#include "util/565_color.h"
#include "util/commands.h"
#include "util/error_helpers.h"

#define MATRIX_WIDTH 64
#define MATRIX_HEIGHT 64

static const char *TAG = "APP_MAIN";
static MatrixHandle matrix;
static DisplayBufferHandle displayBuffer;

esp_err_t appInit() {
  esp_err_t init_ret = nvs_flash_init();
  if (init_ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      init_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_BUBBLE(nvs_flash_erase());
    ESP_ERROR_BUBBLE(nvs_flash_init());
  }

  ESP_ERROR_BUBBLE(esp_event_loop_create_default());

  MatrixInitConfig matrixConfig = {
      .width = MATRIX_WIDTH,
      .height = MATRIX_HEIGHT,
      .pins =
          {
              // green and blue are switched here due to the `ICN2037` wiring
              .a0 = GPIO_NUM_9,     // 9
              .a1 = GPIO_NUM_10,    // 10
              .a2 = GPIO_NUM_11,    // 11
              .a3 = GPIO_NUM_12,    // 12
              .a4 = GPIO_NUM_35,    // MOSI
              .latch = GPIO_NUM_5,  // 5
              .clock = GPIO_NUM_36, // SCK
              .oe = GPIO_NUM_6,     // 6
              .r1 = GPIO_NUM_18,    // A0
              .b1 = GPIO_NUM_17,    // A1
              .g1 = GPIO_NUM_16,    // A2
              .r2 = GPIO_NUM_15,    // A3
              .b2 = GPIO_NUM_14,    // A4
              .g2 = GPIO_NUM_8,     // A5
          },
  };
  ESP_ERROR_BUBBLE(matrixInit(&matrix, &matrixConfig));
  ESP_ERROR_BUBBLE(matrixStart(matrix));
  ESP_ERROR_BUBBLE(
      displayBufferInit(&displayBuffer, MATRIX_WIDTH, MATRIX_HEIGHT));

  displayBufferSetColor(displayBuffer, RGB_TO_565(0, 255, 149));
  displayBufferSetCursor(displayBuffer, 0, (MATRIX_HEIGHT / 2) - 8);
  fontSetSize(displayBuffer->font, FONT_SIZE_LG);

  displayBufferDrawString(displayBuffer, "Starting");

  matrixShow(matrix, displayBuffer->buffer);
  ESP_ERROR_BUBBLE(wifi_init());

  return ESP_OK;
}

esp_err_t fetchAndDisplayData() {
  esp_err_t ret = ESP_OK;
  RequestContextHandle ctx;

  ESP_GOTO_ON_ERROR(requestInit(&ctx), fetchAndDisplayData_cleanup, TAG,
                    "Error initiating the request context");

  ctx->url = CONFIG_ENDPOINT_URL;
  ctx->method = HTTP_METHOD_GET;

  ESP_GOTO_ON_ERROR(requestPerform(ctx), fetchAndDisplayData_cleanup, TAG,
                    "Error fetching data from endpoint");

  ESP_GOTO_ON_FALSE(ctx->response->statusCode < 400, ESP_ERR_INVALID_RESPONSE,
                    fetchAndDisplayData_cleanup, TAG,
                    "Invalid response status code \"%d\"",
                    ctx->response->statusCode);

  displayBufferClear(displayBuffer);
  displayBufferSetCursor(displayBuffer, 0, 0);
  displayBufferSetColor(displayBuffer, 0b1111111111111111);

  ESP_GOTO_ON_ERROR(parseAndShowCommands(displayBuffer, ctx->response->data,
                                         ctx->response->length),
                    fetchAndDisplayData_cleanup, TAG,
                    "Invalid JSON response or content length");

  ESP_GOTO_ON_ERROR(matrixShow(matrix, displayBuffer->buffer),
                    fetchAndDisplayData_cleanup, TAG,
                    "Error setting the new buffer");

fetchAndDisplayData_cleanup:
  requestEnd(ctx);
  return ret;
}

void app_main(void) {
  // TODO
  // For some reason, immediately starting the display and/or wifi causes a
  // crash loop. I'm assuming this is something to do with the USB CDC stack,
  // because the device will turn on and run the display for a few seconds, but
  // never shows up as a USB device, then crashes.
  ESP_LOGI(TAG, "Waiting");
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  esp_err_t initRet = appInit();
  if (initRet != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initiate the application - restarting");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
    return;
  }

  uint8_t loops = 255;
  while (true) {
    ESP_LOGI(TAG, "LOOP");

    if (loops >= CONFIG_ENDPOINT_FETCH_INTERVAL) {
      if (fetchAndDisplayData() == ESP_OK) {
        loops = 0;
      } else {
        // try again in 10 seconds
        loops = CONFIG_ENDPOINT_FETCH_INTERVAL - 10;
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    loops++;
  }
}
