#pragma once

#include "esp_err.h"
#include <inttypes.h>
#include <stdbool.h>

#include "gfx/fonts.h"

// validates that setting an index in the buffer is not an overflow
#define safeSetBufferValue(db, index, value)                                   \
  ({                                                                           \
    if ((index) < db->width * db->height) {                                    \
      db->buffer[(index)] = (value);                                           \
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

#define displayBufferSetColor(db, scolor) ({ db->color = (scolor); })

#define displayBufferLineFeed(db)                                              \
  ({                                                                           \
    db->cursor.x = 0;                                                          \
    db->cursor.y += db->font->height;                                          \
  })

typedef struct {
  uint16_t *buffer;
  uint8_t width;
  uint8_t height;
  FontHandle font;
  uint16_t color;
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
                             uint8_t height, uint16_t *buffer);