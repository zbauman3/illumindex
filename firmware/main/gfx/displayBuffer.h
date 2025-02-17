#pragma once

#include "esp_err.h"
#include <inttypes.h>

#include "drivers/matrix.h"

#define DISPLAY_BUFFER_SIZE MATRIX_RAW_BUFFER_SIZE

typedef struct {
  // DISPLAY_BUFFER_SIZE
  uint16_t *buffer;
} DisplayBuffer;

typedef DisplayBuffer *DisplayBufferHandle;

esp_err_t displayBufferInit(DisplayBufferHandle *displayBufferHandle);
void displayBufferEnd(DisplayBufferHandle displayBufferHandle);

void displayBufferTest(DisplayBufferHandle displayBuffer);