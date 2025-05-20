#include <math.h>
#include <memory.h>

#include "esp_log.h"

#include "gfx/displayBuffer.h"
#include "gfx/fonts.h"
#include "util/565_color.h"
#include "util/helpers.h"

const static char *TAG = "DISPLAY_BUFFER";

// allocates all memory required for the DisplayBuffer and initiates the values
esp_err_t displayBufferInit(DisplayBufferHandle *displayBufferHandle,
                            uint8_t width, uint8_t height) {
  DisplayBufferHandle db = (DisplayBufferHandle)malloc(sizeof(DisplayBuffer));

  displayBufferSetColor(db, 0b1111100000000000);
  displayBufferSetCursor(db, 0, 0);
  db->width = width;
  db->height = height;
  db->animation = NULL;

  db->buffer = (uint16_t *)malloc(sizeof(uint16_t) * db->width * db->height);
  displayBufferClear(db);

  fontInit(&db->font);

  *displayBufferHandle = db;

  return ESP_OK;
}

void displayBufferAnimationInit(DisplayBufferHandle db, uint16_t delay,
                                uint8_t posX, uint8_t posY, uint8_t width,
                                uint8_t height, uint16_t frameCount) {

  // always clear previous data
  displayBufferAnimationEnd(db);

  db->animation = (AnimationHandle)malloc(sizeof(Animation));
  db->animation->delay = delay;
  db->animation->position.x = posX;
  db->animation->position.y = posY;
  db->animation->size.width = width;
  db->animation->size.height = height;
  db->animation->size.length =
      db->animation->size.width * db->animation->size.height;
  db->animation->lastFrameIndex = 0;
  db->animation->frameCount = frameCount;

  // allocate the first dimension for all frames
  db->animation->frames =
      (uint16_t **)malloc(sizeof(uint16_t *) * db->animation->frameCount);

  for (uint16_t i = 0; i < db->animation->frameCount; i++) {
    // for each frame, allocate space for the bitmap
    db->animation->frames[i] =
        (uint16_t *)malloc(sizeof(uint16_t) * db->animation->size.length);
    // set to all empty data. This will be the default if there's invalid data
    memset(db->animation->frames[i], 0,
           sizeof(uint16_t) * db->animation->size.length);
  }
}

void displayBufferAnimationEnd(DisplayBufferHandle db) {
  if (db->animation == NULL) {
    return;
  }

  for (uint16_t i = 0; i < db->animation->frameCount; i++) {
    free(db->animation->frames[i]);
  }

  free(db->animation->frames);
  free(db->animation);

  db->animation = NULL;
}

void displayBufferAnimationShowNext(DisplayBufferHandle db) {
  displayBufferSetCursor(db, db->animation->position.x,
                         db->animation->position.y);

  db->animation->lastFrameIndex++;
  if (db->animation->lastFrameIndex >= db->animation->frameCount) {
    db->animation->lastFrameIndex = 0;
  }

  // no need to do anything if there are no frames to show
  if (db->animation->frameCount == 0) {
    return;
  }

  displayBufferDrawBitmap(db, db->animation->size.width,
                          db->animation->size.height,
                          db->animation->frames[db->animation->lastFrameIndex]);
}

// resets all values in the buffer to `0`
void displayBufferClear(DisplayBufferHandle db) {
  memset(db->buffer, 0, sizeof(uint16_t) * db->width * db->height);
}

// cleans up all memory associated with the buffer
void displayBufferEnd(DisplayBufferHandle db) {
  fontEnd(db->font);
  free(db->buffer);
  free(db);
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
        safeSetBufferValue(
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
    moveCursorOneCharWrap(db);
  }
}

void displayBufferDrawFastVertLine(DisplayBufferHandle db, uint8_t to) {
  // faster to write it twice ¯\_(ツ)_/¯
  if (db->cursor.y > to) {
    while (db->cursor.y >= to) {
      if (displayBufferPointIsVisible(db, db->cursor.x, db->cursor.y)) {
        safeSetBufferValue(
            db, displayBufferPointToIndex(db, db->cursor.x, db->cursor.y),
            db->color);
      }

      db->cursor.y--;
    }
  } else {
    while (db->cursor.y <= to) {
      if (displayBufferPointIsVisible(db, db->cursor.x, db->cursor.y)) {
        safeSetBufferValue(
            db, displayBufferPointToIndex(db, db->cursor.x, db->cursor.y),
            db->color);
      }

      db->cursor.y++;
    }
  }

  displayBufferSetCursor(db, db->cursor.x, to);
}

