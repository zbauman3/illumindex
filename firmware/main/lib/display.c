#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lib/display.h"
#include "lib/time.h"
#include "network/request.h"
#include "util/565_color.h"
#include "util/error_helpers.h"

static const char *TAG = "DISPLAY";
static const char *MAIN_TASK_NAME = "DISPLAY:MAIN_TASK";
static const char *ANIMATION_TASK_NAME = "DISPLAY:ANIMATION_TASK";

// maps month integers to strings. This is zero-indexed.
char *monthNameStrings[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void setState(DisplayBufferHandle db, CommandState *state) {
  if (state == NULL) {
    return;
  }

  if (commandStateHasColor(state)) {
    displayBufferSetColor(db, state->color);
  }

  if (commandStateHasFont(state)) {
    fontSetSize(db->font, state->fontSize);
  }

  if (commandStateHasPosition(state)) {
    displayBufferSetCursor(db, state->posX, state->posY);
  }
}

esp_err_t displayBuildAndShow(DisplayHandle display) {
  displayBufferClear(display->displayBuffer);

  CommandListNode *loopNode = display->commands->head;
  while (loopNode != NULL) {
    switch (loopNode->command->type) {
    case COMMAND_TYPE_STRING:
      setState(display->displayBuffer, loopNode->command->value.string->state);
      displayBufferDrawString(display->displayBuffer,
                              loopNode->command->value.string->value);
      break;
    case COMMAND_TYPE_LINE:
      setState(display->displayBuffer, loopNode->command->value.line->state);
      displayBufferDrawLine(display->displayBuffer,
                            loopNode->command->value.line->toX,
                            loopNode->command->value.line->toY);
      break;
    case COMMAND_TYPE_BITMAP:
      setState(display->displayBuffer, loopNode->command->value.bitmap->state);
      displayBufferDrawBitmap(display->displayBuffer,
                              loopNode->command->value.bitmap->width,
                              loopNode->command->value.bitmap->height,
                              loopNode->command->value.bitmap->data);
      break;
    case COMMAND_TYPE_SETSTATE:
      setState(display->displayBuffer,
               loopNode->command->value.setState->state);
      break;
    case COMMAND_TYPE_LINEFEED:
      displayBufferLineFeed(display->displayBuffer);
      break;
    case COMMAND_TYPE_ANIMATION: {
      loopNode->command->value.animation->lastShowFrame++;
      if (loopNode->command->value.animation->lastShowFrame >=
          loopNode->command->value.animation->frameCount) {
        loopNode->command->value.animation->lastShowFrame = 0;
      }

      uint8_t prevX = display->displayBuffer->cursor.x;
      uint8_t prevY = display->displayBuffer->cursor.y;
      displayBufferSetCursor(display->displayBuffer,
                             loopNode->command->value.animation->posX,
                             loopNode->command->value.animation->posY);

      displayBufferDrawBitmap(
          display->displayBuffer, loopNode->command->value.animation->width,
          loopNode->command->value.animation->height,
          loopNode->command->value.animation->frames +
              (loopNode->command->value.animation->lastShowFrame *
               loopNode->command->value.animation->width *
               loopNode->command->value.animation->height));

      displayBufferSetCursor(display->displayBuffer, prevX, prevY);

      break;
    }
    case COMMAND_TYPE_TIME: {
      setState(display->displayBuffer, loopNode->command->value.time->state);

      char timeString[8];
      TimeInfo timeInfo;
      timeGet(&timeInfo);
      snprintf(timeString, sizeof(timeString), "%u:%u %s", timeInfo.hour12,
               timeInfo.minute, timeInfo.isPM ? "PM" : "AM");

      displayBufferDrawString(display->displayBuffer, timeString);
      break;
    }
    case COMMAND_TYPE_DATE: {
      setState(display->displayBuffer, loopNode->command->value.date->state);

      TimeInfo timeInfo;
      char timeString[13];
      timeGet(&timeInfo);
      snprintf(timeString, sizeof(timeString), "%s %u, %u",
               monthNameStrings[timeInfo.month - 1], timeInfo.dayOfMonth,
               timeInfo.year);

      displayBufferDrawString(display->displayBuffer, timeString);
      break;
    }
    }

    loopNode = loopNode->next;
  }

  displayBufferAddFeedback(
      display->displayBuffer, display->state->remoteStateInvalid,
      display->state->commandsInvalid, display->state->isDevMode);

  display->commands->hasShown = true;

  return matrixShow(display->matrix, display->displayBuffer->buffer);
}

esp_err_t fetchCommands(DisplayHandle display) {
  ESP_LOGD(MAIN_TASK_NAME, "FETCHING DATA");
  esp_err_t ret = ESP_OK;
  RequestContextHandle ctx;

  ESP_GOTO_ON_ERROR(requestInit(&ctx), displayFetchData_cleanup, MAIN_TASK_NAME,
                    "Error initiating the request context");

  ctx->url = display->state->commandEndpoint;
  ctx->method = HTTP_METHOD_GET;
  if (display->lastEtag != NULL) {
    requestEtagInit(&ctx->eTag);
    requestEtagCopy(ctx->eTag, display->lastEtag);
  }

  ESP_GOTO_ON_ERROR(requestPerform(ctx), displayFetchData_cleanup,
                    MAIN_TASK_NAME, "Error fetching data from endpoint");

  if (ctx->response->statusCode == 304) {
    ESP_LOGD(MAIN_TASK_NAME, "Content not modified. Not updating buffer.");
    goto displayFetchData_cleanup;
  }

  ESP_GOTO_ON_FALSE(ctx->response->statusCode < 300, ESP_ERR_INVALID_RESPONSE,
                    displayFetchData_cleanup, MAIN_TASK_NAME,
                    "Invalid response status code \"%d\"",
                    ctx->response->statusCode);

  if (ctx->response->eTag == NULL) {
    ESP_LOGD(MAIN_TASK_NAME, "ETag is null");
  } else {
    requestEtagInit(&display->lastEtag);
    requestEtagCopy(display->lastEtag, ctx->response->eTag);
  }

  CommandListHandle newCommands;
  ESP_GOTO_ON_ERROR(
      parseCommands(&newCommands, ctx->response->data, ctx->response->length),
      displayFetchData_cleanup, MAIN_TASK_NAME,
      "Invalid JSON response or content length");

  // hot-swap commands and cleanup the old one
  CommandListHandle oldCommands = display->commands;
  display->commands = newCommands;
  commandListCleanup(oldCommands);

  display->state->commandsInvalid = false;

displayFetchData_cleanup:
  requestEnd(ctx);
  if (ret != ESP_OK) {
    requestEtagEnd(&display->lastEtag);
  }
  return ret;
}

void mainTask(void *pvParameters) {
  DisplayHandle display = (DisplayHandle)pvParameters;

  // the `loopSeconds` could be really long, so instead of using that for
  // `vTaskDelay`, we use a separate variable to track the delay in increments
  // of 1s.
  while (true) {
    if (display->state->loopSeconds >= display->state->fetchInterval) {
      if (fetchCommands(display) == ESP_OK) {
        display->state->loopSeconds = 0;
        display->state->fetchFailureCount = 0;
      } else {
        // try again in 5 seconds if possible, otherwise 1 second
        if (display->state->fetchInterval >= 5) {
          display->state->loopSeconds = display->state->fetchInterval - 5;
        } else {
          display->state->loopSeconds = display->state->fetchInterval;
        }

        display->state->fetchFailureCount++;
        if (display->state->fetchFailureCount > 5) {
          display->state->commandsInvalid = true;
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

  while (true) {
    // We only need to do another "show" if there is an animation that needs to
    // be updated, or if this is a new command list
    if (!display->commands->hasShown || display->commands->hasAnimation) {
      displayBuildAndShow(display);
    }
    // max animation speed is limited by the freertos tick...
    vTaskDelay(display->commands->config.animationDelay / portTICK_PERIOD_MS);
  }
}

esp_err_t displayInit(DisplayHandle *displayHandle,
                      MatrixInitConfig *matrixConfig) {
  // allocate the the display
  DisplayHandle display = (DisplayHandle)malloc(sizeof(Display));

  // reset ETag
  display->lastEtag = NULL;

  // setup matrix
  ESP_ERROR_BUBBLE(matrixInit(&display->matrix, matrixConfig));
  ESP_ERROR_BUBBLE(matrixStart(display->matrix));

  // setup db
  ESP_ERROR_BUBBLE(displayBufferInit(
      &display->displayBuffer, matrixConfig->width, matrixConfig->height));

  // setup state
  ESP_ERROR_BUBBLE(stateInit(&display->state));

  // setup commands
  commandListInit(&display->commands);

  CommandHandle startCommand;
  ESP_ERROR_BUBBLE(commandListNodeInit(display->commands, COMMAND_TYPE_STRING,
                                       &startCommand));

  commandStateInit(&startCommand->value.string->state);

  startCommand->value.string->state->color = RGB_TO_565(0, 255, 149);
  commandStateSetColorFlag(startCommand->value.string->state);

  startCommand->value.string->state->posX = 0;
  startCommand->value.string->state->posY =
      (display->displayBuffer->height / 2) - 8;
  commandStateSetPositionFlag(startCommand->value.string->state);

  startCommand->value.string->state->fontSize = FONT_SIZE_LG;
  commandStateSetFontFlag(startCommand->value.string->state);

  startCommand->value.string->value = malloc(sizeof(char) * 9);
  strcpy(startCommand->value.string->value, "Starting");

  displayBuildAndShow(display);

  // pass back the display
  *displayHandle = display;

  return ESP_OK;
}

void displayEnd(DisplayHandle display) {
  matrixStop(display->matrix);
  matrixEnd(display->matrix);
  displayBufferEnd(display->displayBuffer);
  stateEnd(display->state);
  commandListCleanup(display->commands);

  TaskHandle_t taskHandle;
  taskHandle = xTaskGetHandle(MAIN_TASK_NAME);
  if (taskHandle != NULL && taskHandle == display->mainTaskHandle) {
    vTaskDelete(display->mainTaskHandle);
  }

  taskHandle = xTaskGetHandle(ANIMATION_TASK_NAME);
  if (taskHandle != NULL && taskHandle == display->animationTaskHandle) {
    vTaskDelete(display->animationTaskHandle);
  }

  if (display->lastEtag != NULL) {
    free(display->lastEtag);
  }
  free(display);
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

  BaseType_t taskCreate;
  taskCreate = xTaskCreatePinnedToCore(animationTask, ANIMATION_TASK_NAME, 4096,
                                       display, tskIDLE_PRIORITY + 2,
                                       &display->animationTaskHandle, 1);

  if (taskCreate != pdPASS || display->animationTaskHandle == NULL) {
    ESP_LOGE(TAG, "Failed to create task '%s'", ANIMATION_TASK_NAME);
    return ESP_ERR_INVALID_STATE;
  }

  taskCreate = xTaskCreatePinnedToCore(mainTask, MAIN_TASK_NAME, 4096, display,
                                       tskIDLE_PRIORITY + 1,
                                       &display->mainTaskHandle, 1);

  if (taskCreate != pdPASS || display->mainTaskHandle == NULL) {
    ESP_LOGE(TAG, "Failed to create task '%s'", MAIN_TASK_NAME);
    return ESP_ERR_INVALID_STATE;
  }

  return ESP_OK;
}
