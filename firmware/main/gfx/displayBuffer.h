#pragma once

#include "esp_err.h"
#include <inttypes.h>

#include "drivers/matrix.h"
#include "gfx/fonts.h"

#define DISPLAY_BUFFER_SIZE MATRIX_RAW_BUFFER_SIZE

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

esp_err_t displayBufferInit(DisplayBufferHandle *displayBufferHandle);
void displayBufferEnd(DisplayBufferHandle displayBufferHandle);
void displayBufferClear(DisplayBufferHandle displayBufferHandle);
void displayBufferDrawString(DisplayBufferHandle displayBuffer,
                             char *stringBuff);
void displayBufferDrawFastVertLine(DisplayBufferHandle displayBuffer,
                                   uint8_t to);
void displayBufferDrawFastHorizonLine(DisplayBufferHandle displayBuffer,
                                      uint8_t to);
void displayBufferDrawFastDiagLine(DisplayBufferHandle displayBuffer,
                                   uint8_t toX, uint8_t toY);
void displayBufferDrawLine(DisplayBufferHandle displayBuffer, uint8_t toX,
                           uint8_t toY);
void displayBufferDrawBitmap(DisplayBufferHandle displayBuffer, uint8_t width,
                             uint8_t height, uint16_t *buffer);