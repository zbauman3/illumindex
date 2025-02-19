#include <memory.h>

#include "esp_log.h"

#include "gfx/displayBuffer.h"
#include "util/565_color.h"

#include "gfx/strings.h"
#include "util/helpers.h"

const static char *TAG = "DISPLAY_BUFFER";

esp_err_t displayBufferInit(DisplayBufferHandle *displayBufferHandle) {
  DisplayBufferHandle displayBuffer =
      (DisplayBufferHandle)malloc(sizeof(DisplayBuffer));

  displayBuffer->buffer = (uint16_t *)malloc(DISPLAY_BUFFER_SIZE);
  displayBufferClear(displayBuffer);

  *displayBufferHandle = displayBuffer;

  return ESP_OK;
}

void displayBufferClear(DisplayBufferHandle displayBuffer) {
  memset(displayBuffer->buffer, 0, DISPLAY_BUFFER_SIZE);
}

void displayBufferEnd(DisplayBufferHandle displayBuffer) {
  free(displayBuffer->buffer);
  free(displayBuffer);
}

void drawString(DisplayBufferHandle displayBuffer, char *stringBuff,
                uint16_t startOffset, uint16_t color) {
  uint16_t currentOffset;

  for (uint16_t stringIndex = 0; stringIndex < strlen(stringBuff);
       stringIndex++) {

    currentOffset = startOffset + (8 * stringIndex);

    if (stringBuff[stringIndex] < 32 || stringBuff[stringIndex] > 126) {
      ESP_LOGW(TAG, "Unsupported ASCII character \"%d\"",
               stringBuff[stringIndex]);
      return;
    }

    uint16_t bitmapCharacterIndex = (stringBuff[stringIndex] - 32) * 3;
    uint16_t bufferOffset = currentOffset;
    bool bitmapValue = false;

    for (uint8_t bitmapBit = 0; bitmapBit < 32 * 3; bitmapBit++) {
      if (bitmapBit != 0) {
        if (bitmapBit % 32 == 0) {
          bitmapCharacterIndex++;
        }
        if (bitmapBit % 8 == 0) {
          bufferOffset = bufferOffset + 64;
        }
      }

      bitmapValue = (bool)(ascii8By12[bitmapCharacterIndex] &
                           _BV_1ULL(31 - (bitmapBit % 32)));

      displayBuffer->buffer[bufferOffset + (bitmapBit % 8)] =
          bitmapValue ? color : 0;
    }
  }
}