#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lib/cjson/cJSON.h"
#include <string.h>

#include "gfx/displayBuffer.h"

static const char *TAG = "COMMANDS";

void parseAndShowString(DisplayBufferHandle db, const cJSON *command) {
  const cJSON *value = cJSON_GetObjectItemCaseSensitive(command, "value");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(command, "size");
  const cJSON *color = cJSON_GetObjectItemCaseSensitive(command, "color");
  const cJSON *pos = cJSON_GetObjectItemCaseSensitive(command, "position");

  if (!cJSON_IsString(value) || !cJSON_IsString(size) ||
      !cJSON_IsNumber(color) || !cJSON_IsObject(pos)) {
    ESP_LOGW(TAG, "Command of type 'string' does not have a valid shape.");
    return;
  }

  const cJSON *x = cJSON_GetObjectItemCaseSensitive(pos, "x");
  const cJSON *y = cJSON_GetObjectItemCaseSensitive(pos, "y");

  if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y)) {
    ESP_LOGW(TAG, "Command of type 'string' does not have a valid shape.");
    return;
  }

  displayBufferSetCursor(db, x->valueint, y->valueint);
  displayBufferSetColor(db, color->valueint);

  if (strcmp(size->valuestring, "sm") == 0) {
    fontSetSize(db->font, FONT_SIZE_SM);
  } else if (strcmp(size->valuestring, "lg") == 0) {
    fontSetSize(db->font, FONT_SIZE_LG);
  } else {
    fontSetSize(db->font, FONT_SIZE_MD);
  }

  displayBufferDrawString(db, value->valuestring);
}

void parseAndShowLine(DisplayBufferHandle db, const cJSON *command) {
  const cJSON *color = cJSON_GetObjectItemCaseSensitive(command, "color");
  const cJSON *from = cJSON_GetObjectItemCaseSensitive(command, "from");
  const cJSON *to = cJSON_GetObjectItemCaseSensitive(command, "to");

  if (!cJSON_IsNumber(color) || !cJSON_IsObject(from) || !cJSON_IsObject(to)) {
    ESP_LOGW(TAG, "Command of type 'line' does not have a valid shape.");
    return;
  }

  const cJSON *fromX = cJSON_GetObjectItemCaseSensitive(from, "x");
  const cJSON *fromY = cJSON_GetObjectItemCaseSensitive(from, "y");
  const cJSON *toX = cJSON_GetObjectItemCaseSensitive(to, "x");
  const cJSON *toY = cJSON_GetObjectItemCaseSensitive(to, "y");

  if (!cJSON_IsNumber(fromX) || !cJSON_IsNumber(fromY) ||
      !cJSON_IsNumber(toX) || !cJSON_IsNumber(toY)) {
    ESP_LOGW(TAG, "Command of type 'line' does not have a valid shape.");
    return;
  }

  displayBufferSetColor(db, color->valueint);
  displayBufferSetCursor(db, fromX->valueint, fromY->valueint);
  displayBufferDrawLine(db, toX->valueint, toY->valueint);
}

void parseAndShowBitmap(DisplayBufferHandle db, const cJSON *command) {
  const cJSON *data = cJSON_GetObjectItemCaseSensitive(command, "data");
  const cJSON *pos = cJSON_GetObjectItemCaseSensitive(command, "position");
  const cJSON *size = cJSON_GetObjectItemCaseSensitive(command, "size");

  if (!cJSON_IsArray(data) || !cJSON_IsObject(pos) || !cJSON_IsObject(size)) {
    ESP_LOGW(TAG, "Command of type 'bitmap' does not have a valid shape.");
    return;
  }

  const cJSON *posX = cJSON_GetObjectItemCaseSensitive(pos, "x");
  const cJSON *posY = cJSON_GetObjectItemCaseSensitive(pos, "y");
  const cJSON *sizeW = cJSON_GetObjectItemCaseSensitive(size, "width");
  const cJSON *sizeH = cJSON_GetObjectItemCaseSensitive(size, "height");

  if (!cJSON_IsNumber(posX) || !cJSON_IsNumber(posY) ||
      !cJSON_IsNumber(sizeW) || !cJSON_IsNumber(sizeH)) {
    ESP_LOGW(TAG, "Command of type 'bitmap' does not have a valid shape.");
    return;
  }

  displayBufferSetCursor(db, posX->valueint, posY->valueint);

  // we cannot access the array directly, so we have to loop and put the values
  // into a buffer that we can use with the display buffer
  const cJSON *pixelValue = NULL;
  uint16_t bufIndex = 0;
  uint16_t *bmBuffer = malloc(cJSON_GetArraySize(data));
  cJSON_ArrayForEach(pixelValue, data) {
    if (cJSON_IsNumber(pixelValue)) {
      bmBuffer[bufIndex] = pixelValue->valueint;
    } else {
      ESP_LOGW(TAG, "Command of type 'bitmap' has an invalid pixel value.");
      bmBuffer[bufIndex] = 0;
    }
    bufIndex++;
  }

  displayBufferDrawBitmap(db, sizeW->valueint, sizeH->valueint, bmBuffer);

  free(bmBuffer);
}

esp_err_t parseAndShowCommands(DisplayBufferHandle db, char *data,
                               size_t length) {
  esp_err_t ret = ESP_OK;
  cJSON *json = NULL;
  const cJSON *command = NULL;
  const cJSON *type = NULL;
  uint16_t commandIndex = 0;

  json = cJSON_ParseWithLength(data, length);

  ESP_GOTO_ON_FALSE(json != NULL, ESP_ERR_INVALID_RESPONSE,
                    parseAndShowCommands_cleanup, TAG,
                    "Invalid JSON response or content length");

  ESP_GOTO_ON_FALSE(cJSON_IsArray(json), ESP_ERR_INVALID_RESPONSE,
                    parseAndShowCommands_cleanup, TAG,
                    "JSON response is not an array");

  cJSON_ArrayForEach(command, json) {
    if (cJSON_IsObject(command)) {
      type = cJSON_GetObjectItemCaseSensitive(command, "type");
      if (cJSON_IsString(type)) {
        if (strcmp(type->valuestring, "string") == 0) {
          parseAndShowString(db, command);
        } else if (strcmp(type->valuestring, "line") == 0) {
          parseAndShowLine(db, command);
        } else if (strcmp(type->valuestring, "bitmap") == 0) {
          parseAndShowBitmap(db, command);
        } else {
          ESP_LOGI(TAG, "Command %u does not have a valid 'type'",
                   commandIndex);
        }
      } else {
        ESP_LOGI(TAG, "Command %u does not have a 'string' 'type'",
                 commandIndex);
      }
    } else {
      ESP_LOGI(TAG, "Command %u is not an object", commandIndex);
    }

    commandIndex++;
  }

parseAndShowCommands_cleanup:
  cJSON_Delete(json);
  return ret;
}