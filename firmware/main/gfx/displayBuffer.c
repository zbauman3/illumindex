#include <memory.h>

#include "esp_log.h"

#include "gfx/displayBuffer.h"
#include "util/565_color.h"

#include "gfx/fonts.h"
#include "util/helpers.h"

const static char *TAG = "DISPLAY_BUFFER";

esp_err_t displayBufferInit(DisplayBufferHandle *displayBufferHandle) {
  DisplayBufferHandle displayBuffer =
      (DisplayBufferHandle)malloc(sizeof(DisplayBuffer));

  displayBuffer->buffer = (uint16_t *)malloc(DISPLAY_BUFFER_SIZE);
  displayBufferClear(displayBuffer);

  fontInit(&displayBuffer->font, FONT_SIZE_MD);

  displayBuffer->width = 64;
  displayBuffer->height = 32;

  *displayBufferHandle = displayBuffer;

  return ESP_OK;
}

void displayBufferClear(DisplayBufferHandle displayBuffer) {
  memset(displayBuffer->buffer, 0, DISPLAY_BUFFER_SIZE);
}

void displayBufferEnd(DisplayBufferHandle displayBuffer) {
  fontEnd(displayBuffer->font);
  free(displayBuffer->buffer);
  free(displayBuffer);
}

void drawString(DisplayBufferHandle displayBuffer, char *string,
                uint16_t startOffset, uint16_t color) {
  uint16_t bitmapCharacterIndex;
  uint16_t bufferOffset;
  bool bitmapValue;

  for (uint16_t stringIndex = 0; stringIndex < strlen(string); stringIndex++) {
    // we only have bitmaps for these characters. Anything else isn't allowed
    if (string[stringIndex] < DISPLAY_BUFFER_ASCII_MIN ||
        string[stringIndex] > DISPLAY_BUFFER_ASCII_MAX) {
      ESP_LOGW(TAG, "Unsupported ASCII character \"%d\"", string[stringIndex]);
      return;
    }

    // get the starting index for the character in the bitmap array
    bitmapCharacterIndex =
        (string[stringIndex] - 32) * displayBuffer->font->chunksPerChar;
    // Get the starting xy position for this character
    bufferOffset = startOffset + (displayBuffer->font->width * stringIndex);

    for (uint8_t bitmapBit = 0;
         bitmapBit <
         displayBuffer->font->bitsPerChunk * displayBuffer->font->chunksPerChar;
         bitmapBit++) {
      if (bitmapBit != 0) {
        if (bitmapBit % displayBuffer->font->bitsPerChunk == 0) {
          bitmapCharacterIndex++;
        }
        if (bitmapBit % displayBuffer->font->width == 0) {
          bufferOffset = bufferOffset + displayBuffer->width;
        }
      }

      bitmapValue =
          (bool)(fontGetChunk(displayBuffer->font, bitmapCharacterIndex) &
                 _BV_1ULL(displayBuffer->font->bitsPerChunk - 1 -
                          (bitmapBit % displayBuffer->font->bitsPerChunk)));

      displayBuffer
          ->buffer[bufferOffset + (bitmapBit % displayBuffer->font->width)] =
          bitmapValue ? color : 0;
    }
  }
}