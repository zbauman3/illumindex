#include <math.h>
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

// wraps, but does not check that the new row (y) is within range
void _moveCursorOneCharWrap(DisplayBufferHandle displayBuffer) {
  // no overflowing, just update
  if (displayBuffer->cursor.x + (displayBuffer->font->width * 2) +
          displayBuffer->font->spacing <=
      displayBuffer->width) {
    displayBuffer->cursor.x +=
        displayBuffer->font->width + displayBuffer->font->spacing;
    return;
  }

  // otherwise, wrap
  displayBufferLineFeed(displayBuffer);
}

esp_err_t displayBufferInit(DisplayBufferHandle *displayBufferHandle) {
  DisplayBufferHandle displayBuffer =
      (DisplayBufferHandle)malloc(sizeof(DisplayBuffer));

  displayBuffer->buffer = (uint16_t *)malloc(DISPLAY_BUFFER_SIZE);
  displayBufferClear(displayBuffer);

  fontInit(&displayBuffer->font);

  displayBufferSetColor(displayBuffer, 0b1111100000000000);
  displayBufferSetCursor(displayBuffer, 0, 0);
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

// This will apply the provided string to the buffer, using the buffer's current
// cursor and font. The string will be wrapped until it is out of the matrix.
void displayBufferDrawString(DisplayBufferHandle db, char *string) {
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
    bufferStartIdx = displayBufferCursorToIndex(db);

    // If we have moved past the buffer's space, we can go ahead and end
    if (!displayBufferCursorIsVisible(db)) {
      return;
    }

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
            (chunkVal & _BV_1ULL(db->font->bitsPerChunk - chunkBitN))
                ? db->color
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

void displayBufferDrawFastVertLine(DisplayBufferHandle displayBuffer,
                                   uint8_t to) {
  // faster to write it twice
  if (displayBuffer->cursor.y > to) {
    while (displayBuffer->cursor.y >= to) {
      if (displayBufferPointIsVisible(displayBuffer, displayBuffer->cursor.x,
                                      displayBuffer->cursor.y)) {
        _safeSetBufferValue(displayBuffer,
                            displayBufferPointToIndex(displayBuffer,
                                                      displayBuffer->cursor.x,
                                                      displayBuffer->cursor.y),
                            displayBuffer->color);
      }

      displayBuffer->cursor.y--;
    }
  } else {
    while (displayBuffer->cursor.y <= to) {
      if (displayBufferPointIsVisible(displayBuffer, displayBuffer->cursor.x,
                                      displayBuffer->cursor.y)) {
        _safeSetBufferValue(displayBuffer,
                            displayBufferPointToIndex(displayBuffer,
                                                      displayBuffer->cursor.x,
                                                      displayBuffer->cursor.y),
                            displayBuffer->color);
      }

      displayBuffer->cursor.y++;
    }
  }

  displayBufferSetCursor(displayBuffer, displayBuffer->cursor.x, to);
}

void displayBufferDrawFastHorizonLine(DisplayBufferHandle displayBuffer,
                                      uint8_t to) {
  // faster to write it twice
  if (displayBuffer->cursor.x > to) {
    while (displayBuffer->cursor.x >= to) {
      if (displayBufferPointIsVisible(displayBuffer, displayBuffer->cursor.x,
                                      displayBuffer->cursor.y)) {
        _safeSetBufferValue(displayBuffer,
                            displayBufferPointToIndex(displayBuffer,
                                                      displayBuffer->cursor.x,
                                                      displayBuffer->cursor.y),
                            displayBuffer->color);
      }

      displayBuffer->cursor.x--;
    }
  } else {
    while (displayBuffer->cursor.x <= to) {
      if (displayBufferPointIsVisible(displayBuffer, displayBuffer->cursor.x,
                                      displayBuffer->cursor.y)) {
        _safeSetBufferValue(displayBuffer,
                            displayBufferPointToIndex(displayBuffer,
                                                      displayBuffer->cursor.x,
                                                      displayBuffer->cursor.y),
                            displayBuffer->color);
      }

      displayBuffer->cursor.x++;
    }
  }

  displayBufferSetCursor(displayBuffer, to, displayBuffer->cursor.y);
}

void displayBufferDrawFastDiagLine(DisplayBufferHandle displayBuffer,
                                   uint8_t toX, uint8_t toY) {
  float change = ((float)displayBuffer->cursor.y - (float)toY) /
                 ((float)displayBuffer->cursor.x - (float)toX);
  float unroundedY = (float)displayBuffer->cursor.y;

  if (displayBuffer->cursor.x <= toX) {
    while (displayBuffer->cursor.x <= toX) {
      if (displayBufferPointIsVisible(displayBuffer, displayBuffer->cursor.x,
                                      (uint8_t)round(unroundedY))) {
        _safeSetBufferValue(
            displayBuffer,
            displayBufferPointToIndex(displayBuffer, displayBuffer->cursor.x,
                                      (uint8_t)round(unroundedY)),
            displayBuffer->color);
      }

      displayBuffer->cursor.x++;
      unroundedY += change;
    }
  } else {
    while (displayBuffer->cursor.x >= toX) {
      if (displayBufferPointIsVisible(displayBuffer, displayBuffer->cursor.x,
                                      (uint8_t)round(unroundedY))) {
        _safeSetBufferValue(
            displayBuffer,
            displayBufferPointToIndex(displayBuffer, displayBuffer->cursor.x,
                                      (uint8_t)round(unroundedY)),
            displayBuffer->color);
      }

      displayBuffer->cursor.x--;
      unroundedY -= change;
    }
  }

  displayBufferSetCursor(displayBuffer, toX, toY);
}

void displayBufferDrawLine(DisplayBufferHandle displayBuffer, uint8_t toX,
                           uint8_t toY) {
  if (displayBuffer->cursor.x == toX) {
    displayBufferDrawFastVertLine(displayBuffer, toY);
  } else if (displayBuffer->cursor.y == toY) {
    displayBufferDrawFastHorizonLine(displayBuffer, toX);
  } else {
    displayBufferDrawFastDiagLine(displayBuffer, toX, toY);
  }
}

void displayBufferDrawBitmap(DisplayBufferHandle displayBuffer, uint8_t width,
                             uint8_t height, uint16_t *buffer) {
  for (uint8_t row = 0; row < height; row++) {
    for (uint8_t col = 0; col < width; col++) {
      if (displayBufferPointIsVisible(displayBuffer,
                                      displayBuffer->cursor.x + col,
                                      displayBuffer->cursor.y + row)) {
        _safeSetBufferValue(displayBuffer,
                            displayBufferPointToIndex(
                                displayBuffer, displayBuffer->cursor.x + col,
                                displayBuffer->cursor.y + row),
                            buffer[(row * width) + col]);
      }
    }
  }
}