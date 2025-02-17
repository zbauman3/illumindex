#include "gfx/displayBuffer.h"
#include <memory.h>

void displayBufferInit(DisplayBufferHandle *displayBufferHandle) {
  DisplayBuffer *displayBuffer = (DisplayBuffer *)malloc(sizeof(DisplayBuffer));

  displayBuffer->buffer = (uint16_t *)malloc(DISPLAY_BUFFER_SIZE);

  *displayBufferHandle = displayBuffer;
}

void displayBufferEnd(DisplayBufferHandle displayBuffer) {
  free(displayBuffer->buffer);
  free(displayBuffer);
}

void displayBufferTest(DisplayBufferHandle displayBuffer) {
  static uint16_t topRow[64] = {
      0xf800, 0x07e0, 0x001f, 0x07ff,
      0xffe0, 0xf81f, 0xffff, 0b0000100000100001,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, // 21 red
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000,
      0x0000, // 22 green
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000,
      0x0000 // 21 blue
  };
  // generate an image
  for (uint8_t row = 0; row < 16; row++) {
    for (uint8_t col = 0; col < 64; col++) {
      displayBuffer->buffer[(row * 64) + col] = topRow[col];
      displayBuffer->buffer[1024 + (row * 64) + col] = topRow[col];
    }
  }
}