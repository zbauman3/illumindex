#include "cJSON.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>

#include "gfx/display_buffer.h"
#include "time_util.h"

#include "lib/commands.h"

static const char *TAG = "COMMANDS";

#define invalid_shape_warn(type)                                               \
  ESP_LOGW(TAG, "command_t of type '%s' does not have a valid shape", type)

#define invalid_prop_warn(type, prop)                                          \
  ESP_LOGW(TAG, "command_t of type '%s' has an invalid '%s'", type, prop)

void parse_command_array(command_list_handle_t command_list_handle,
                         const cJSON *commandArray, bool is_in_animation);

// --------
// Below are the functions related to initiating and cleaning up the structures
// for commands. Ideally, all memory allocation and deallocation should occur
// within these functions
// --------

void command_state_init(command_state_t **state_handle) {
  command_state_t *state = (command_state_t *)malloc(sizeof(command_state_t));

  state->color_red = 255;
  state->color_green = 255;
  state->color_blue = 255;
  state->font_size = FONT_SIZE_MD;
  state->pos_x = 0;
  state->pos_y = 0;
  state->flags = 0;

  *state_handle = state;
}

void command_state_end(command_state_t *state) {
  // state could possibly be null. But we only are calling _free_ so NBD
  free(state);
}

esp_err_t command_init(command_handle_t *command_handle,
                       command_type_enum_t type) {
  command_handle_t command = (command_t *)malloc(sizeof(command_t));
  command->type = type;

  switch (command->type) {
  case COMMAND_TYPE_STRING:
    command->value.string =
        (command_value_string_t *)malloc(sizeof(command_value_string_t));
    command->value.string->state = NULL;
    command->value.string->value = NULL;
    break;
  case COMMAND_TYPE_LINE:
    command->value.line =
        (command_value_line_t *)malloc(sizeof(command_value_line_t));
    command->value.line->state = NULL;
    break;
  case COMMAND_TYPE_BITMAP:
    command->value.bitmap =
        (command_value_bitmap_t *)malloc(sizeof(command_value_bitmap_t));
    command->value.bitmap->state = NULL;
    command->value.bitmap->data_red = NULL;
    command->value.bitmap->data_green = NULL;
    command->value.bitmap->data_blue = NULL;
    break;
  case COMMAND_TYPE_SETSTATE:
    command->value.set_state =
        (command_value_set_state_t *)malloc(sizeof(command_value_set_state_t));
    command->value.set_state->state = NULL;
    break;
  case COMMAND_TYPE_LINEFEED:
    command->value.line_feed =
        (command_value_line_feed_t *)malloc(sizeof(command_value_line_feed_t));
    break;
  case COMMAND_TYPE_ANIMATION:
    command->value.animation =
        (command_value_animation_t *)malloc(sizeof(command_value_animation_t));
    command->value.animation->frame_count = 0;
    command->value.animation->last_show_frame = 0;
    command->value.animation->frames = NULL;
    break;
  case COMMAND_TYPE_TIME:
    command->value.time =
        (command_value_time_t *)malloc(sizeof(command_value_time_t));
    command->value.time->state = NULL;
    break;
  case COMMAND_TYPE_DATE:
    command->value.date =
        (command_value_date_t *)malloc(sizeof(command_value_date_t));
    command->value.date->state = NULL;
    break;
  case COMMAND_TYPE_GRAPH:
    command->value.graph =
        (command_value_graph_t *)malloc(sizeof(command_value_graph_t));
    command->value.graph->state = NULL;
    command->value.graph->height = 0;
    command->value.graph->width = 0;
    command->value.graph->values = NULL;
    command->value.graph->bg_color_red = 0;
    command->value.graph->bg_color_green = 0;
    command->value.graph->bg_color_blue = 0;
    break;
  default:
    free(command);
    return ESP_ERR_INVALID_ARG;
  }

  *command_handle = command;

  return ESP_OK;
}

