#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "drivers/matrix.h"
#include "gfx/displayBuffer.h"
#include "lib/cjson/cJSON.h"
#include "network/request.h"
#include "network/wifi.h"
#include "nvs_flash.h"
#include "util/565_color.h"
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
  ESP_ERROR_BUBBLE(wifi_init());

  return ESP_OK;
}

void app_main(void) {
  esp_err_t initRet = appInit();
  if (initRet != ESP_OK) {
    // if there's an error initiating the app, delay then restart
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP_LOGW(TAG, "Failed to initiate the application - restarting");
    esp_restart();
    return;
  }

  ESP_LOGI(TAG, "Starting matrix test");
  displayBufferInit(&displayBuffer);
  displayBufferTest(displayBuffer);
  matrixShow(matrix, displayBuffer->buffer);
  ESP_LOGI(TAG, "Ended matrix test");

  uint8_t loops = 0;
  while (true) {
    ESP_LOGI(TAG, "LOOP!");

    if (loops == 10) {
      loops = 0;
      ESP_LOGI(TAG, "Starting wifi test");
      RequestContextHandle ctx;
      requestInit(&ctx);
      ctx->url = "https://illumindex.vercel.app/api/green";
      ctx->method = HTTP_METHOD_GET;
      esp_err_t reqRet = requestPerform(ctx);
      if (reqRet != ESP_OK || ctx->response->statusCode >= 400) {
        ESP_LOGI(TAG, "ERROR in wifi test");
      } else {
        cJSON *json =
            cJSON_ParseWithLength(ctx->response->data, ctx->response->length);
        if (json == NULL) {
          ESP_LOGE(TAG, "JSON is NULL");
        } else {
          const cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
          if (cJSON_IsString(data) && (data->valuestring != NULL)) {
            ESP_LOGI(TAG, "Parsed JSON - data: \"%s\"", data->valuestring);
          }
        }
        cJSON_Delete(json);
      }
      requestEnd(ctx);
      ESP_LOGI(TAG, "Ended wifi test");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    loops++;
  }
}
