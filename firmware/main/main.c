#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
#include "util/config.h"
#include "util/error_helpers.h"

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
  ESP_ERROR_BUBBLE(matrixInit(&matrix, &matrixConfig));
  ESP_ERROR_BUBBLE(matrixStart(matrix));
  ESP_ERROR_BUBBLE(displayBufferInit(&displayBuffer));

  displayBufferSetColor(displayBuffer, RGB_TO_565(0, 255, 149));
  displayBufferSetCursor(displayBuffer, 0, 10);
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

  ctx->url = API_ENDPOINT_URL;
  ctx->method = API_ENDPOINT_METHOD;

  ESP_GOTO_ON_ERROR(requestPerform(ctx), fetchAndDisplayData_cleanup, TAG,
                    "Error fetching data from endpoint");

  ESP_GOTO_ON_FALSE(ctx->response->statusCode < 400, ESP_ERR_INVALID_RESPONSE,
                    fetchAndDisplayData_cleanup, TAG,
                    "Invalid response status code \"%d\"",
                    ctx->response->statusCode);

  displayBufferClear(displayBuffer);

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
  esp_err_t initRet = appInit();
  if (initRet != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initiate the application - restarting");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
    return;
  }

  uint8_t loops = 255;
  const uint8_t loopReset = 136;
  while (true) {
    if (loops >= loopReset) {
      if (fetchAndDisplayData() == ESP_OK) {
        loops = 0;
      } else {
        // try again in 10 seconds
        loops = loopReset - 10;
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    loops++;
  }
}
