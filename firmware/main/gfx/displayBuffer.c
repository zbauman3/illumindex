#include <memory.h>

#include "esp_log.h"

#include "gfx/displayBuffer.h"
#include "util/565_color.h"

#include "gfx/fonts.h"
#include "util/helpers.h"
#include "util/math.h"

const static char *TAG = "DISPLAY_BUFFER";

// avoids setting values that are out of range
void _safeSetBufferValue(DisplayBufferHandle displayBuffer, uint16_t index,
                         uint16_t value) {
  if (index < displayBuffer->width * displayBuffer->height) {
    displayBuffer->buffer[index] = value;
  }
}

// does not check if the returned index is within range
uint16_t _getBufferIndexFromCursor(DisplayBufferHandle displayBuffer) {
  return (displayBuffer->cursor.y * displayBuffer->width) +
         displayBuffer->cursor.x;
}

// wraps, but does not check that the new row (y) is within range
void _moveCursorOneCharWrap(DisplayBufferHandle displayBuffer) {
  // no overflowing, just update
  if (displayBuffer->cursor.x + (displayBuffer->font->width * 2) <
      displayBuffer->width) {
    displayBuffer->cursor.x += displayBuffer->font->width;
    return;
  }

  // otherwise, wrap
  displayBuffer->cursor.y += displayBuffer->font->height;
  displayBuffer->cursor.x = 0;
}

esp_err_t displayBufferInit(DisplayBufferHandle *displayBufferHandle) {
  DisplayBufferHandle displayBuffer =
      (DisplayBufferHandle)malloc(sizeof(DisplayBuffer));

  displayBuffer->buffer = (uint16_t *)malloc(DISPLAY_BUFFER_SIZE);
  displayBufferClear(displayBuffer);

  fontInit(&displayBuffer->font, FONT_SIZE_MD);

  displayBuffer->cursor.x = 0;
  displayBuffer->cursor.y = 0;
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
                uint16_t color) {
  const size_t stringLength = strlen(string);
  bool shouldSetIndex;
  uint16_t bufferIndexRowStart;
  uint8_t bitmapBit;
  uint8_t currentCharChunk;
  uint16_t stringIndex;
  char character;

  for (stringIndex = 0; stringIndex < stringLength; stringIndex++) {
    character = string[stringIndex];
    // we only have bitmaps for these characters. Anything else isn't
    // allowed
    if (!fontIsValidAscii(character)) {
      ESP_LOGW(TAG, "Unsupported ASCII character \"%d\"", string[stringIndex]);
      character = 63; // assign to `?`
    }

    bufferIndexRowStart = _getBufferIndexFromCursor(displayBuffer);

    // get the starting index for the character in the bitmap array
    currentCharChunk = 0;

    for (bitmapBit = 0; bitmapBit < displayBuffer->font->bitPerChar;
         bitmapBit++) {
      if (bitmapBit != 0) {
        if (bitmapBit % displayBuffer->font->bitsPerChunk == 0) {
          currentCharChunk++;
        }
        if (bitmapBit % displayBuffer->font->width == 0) {
          bufferIndexRowStart += displayBuffer->width;
        }
      }

      shouldSetIndex =
          (fontGetChunk(displayBuffer->font, character, currentCharChunk) &
           _BV_1ULL(displayBuffer->font->bitsPerChunk - 1 -
                    (bitmapBit % displayBuffer->font->bitsPerChunk)));

      _safeSetBufferValue(
          displayBuffer,
          (bufferIndexRowStart + (bitmapBit % displayBuffer->font->width)),
          (shouldSetIndex ? color : 0));
    }

    _moveCursorOneCharWrap(displayBuffer);
  }
}