#pragma once

#include "esp_err.h"
#include <inttypes.h>
#include <stdbool.h>

#include "gfx/fonts.h"

// validates that setting an index in the buffer is not an overflow
#define safeSetBufferValue(db, index, red, green, blue)                        \
  ({                                                                           \
    if ((index) < db->length) {                                                \
      db->bufferRed[(index)] = (red);                                          \
      db->bufferGreen[(index)] = (green);                                      \
      db->bufferBlue[(index)] = (blue);                                        \
    }                                                                          \
  })

// wraps, but does not check that the new row (y) is within range
#define moveCursorOneCharWrap(db)                                              \
  ({                                                                           \
    if (db->cursor.x + (db->font->width * 2) + db->font->spacing <=            \
        db->width) {                                                           \
      db->cursor.x += db->font->width + db->font->spacing;                     \
    } else {                                                                   \
      displayBufferLineFeed(db);                                               \
    }                                                                          \
  })

#define displayBufferCursorToIndex(db)                                         \
  displayBufferPointToIndex(db, db->cursor.x, db->cursor.y)

#define displayBufferPointToIndex(db, x, y)                                    \
  ((uint16_t)(((y) * db->width) + (x)))

#define displayBufferCursorIsVisible(db)                                       \
  displayBufferPointIsVisible(db, db->cursor.x, db->cursor.y)

#define displayBufferPointIsVisible(db, x, y)                                  \
  ((bool)((x) < db->width && (y) < db->height))

#define displayBufferSetCursor(db, sx, sy)                                     \
  ({                                                                           \
    db->cursor.x = (sx);                                                       \
    db->cursor.y = (sy);                                                       \
  })

#define displayBufferSetColor(db, red, green, blue)                            \
  ({                                                                           \
    db->colorRed = (red);                                                      \
    db->colorGreen = (green);                                                  \
    db->colorBlue = (blue);                                                    \
  })

#define displayBufferLineFeed(db)                                              \
  ({                                                                           \
    db->cursor.x = 0;                                                          \
    db->cursor.y += db->font->height;                                          \
  })

typedef struct {
  uint8_t *bufferRed;
  uint8_t *bufferGreen;
  uint8_t *bufferBlue;
  uint8_t width;
  uint8_t height;
  uint16_t length;
  FontHandle font;
  uint8_t colorRed;
  uint8_t colorGreen;
  uint8_t colorBlue;
  struct {
    uint8_t x;
    uint8_t y;
  } cursor;
} DisplayBuffer;

typedef DisplayBuffer *DisplayBufferHandle;

esp_err_t displayBufferInit(DisplayBufferHandle *dbHandle, uint8_t width,
                            uint8_t height);
void displayBufferEnd(DisplayBufferHandle dbHandle);
void displayBufferClear(DisplayBufferHandle dbHandle);
void displayBufferDrawString(DisplayBufferHandle db, char *string);
void displayBufferDrawFastVertLine(DisplayBufferHandle db, uint8_t to);
void displayBufferDrawFastHorizonLine(DisplayBufferHandle db, uint8_t to);
void displayBufferDrawFastDiagLine(DisplayBufferHandle db, uint8_t toX,
                                   uint8_t toY);
void displayBufferDrawLine(DisplayBufferHandle db, uint8_t toX, uint8_t toY);
void displayBufferDrawBitmap(DisplayBufferHandle db, uint8_t width,
                             uint8_t height, uint8_t *bufferRed,
                             uint8_t *bufferGreen, uint8_t *bufferBlue);
void displayBufferAddFeedback(DisplayBufferHandle db, bool remoteStateInvalid,
                              bool commandsInvalid, bool isDevMode);
