#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "network/request.h"
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

  request_ctx_t ctx =
      requestCtxCreate("https://api-tests-ten.vercel.app/esp/green");

  getRequest(&ctx);

  ESP_LOGI(TAG, "GOT DATA: \"%s\"", ctx.data);

  requestCtxCleanup(ctx);
}