void command_end(command_handle_t command) {
  switch (command->type) {
  case COMMAND_TYPE_STRING:
    command_state_end(command->value.string->state);
    free(command->value.string->value);
    free(command->value.string);
    break;
  case COMMAND_TYPE_LINE:
    command_state_end(command->value.line->state);
    free(command->value.line);
    break;
  case COMMAND_TYPE_BITMAP:
    command_state_end(command->value.bitmap->state);
    free(command->value.bitmap->data_red);
    free(command->value.bitmap->data_green);
    free(command->value.bitmap->data_blue);
    free(command->value.bitmap);
    break;
  case COMMAND_TYPE_SETSTATE:
    command_state_end(command->value.set_state->state);
    free(command->value.set_state);
    break;
  case COMMAND_TYPE_LINEFEED:
    free(command->value.line_feed);
    break;
  case COMMAND_TYPE_ANIMATION:
    for (uint16_t i = 0; i < command->value.animation->frame_count; i++) {
      command_list_end(command->value.animation->frames[i]);
      command->value.animation->frames[i] = NULL;
    }
    free(command->value.animation->frames);
    free(command->value.animation);
    break;
  case COMMAND_TYPE_TIME:
    command_state_end(command->value.time->state);
    free(command->value.time);
    break;
  case COMMAND_TYPE_DATE:
    command_state_end(command->value.date->state);
    free(command->value.date);
    break;
  case COMMAND_TYPE_GRAPH:
    command_state_end(command->value.graph->state);
    free(command->value.graph->values);
    free(command->value.graph);
    break;
  }

  free(command);
}

esp_err_t command_list_node_init(command_list_handle_t command_list,
                                 command_type_enum_t type,
                                 command_handle_t *command_handle) {
  esp_err_t cmdRet = command_init(command_handle, type);
  if (cmdRet != ESP_OK) {
    return cmdRet;
  }

  command_list_node_t *newNode =
      (command_list_node_t *)malloc(sizeof(command_list_node_t));
  newNode->command = *command_handle;
  newNode->next = NULL;

  // no items yet, add as the first item
  if (command_list->head == NULL || command_list->tail == NULL) {
    command_list->head = newNode;
    command_list->tail = newNode;
  } else {
    command_list->tail->next = newNode;
    command_list->tail = newNode;
  }

  if (type == COMMAND_TYPE_ANIMATION) {
    command_list->has_animation = true;
  }

  return ESP_OK;
}

void command_list_node_end(command_list_node_t *commandListNode) {
  command_end(commandListNode->command);
  commandListNode->next = NULL;
  free(commandListNode);
}

void command_list_init(command_list_handle_t *command_list_handle) {
  command_list_handle_t command_list =
      (command_list_t *)malloc(sizeof(command_list_t));

  command_list->head = NULL;
  command_list->tail = NULL;
  command_list->has_animation = false;
  command_list->has_shown = false;
  command_list->config.animation_delay = COMMAND_CONFIG_ANIMATION_DELAY_DEFAULT;

  *command_list_handle = command_list;
}

void command_list_end(command_list_handle_t command_list) {
  command_list_node_t *current;
  while (command_list->head != NULL) {
    // save current head
    current = command_list->head;
    // move head to next item
    command_list->head = command_list->head->next;
    // delete current item
    command_list_node_end(current);
  }
  command_list->tail = NULL;

  free(command_list);
}

// --------
// Below are functions related to parsing JSON into the relevant command linked
// list, using the above lifecycle functions.
// --------

