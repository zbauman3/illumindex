#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lib/cjson/cJSON.h"
#include <string.h>

#include "gfx/displayBuffer.h"
#include "lib/commands.h"
#include "lib/time.h"

static const char *TAG = "COMMANDS";

#define invalidShapeWarn(type)                                                 \
  ESP_LOGW(TAG, "Command of type '%s' does not have a valid shape", type)

#define invalidPropWarn(type, prop)                                            \
  ESP_LOGW(TAG, "Command of type '%s' has an invalid '%s'", type, prop)

// --------
// Below are the functions related to initiating and cleaning up the structures
// for commands. Ideally, all memory allocation and deallocation should occur
// within these functions
// --------

void commandStateInit(CommandState **stateHandle) {
  CommandState *state = (CommandState *)malloc(sizeof(CommandState));

  state->colorRed = 255;
  state->colorGreen = 255;
  state->colorBlue = 255;
  state->fontSize = FONT_SIZE_MD;
  state->posX = 0;
  state->posY = 0;
  state->flags = 0;

  *stateHandle = state;
}

void commandStateCleanup(CommandState *state) {
  // state could possibly be null. But we only are calling _free_ so NBD
  free(state);
}

esp_err_t commandInit(CommandHandle *commandHandle, CommandType type) {
  CommandHandle command = (Command *)malloc(sizeof(Command));
  command->type = type;

  switch (command->type) {
  case COMMAND_TYPE_STRING:
    command->value.string = (CommandString *)malloc(sizeof(CommandString));
    command->value.string->state = NULL;
    command->value.string->value = NULL;
    break;
  case COMMAND_TYPE_LINE:
    command->value.line = (CommandLine *)malloc(sizeof(CommandLine));
    command->value.line->state = NULL;
    break;
  case COMMAND_TYPE_BITMAP:
    command->value.bitmap = (CommandBitmap *)malloc(sizeof(CommandBitmap));
    command->value.bitmap->state = NULL;
    command->value.bitmap->dataRed = NULL;
    command->value.bitmap->dataGreen = NULL;
    command->value.bitmap->dataBlue = NULL;
    break;
  case COMMAND_TYPE_SETSTATE:
    command->value.setState =
        (CommandSetState *)malloc(sizeof(CommandSetState));
    command->value.setState->state = NULL;
    break;
  case COMMAND_TYPE_LINEFEED:
    command->value.lineFeed =
        (CommandLineFeed *)malloc(sizeof(CommandLineFeed));
    break;
  case COMMAND_TYPE_ANIMATION:
    command->value.animation =
        (CommandAnimation *)malloc(sizeof(CommandAnimation));
    command->value.animation->framesRed = NULL;
    command->value.animation->framesGreen = NULL;
    command->value.animation->framesBlue = NULL;
    command->value.animation->frameCount = 0;
    break;
  case COMMAND_TYPE_TIME:
    command->value.time = (CommandTime *)malloc(sizeof(CommandTime));
    command->value.time->state = NULL;
    break;
  case COMMAND_TYPE_DATE:
    command->value.date = (CommandDate *)malloc(sizeof(CommandDate));
    command->value.date->state = NULL;
    break;
  default:
    free(command);
    return ESP_ERR_INVALID_ARG;
  }

  *commandHandle = command;

  return ESP_OK;
}

void commandCleanup(CommandHandle command) {
  switch (command->type) {
  case COMMAND_TYPE_STRING:
    commandStateCleanup(command->value.string->state);
    free(command->value.string->value);
    free(command->value.string);
    break;
  case COMMAND_TYPE_LINE:
    commandStateCleanup(command->value.line->state);
    free(command->value.line);
    break;
  case COMMAND_TYPE_BITMAP:
    commandStateCleanup(command->value.bitmap->state);
    free(command->value.bitmap->dataRed);
    free(command->value.bitmap->dataGreen);
    free(command->value.bitmap->dataBlue);
    free(command->value.bitmap);
    break;
  case COMMAND_TYPE_SETSTATE:
    commandStateCleanup(command->value.setState->state);
    free(command->value.setState);
    break;
  case COMMAND_TYPE_LINEFEED:
    free(command->value.lineFeed);
    break;
  case COMMAND_TYPE_ANIMATION:
    free(command->value.animation->framesRed);
    free(command->value.animation->framesGreen);
    free(command->value.animation->framesBlue);
    free(command->value.animation);
    break;
  case COMMAND_TYPE_TIME:
    commandStateCleanup(command->value.time->state);
    free(command->value.time);
    break;
  case COMMAND_TYPE_DATE:
    commandStateCleanup(command->value.date->state);
    free(command->value.date);
    break;
  }

  free(command);
}

