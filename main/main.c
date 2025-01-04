#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "lib/cjson/cJSON.h"
#include "network/request.h"
#include "util/setup.h"

#define BTN_1 GPIO_NUM_21
#define BTN_2 GPIO_NUM_19

static const char *TAG = "APP_MAIN";

void buttonTask(void *pvParameters) {
  for (;;) {
    char *endpoint = "https://api-tests-ten.vercel.app/esp/green";

    ESP_LOGI(TAG, "Fetching: \"%s\"", endpoint);

    request_ctx_t ctx = requestCtxCreate(endpoint);

    getRequest(&ctx);
    ESP_LOGI(TAG, "GOT DATA: \"%s\"", ctx.data);

    cJSON *json = cJSON_ParseWithLength(ctx.data, ctx.length);
    if (json == NULL) {
      ESP_LOGE(TAG, "JSON is NULL");
    } else {
      const cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
      if (cJSON_IsString(data) && (data->valuestring != NULL)) {
        ESP_LOGI(TAG, "Parsed JSON - data: \"%s\"", data->valuestring);
      }
    }

    cJSON_Delete(json);
    requestCtxCleanup(ctx);

    vTaskDelay(30000 / portTICK_PERIOD_MS);
  }
}

void app_main(void) {
  esp_err_t init_ret = app_init();
  if (init_ret != ESP_OK) {
    // if there's an error initiating the app, delay then restart
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP_LOGW(TAG, "Failed to initiate the application - restarting");
    esp_restart();
    return;
  }

  xTaskCreate(buttonTask, "BUTTONS", 4096, NULL, 1, NULL);
}