// This is responsible for pulling off shared state data and adding it.
void parse_and_add_state(const cJSON *commandJson, char *type,
                         command_state_t **state) {
  bool didInitState = *state != NULL;

  const cJSON *font_size =
      cJSON_GetObjectItemCaseSensitive(commandJson, "fontSize");
  if (cJSON_IsString(font_size) && font_size->valuestring != NULL) {
    if (!didInitState) {
      didInitState = true;
      command_state_init(state);
    }

    if (strcmp(font_size->valuestring, "sm") == 0) {
      (*state)->font_size = FONT_SIZE_SM;
      command_state_set_flag_font(*state);
    } else if (strcmp(font_size->valuestring, "lg") == 0) {
      (*state)->font_size = FONT_SIZE_LG;
      command_state_set_flag_font(*state);
    } else if (strcmp(font_size->valuestring, "md") == 0) {
      (*state)->font_size = FONT_SIZE_MD;
      command_state_set_flag_font(*state);
    } else {
      invalid_prop_warn(type, "fontSize");
    }
  }

  const cJSON *color = cJSON_GetObjectItemCaseSensitive(commandJson, "color");
  if (cJSON_IsObject(color) && !cJSON_IsNull(color)) {
    const cJSON *red = cJSON_GetObjectItemCaseSensitive(color, "red");
    const cJSON *green = cJSON_GetObjectItemCaseSensitive(color, "green");
    const cJSON *blue = cJSON_GetObjectItemCaseSensitive(color, "blue");
    if (cJSON_IsNumber(red) && cJSON_IsNumber(green) && cJSON_IsNumber(blue)) {
      if (!didInitState) {
        didInitState = true;
        command_state_init(state);
      }

      (*state)->color_red = (uint8_t)red->valueint;
      (*state)->color_green = (uint8_t)green->valueint;
      (*state)->color_blue = (uint8_t)blue->valueint;
      command_state_set_flag_color(*state);
    } else {
      invalid_prop_warn(type, "color");
    }
  }

  const cJSON *pos = cJSON_GetObjectItemCaseSensitive(commandJson, "position");
  if (cJSON_IsObject(pos)) {
    const cJSON *x = cJSON_GetObjectItemCaseSensitive(pos, "x");
    const cJSON *y = cJSON_GetObjectItemCaseSensitive(pos, "y");

    if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y)) {
      invalid_prop_warn(type, "position");
    } else {
      if (!didInitState) {
        didInitState = true;
        command_state_init(state);
      }

      (*state)->pos_x = x->valueint;
      (*state)->pos_y = y->valueint;
      command_state_set_flag_position(*state);
    }
  }
}

void parse_and_add_config(const cJSON *json,
                          command_list_handle_t command_list) {
  const cJSON *config = cJSON_GetObjectItemCaseSensitive(json, "config");

  if (!cJSON_IsObject(config) || cJSON_IsNull(config)) {
    return;
  }

  const cJSON *animation_delay =
      cJSON_GetObjectItemCaseSensitive(config, "animationDelay");

  if (cJSON_IsNumber(animation_delay) && animation_delay->valueint > 5 &&
      animation_delay->valueint <= 65535) {
    command_list->config.animation_delay = (uint16_t)animation_delay->valueint;
  }
}

void parse_and_append_string(command_list_handle_t command_list,
                             const cJSON *commandJson) {
  const cJSON *value = cJSON_GetObjectItemCaseSensitive(commandJson, "value");
  if (!cJSON_IsString(value) || value->valuestring == NULL) {
    invalid_shape_warn("string");
    return;
  }

  command_handle_t command;
  command_list_node_init(command_list, COMMAND_TYPE_STRING, &command);

  parse_and_add_state(commandJson, "string", &command->value.string->state);

  command->value.string->value =
      (char *)malloc((strlen(value->valuestring) + 1) * sizeof(char));

  strcpy(command->value.string->value, value->valuestring);
}

void parse_and_append_line(command_list_handle_t command_list,
                           const cJSON *commandJson) {
  const cJSON *to = cJSON_GetObjectItemCaseSensitive(commandJson, "to");
  if (!cJSON_IsObject(to)) {
    invalid_shape_warn("line");
    return;
  }

  const cJSON *to_x = cJSON_GetObjectItemCaseSensitive(to, "x");
  const cJSON *to_y = cJSON_GetObjectItemCaseSensitive(to, "y");
  if (!cJSON_IsNumber(to_x) || !cJSON_IsNumber(to_y)) {
    invalid_prop_warn("line", "to");
    return;
  }

  command_handle_t command;
  command_list_node_init(command_list, COMMAND_TYPE_LINE, &command);

  parse_and_add_state(commandJson, "line", &command->value.line->state);

  command->value.line->to_x = to_x->valueint;
  command->value.line->to_y = to_y->valueint;
}