esp_err_t commandListNodeInit(CommandListHandle commandList, CommandType type,
                              CommandHandle *commandHandle) {
  esp_err_t cmdRet = commandInit(commandHandle, type);
  if (cmdRet != ESP_OK) {
    return cmdRet;
  }

  CommandListNode *newNode = (CommandListNode *)malloc(sizeof(CommandListNode));
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

void commandListNodeCleanup(CommandListNode *commandListNode) {
  commandCleanup(commandListNode->command);
  commandListNode->next = NULL;
  free(commandListNode);
}

void commandListInit(CommandListHandle *commandListHandle) {
  CommandListHandle commandList = (CommandList *)malloc(sizeof(CommandList));

  commandList->head = NULL;
  commandList->tail = NULL;
  commandList->hasAnimation = false;
  commandList->hasShown = false;
  commandList->config.animationDelay = COMMAND_CONFIG_ANIMATION_DELAY_DEFAULT;

  *commandListHandle = commandList;
}

void commandListCleanup(CommandListHandle commandList) {
  CommandListNode *current;
  while (commandList->head != NULL) {
    // save current head
    current = commandList->head;
    // move head to next item
    commandList->head = commandList->head->next;
    // delete current item
    commandListNodeCleanup(current);
  }
  commandList->tail = NULL;

  free(commandList);
}

// --------
// Below are functions related to parsing JSON into the relevant command linked
// list, using the above lifecycle functions.
// --------

// This is responsible for pulling off shared state data and adding it.
void parseAndAddState(const cJSON *commandJson, char *type,
                      CommandState **state) {
  bool didInitState = *state != NULL;

  const cJSON *fontSize =
      cJSON_GetObjectItemCaseSensitive(commandJson, "fontSize");
  if (cJSON_IsString(fontSize) && fontSize->valuestring != NULL) {
    if (!didInitState) {
      didInitState = true;
      commandStateInit(state);
    }

    if (strcmp(fontSize->valuestring, "sm") == 0) {
      (*state)->fontSize = FONT_SIZE_SM;
      commandStateSetFontFlag(*state);
    } else if (strcmp(fontSize->valuestring, "lg") == 0) {
      (*state)->fontSize = FONT_SIZE_LG;
      commandStateSetFontFlag(*state);
    } else if (strcmp(fontSize->valuestring, "md") == 0) {
      (*state)->fontSize = FONT_SIZE_MD;
      commandStateSetFontFlag(*state);
    } else {
      invalidPropWarn(type, "fontSize");
    }
  }

  const cJSON *color = cJSON_GetObjectItemCaseSensitive(commandJson, "color");
  if (cJSON_IsObject(color)) {
    const cJSON *red = cJSON_GetObjectItemCaseSensitive(color, "red");
    const cJSON *green = cJSON_GetObjectItemCaseSensitive(color, "green");
    const cJSON *blue = cJSON_GetObjectItemCaseSensitive(color, "blue");
    if (cJSON_IsNumber(red) && cJSON_IsNumber(green) && cJSON_IsNumber(blue)) {
      if (!didInitState) {
        didInitState = true;
        commandStateInit(state);
      }

      (*state)->colorRed = (uint8_t)red->valueint;
      (*state)->colorGreen = (uint8_t)green->valueint;
      (*state)->colorBlue = (uint8_t)blue->valueint;
      commandStateSetColorFlag(*state);
    } else {
      invalidPropWarn(type, "color");
    }
  } else {
    invalidPropWarn(type, "color");
  }

  const cJSON *pos = cJSON_GetObjectItemCaseSensitive(commandJson, "position");
  if (cJSON_IsObject(pos)) {
    const cJSON *x = cJSON_GetObjectItemCaseSensitive(pos, "x");
    const cJSON *y = cJSON_GetObjectItemCaseSensitive(pos, "y");

    if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y)) {
      invalidPropWarn(type, "position");
    } else {
      if (!didInitState) {
        didInitState = true;
        commandStateInit(state);
      }

      (*state)->posX = x->valueint;
      (*state)->posY = y->valueint;
      commandStateSetPositionFlag(*state);
    }
  }
}

