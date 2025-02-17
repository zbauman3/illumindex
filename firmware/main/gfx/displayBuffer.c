#include <memory.h>

#include "gfx/displayBuffer.h"
#include "util/565_color.h"

esp_err_t displayBufferInit(DisplayBufferHandle *displayBufferHandle) {
  DisplayBuffer *displayBuffer = (DisplayBuffer *)malloc(sizeof(DisplayBuffer));

  displayBuffer->buffer = (uint16_t *)malloc(DISPLAY_BUFFER_SIZE);

  *displayBufferHandle = displayBuffer;

  return ESP_OK;
}

void displayBufferEnd(DisplayBufferHandle displayBuffer) {
  free(displayBuffer->buffer);
  free(displayBuffer);
}

void displayBufferTest(DisplayBufferHandle displayBuffer) {

  static uint16_t topRow[64] = {
      HEX_TO_565(0xFF0000),
      HEX_TO_565(0x00FF00),
      HEX_TO_565(0x0000FF),
      HEX_TO_565(0xFFFF00),
      HEX_TO_565(0xFF00FF),
      HEX_TO_565(0x00FFFF),
      HEX_TO_565(0xFFFFFF),
      HEX_TO_565(0x101010),
      HEX_TO_565(0xFF0800),
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      RGB_TO_565(255, 0, 0),
      RGB_TO_565(0, 255, 0),
      RGB_TO_565(0, 0, 255),
      RGB_TO_565(255, 255, 0),
      RGB_TO_565(255, 0, 255),
      RGB_TO_565(0, 255, 255),
      RGB_TO_565(255, 255, 255),
      RGB_TO_565(8, 8, 8),
      RGB_TO_565(0, 0, 0),
  };

  // generate an image
  for (uint8_t row = 0; row < 16; row++) {
    for (uint8_t col = 0; col < 64; col++) {
      displayBuffer->buffer[(row * 64) + col] = topRow[col];
      displayBuffer->buffer[1024 + (row * 64) + col] = topRow[col];
    }
  }
}