void parse_and_append_bitmap(command_list_handle_t command_list,
                             const cJSON *commandJson) {
  const cJSON *dataObj = cJSON_GetObjectItemCaseSensitive(commandJson, "data");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(commandJson, "size");
  if (!cJSON_IsObject(dataObj) || cJSON_IsNull(dataObj) ||
      !cJSON_IsObject(size)) {
    invalid_shape_warn("bitmap");
    return;
  }

  const cJSON *data_red = cJSON_GetObjectItemCaseSensitive(dataObj, "red");
  const cJSON *data_green = cJSON_GetObjectItemCaseSensitive(dataObj, "green");
  const cJSON *data_blue = cJSON_GetObjectItemCaseSensitive(dataObj, "blue");
  if (!cJSON_IsArray(data_red) || !cJSON_IsArray(data_green) ||
      !cJSON_IsArray(data_blue)) {
    invalid_prop_warn("bitmap", "data");
    return;
  }

  const cJSON *sizeW = cJSON_GetObjectItemCaseSensitive(size, "width");
  const cJSON *sizeH = cJSON_GetObjectItemCaseSensitive(size, "height");
  if (!cJSON_IsNumber(sizeW) || !cJSON_IsNumber(sizeH)) {
    invalid_prop_warn("bitmap", "size");
    return;
  }

  command_handle_t command;
  command_list_node_init(command_list, COMMAND_TYPE_BITMAP, &command);

  parse_and_add_state(commandJson, "bitmap", &command->value.bitmap->state);

  command->value.bitmap->width = sizeW->valueint;
  command->value.bitmap->height = sizeH->valueint;
  command->value.bitmap->data_red =
      (uint8_t *)malloc(cJSON_GetArraySize(data_red) * sizeof(uint8_t));
  command->value.bitmap->data_green =
      (uint8_t *)malloc(cJSON_GetArraySize(data_green) * sizeof(uint8_t));
  command->value.bitmap->data_blue =
      (uint8_t *)malloc(cJSON_GetArraySize(data_blue) * sizeof(uint8_t));

  // we cannot access the array directly, so we have to loop and put the values
  // into a buffer that we can use with the display buffer
  const cJSON *pixelValueRed = NULL;
  const cJSON *pixelValueGreen = NULL;
  const cJSON *pixelValueBlue = NULL;
  uint16_t bufIndex = 0;
  cJSON_ArrayForEach(pixelValueRed, data_red) {
    pixelValueGreen = cJSON_GetArrayItem(data_green, bufIndex);
    pixelValueBlue = cJSON_GetArrayItem(data_blue, bufIndex);
    if (cJSON_IsNumber(pixelValueRed) && cJSON_IsNumber(pixelValueGreen) &&
        cJSON_IsNumber(pixelValueBlue)) {
      command->value.bitmap->data_red[bufIndex] =
          (uint8_t)pixelValueRed->valueint;
      command->value.bitmap->data_green[bufIndex] =
          (uint8_t)pixelValueGreen->valueint;
      command->value.bitmap->data_blue[bufIndex] =
          (uint8_t)pixelValueBlue->valueint;
    } else {
      invalid_prop_warn("bitmap", "pixel value");
      command->value.bitmap->data_red[bufIndex] = 0;
      command->value.bitmap->data_green[bufIndex] = 0;
      command->value.bitmap->data_blue[bufIndex] = 0;
    }
    bufIndex++;
  }
}