void parseAndAddConfig(const cJSON *json, CommandListHandle commandList) {
  const cJSON *config = cJSON_GetObjectItemCaseSensitive(json, "config");

  if (!cJSON_IsObject(config) || cJSON_IsNull(config)) {
    return;
  }

  const cJSON *animationDelay =
      cJSON_GetObjectItemCaseSensitive(config, "animationDelay");

  if (cJSON_IsNumber(animationDelay) && animationDelay->valueint > 5) {
    commandList->config.animationDelay = (uint16_t)animationDelay->valueint;
  }
}

void parseAndAppendString(CommandListHandle commandList,
                          const cJSON *commandJson) {
  const cJSON *value = cJSON_GetObjectItemCaseSensitive(commandJson, "value");
  if (!cJSON_IsString(value) || value->valuestring == NULL) {
    invalidShapeWarn("string");
    return;
  }

  CommandHandle command;
  commandListNodeInit(commandList, COMMAND_TYPE_STRING, &command);

  parseAndAddState(commandJson, "string", &command->value.string->state);

  command->value.string->value =
      (char *)malloc((strlen(value->valuestring) + 1) * sizeof(char));

  strcpy(command->value.string->value, value->valuestring);
}

void parseAndAppendLine(CommandListHandle commandList,
                        const cJSON *commandJson) {
  const cJSON *to = cJSON_GetObjectItemCaseSensitive(commandJson, "to");
  if (!cJSON_IsObject(to)) {
    invalidShapeWarn("line");
    return;
  }

  const cJSON *toX = cJSON_GetObjectItemCaseSensitive(to, "x");
  const cJSON *toY = cJSON_GetObjectItemCaseSensitive(to, "y");
  if (!cJSON_IsNumber(toX) || !cJSON_IsNumber(toY)) {
    invalidPropWarn("line", "to");
    return;
  }

  CommandHandle command;
  commandListNodeInit(commandList, COMMAND_TYPE_LINE, &command);

  parseAndAddState(commandJson, "line", &command->value.line->state);

  command->value.line->toX = toX->valueint;
  command->value.line->toY = toY->valueint;
}

void parseAndAppendBitmap(CommandListHandle commandList,
                          const cJSON *commandJson) {
  const cJSON *dataObj = cJSON_GetObjectItemCaseSensitive(commandJson, "data");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(commandJson, "size");
  if (!cJSON_IsObject(dataObj) || cJSON_IsNull(dataObj) ||
      !cJSON_IsObject(size)) {
    invalidShapeWarn("bitmap");
    return;
  }

  const cJSON *dataRed = cJSON_GetObjectItemCaseSensitive(dataObj, "red");
  const cJSON *dataGreen = cJSON_GetObjectItemCaseSensitive(dataObj, "green");
  const cJSON *dataBlue = cJSON_GetObjectItemCaseSensitive(dataObj, "blue");
  if (!cJSON_IsArray(dataRed) || !cJSON_IsArray(dataGreen) ||
      !cJSON_IsArray(dataBlue)) {
    invalidPropWarn("bitmap", "data");
    return;
  }

  const cJSON *sizeW = cJSON_GetObjectItemCaseSensitive(size, "width");
  const cJSON *sizeH = cJSON_GetObjectItemCaseSensitive(size, "height");
  if (!cJSON_IsNumber(sizeW) || !cJSON_IsNumber(sizeH)) {
    invalidPropWarn("bitmap", "size");
    return;
  }

  CommandHandle command;
  commandListNodeInit(commandList, COMMAND_TYPE_BITMAP, &command);

  parseAndAddState(commandJson, "bitmap", &command->value.bitmap->state);

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
      invalidPropWarn("bitmap", "pixel value");
      command->value.bitmap->dataRed[bufIndex] = 0;
      command->value.bitmap->dataGreen[bufIndex] = 0;
      command->value.bitmap->dataBlue[bufIndex] = 0;
    }
    bufIndex++;
  }
}

