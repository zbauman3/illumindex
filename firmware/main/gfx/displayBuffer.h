#pragma once

#include "esp_err.h"
#include <inttypes.h>

#include "drivers/matrix.h"
#include "gfx/fonts.h"

#define DISPLAY_BUFFER_SIZE MATRIX_RAW_BUFFER_SIZE

#define displayBufferCursorToIndex(displayBuffer)                              \
  ((uint16_t)((displayBuffer->cursor.y * displayBuffer->width) +               \
              displayBuffer->cursor.x))

#define displayBufferCursorIsVisible(displayBuffer)                            \
  ((bool)(displayBuffer->cursor.x < displayBuffer->width &&                    \
          displayBuffer->cursor.y < displayBuffer->height))

typedef struct {
  uint16_t *buffer;
  uint8_t width;
  uint8_t height;
  FontHandle font;
  struct {
    uint8_t x;
    uint8_t y;
  } cursor;
} DisplayBuffer;

typedef DisplayBuffer *DisplayBufferHandle;

esp_err_t displayBufferInit(DisplayBufferHandle *displayBufferHandle);
void displayBufferEnd(DisplayBufferHandle displayBufferHandle);
void displayBufferClear(DisplayBufferHandle displayBufferHandle);
void drawString(DisplayBufferHandle displayBuffer, char *stringBuff);