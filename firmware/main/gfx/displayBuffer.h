#pragma once

#include "drivers/matrix.h"
#include <inttypes.h>

#define DISPLAY_BUFFER_SIZE MATRIX_RAW_BUFFER_SIZE

typedef struct {
  // DISPLAY_BUFFER_SIZE
  uint16_t *buffer;
} DisplayBuffer;

typedef DisplayBuffer *DisplayBufferHandle;

void displayBufferInit(DisplayBufferHandle *displayBufferHandle);
void displayBufferEnd(DisplayBufferHandle displayBufferHandle);

void displayBufferTest(DisplayBufferHandle displayBuffer);