void parse_and_append_animation(command_list_handle_t command_list,
                                const cJSON *commandJson) {
  const cJSON *framesArr =
      cJSON_GetObjectItemCaseSensitive(commandJson, "frames");
  if (!cJSON_IsArray(framesArr) || cJSON_IsNull(framesArr)) {
    invalid_shape_warn("animation");
    return;
  }

  command_handle_t command;
  command_list_node_init(command_list, COMMAND_TYPE_ANIMATION, &command);

  command->value.animation->frame_count = cJSON_GetArraySize(framesArr);
  // init the as the "last frame", so that we always start in the first
  command->value.animation->last_show_frame =
      command->value.animation->frame_count - 1;
  command->value.animation->frames = (void *)malloc(
      sizeof(command_list_handle_t) * command->value.animation->frame_count);

  // the frame we're iterating
  uint16_t frameI = 0;
  const cJSON *frameCommandsArr = NULL;
  // now loop all frames and extract their values
  cJSON_ArrayForEach(frameCommandsArr, framesArr) {
    command_list_init(&command->value.animation->frames[frameI]);
    parse_command_array(command->value.animation->frames[frameI],
                        frameCommandsArr, true);
    frameI++;
  }
}

void parse_and_append_time(command_list_handle_t command_list,
                           const cJSON *commandJson) {
  command_handle_t command;
  command_list_node_init(command_list, COMMAND_TYPE_TIME, &command);

  parse_and_add_state(commandJson, "time", &command->value.time->state);
}

void parse_and_append_date(command_list_handle_t command_list,
                           const cJSON *commandJson) {
  command_handle_t command;
  command_list_node_init(command_list, COMMAND_TYPE_DATE, &command);

  parse_and_add_state(commandJson, "date", &command->value.date->state);
}

void parse_and_append_line_feed(command_list_handle_t command_list,
                                const cJSON *commandJson) {
  command_handle_t command;
  command_list_node_init(command_list, COMMAND_TYPE_LINEFEED, &command);
}

void parse_and_append_set_state(command_list_handle_t command_list,
                                const cJSON *commandJson) {
  command_handle_t command;
  command_list_node_init(command_list, COMMAND_TYPE_SETSTATE, &command);

  parse_and_add_state(commandJson, "setState",
                      &command->value.set_state->state);
}

void parse_and_append_graph(command_list_handle_t command_list,
                            const cJSON *commandJson) {
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(commandJson, "size");
  const cJSON *values = cJSON_GetObjectItemCaseSensitive(commandJson, "values");
  if (!cJSON_IsObject(size) || !cJSON_IsArray(values)) {
    invalid_shape_warn("graph");
    return;
  }

  const cJSON *sizeW = cJSON_GetObjectItemCaseSensitive(size, "width");
  const cJSON *sizeH = cJSON_GetObjectItemCaseSensitive(size, "height");
  if (!cJSON_IsNumber(sizeW) || !cJSON_IsNumber(sizeH)) {
    invalid_prop_warn("graph", "size");
    return;
  }

  command_handle_t command;
  command_list_node_init(command_list, COMMAND_TYPE_GRAPH, &command);

  parse_and_add_state(commandJson, "graph", &command->value.graph->state);

  const cJSON *bgColor =
      cJSON_GetObjectItemCaseSensitive(commandJson, "backgroundColor");
  if (cJSON_IsObject(bgColor) && !cJSON_IsNull(bgColor)) {
    const cJSON *red = cJSON_GetObjectItemCaseSensitive(bgColor, "red");
    const cJSON *green = cJSON_GetObjectItemCaseSensitive(bgColor, "green");
    const cJSON *blue = cJSON_GetObjectItemCaseSensitive(bgColor, "blue");
    if (cJSON_IsNumber(red) && cJSON_IsNumber(green) && cJSON_IsNumber(blue)) {
      command->value.graph->bg_color_red = (uint8_t)red->valueint;
      command->value.graph->bg_color_green = (uint8_t)green->valueint;
      command->value.graph->bg_color_blue = (uint8_t)blue->valueint;
    } else {
      invalid_prop_warn("graph", "backgroundColor");
    }
  }

  command->value.graph->width = sizeW->valueint;
  command->value.graph->height = sizeH->valueint;
  command->value.graph->values =
      (uint8_t *)malloc(cJSON_GetArraySize(values) * sizeof(uint8_t));

  const cJSON *value = NULL;
  uint16_t valIndex = 0;
  cJSON_ArrayForEach(value, values) {
    if (cJSON_IsNumber(value)) {
      command->value.graph->values[valIndex] = (uint8_t)value->valueint;
    } else {
      invalid_prop_warn("graph", "value");
      command->value.graph->values[valIndex] = 0;
    }
    valIndex++;
  }
}

