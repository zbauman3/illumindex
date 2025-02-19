#pragma once

#include "esp_err.h"
#include <inttypes.h>

#include "drivers/matrix.h"
#include "gfx/fonts.h"

#define DISPLAY_BUFFER_SIZE MATRIX_RAW_BUFFER_SIZE
#define DISPLAY_BUFFER_ASCII_MIN 32
#define DISPLAY_BUFFER_ASCII_MAX 126

typedef struct {
  uint16_t *buffer;
  uint8_t width;
  uint8_t height;
  FontHandle font;
} DisplayBuffer;

typedef DisplayBuffer *DisplayBufferHandle;

esp_err_t displayBufferInit(DisplayBufferHandle *displayBufferHandle);
void displayBufferEnd(DisplayBufferHandle displayBufferHandle);
void displayBufferClear(DisplayBufferHandle displayBufferHandle);

void drawString(DisplayBufferHandle displayBuffer, char *stringBuff,
                uint16_t startOffset, uint16_t color);