void parseAndAppendAnimation(CommandListHandle commandList,
                             const cJSON *commandJson) {
  const cJSON *position =
      cJSON_GetObjectItemCaseSensitive(commandJson, "position");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(commandJson, "size");
  const cJSON *framesObj =
      cJSON_GetObjectItemCaseSensitive(commandJson, "frames");
  if (!cJSON_IsObject(position) || !cJSON_IsObject(size) ||
      !cJSON_IsObject(framesObj) || cJSON_IsNull(framesObj)) {
    invalidShapeWarn("animation");
    return;
  }

  const cJSON *framesRed = cJSON_GetObjectItemCaseSensitive(framesObj, "red");
  const cJSON *framesGreen =
      cJSON_GetObjectItemCaseSensitive(framesObj, "green");
  const cJSON *framesBlue = cJSON_GetObjectItemCaseSensitive(framesObj, "blue");
  if (!cJSON_IsArray(framesRed) || !cJSON_IsArray(framesGreen) ||
      !cJSON_IsArray(framesBlue)) {
    invalidPropWarn("animation", "frames");
    return;
  }

  const cJSON *posX = cJSON_GetObjectItemCaseSensitive(position, "x");
  const cJSON *posY = cJSON_GetObjectItemCaseSensitive(position, "y");
  if (!cJSON_IsNumber(posX) || !cJSON_IsNumber(posY)) {
    invalidPropWarn("animation", "position");
    return;
  }

  const cJSON *sizeW = cJSON_GetObjectItemCaseSensitive(size, "width");
  const cJSON *sizeH = cJSON_GetObjectItemCaseSensitive(size, "height");
  if (!cJSON_IsNumber(sizeW) || !cJSON_IsNumber(sizeH)) {
    invalidPropWarn("animation", "size");
    return;
  }

  CommandHandle command;
  commandListNodeInit(commandList, COMMAND_TYPE_ANIMATION, &command);

  command->value.animation->frameCount = cJSON_GetArraySize(framesRed);
  // init the as the "last frame", so that we always start in the first
  command->value.animation->lastShowFrame =
      command->value.animation->frameCount - 1;
  command->value.animation->posX = posX->valueint;
  command->value.animation->posY = posY->valueint;
  command->value.animation->width = sizeW->valueint;
  command->value.animation->height = sizeH->valueint;
  command->value.animation->framesRed = (uint8_t *)malloc(
      command->value.animation->frameCount * command->value.animation->width *
      command->value.animation->height * sizeof(uint8_t));
  command->value.animation->framesGreen = (uint8_t *)malloc(
      command->value.animation->frameCount * command->value.animation->width *
      command->value.animation->height * sizeof(uint8_t));
  command->value.animation->framesBlue = (uint8_t *)malloc(
      command->value.animation->frameCount * command->value.animation->width *
      command->value.animation->height * sizeof(uint8_t));

  uint32_t frameLength =
      command->value.animation->width * command->value.animation->height;

  // the frame we're iterating
  uint16_t frameI = 0;
  // the value within the frame
  uint16_t pixelValueI = 0;
  const cJSON *frameRed = NULL;
  const cJSON *frameGreen = NULL;
  const cJSON *frameBlue = NULL;
  const cJSON *pixelValueRed = NULL;
  const cJSON *pixelValueGreen = NULL;
  const cJSON *pixelValueBlue = NULL;

  // now loop all frames and extract their values
  cJSON_ArrayForEach(frameRed, framesRed) {
    frameGreen = cJSON_GetArrayItem(framesGreen, frameI);
    frameBlue = cJSON_GetArrayItem(framesBlue, frameI);
    if (cJSON_IsArray(frameRed) && cJSON_IsArray(frameGreen) &&
        cJSON_IsArray(frameBlue)) {
      pixelValueI = 0;
      cJSON_ArrayForEach(pixelValueRed, frameRed) {
        pixelValueGreen = cJSON_GetArrayItem(frameGreen, pixelValueI);
        pixelValueBlue = cJSON_GetArrayItem(frameBlue, pixelValueI);
        if (pixelValueI < frameLength) {
          if (cJSON_IsNumber(pixelValueRed) &&
              cJSON_IsNumber(pixelValueGreen) &&
              cJSON_IsNumber(pixelValueBlue)) {
            command->value.animation
                ->framesRed[(frameI * command->value.animation->width *
                             command->value.animation->height) +
                            pixelValueI] = (uint8_t)pixelValueRed->valueint;
            command->value.animation
                ->framesGreen[(frameI * command->value.animation->width *
                               command->value.animation->height) +
                              pixelValueI] = (uint8_t)pixelValueGreen->valueint;
            command->value.animation
                ->framesBlue[(frameI * command->value.animation->width *
                              command->value.animation->height) +
                             pixelValueI] = (uint8_t)pixelValueBlue->valueint;
          } else {
            invalidPropWarn("animation", "frames > frame > value");
          }
        } else {
          invalidPropWarn("animation", "frames > frame.length");
          // no need to continue if we're past the frame length
          break;
        }

        pixelValueI++;
      }
    } else {
      invalidPropWarn("animation", "frames > frame");
    }

    frameI++;
  }
}

