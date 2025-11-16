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

void parse_command_array(command_list_handle_t commandListHandle,
                         const cJSON *commandArray, bool isInsideAnimation);

// --------
// Below are the functions related to initiating and cleaning up the structures
// for commands. Ideally, all memory allocation and deallocation should occur
// within these functions
// --------

void command_state_init(command_state_t **stateHandle) {
  command_state_t *state = (command_state_t *)malloc(sizeof(command_state_t));

  state->colorRed = 255;
  state->colorGreen = 255;
  state->colorBlue = 255;
  state->fontSize = FONT_SIZE_MD;
  state->posX = 0;
  state->posY = 0;
  state->flags = 0;

  *stateHandle = state;
}

void command_state_end(command_state_t *state) {
  // state could possibly be null. But we only are calling _free_ so NBD
  free(state);
}

esp_err_t command_init(command_handle_t *commandHandle,
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
    command->value.bitmap->dataRed = NULL;
    command->value.bitmap->dataGreen = NULL;
    command->value.bitmap->dataBlue = NULL;
    break;
  case COMMAND_TYPE_SETSTATE:
    command->value.setState =
        (command_value_set_state_t *)malloc(sizeof(command_value_set_state_t));
    command->value.setState->state = NULL;
    break;
  case COMMAND_TYPE_LINEFEED:
    command->value.lineFeed =
        (command_value_line_feed_t *)malloc(sizeof(command_value_line_feed_t));
    break;
  case COMMAND_TYPE_ANIMATION:
    command->value.animation =
        (command_value_animation_t *)malloc(sizeof(command_value_animation_t));
    command->value.animation->frameCount = 0;
    command->value.animation->lastShowFrame = 0;
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
    command->value.graph->bgColorRed = 0;
    command->value.graph->bgColorGreen = 0;
    command->value.graph->bgColorBlue = 0;
    break;
  default:
    free(command);
    return ESP_ERR_INVALID_ARG;
  }

  *commandHandle = command;

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
    free(command->value.bitmap->dataRed);
    free(command->value.bitmap->dataGreen);
    free(command->value.bitmap->dataBlue);
    free(command->value.bitmap);
    break;
  case COMMAND_TYPE_SETSTATE:
    command_state_end(command->value.setState->state);
    free(command->value.setState);
    break;
  case COMMAND_TYPE_LINEFEED:
    free(command->value.lineFeed);
    break;
  case COMMAND_TYPE_ANIMATION:
    for (uint16_t i = 0; i < command->value.animation->frameCount; i++) {
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

esp_err_t command_list_node_init(command_list_handle_t commandList,
                                 command_type_enum_t type,
                                 command_handle_t *commandHandle) {
  esp_err_t cmdRet = command_init(commandHandle, type);
  if (cmdRet != ESP_OK) {
    return cmdRet;
  }

  command_list_node_t *newNode =
      (command_list_node_t *)malloc(sizeof(command_list_node_t));
  newNode->command = *commandHandle;
  newNode->next = NULL;

  // no items yet, add as the first item
  if (commandList->head == NULL || commandList->tail == NULL) {
    commandList->head = newNode;
    commandList->tail = newNode;
  } else {
    commandList->tail->next = newNode;
    commandList->tail = newNode;
  }

  if (type == COMMAND_TYPE_ANIMATION) {
    commandList->hasAnimation = true;
  }

  return ESP_OK;
}

void command_list_node_end(command_list_node_t *commandListNode) {
  command_end(commandListNode->command);
  commandListNode->next = NULL;
  free(commandListNode);
}

void command_list_init(command_list_handle_t *commandListHandle) {
  command_list_handle_t commandList =
      (command_list_t *)malloc(sizeof(command_list_t));

  commandList->head = NULL;
  commandList->tail = NULL;
  commandList->hasAnimation = false;
  commandList->hasShown = false;
  commandList->config.animationDelay = COMMAND_CONFIG_ANIMATION_DELAY_DEFAULT;

  *commandListHandle = commandList;
}

void command_list_end(command_list_handle_t commandList) {
  command_list_node_t *current;
  while (commandList->head != NULL) {
    // save current head
    current = commandList->head;
    // move head to next item
    commandList->head = commandList->head->next;
    // delete current item
    command_list_node_end(current);
  }
  commandList->tail = NULL;

  free(commandList);
}

// --------
// Below are functions related to parsing JSON into the relevant command linked
// list, using the above lifecycle functions.
// --------

// This is responsible for pulling off shared state data and adding it.
void parse_and_add_state(const cJSON *commandJson, char *type,
                         command_state_t **state) {
  bool didInitState = *state != NULL;

  const cJSON *fontSize =
      cJSON_GetObjectItemCaseSensitive(commandJson, "fontSize");
  if (cJSON_IsString(fontSize) && fontSize->valuestring != NULL) {
    if (!didInitState) {
      didInitState = true;
      command_state_init(state);
    }

    if (strcmp(fontSize->valuestring, "sm") == 0) {
      (*state)->fontSize = FONT_SIZE_SM;
      command_state_set_flag_font(*state);
    } else if (strcmp(fontSize->valuestring, "lg") == 0) {
      (*state)->fontSize = FONT_SIZE_LG;
      command_state_set_flag_font(*state);
    } else if (strcmp(fontSize->valuestring, "md") == 0) {
      (*state)->fontSize = FONT_SIZE_MD;
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

      (*state)->colorRed = (uint8_t)red->valueint;
      (*state)->colorGreen = (uint8_t)green->valueint;
      (*state)->colorBlue = (uint8_t)blue->valueint;
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

      (*state)->posX = x->valueint;
      (*state)->posY = y->valueint;
      command_state_set_flag_position(*state);
    }
  }
}

void parse_and_add_config(const cJSON *json,
                          command_list_handle_t commandList) {
  const cJSON *config = cJSON_GetObjectItemCaseSensitive(json, "config");

  if (!cJSON_IsObject(config) || cJSON_IsNull(config)) {
    return;
  }

  const cJSON *animationDelay =
      cJSON_GetObjectItemCaseSensitive(config, "animationDelay");

  if (cJSON_IsNumber(animationDelay) && animationDelay->valueint > 5 &&
      animationDelay->valueint <= 65535) {
    commandList->config.animationDelay = (uint16_t)animationDelay->valueint;
  }
}

void parse_and_append_string(command_list_handle_t commandList,
                             const cJSON *commandJson) {
  const cJSON *value = cJSON_GetObjectItemCaseSensitive(commandJson, "value");
  if (!cJSON_IsString(value) || value->valuestring == NULL) {
    invalid_shape_warn("string");
    return;
  }

  command_handle_t command;
  command_list_node_init(commandList, COMMAND_TYPE_STRING, &command);

  parse_and_add_state(commandJson, "string", &command->value.string->state);

  command->value.string->value =
      (char *)malloc((strlen(value->valuestring) + 1) * sizeof(char));

  strcpy(command->value.string->value, value->valuestring);
}

void parse_and_append_line(command_list_handle_t commandList,
                           const cJSON *commandJson) {
  const cJSON *to = cJSON_GetObjectItemCaseSensitive(commandJson, "to");
  if (!cJSON_IsObject(to)) {
    invalid_shape_warn("line");
    return;
  }

  const cJSON *toX = cJSON_GetObjectItemCaseSensitive(to, "x");
  const cJSON *toY = cJSON_GetObjectItemCaseSensitive(to, "y");
  if (!cJSON_IsNumber(toX) || !cJSON_IsNumber(toY)) {
    invalid_prop_warn("line", "to");
    return;
  }

  command_handle_t command;
  command_list_node_init(commandList, COMMAND_TYPE_LINE, &command);

  parse_and_add_state(commandJson, "line", &command->value.line->state);

  command->value.line->toX = toX->valueint;
  command->value.line->toY = toY->valueint;
}

void parse_and_append_bitmap(command_list_handle_t commandList,
                             const cJSON *commandJson) {
  const cJSON *dataObj = cJSON_GetObjectItemCaseSensitive(commandJson, "data");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(commandJson, "size");
  if (!cJSON_IsObject(dataObj) || cJSON_IsNull(dataObj) ||
      !cJSON_IsObject(size)) {
    invalid_shape_warn("bitmap");
    return;
  }

  const cJSON *dataRed = cJSON_GetObjectItemCaseSensitive(dataObj, "red");
  const cJSON *dataGreen = cJSON_GetObjectItemCaseSensitive(dataObj, "green");
  const cJSON *dataBlue = cJSON_GetObjectItemCaseSensitive(dataObj, "blue");
  if (!cJSON_IsArray(dataRed) || !cJSON_IsArray(dataGreen) ||
      !cJSON_IsArray(dataBlue)) {
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
  command_list_node_init(commandList, COMMAND_TYPE_BITMAP, &command);

  parse_and_add_state(commandJson, "bitmap", &command->value.bitmap->state);

  command->value.bitmap->width = sizeW->valueint;
  command->value.bitmap->height = sizeH->valueint;
  command->value.bitmap->dataRed =
      (uint8_t *)malloc(cJSON_GetArraySize(dataRed) * sizeof(uint8_t));
  command->value.bitmap->dataGreen =
      (uint8_t *)malloc(cJSON_GetArraySize(dataGreen) * sizeof(uint8_t));
  command->value.bitmap->dataBlue =
      (uint8_t *)malloc(cJSON_GetArraySize(dataBlue) * sizeof(uint8_t));

  // we cannot access the array directly, so we have to loop and put the values
  // into a buffer that we can use with the display buffer
  const cJSON *pixelValueRed = NULL;
  const cJSON *pixelValueGreen = NULL;
  const cJSON *pixelValueBlue = NULL;
  uint16_t bufIndex = 0;
  cJSON_ArrayForEach(pixelValueRed, dataRed) {
    pixelValueGreen = cJSON_GetArrayItem(dataGreen, bufIndex);
    pixelValueBlue = cJSON_GetArrayItem(dataBlue, bufIndex);
    if (cJSON_IsNumber(pixelValueRed) && cJSON_IsNumber(pixelValueGreen) &&
        cJSON_IsNumber(pixelValueBlue)) {
      command->value.bitmap->dataRed[bufIndex] =
          (uint8_t)pixelValueRed->valueint;
      command->value.bitmap->dataGreen[bufIndex] =
          (uint8_t)pixelValueGreen->valueint;
      command->value.bitmap->dataBlue[bufIndex] =
          (uint8_t)pixelValueBlue->valueint;
    } else {
      invalid_prop_warn("bitmap", "pixel value");
      command->value.bitmap->dataRed[bufIndex] = 0;
      command->value.bitmap->dataGreen[bufIndex] = 0;
      command->value.bitmap->dataBlue[bufIndex] = 0;
    }
    bufIndex++;
  }
}

void parse_and_append_animation(command_list_handle_t commandList,
                                const cJSON *commandJson) {
  const cJSON *framesArr =
      cJSON_GetObjectItemCaseSensitive(commandJson, "frames");
  if (!cJSON_IsArray(framesArr) || cJSON_IsNull(framesArr)) {
    invalid_shape_warn("animation");
    return;
  }

  command_handle_t command;
  command_list_node_init(commandList, COMMAND_TYPE_ANIMATION, &command);

  command->value.animation->frameCount = cJSON_GetArraySize(framesArr);
  // init the as the "last frame", so that we always start in the first
  command->value.animation->lastShowFrame =
      command->value.animation->frameCount - 1;
  command->value.animation->frames = (void *)malloc(
      sizeof(command_list_handle_t) * command->value.animation->frameCount);

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

void parse_and_append_time(command_list_handle_t commandList,
                           const cJSON *commandJson) {
  command_handle_t command;
  command_list_node_init(commandList, COMMAND_TYPE_TIME, &command);

  parse_and_add_state(commandJson, "time", &command->value.time->state);
}

void parse_and_append_date(command_list_handle_t commandList,
                           const cJSON *commandJson) {
  command_handle_t command;
  command_list_node_init(commandList, COMMAND_TYPE_DATE, &command);

  parse_and_add_state(commandJson, "date", &command->value.date->state);
}

void parse_and_append_line_feed(command_list_handle_t commandList,
                                const cJSON *commandJson) {
  command_handle_t command;
  command_list_node_init(commandList, COMMAND_TYPE_LINEFEED, &command);
}

void parse_and_append_set_state(command_list_handle_t commandList,
                                const cJSON *commandJson) {
  command_handle_t command;
  command_list_node_init(commandList, COMMAND_TYPE_SETSTATE, &command);

  parse_and_add_state(commandJson, "setState", &command->value.setState->state);
}

void parse_and_append_graph(command_list_handle_t commandList,
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
  command_list_node_init(commandList, COMMAND_TYPE_GRAPH, &command);

  parse_and_add_state(commandJson, "graph", &command->value.graph->state);

  const cJSON *bgColor =
      cJSON_GetObjectItemCaseSensitive(commandJson, "backgroundColor");
  if (cJSON_IsObject(bgColor) && !cJSON_IsNull(bgColor)) {
    const cJSON *red = cJSON_GetObjectItemCaseSensitive(bgColor, "red");
    const cJSON *green = cJSON_GetObjectItemCaseSensitive(bgColor, "green");
    const cJSON *blue = cJSON_GetObjectItemCaseSensitive(bgColor, "blue");
    if (cJSON_IsNumber(red) && cJSON_IsNumber(green) && cJSON_IsNumber(blue)) {
      command->value.graph->bgColorRed = (uint8_t)red->valueint;
      command->value.graph->bgColorGreen = (uint8_t)green->valueint;
      command->value.graph->bgColorBlue = (uint8_t)blue->valueint;
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

void parse_command_array(command_list_handle_t commandListHandle,
                         const cJSON *commandArray, bool isInsideAnimation) {
  uint16_t commandIndex = 0;
  const cJSON *commandJson = NULL;
  const cJSON *commandType = NULL;

  cJSON_ArrayForEach(commandJson, commandArray) {
    if (cJSON_IsObject(commandJson)) {
      commandType = cJSON_GetObjectItemCaseSensitive(commandJson, "type");
      if (cJSON_IsString(commandType) && commandType->valuestring != NULL) {
        if (strcmp(commandType->valuestring, "string") == 0) {
          parse_and_append_string(commandListHandle, commandJson);
        } else if (strcmp(commandType->valuestring, "line") == 0) {
          parse_and_append_line(commandListHandle, commandJson);
        } else if (strcmp(commandType->valuestring, "bitmap") == 0) {
          parse_and_append_bitmap(commandListHandle, commandJson);
        } else if (strcmp(commandType->valuestring, "line-feed") == 0) {
          parse_and_append_line_feed(commandListHandle, commandJson);
        } else if (strcmp(commandType->valuestring, "set-state") == 0) {
          parse_and_append_set_state(commandListHandle, commandJson);
        } else if (strcmp(commandType->valuestring, "animation") == 0) {
          if (isInsideAnimation == true) {
            ESP_LOGW(TAG, "Nested animations are not supported (command %u)",
                     commandIndex);
          } else {
            parse_and_append_animation(commandListHandle, commandJson);
          }
        } else if (strcmp(commandType->valuestring, "time") == 0) {
          parse_and_append_time(commandListHandle, commandJson);
        } else if (strcmp(commandType->valuestring, "date") == 0) {
          parse_and_append_date(commandListHandle, commandJson);
        } else if (strcmp(commandType->valuestring, "graph") == 0) {
          parse_and_append_graph(commandListHandle, commandJson);
        } else {
          ESP_LOGW(TAG, "command_t %u does not have a valid 'type'",
                   commandIndex);
        }
      } else {
        ESP_LOGW(TAG, "command_t %u does not have a 'string' 'type'",
                 commandIndex);
      }
    } else {
      ESP_LOGW(TAG, "command_t %u is not an object", commandIndex);
    }

    commandIndex++;
  }
}

esp_err_t command_list_parse(command_list_handle_t *commandListHandle,
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

  command_list_init(commandListHandle);

  parse_and_add_config(json, *commandListHandle);

  parse_command_array(*commandListHandle, commandArray, false);

command_list_parse_cleanup:
  cJSON_Delete(json);
  return ret;
}
