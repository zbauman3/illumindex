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