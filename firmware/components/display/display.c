#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "color_utils.h"
#include "helper_utils.h"
#include "network/fetch.h"
#include "time_util.h"

#include "display.h"

static const char *TAG = "DISPLAY";
static const char *FETCH_TASK_NAME = "DISPLAY:FETCH_TASK";
static const char *ANIMATION_TASK_NAME = "DISPLAY:ANIMATION_TASK";

// maps month integers to strings. This is zero-indexed.
char *month_name_strings[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void set_state(display_buffer_handle_t db, command_state_t *state) {
  if (state == NULL) {
    return;
  }

  if (command_state_has_color(state)) {
    display_buffer_set_color(db, state->color_red, state->color_green,
                             state->color_blue);
  }

  if (command_state_has_font(state)) {
    font_set_size(db->font, state->font_size);
  }

  if (command_state_has_position(state)) {
    display_buffer_set_cursor(db, state->pos_x, state->pos_y);
  }
}

void apply_command_list(display_handle_t display,
                        command_list_handle_t command_list,
                        bool is_in_animation) {
  command_list_node_t *loopNode = command_list->head;
  while (loopNode != NULL) {
    switch (loopNode->command->type) {
    case COMMAND_TYPE_STRING: {
      set_state(display->display_buffer,
                loopNode->command->value.string->state);
      display_buffer_draw_string(display->display_buffer,
                                 loopNode->command->value.string->value);
      break;
    }
    case COMMAND_TYPE_LINE: {
      set_state(display->display_buffer, loopNode->command->value.line->state);
      display_buffer_draw_line(display->display_buffer,
                               loopNode->command->value.line->to_x,
                               loopNode->command->value.line->to_y);
      break;
    }
    case COMMAND_TYPE_BITMAP: {
      set_state(display->display_buffer,
                loopNode->command->value.bitmap->state);
      display_buffer_draw_bitmap(
          display->display_buffer, loopNode->command->value.bitmap->width,
          loopNode->command->value.bitmap->height,
          loopNode->command->value.bitmap->data_red,
          loopNode->command->value.bitmap->data_green,
          loopNode->command->value.bitmap->data_blue, true);
      break;
    }
    case COMMAND_TYPE_SETSTATE: {
      set_state(display->display_buffer,
                loopNode->command->value.set_state->state);
      break;
    }
    case COMMAND_TYPE_LINEFEED: {
      display_buffer_line_feed(display->display_buffer);
      break;
    }
    case COMMAND_TYPE_ANIMATION: {
      if (is_in_animation) {
        ESP_LOGW(
            TAG,
            "Nested animations are not supported. Skipping inner animation.");
        break;
      }
      loopNode->command->value.animation->last_show_frame++;
      if (loopNode->command->value.animation->last_show_frame >=
          loopNode->command->value.animation->frame_count) {
        loopNode->command->value.animation->last_show_frame = 0;
      }

      apply_command_list(
          display,
          loopNode->command->value.animation
              ->frames[loopNode->command->value.animation->last_show_frame],
          true);

      break;
    }
    case COMMAND_TYPE_TIME: {
      set_state(display->display_buffer, loopNode->command->value.time->state);

      char timeString[8];
      time_util_info_t time_info;
      time_util_get(&time_info);
      snprintf(timeString, sizeof(timeString), "%u:%02u %s", time_info.hour12,
               time_info.minute, time_info.isPM ? "PM" : "AM");

      display_buffer_draw_string(display->display_buffer, timeString);
      break;
    }
    case COMMAND_TYPE_DATE: {
      set_state(display->display_buffer, loopNode->command->value.date->state);

      time_util_info_t time_info;
      char timeString[13];
      time_util_get(&time_info);
      snprintf(timeString, sizeof(timeString), "%s %u, %u",
               month_name_strings[time_info.month - 1], time_info.dayOfMonth,
               time_info.year);

      display_buffer_draw_string(display->display_buffer, timeString);
      break;
    }
    case COMMAND_TYPE_GRAPH: {
      set_state(display->display_buffer, loopNode->command->value.graph->state);
      display_buffer_draw_graph(display->display_buffer,
                                loopNode->command->value.graph->width,
                                loopNode->command->value.graph->height,
                                loopNode->command->value.graph->values,
                                loopNode->command->value.graph->bg_color_red,
                                loopNode->command->value.graph->bg_color_green,
                                loopNode->command->value.graph->bg_color_blue);
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
  display_buffer_clear(display->display_buffer);
  display_buffer_set_cursor(display->display_buffer, 0, 0);

  apply_command_list(display, display->commands, false);

  display_buffer_add_feedback(
      display->display_buffer, display->state->invalid_remote_state,
      display->state->invalid_commands, display->state->invalid_wifi_state,
      display->state->is_dev_mode);

  display->commands->has_shown = true;

  return led_matrix_show(display->matrix, display->display_buffer->buffer_red,
                         display->display_buffer->buffer_green,
                         display->display_buffer->buffer_blue);
}

esp_err_t fetch_commands(display_handle_t display) {
  ESP_LOGD(FETCH_TASK_NAME, "FETCHING DATA");
  esp_err_t ret = ESP_OK;
  fetch_ctx_handle_t ctx;

  ESP_GOTO_ON_ERROR(fetch_init(&ctx), fetch_commands_cleanup, FETCH_TASK_NAME,
                    "Error initiating the request context");

  ctx->url = display->state->command_endpoint;
  ctx->method = HTTP_METHOD_GET;
  if (display->last_etag != NULL) {
    if (fetch_etag_init(&ctx->etag) == ESP_OK) {
      fetch_etag_copy(ctx->etag, display->last_etag);
    } else {
      ESP_LOGW(FETCH_TASK_NAME, "Failed to init ETag for request");
    }
  }

  ESP_GOTO_ON_ERROR(fetch_perform(ctx), fetch_commands_cleanup, FETCH_TASK_NAME,
                    "Error fetching data from endpoint");

  if (ctx->response->status_code == 304) {
    ESP_LOGD(FETCH_TASK_NAME, "Content not modified. Not updating buffer.");
    goto fetch_commands_cleanup;
  }

  ESP_GOTO_ON_FALSE(ctx->response->status_code < 300, ESP_ERR_INVALID_RESPONSE,
                    fetch_commands_cleanup, FETCH_TASK_NAME,
                    "Invalid response status code \"%d\"",
                    ctx->response->status_code);

  if (ctx->response->etag == NULL) {
    ESP_LOGD(FETCH_TASK_NAME, "ETag is null");
  } else {
    if (fetch_etag_init(&display->last_etag) == ESP_OK) {
      fetch_etag_copy(display->last_etag, ctx->response->etag);
    } else {
      ESP_LOGW(FETCH_TASK_NAME, "Failed to init ETag storage");
    }
  }

  command_list_handle_t newCommands;
  ESP_GOTO_ON_ERROR(command_list_parse(&newCommands, ctx->response->data,
                                       ctx->response->length),
                    fetch_commands_cleanup, FETCH_TASK_NAME,
                    "Invalid JSON response or content length");

  // hot-swap commands and cleanup the old one
  command_list_handle_t oldCommands = display->commands;
  display->commands = newCommands;
  command_list_end(oldCommands);

  display->state->invalid_commands = false;

fetch_commands_cleanup:
  fetch_end(ctx);
  if (ret != ESP_OK) {
    fetch_etag_end(&display->last_etag);
  }
  return ret;
}

void fetch_task(void *pvParameters) {
  display_handle_t display = (display_handle_t)pvParameters;

  // the `loopSeconds` could be really long, so instead of using that for
  // `vTaskDelay`, we use a separate variable to track the delay in increments
  // of 1s.
  while (true) {
    if (display->state->loop_seconds >= display->state->fetch_interval) {
      // if wifi is not connected, skip fetch
      if (display->state->invalid_wifi_state) {
        display->state->loop_seconds = 0;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
      }

      if (fetch_commands(display) == ESP_OK) {
        display->state->loop_seconds = 0;
        display->state->fetch_failure_count = 0;
      } else {
        // try again in 5 seconds if possible, otherwise 1 second
        if (display->state->fetch_interval >= 5) {
          display->state->loop_seconds = display->state->fetch_interval - 5;
        } else {
          display->state->loop_seconds = display->state->fetch_interval;
        }

        display->state->fetch_failure_count++;
        if (display->state->fetch_failure_count > 5) {
          display->state->invalid_commands = true;
        }
      }
    } else {
      display->state->loop_seconds++;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void animation_task(void *pvParameters) {
  display_handle_t display = (display_handle_t)pvParameters;

  while (true) {
    // We only need to do another "show" if there is an animation that needs to
    // be updated, or if this is a new command list
    if (!display->commands->has_shown || display->commands->has_animation) {
      build_and_show(display);
    }
    // max animation speed is limited by the freertos tick...
    vTaskDelay(display->commands->config.animation_delay / portTICK_PERIOD_MS);
  }
}

esp_err_t display_init(display_handle_t *display_handle,
                       led_matrix_config_t *led_matrix_config) {
  // allocate the the display
  display_handle_t display = (display_handle_t)malloc(sizeof(display_t));
  if (display == NULL) {
    ESP_LOGE(TAG, "Failed to allocate display handle");
    *display_handle = NULL;
    return ESP_ERR_NO_MEM;
  }

  // reset ETag
  display->last_etag = NULL;

  // setup matrix
  ESP_ERROR_BUBBLE(led_matrix_init(&display->matrix, led_matrix_config));
  ESP_ERROR_BUBBLE(led_matrix_start(display->matrix));

  // setup db
  ESP_ERROR_BUBBLE(display_buffer_init(&display->display_buffer,
                                       led_matrix_config->width,
                                       led_matrix_config->height));

  // setup state
  ESP_ERROR_BUBBLE(state_init(&display->state));

  // setup commands
  ESP_ERROR_BUBBLE(command_list_init(&display->commands));

  command_handle_t startCommand;
  ESP_ERROR_BUBBLE(command_list_node_init(display->commands,
                                          COMMAND_TYPE_STRING, &startCommand));

  ESP_ERROR_BUBBLE(command_state_init(&startCommand->value.string->state));

  startCommand->value.string->state->color_red = 0;
  startCommand->value.string->state->color_green = 255;
  startCommand->value.string->state->color_blue = 149;
  command_state_set_flag_color(startCommand->value.string->state);

  startCommand->value.string->state->pos_x = 0;
  startCommand->value.string->state->pos_y =
      (display->display_buffer->height / 2) - 8;
  command_state_set_flag_position(startCommand->value.string->state);

  startCommand->value.string->state->font_size = FONT_SIZE_LG;
  command_state_set_flag_font(startCommand->value.string->state);

  startCommand->value.string->value = malloc(sizeof(char) * 9);
  if (startCommand->value.string->value == NULL) {
    ESP_LOGE(TAG, "Failed to allocate start command string value");
    display_end(display);
    *display_handle = NULL;
    return ESP_ERR_NO_MEM;
  }
  strcpy(startCommand->value.string->value, "Starting");

  build_and_show(display);

  // pass back the display
  *display_handle = display;

  return ESP_OK;
}

void display_end(display_handle_t display) {
  led_matrix_stop(display->matrix);
  led_matrix_end(display->matrix);
  display_buffer_end(display->display_buffer);
  state_end(display->state);
  command_list_end(display->commands);

  TaskHandle_t taskHandle;
  taskHandle = xTaskGetHandle(FETCH_TASK_NAME);
  if (taskHandle != NULL && taskHandle == display->fetch_task_handle) {
    vTaskDelete(display->fetch_task_handle);
  }

  taskHandle = xTaskGetHandle(ANIMATION_TASK_NAME);
  if (taskHandle != NULL && taskHandle == display->animation_task_handle) {
    vTaskDelete(display->animation_task_handle);
  }

  free(display->last_etag);
  free(display);
}

esp_err_t display_start(display_handle_t display) {
  if (state_fetch_remote(display->state) != ESP_OK) {
    display->state->invalid_remote_state = true;
    ESP_LOGE(TAG, "Failed to fetch remote state");
  }

  if (display->state->is_dev_mode) {
    ESP_LOGI(TAG, "Running in dev mode");
  }

  ESP_LOGI(TAG, "Pointing to \"%s\"", display->state->command_endpoint);
  ESP_LOGI(TAG, "Fetch interval \"%u\"", display->state->fetch_interval);

  BaseType_t taskCreate;
  taskCreate = xTaskCreatePinnedToCore(animation_task, ANIMATION_TASK_NAME,
                                       4096, display, tskIDLE_PRIORITY + 2,
                                       &display->animation_task_handle, 0);

  if (taskCreate != pdPASS || display->animation_task_handle == NULL) {
    ESP_LOGE(TAG, "Failed to create task '%s'", ANIMATION_TASK_NAME);
    return ESP_ERR_INVALID_STATE;
  }

  taskCreate = xTaskCreatePinnedToCore(fetch_task, FETCH_TASK_NAME, 4096,
                                       display, tskIDLE_PRIORITY + 1,
                                       &display->fetch_task_handle, 0);

  if (taskCreate != pdPASS || display->fetch_task_handle == NULL) {
    ESP_LOGE(TAG, "Failed to create task '%s'", FETCH_TASK_NAME);
    return ESP_ERR_INVALID_STATE;
  }

  return ESP_OK;
}