void parseAndAppendTime(CommandListHandle commandList,
                        const cJSON *commandJson) {
  CommandHandle command;
  commandListNodeInit(commandList, COMMAND_TYPE_TIME, &command);

  parseAndAddState(commandJson, "time", &command->value.time->state);
}

void parseAndAppendDate(CommandListHandle commandList,
                        const cJSON *commandJson) {
  CommandHandle command;
  commandListNodeInit(commandList, COMMAND_TYPE_DATE, &command);

  parseAndAddState(commandJson, "date", &command->value.date->state);
}

void parseAndAppendLineFeed(CommandListHandle commandList,
                            const cJSON *commandJson) {
  CommandHandle command;
  commandListNodeInit(commandList, COMMAND_TYPE_LINEFEED, &command);
}

void parseAndAppendSetState(CommandListHandle commandList,
                            const cJSON *commandJson) {
  CommandHandle command;
  commandListNodeInit(commandList, COMMAND_TYPE_SETSTATE, &command);

  parseAndAddState(commandJson, "setState", &command->value.setState->state);
}

esp_err_t parseCommands(CommandListHandle *commandListHandle, char *data,
                        size_t length) {
  esp_err_t ret = ESP_OK;
  const cJSON *commandJson = NULL;
  const cJSON *type = NULL;
  uint16_t commandIndex = 0;
  cJSON *json = cJSON_ParseWithLength(data, length);

  ESP_GOTO_ON_FALSE(json != NULL, ESP_ERR_INVALID_RESPONSE,
                    parseCommands_cleanup, TAG,
                    "Invalid JSON response or content length");

  ESP_GOTO_ON_FALSE(cJSON_IsObject(json) && !cJSON_IsNull(json),
                    ESP_ERR_INVALID_RESPONSE, parseCommands_cleanup, TAG,
                    "JSON response is not an object");

  const cJSON *commandArray =
      cJSON_GetObjectItemCaseSensitive(json, "commands");

  ESP_GOTO_ON_FALSE(cJSON_IsArray(commandArray), ESP_ERR_INVALID_RESPONSE,
                    parseCommands_cleanup, TAG,
                    "response.commands is not an array");

  commandListInit(commandListHandle);

  parseAndAddConfig(json, *commandListHandle);

  cJSON_ArrayForEach(commandJson, commandArray) {
    if (cJSON_IsObject(commandJson)) {
      type = cJSON_GetObjectItemCaseSensitive(commandJson, "type");
      if (cJSON_IsString(type) && type->valuestring != NULL) {
        if (strcmp(type->valuestring, "string") == 0) {
          parseAndAppendString(*commandListHandle, commandJson);
        } else if (strcmp(type->valuestring, "line") == 0) {
          parseAndAppendLine(*commandListHandle, commandJson);
        } else if (strcmp(type->valuestring, "bitmap") == 0) {
          parseAndAppendBitmap(*commandListHandle, commandJson);
        } else if (strcmp(type->valuestring, "line-feed") == 0) {
          parseAndAppendLineFeed(*commandListHandle, commandJson);
        } else if (strcmp(type->valuestring, "set-state") == 0) {
          parseAndAppendSetState(*commandListHandle, commandJson);
        } else if (strcmp(type->valuestring, "animation") == 0) {
          parseAndAppendAnimation(*commandListHandle, commandJson);
        } else if (strcmp(type->valuestring, "time") == 0) {
          parseAndAppendTime(*commandListHandle, commandJson);
        } else if (strcmp(type->valuestring, "date") == 0) {
          parseAndAppendDate(*commandListHandle, commandJson);
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

parseCommands_cleanup:
  cJSON_Delete(json);
  return ret;
}
