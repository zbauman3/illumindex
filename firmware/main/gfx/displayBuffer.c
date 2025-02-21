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
  if (displayBuffer->cursor.x + (displayBuffer->font->width * 2) <=
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

void drawString(DisplayBufferHandle db, char *string, uint16_t color) {
  const size_t stringLength = strlen(string);
  // the index within the string we are working on
  uint16_t stringIndex;
  // the actual ascii character we are working on
  char asciiChar;
  // the starting point to draw bits at in the buffer
  uint16_t bufferStartIdx;
  // which bit of the font->width we are on working on
  uint8_t bitmapRowIdx;
  // the index of the character's bitmap chunk we are working on
  uint8_t chunkIdx;
  // the actual chunk value, pulled once to be used multiple
  uint32_t chunkVal;
  // the number of the bit we are working on, within the chunk. This is the
  // _number_, not the zero-based index
  uint8_t chunkBitN;

  // loop all the characters in the string
  for (stringIndex = 0; stringIndex < stringLength; stringIndex++) {
    asciiChar = string[stringIndex];
    bitmapRowIdx = 0;
    // convert the buffers cursor to an index, where we start this char
    bufferStartIdx = _getBufferIndexFromCursor(db);

    // if not a character that we support, change to `?`
    if (!fontIsValidAscii(asciiChar)) {
      ESP_LOGW(TAG, "Unsupported ASCII character \"%d\"", asciiChar);
      asciiChar = 63;
    }

    for (chunkIdx = 0; chunkIdx < db->font->chunksPerChar; chunkIdx++) {
      chunkVal = fontGetChunk(db->font, asciiChar, chunkIdx);

      for (chunkBitN = 1; chunkBitN <= db->font->bitsPerChunk; chunkBitN++) {
        // mask the chunk bit, then AND it to the chunk value. Use the result as
        // a boolean to check if we should set the value to the color or blank
        _safeSetBufferValue(
            db, bufferStartIdx + bitmapRowIdx,
            (chunkVal & _BV_1ULL(db->font->bitsPerChunk - chunkBitN)) ? color
                                                                      : 0);

        // increase the rows index. Move down a line if at the end
        bitmapRowIdx++;
        if (bitmapRowIdx == db->font->width) {
          bitmapRowIdx = 0;
          bufferStartIdx += db->width;
        }
      }
    }

    // we're done with this character, move to the next position.
    _moveCursorOneCharWrap(db);
  }
}