void displayBufferDrawFastHorizonLine(DisplayBufferHandle db, uint8_t to) {
  // faster to write it twice ¯\_(ツ)_/¯
  if (db->cursor.x > to) {
    while (db->cursor.x >= to) {
      if (displayBufferPointIsVisible(db, db->cursor.x, db->cursor.y)) {
        safeSetBufferValue(
            db, displayBufferPointToIndex(db, db->cursor.x, db->cursor.y),
            db->color);
      }

      db->cursor.x--;
    }
  } else {
    while (db->cursor.x <= to) {
      if (displayBufferPointIsVisible(db, db->cursor.x, db->cursor.y)) {
        safeSetBufferValue(
            db, displayBufferPointToIndex(db, db->cursor.x, db->cursor.y),
            db->color);
      }

      db->cursor.x++;
    }
  }

  displayBufferSetCursor(db, to, db->cursor.y);
}

void displayBufferDrawFastDiagLine(DisplayBufferHandle db, uint8_t toX,
                                   uint8_t toY) {
  float change =
      ((float)db->cursor.y - (float)toY) / ((float)db->cursor.x - (float)toX);
  float unroundedY = (float)db->cursor.y;

  // faster to write it twice ¯\_(ツ)_/¯
  if (db->cursor.x <= toX) {
    while (db->cursor.x <= toX) {
      if (displayBufferPointIsVisible(db, db->cursor.x,
                                      (uint8_t)round(unroundedY))) {
        safeSetBufferValue(db,
                           displayBufferPointToIndex(
                               db, db->cursor.x, (uint8_t)round(unroundedY)),
                           db->color);
      }

      db->cursor.x++;
      unroundedY += change;
    }
  } else {
    while (db->cursor.x >= toX) {
      if (displayBufferPointIsVisible(db, db->cursor.x,
                                      (uint8_t)round(unroundedY))) {
        safeSetBufferValue(db,
                           displayBufferPointToIndex(
                               db, db->cursor.x, (uint8_t)round(unroundedY)),
                           db->color);
      }

      db->cursor.x--;
      unroundedY -= change;
    }
  }

  displayBufferSetCursor(db, toX, toY);
}

// if you know the shape of your line already, you can save a few clocks by
// using the above `displayBufferDrawFast*` versions
void displayBufferDrawLine(DisplayBufferHandle db, uint8_t toX, uint8_t toY) {
  if (db->cursor.x == toX) {
    displayBufferDrawFastVertLine(db, toY);
  } else if (db->cursor.y == toY) {
    displayBufferDrawFastHorizonLine(db, toX);
  } else {
    displayBufferDrawFastDiagLine(db, toX, toY);
  }
}

void displayBufferDrawBitmap(DisplayBufferHandle db, uint8_t width,
                             uint8_t height, uint16_t *buffer) {
  for (uint8_t row = 0; row < height; row++) {
    for (uint8_t col = 0; col < width; col++) {
      if (displayBufferPointIsVisible(db, db->cursor.x + col,
                                      db->cursor.y + row)) {
        safeSetBufferValue(db,
                           displayBufferPointToIndex(db, db->cursor.x + col,
                                                     db->cursor.y + row),
                           buffer[(row * width) + col]);
      }
    }
  }
}

void displayBufferAddFeedback(DisplayBufferHandle db, bool remoteStateInvalid,
                              bool commandsInvalid, bool isDevMode) {
  if (isDevMode) {
    db->buffer[0] = 0b0000000000011111;
  }

  if (remoteStateInvalid) {
    db->buffer[2] = 0b1111111111100000;
  }

  if (commandsInvalid) {
    db->buffer[4] = 0b1111100000000000;
  }
}