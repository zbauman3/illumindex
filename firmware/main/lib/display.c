#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lib/display.h"
#include "network/request.h"
#include "util/565_color.h"
#include "util/commands.h"
#include "util/error_helpers.h"

static const char *TAG = "DISPLAY";
static const char *MAIN_TASK_NAME = "DISPLAY:MAIN_TASK";
static const char *ANIMATION_TASK_NAME = "DISPLAY:ANIMATION_TASK";

esp_err_t displaySend(DisplayHandle display) {
  displayBufferAddFeedback(
      display->displayBuffer, display->state->remoteStateInvalid,
      display->state->commandsInvalid, display->state->isDevMode);
  return matrixShow(display->matrix, display->displayBuffer->buffer);
}

esp_err_t fetchAndDisplayData(DisplayHandle display) {
  ESP_LOGD(MAIN_TASK_NAME, "FETCHING DATA");
  esp_err_t ret = ESP_OK;
  RequestContextHandle ctx;

  ESP_GOTO_ON_ERROR(requestInit(&ctx), displayFetchData_cleanup, MAIN_TASK_NAME,
                    "Error initiating the request context");

  ctx->url = display->state->commandEndpoint;
  ctx->method = HTTP_METHOD_GET;

  ESP_GOTO_ON_ERROR(requestPerform(ctx), displayFetchData_cleanup,
                    MAIN_TASK_NAME, "Error fetching data from endpoint");

  ESP_GOTO_ON_FALSE(ctx->response->statusCode < 300, ESP_ERR_INVALID_RESPONSE,
                    displayFetchData_cleanup, MAIN_TASK_NAME,
                    "Invalid response status code \"%d\"",
                    ctx->response->statusCode);

  // stop the animation task to prevent it from updating the display buffer
  vTaskSuspend(display->animationTaskHandle);

  displayBufferClear(display->displayBuffer);
  displayBufferSetCursor(display->displayBuffer, 0, 0);
  displayBufferSetColor(display->displayBuffer, 0b1111111111111111);

  ESP_GOTO_ON_ERROR(parseAndShowCommands(display->displayBuffer,
                                         ctx->response->data,
                                         ctx->response->length),
                    displayFetchData_cleanup, MAIN_TASK_NAME,
                    "Invalid JSON response or content length");

  if (display->displayBuffer->animation != NULL) {
    ESP_LOGD(MAIN_TASK_NAME, "Animation detected. Starting %s.",
             ANIMATION_TASK_NAME);
    vTaskResume(display->animationTaskHandle);
  } else {
    ESP_LOGD(MAIN_TASK_NAME, "No animation. Leaving %s suspended.",
             ANIMATION_TASK_NAME);
  }

  display->state->commandsInvalid = false;
  ESP_GOTO_ON_ERROR(displaySend(display), displayFetchData_cleanup,
                    MAIN_TASK_NAME, "Error setting the new buffer");

displayFetchData_cleanup:
  requestEnd(ctx);
  return ret;
}

void mainTask(void *pvParameters) {
  DisplayHandle display = (DisplayHandle)pvParameters;

  for (;;) {
    if (display->state->loopSeconds >= display->state->fetchInterval) {
      if (fetchAndDisplayData(display) == ESP_OK) {
        display->state->loopSeconds = 0;
        display->state->fetchFailureCount = 0;
      } else {
        // try again in 5 seconds
        display->state->loopSeconds = display->state->fetchInterval - 5;

        display->state->fetchFailureCount++;
        if (display->state->fetchFailureCount > 5) {
          display->state->commandsInvalid = true;
          displaySend(display);
        }
      }
    } else {
      display->state->loopSeconds++;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void animationTask(void *pvParameters) {
  DisplayHandle display = (DisplayHandle)pvParameters;

  for (;;) {
  animationTask_restart:
    if (display->displayBuffer->animation == NULL) {
      ESP_LOGI(ANIMATION_TASK_NAME, "Ran with NULL. Suspending self.");
      vTaskSuspend(NULL);
      // use a goto to make sure we always check the state after resuming
      goto animationTask_restart;
    }

    displayBufferAnimationShowNext(display->displayBuffer);
    displaySend(display);
    // max animation speed is limited by the freertos tick...
    vTaskDelay(display->displayBuffer->animation->delay / portTICK_PERIOD_MS);
  }
}

esp_err_t displayInit(DisplayHandle *displayHandle,
                      MatrixInitConfig *matrixConfig) {
  // allocate the the display
  DisplayHandle display = (DisplayHandle)malloc(sizeof(Display));

  // setup matrix
  ESP_ERROR_BUBBLE(matrixInit(&display->matrix, matrixConfig));
  ESP_ERROR_BUBBLE(matrixStart(display->matrix));

  // setup db
  ESP_ERROR_BUBBLE(displayBufferInit(
      &display->displayBuffer, matrixConfig->width, matrixConfig->height));

  // setup state
  ESP_ERROR_BUBBLE(stateInit(&display->state));

  // show "starting" text
  displayBufferSetColor(display->displayBuffer, RGB_TO_565(0, 255, 149));
  displayBufferSetCursor(display->displayBuffer, 0,
                         (display->displayBuffer->height / 2) - 8);
  fontSetSize(display->displayBuffer->font, FONT_SIZE_LG);
  displayBufferDrawString(display->displayBuffer, "Starting");
  displaySend(display);

  // pass back the display
  *displayHandle = display;

  return ESP_OK;
}

void displayEnd(DisplayHandle display) {
  matrixStop(display->matrix);
  matrixEnd(display->matrix);
  displayBufferEnd(display->displayBuffer);
  stateEnd(display->state);

  TaskHandle_t taskHandle;
  taskHandle = xTaskGetHandle(MAIN_TASK_NAME);
  if (taskHandle != NULL && taskHandle == display->mainTaskHandle) {
    vTaskDelete(display->mainTaskHandle);
  }

  taskHandle = xTaskGetHandle(ANIMATION_TASK_NAME);
  if (taskHandle != NULL && taskHandle == display->animationTaskHandle) {
    vTaskDelete(display->animationTaskHandle);
  }
}

esp_err_t displayStart(DisplayHandle display) {
  if (stateFetchRemote(display->state) != ESP_OK) {
    display->state->remoteStateInvalid = true;
    ESP_LOGE(TAG, "Failed to fetch remote state");
  }

  if (display->state->isDevMode) {
    ESP_LOGI(TAG, "Running in dev mode");
  }

  ESP_LOGI(TAG, "Pointing to \"%s\"", display->state->commandEndpoint);
  ESP_LOGI(TAG, "Fetch interval \"%u\"", display->state->fetchInterval);

  displaySend(display);

  BaseType_t taskCreate;
  taskCreate = xTaskCreatePinnedToCore(mainTask, MAIN_TASK_NAME, 4096, display,
                                       tskIDLE_PRIORITY + 1,
                                       &display->mainTaskHandle, 1);

  if (taskCreate != pdPASS || display->mainTaskHandle == NULL) {
    ESP_LOGE(TAG, "Failed to create task '%s'", MAIN_TASK_NAME);
    return ESP_ERR_INVALID_STATE;
  }

  taskCreate = xTaskCreatePinnedToCore(animationTask, ANIMATION_TASK_NAME, 4096,
                                       display, tskIDLE_PRIORITY + 2,
                                       &display->animationTaskHandle, 1);

  if (taskCreate != pdPASS || display->animationTaskHandle == NULL) {
    ESP_LOGE(TAG, "Failed to create task '%s'", ANIMATION_TASK_NAME);
    return ESP_ERR_INVALID_STATE;
  }

  return ESP_OK;
}
