#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lib/cjson/cJSON.h"
#include <string.h>

#include "gfx/displayBuffer.h"

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

esp_err_t parseAndShowCommands(DisplayBufferHandle db, char *data,
                               size_t length) {
  esp_err_t ret = ESP_OK;
  const cJSON *command = NULL;
  const cJSON *type = NULL;
  uint16_t commandIndex = 0;

  cJSON *json = cJSON_ParseWithLength(data, length);

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