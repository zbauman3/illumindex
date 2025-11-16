#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "color_utils.h"
#include "helper_utils.h"
#include "network/fetch.h"
#include "time_util.h"

#include "lib/display.h"

static const char *TAG = "DISPLAY";
static const char *MAIN_TASK_NAME = "DISPLAY:MAIN_TASK";
static const char *ANIMATION_TASK_NAME = "DISPLAY:ANIMATION_TASK";

// maps month integers to strings. This is zero-indexed.
char *month_name_strings[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void set_state(display_buffer_handle_t db, command_state_t *state) {
  if (state == NULL) {
    return;
  }

  if (command_state_has_color(state)) {
    display_buffer_set_color(db, state->colorRed, state->colorGreen,
                             state->colorBlue);
  }

  if (command_state_has_font(state)) {
    font_set_size(db->font, state->fontSize);
  }

  if (command_state_has_position(state)) {
    display_buffer_set_cursor(db, state->posX, state->posY);
  }
}

void apply_command_list(display_handle_t display,
                        command_list_handle_t commandList,
                        bool isInsideAnimation) {
  command_list_node_t *loopNode = commandList->head;
  while (loopNode != NULL) {
    switch (loopNode->command->type) {
    case COMMAND_TYPE_STRING: {
      set_state(display->displayBuffer, loopNode->command->value.string->state);
      display_buffer_draw_string(display->displayBuffer,
                                 loopNode->command->value.string->value);
      break;
    }
    case COMMAND_TYPE_LINE: {
      set_state(display->displayBuffer, loopNode->command->value.line->state);
      display_buffer_draw_line(display->displayBuffer,
                               loopNode->command->value.line->toX,
                               loopNode->command->value.line->toY);
      break;
    }
    case COMMAND_TYPE_BITMAP: {
      set_state(display->displayBuffer, loopNode->command->value.bitmap->state);
      display_buffer_draw_bitmap(display->displayBuffer,
                                 loopNode->command->value.bitmap->width,
                                 loopNode->command->value.bitmap->height,
                                 loopNode->command->value.bitmap->dataRed,
                                 loopNode->command->value.bitmap->dataGreen,
                                 loopNode->command->value.bitmap->dataBlue);
      break;
    }
    case COMMAND_TYPE_SETSTATE: {
      set_state(display->displayBuffer,
                loopNode->command->value.setState->state);
      break;
    }
    case COMMAND_TYPE_LINEFEED: {
      display_buffer_line_feed(display->displayBuffer);
      break;
    }
    case COMMAND_TYPE_ANIMATION: {
      if (isInsideAnimation) {
        ESP_LOGW(
            TAG,
            "Nested animations are not supported. Skipping inner animation.");
        break;
      }
      loopNode->command->value.animation->lastShowFrame++;
      if (loopNode->command->value.animation->lastShowFrame >=
          loopNode->command->value.animation->frameCount) {
        loopNode->command->value.animation->lastShowFrame = 0;
      }

      apply_command_list(
          display,
          loopNode->command->value.animation
              ->frames[loopNode->command->value.animation->lastShowFrame],
          true);

      break;
    }
    case COMMAND_TYPE_TIME: {
      set_state(display->displayBuffer, loopNode->command->value.time->state);

      char timeString[8];
      time_util_info_t timeInfo;
      time_util_get(&timeInfo);
      snprintf(timeString, sizeof(timeString), "%u:%02u %s", timeInfo.hour12,
               timeInfo.minute, timeInfo.isPM ? "PM" : "AM");

      display_buffer_draw_string(display->displayBuffer, timeString);
      break;
    }
    case COMMAND_TYPE_DATE: {
      set_state(display->displayBuffer, loopNode->command->value.date->state);

      time_util_info_t timeInfo;
      char timeString[13];
      time_util_get(&timeInfo);
      snprintf(timeString, sizeof(timeString), "%s %u, %u",
               month_name_strings[timeInfo.month - 1], timeInfo.dayOfMonth,
               timeInfo.year);

      display_buffer_draw_string(display->displayBuffer, timeString);
      break;
    }
    case COMMAND_TYPE_GRAPH: {
      set_state(display->displayBuffer, loopNode->command->value.graph->state);
      display_buffer_draw_graph(display->displayBuffer,
                                loopNode->command->value.graph->width,
                                loopNode->command->value.graph->height,
                                loopNode->command->value.graph->values,
                                loopNode->command->value.graph->bgColorRed,
                                loopNode->command->value.graph->bgColorGreen,
                                loopNode->command->value.graph->bgColorBlue);
      break;
    }
    default: {
      ESP_LOGW(TAG, "Unknown command type %d", loopNode->command->type);
      break;
    }
    }

    loopNode = loopNode->next;
  }
}

esp_err_t build_and_show(display_handle_t display) {
  display_buffer_clear(display->displayBuffer);
  display_buffer_set_cursor(display->displayBuffer, 0, 0);

  apply_command_list(display, display->commands, false);

  display_buffer_add_feedback(
      display->displayBuffer, display->state->remoteStateInvalid,
      display->state->commandsInvalid, display->state->isDevMode);

  display->commands->hasShown = true;

  return led_matrix_show(display->matrix, display->displayBuffer->bufferRed,
                         display->displayBuffer->bufferGreen,
                         display->displayBuffer->bufferBlue);
}

esp_err_t fetch_commands(display_handle_t display) {
  ESP_LOGD(MAIN_TASK_NAME, "FETCHING DATA");
  esp_err_t ret = ESP_OK;
  fetch_ctx_handle_t ctx;

  ESP_GOTO_ON_ERROR(fetch_init(&ctx), fetch_commands_cleanup, MAIN_TASK_NAME,
                    "Error initiating the request context");

  ctx->url = display->state->commandEndpoint;
  ctx->method = HTTP_METHOD_GET;
  if (display->lastEtag != NULL) {
    fetch_etag_init(&ctx->eTag);
    fetch_etag_copy(ctx->eTag, display->lastEtag);
  }

  ESP_GOTO_ON_ERROR(fetch_perform(ctx), fetch_commands_cleanup, MAIN_TASK_NAME,
                    "Error fetching data from endpoint");

  if (ctx->response->status_code == 304) {
    ESP_LOGD(MAIN_TASK_NAME, "Content not modified. Not updating buffer.");
    goto fetch_commands_cleanup;
  }

  ESP_GOTO_ON_FALSE(ctx->response->status_code < 300, ESP_ERR_INVALID_RESPONSE,
                    fetch_commands_cleanup, MAIN_TASK_NAME,
                    "Invalid response status code \"%d\"",
                    ctx->response->status_code);

  if (ctx->response->eTag == NULL) {
    ESP_LOGD(MAIN_TASK_NAME, "ETag is null");
  } else {
    fetch_etag_init(&display->lastEtag);
    fetch_etag_copy(display->lastEtag, ctx->response->eTag);
  }

  command_list_handle_t newCommands;
  ESP_GOTO_ON_ERROR(command_list_parse(&newCommands, ctx->response->data,
                                       ctx->response->length),
                    fetch_commands_cleanup, MAIN_TASK_NAME,
                    "Invalid JSON response or content length");

  // hot-swap commands and cleanup the old one
  command_list_handle_t oldCommands = display->commands;
  display->commands = newCommands;
  command_list_end(oldCommands);

  display->state->commandsInvalid = false;

fetch_commands_cleanup:
  fetch_end(ctx);
  if (ret != ESP_OK) {
    fetch_etag_end(&display->lastEtag);
  }
  return ret;
}

void main_task(void *pvParameters) {
  display_handle_t display = (display_handle_t)pvParameters;

  // the `loopSeconds` could be really long, so instead of using that for
  // `vTaskDelay`, we use a separate variable to track the delay in increments
  // of 1s.
  while (true) {
    if (display->state->loopSeconds >= display->state->fetchInterval) {
      if (fetch_commands(display) == ESP_OK) {
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

void animation_task(void *pvParameters) {
  display_handle_t display = (display_handle_t)pvParameters;

  while (true) {
    // We only need to do another "show" if there is an animation that needs to
    // be updated, or if this is a new command list
    if (!display->commands->hasShown || display->commands->hasAnimation) {
      build_and_show(display);
    }
    // max animation speed is limited by the freertos tick...
    vTaskDelay(display->commands->config.animationDelay / portTICK_PERIOD_MS);
  }
}

esp_err_t display_init(display_handle_t *displayHandle,
                       led_matrix_config_t *matrixConfig) {
  // allocate the the display
  display_handle_t display = (display_handle_t)malloc(sizeof(display_t));

  // reset ETag
  display->lastEtag = NULL;

  // setup matrix
  ESP_ERROR_BUBBLE(led_matrix_init(&display->matrix, matrixConfig));
  ESP_ERROR_BUBBLE(led_matrix_start(display->matrix));

  // setup db
  ESP_ERROR_BUBBLE(display_buffer_init(
      &display->displayBuffer, matrixConfig->width, matrixConfig->height));

  // setup state
  ESP_ERROR_BUBBLE(state_init(&display->state));

  // setup commands
  command_list_init(&display->commands);

  command_handle_t startCommand;
  ESP_ERROR_BUBBLE(command_list_node_init(display->commands,
                                          COMMAND_TYPE_STRING, &startCommand));

  command_state_init(&startCommand->value.string->state);

  startCommand->value.string->state->colorRed = 0;
  startCommand->value.string->state->colorGreen = 255;
  startCommand->value.string->state->colorBlue = 149;
  command_state_set_flag_color(startCommand->value.string->state);

  startCommand->value.string->state->posX = 0;
  startCommand->value.string->state->posY =
      (display->displayBuffer->height / 2) - 8;
  command_state_set_flag_position(startCommand->value.string->state);

  startCommand->value.string->state->fontSize = FONT_SIZE_LG;
  command_state_set_flag_font(startCommand->value.string->state);

  startCommand->value.string->value = malloc(sizeof(char) * 9);
  strcpy(startCommand->value.string->value, "Starting");

  build_and_show(display);

  // pass back the display
  *displayHandle = display;

  return ESP_OK;
}

void display_end(display_handle_t display) {
  led_matrix_stop(display->matrix);
  led_matrix_end(display->matrix);
  display_buffer_end(display->displayBuffer);
  state_end(display->state);
  command_list_end(display->commands);

  TaskHandle_t taskHandle;
  taskHandle = xTaskGetHandle(MAIN_TASK_NAME);
  if (taskHandle != NULL && taskHandle == display->mainTaskHandle) {
    vTaskDelete(display->mainTaskHandle);
  }

  taskHandle = xTaskGetHandle(ANIMATION_TASK_NAME);
  if (taskHandle != NULL && taskHandle == display->animationTaskHandle) {
    vTaskDelete(display->animationTaskHandle);
  }

  free(display->lastEtag);
  free(display);
}

esp_err_t display_start(display_handle_t display) {
  if (state_fetch_remote(display->state) != ESP_OK) {
    display->state->remoteStateInvalid = true;
    ESP_LOGE(TAG, "Failed to fetch remote state");
  }

  if (display->state->isDevMode) {
    ESP_LOGI(TAG, "Running in dev mode");
  }

  ESP_LOGI(TAG, "Pointing to \"%s\"", display->state->commandEndpoint);
  ESP_LOGI(TAG, "Fetch interval \"%u\"", display->state->fetchInterval);

  BaseType_t taskCreate;
  taskCreate = xTaskCreatePinnedToCore(animation_task, ANIMATION_TASK_NAME,
                                       4096, display, tskIDLE_PRIORITY + 2,
                                       &display->animationTaskHandle, 0);

  if (taskCreate != pdPASS || display->animationTaskHandle == NULL) {
    ESP_LOGE(TAG, "Failed to create task '%s'", ANIMATION_TASK_NAME);
    return ESP_ERR_INVALID_STATE;
  }

  taskCreate = xTaskCreatePinnedToCore(main_task, MAIN_TASK_NAME, 4096, display,
                                       tskIDLE_PRIORITY + 1,
                                       &display->mainTaskHandle, 0);

  if (taskCreate != pdPASS || display->mainTaskHandle == NULL) {
    ESP_LOGE(TAG, "Failed to create task '%s'", MAIN_TASK_NAME);
    return ESP_ERR_INVALID_STATE;
  }

  return ESP_OK;
}
