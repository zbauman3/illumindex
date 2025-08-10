#include "util/commands.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lib/cjson/cJSON.h"
#include <string.h>

#include "gfx/displayBuffer.h"
#include "lib/time.h"

static const char *TAG = "COMMANDS";

#define invalidShapeWarn(type)                                                 \
  ESP_LOGW(TAG, "Command of type '%s' does not have a valid shape", type)

#define invalidPropWarn(type, prop)                                            \
  ESP_LOGW(TAG, "Command of type '%s' has an invalid '%s'", type, prop)

// This is responsible for pulling off shared state data and setting it. Not all
// of these make sense in the context of the command they're set in, but it's
// just easier to allow any to be set from any command.
void parseAndSetState(DisplayBufferHandle db, const cJSON *command,
                      char *type) {
  const cJSON *fontSize = cJSON_GetObjectItemCaseSensitive(command, "fontSize");
  if (cJSON_IsString(fontSize) && fontSize->valuestring != NULL) {
    if (strcmp(fontSize->valuestring, "sm") == 0) {
      fontSetSize(db->font, FONT_SIZE_SM);
    } else if (strcmp(fontSize->valuestring, "lg") == 0) {
      fontSetSize(db->font, FONT_SIZE_LG);
    } else if (strcmp(fontSize->valuestring, "md") == 0) {
      fontSetSize(db->font, FONT_SIZE_MD);
    } else {
      invalidPropWarn(type, "fontSize");
    }
  }

  const cJSON *color = cJSON_GetObjectItemCaseSensitive(command, "color");
  if (cJSON_IsNumber(color)) {
    displayBufferSetColor(db, color->valueint);
  }

  const cJSON *pos = cJSON_GetObjectItemCaseSensitive(command, "position");
  if (cJSON_IsObject(pos)) {
    const cJSON *x = cJSON_GetObjectItemCaseSensitive(pos, "x");
    const cJSON *y = cJSON_GetObjectItemCaseSensitive(pos, "y");

    if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y)) {
      invalidPropWarn(type, "position");
    } else {
      displayBufferSetCursor(db, x->valueint, y->valueint);
    }
  }
}

void parseAndShowString(DisplayBufferHandle db, const cJSON *command) {
  const cJSON *value = cJSON_GetObjectItemCaseSensitive(command, "value");
  if (!cJSON_IsString(value) && value->valuestring != NULL) {
    invalidShapeWarn("string");
    return;
  }

  parseAndSetState(db, command, "string");

  displayBufferDrawString(db, value->valuestring);
}

void parseAndShowLine(DisplayBufferHandle db, const cJSON *command) {
  const cJSON *to = cJSON_GetObjectItemCaseSensitive(command, "to");
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

  parseAndSetState(db, command, "line");

  displayBufferDrawLine(db, toX->valueint, toY->valueint);
}

void parseAndShowBitmap(DisplayBufferHandle db, const cJSON *command) {
  const cJSON *data = cJSON_GetObjectItemCaseSensitive(command, "data");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(command, "size");
  if (!cJSON_IsArray(data) || !cJSON_IsObject(size)) {
    invalidShapeWarn("bitmap");
    return;
  }

  const cJSON *sizeW = cJSON_GetObjectItemCaseSensitive(size, "width");
  const cJSON *sizeH = cJSON_GetObjectItemCaseSensitive(size, "height");
  if (!cJSON_IsNumber(sizeW) || !cJSON_IsNumber(sizeH)) {
    invalidPropWarn("bitmap", "size");
    return;
  }

  parseAndSetState(db, command, "bitmap");

  // we cannot access the array directly, so we have to loop and put the values
  // into a buffer that we can use with the display buffer
  const cJSON *pixelValue = NULL;
  uint16_t bufIndex = 0;
  uint16_t *bmBuffer = malloc(cJSON_GetArraySize(data) * sizeof(uint16_t));
  cJSON_ArrayForEach(pixelValue, data) {
    if (cJSON_IsNumber(pixelValue)) {
      bmBuffer[bufIndex] = (uint16_t)pixelValue->valueint;
    } else {
      invalidPropWarn("bitmap", "pixel value");
      bmBuffer[bufIndex] = (uint16_t)0;
    }
    bufIndex++;
  }

  displayBufferDrawBitmap(db, sizeW->valueint, sizeH->valueint, bmBuffer);

  free(bmBuffer);
}