void parse_command_array(command_list_handle_t command_list_handle,
                         const cJSON *commandArray, bool is_in_animation) {
  uint16_t commandIndex = 0;
  const cJSON *commandJson = NULL;
  const cJSON *commandType = NULL;

  cJSON_ArrayForEach(commandJson, commandArray) {
    if (cJSON_IsObject(commandJson)) {
      commandType = cJSON_GetObjectItemCaseSensitive(commandJson, "type");
      if (cJSON_IsString(commandType) && commandType->valuestring != NULL) {
        if (strcmp(commandType->valuestring, "string") == 0) {
          parse_and_append_string(command_list_handle, commandJson);
        } else if (strcmp(commandType->valuestring, "line") == 0) {
          parse_and_append_line(command_list_handle, commandJson);
        } else if (strcmp(commandType->valuestring, "bitmap") == 0) {
          parse_and_append_bitmap(command_list_handle, commandJson);
        } else if (strcmp(commandType->valuestring, "line-feed") == 0) {
          parse_and_append_line_feed(command_list_handle, commandJson);
        } else if (strcmp(commandType->valuestring, "set-state") == 0) {
          parse_and_append_set_state(command_list_handle, commandJson);
        } else if (strcmp(commandType->valuestring, "animation") == 0) {
          if (is_in_animation == true) {
            ESP_LOGW(TAG, "Nested animations are not supported (command %u)",
                     commandIndex);
          } else {
            parse_and_append_animation(command_list_handle, commandJson);
          }
        } else if (strcmp(commandType->valuestring, "time") == 0) {
          parse_and_append_time(command_list_handle, commandJson);
        } else if (strcmp(commandType->valuestring, "date") == 0) {
          parse_and_append_date(command_list_handle, commandJson);
        } else if (strcmp(commandType->valuestring, "graph") == 0) {
          parse_and_append_graph(command_list_handle, commandJson);
        } else {
          ESP_LOGW(TAG, "Command %u does not have a valid 'type'",
                   commandIndex);
        }
      } else {
        ESP_LOGW(TAG, "Command %u does not have a 'string' 'type'",
                 commandIndex);
      }
    } else {
      ESP_LOGW(TAG, "Command %u is not an object", commandIndex);
    }

    commandIndex++;
  }
}

esp_err_t command_list_parse(command_list_handle_t *command_list_handle,
                             char *data, size_t length) {
  esp_err_t ret = ESP_OK;
  cJSON *json = cJSON_ParseWithLength(data, length);

  ESP_GOTO_ON_FALSE(json != NULL, ESP_ERR_INVALID_RESPONSE,
                    command_list_parse_cleanup, TAG,
                    "Invalid JSON response or content length");

  ESP_GOTO_ON_FALSE(cJSON_IsObject(json) && !cJSON_IsNull(json),
                    ESP_ERR_INVALID_RESPONSE, command_list_parse_cleanup, TAG,
                    "JSON response is not an object");

  const cJSON *commandArray =
      cJSON_GetObjectItemCaseSensitive(json, "commands");

  ESP_GOTO_ON_FALSE(cJSON_IsArray(commandArray), ESP_ERR_INVALID_RESPONSE,
                    command_list_parse_cleanup, TAG,
                    "response.commands is not an array");

  command_list_init(command_list_handle);

  parse_and_add_config(json, *command_list_handle);

  parse_command_array(*command_list_handle, commandArray, false);

command_list_parse_cleanup:
  cJSON_Delete(json);
  return ret;
}