void parseAndShowAnimation(DisplayBufferHandle db, const cJSON *command) {
  const cJSON *position = cJSON_GetObjectItemCaseSensitive(command, "position");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(command, "size");
  const cJSON *delay = cJSON_GetObjectItemCaseSensitive(command, "delay");
  const cJSON *frames = cJSON_GetObjectItemCaseSensitive(command, "frames");
  if (!cJSON_IsObject(position) || !cJSON_IsObject(size) ||
      !cJSON_IsNumber(delay) || !cJSON_IsArray(frames)) {
    invalidShapeWarn("animation");
    return;
  }

  const cJSON *positionX = cJSON_GetObjectItemCaseSensitive(position, "x");
  const cJSON *positionY = cJSON_GetObjectItemCaseSensitive(position, "y");
  if (!cJSON_IsNumber(positionX) || !cJSON_IsNumber(positionY)) {
    invalidPropWarn("animation", "position");
    return;
  }

  const cJSON *sizeW = cJSON_GetObjectItemCaseSensitive(size, "width");
  const cJSON *sizeH = cJSON_GetObjectItemCaseSensitive(size, "height");
  if (!cJSON_IsNumber(sizeW) || !cJSON_IsNumber(sizeH)) {
    invalidPropWarn("animation", "size");
    return;
  }

  uint16_t frameCount = 0;
  // the frame we're iterating
  uint16_t frameI = 0;
  // the value within the frame
  uint16_t pixelValueI;
  const cJSON *frame = NULL;
  const cJSON *pixelValue = NULL;
  // we need to know how many frames there are in order to know how big of an
  // array to allocate. This could be done by dynamically reallocating the
  // array, but this code doesn't need to be incredibly performant, so looping
  // the elements twice isn't a big deal.
  cJSON_ArrayForEach(frame, frames) { frameCount++; }

  displayBufferAnimationInit(db, delay->valueint, positionX->valueint,
                             positionY->valueint, sizeW->valueint,
                             sizeH->valueint, frameCount);

  // now loop all frames and extract their values
  cJSON_ArrayForEach(frame, frames) {
    if (cJSON_IsArray(frame)) {
      pixelValueI = 0;
      cJSON_ArrayForEach(pixelValue, frame) {
        if (pixelValueI < db->animation->size.length) {
          if (cJSON_IsNumber(pixelValue)) {
            db->animation->frames[frameI][pixelValueI] = pixelValue->valueint;
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

  // Pretend like we've just shown the last frame
  db->animation->lastFrameIndex = db->animation->frameCount;
  // output the "next" frame, which will be the first one
  displayBufferAnimationShowNext(db);
}

void parseAndShowTime(DisplayBufferHandle db, const cJSON *command) {
  TimeInfo timeInfo;
  time_get(&timeInfo);

  char timeString[9];
  snprintf(timeString, sizeof(timeString), "%u:%u %s", timeInfo.hour12,
           timeInfo.minute, timeInfo.isPM ? "PM" : "AM");

  parseAndSetState(db, command, "string");

  displayBufferDrawString(db, timeString);
}

// month names for the date display
static const char *monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void parseAndShowDate(DisplayBufferHandle db, const cJSON *command) {
  TimeInfo timeInfo;
  time_get(&timeInfo);

  // Jan 31, 2025
  char timeString[13];
  snprintf(timeString, sizeof(timeString), "%s %u, %u",
           monthNames[timeInfo.month - 1], timeInfo.dayOfMonth, timeInfo.year);

  parseAndSetState(db, command, "string");

  displayBufferDrawString(db, timeString);
}

esp_err_t parseAndShowCommands(DisplayBufferHandle db, char *data,
                               size_t length) {
  esp_err_t ret = ESP_OK;
  const cJSON *command = NULL;
  const cJSON *type = NULL;
  uint16_t commandIndex = 0;
  cJSON *json = cJSON_ParseWithLength(data, length);

  // we're restarting with the db, clear any animation data
  displayBufferAnimationEnd(db);

  ESP_GOTO_ON_FALSE(json != NULL, ESP_ERR_INVALID_RESPONSE,
                    parseAndShowCommands_cleanup, TAG,
                    "Invalid JSON response or content length");

  ESP_GOTO_ON_FALSE(cJSON_IsArray(json), ESP_ERR_INVALID_RESPONSE,
                    parseAndShowCommands_cleanup, TAG,
                    "JSON response is not an array");

  cJSON_ArrayForEach(command, json) {
    if (cJSON_IsObject(command)) {
      type = cJSON_GetObjectItemCaseSensitive(command, "type");
      if (cJSON_IsString(type) && type->valuestring != NULL) {
        if (strcmp(type->valuestring, "string") == 0) {
          parseAndShowString(db, command);
        } else if (strcmp(type->valuestring, "line") == 0) {
          parseAndShowLine(db, command);
        } else if (strcmp(type->valuestring, "bitmap") == 0) {
          parseAndShowBitmap(db, command);
        } else if (strcmp(type->valuestring, "line-feed") == 0) {
          displayBufferLineFeed(db);
        } else if (strcmp(type->valuestring, "set-state") == 0) {
          parseAndSetState(db, command, "set-state");
        } else if (strcmp(type->valuestring, "animation") == 0) {
          parseAndShowAnimation(db, command);
        } else if (strcmp(type->valuestring, "time") == 0) {
          parseAndShowTime(db, command);
        } else if (strcmp(type->valuestring, "date") == 0) {
          parseAndShowDate(db, command);
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

parseAndShowCommands_cleanup:
  cJSON_Delete(json);
  return ret;
}

/**
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

void commandStateInit(CommandState **stateHandle) {
  CommandState *state = (CommandState *)malloc(sizeof(CommandState));

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
    command->value.bitmap->data = NULL;
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
    command->value.animation->frames = NULL;
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
    free(command->value.bitmap->data);
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
    free(command->value.animation->frames);
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
  CommandListNode *newNode = (CommandListNode *)malloc(sizeof(CommandListNode));

  esp_err_t cmdRet = commandInit(commandHandle, type);
  if (cmdRet != ESP_OK) {
    free(newNode);
    return cmdRet;
  }

  newNode->command = *commandHandle;
  newNode->next = NULL;

  // no items yet, add as the first item
  if (commandList->head == NULL || commandList->tail == NULL) {
    commandList->head = newNode;
    commandList->tail = newNode;
    return;
  }

  commandList->tail->next = newNode;
  commandList->tail = newNode;

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

/**
 *
 *
 *
 *
 * Need to think about `state` a little more. What if the
 * color, position, or font size is undefined? The structure
 * includes it by default, but it is technically optional...
 * These should be pointers that are default NULL
 *
 *
 *
 *
 */

// This is responsible for pulling off shared state data and adding it.
// `CommandState **state` must not be initiated or else this will leak data.
void parseAndAddState(CommandState **state, const cJSON *commandJson,
                      char *type) {
  bool didInitState = false;

  const cJSON *fontSize =
      cJSON_GetObjectItemCaseSensitive(commandJson, "fontSize");
  if (cJSON_IsString(fontSize) && fontSize->valuestring != NULL) {
    if (!didInitState) {
      didInitState = true;
      commandStateInit(state);
    }

    if (strcmp(fontSize->valuestring, "sm") == 0) {
      (*state)->fontSize = FONT_SIZE_SM;
    } else if (strcmp(fontSize->valuestring, "lg") == 0) {
      (*state)->fontSize = FONT_SIZE_LG;
    } else if (strcmp(fontSize->valuestring, "md") == 0) {
      (*state)->fontSize = FONT_SIZE_MD;
    } else {
      invalidPropWarn(type, "fontSize");
    }
  }

  const cJSON *color = cJSON_GetObjectItemCaseSensitive(commandJson, "color");
  if (cJSON_IsNumber(color)) {
    if (!didInitState) {
      didInitState = true;
      commandStateInit(state);
    }

    (*state)->color = color->valueint;
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
    }
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
  commandListNodeInit(commandList, &command, COMMAND_TYPE_STRING);
  parseAndAddState(&command->value.string->state, commandJson, "string");

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
  commandListNodeInit(commandList, &command, COMMAND_TYPE_LINE);
  parseAndAddState(&command->value.line->state, commandJson, "line");

  command->value.line->toX = toX->valueint;
  command->value.line->toY = toY->valueint;
}

void parseAndAppendBitmap(CommandListHandle commandList,
                          const cJSON *commandJson) {
  const cJSON *data = cJSON_GetObjectItemCaseSensitive(commandJson, "data");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(commandJson, "size");
  if (!cJSON_IsArray(data) || !cJSON_IsObject(size)) {
    invalidShapeWarn("bitmap");
    return;
  }

  const cJSON *sizeW = cJSON_GetObjectItemCaseSensitive(size, "width");
  const cJSON *sizeH = cJSON_GetObjectItemCaseSensitive(size, "height");
  if (!cJSON_IsNumber(sizeW) || !cJSON_IsNumber(sizeH)) {
    invalidPropWarn("bitmap", "size");
    return;
  }

  CommandHandle command;
  commandListNodeInit(commandList, &command, COMMAND_TYPE_BITMAP);
  parseAndAddState(&command->value.bitmap->state, commandJson, "bitmap");

  command->value.bitmap->width = sizeW->valueint;
  command->value.bitmap->height = sizeH->valueint;
  command->value.bitmap->data =
      (uint16_t *)malloc(cJSON_GetArraySize(data) * sizeof(uint16_t));

  // we cannot access the array directly, so we have to loop and put the values
  // into a buffer that we can use with the display buffer
  const cJSON *pixelValue = NULL;
  uint16_t bufIndex = 0;
  cJSON_ArrayForEach(pixelValue, data) {
    if (cJSON_IsNumber(pixelValue)) {
      command->value.bitmap->data[bufIndex] = (uint16_t)pixelValue->valueint;
    } else {
      invalidPropWarn("bitmap", "pixel value");
      command->value.bitmap->data[bufIndex] = (uint16_t)0;
    }
    bufIndex++;
  }
}

void parseAndAppendAnimation(CommandListHandle commandList,
                             const cJSON *commandJson) {
  const cJSON *position =
      cJSON_GetObjectItemCaseSensitive(commandJson, "position");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(commandJson, "size");
  const cJSON *frames = cJSON_GetObjectItemCaseSensitive(commandJson, "frames");
  if (!cJSON_IsObject(position) || !cJSON_IsObject(size) ||
      !cJSON_IsArray(frames)) {
    invalidShapeWarn("animation");
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
  commandListNodeInit(commandList, &command, COMMAND_TYPE_ANIMATION);

  command->value.animation->frameCount = cJSON_GetArraySize(frames);
  command->value.animation->posX = posX->valueint;
  command->value.animation->posY = posY->valueint;
  command->value.animation->width = sizeW->valueint;
  command->value.animation->height = sizeH->valueint;

  uint16_t fameSize =
      command->value.animation->width * command->value.animation->height;

  command->value.animation->frames = (uint16_t *)malloc(
      command->value.animation->frameCount * fameSize * sizeof(uint16_t));

  // the frame we're iterating
  uint16_t frameI = 0;
  // the value within the frame
  uint16_t pixelValueI;
  const cJSON *frame = NULL;
  const cJSON *pixelValue = NULL;

  // now loop all frames and extract their values
  cJSON_ArrayForEach(frame, frames) {
    if (cJSON_IsArray(frame)) {
      pixelValueI = 0;
      cJSON_ArrayForEach(pixelValue, frame) {
        if (pixelValueI < fameSize) {
          if (cJSON_IsNumber(pixelValue)) {
            command->value.animation
                ->frames[(frameI * fameSize) + pixelValueI] =
                (uint16_t)pixelValue->valueint;
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
  commandListNodeInit(commandList, &command, COMMAND_TYPE_TIME);
  parseAndAddState(&command->value.time->state, commandJson, "time");
}

void parseAndAppendDate(CommandListHandle commandList,
                        const cJSON *commandJson) {
  CommandHandle command;
  commandListNodeInit(commandList, &command, COMMAND_TYPE_DATE);
  parseAndAddState(&command->value.date->state, commandJson, "date");
}

void parseAndAppendLineFeed(CommandListHandle commandList,
                            const cJSON *commandJson) {
  CommandHandle command;
  commandListNodeInit(commandList, &command, COMMAND_TYPE_LINEFEED);
}

void parseAndAppendSetState(CommandListHandle commandList,
                            const cJSON *commandJson) {
  CommandHandle command;
  commandListNodeInit(commandList, &command, COMMAND_TYPE_SETSTATE);
  parseAndAddState(&command->value.setState->state, commandJson, "setState");
}

esp_err_t parseCommands(CommandListHandle *commandListHandle, char *data,
                        size_t length) {
  esp_err_t ret = ESP_OK;
  const cJSON *commandJson = NULL;
  const cJSON *type = NULL;
  uint16_t commandIndex = 0;
  cJSON *json = cJSON_ParseWithLength(data, length);

  ESP_GOTO_ON_FALSE(json != NULL, ESP_ERR_INVALID_RESPONSE,
                    parseAndShowCommands_cleanup, TAG,
                    "Invalid JSON response or content length");

  ESP_GOTO_ON_FALSE(cJSON_IsArray(json), ESP_ERR_INVALID_RESPONSE,
                    parseAndShowCommands_cleanup, TAG,
                    "JSON response is not an array");

  commandListInit(commandListHandle);
  CommandListHandle commandList = *commandListHandle;

  cJSON_ArrayForEach(commandJson, json) {
    if (cJSON_IsObject(commandJson)) {
      type = cJSON_GetObjectItemCaseSensitive(commandJson, "type");
      if (cJSON_IsString(type) && type->valuestring != NULL) {
        if (strcmp(type->valuestring, "string") == 0) {
          parseAndAppendString(commandList, commandJson);
        } else if (strcmp(type->valuestring, "line") == 0) {
          parseAndAppendLine(commandList, commandJson);
        } else if (strcmp(type->valuestring, "bitmap") == 0) {
          parseAndAppendBitmap(commandList, commandJson);
        } else if (strcmp(type->valuestring, "line-feed") == 0) {
          parseAndAppendLineFeed(commandList, commandJson);
        } else if (strcmp(type->valuestring, "set-state") == 0) {
          parseAndAppendSetState(commandList, commandJson);
        } else if (strcmp(type->valuestring, "animation") == 0) {
          parseAndAppendAnimation(commandList, commandJson);
        } else if (strcmp(type->valuestring, "time") == 0) {
          parseAndAppendTime(commandList, commandJson);
        } else if (strcmp(type->valuestring, "date") == 0) {
          parseAndAppendDate(commandList, commandJson);
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

parseAndShowCommands_cleanup:
  cJSON_Delete(json);
  return ret;
}