#pragma once

#include "esp_err.h"

typedef struct {
  uint8_t r1;
  uint8_t r2;
  uint8_t g1;
  uint8_t g2;
  uint8_t b1;
  uint8_t b2;

  uint8_t a0;
  uint8_t a1;
  uint8_t a2;
  uint8_t a3;

  uint8_t latch;
  uint8_t clock;
  uint8_t oe;
} MatrixPins;

esp_err_t matrixInit(MatrixPins pins);

/**
 * @param uint16_t* `bufferPtr` is a pointer to `uint16_t buffer[2048]`. The
 * buffer should be structured with 565 color codes, with each row being 64
 * pixes long. Example:
 *
 * ```
 * uint16_t buffer[2048] = {
 *   0xF800, 0x07E0, 0x001F, // Row 1 pixels 0, 1, 2: Red, Green, Blue
 *   // ...
 *   0xF800, 0x07E0, 0x001F  // Row 32 pixes 61, 62, 63: Red, Green, Blue
 * };
 * ```
 *
 * When this function returns, it is safe to manipulate `bufferPtr` and the
 * data it points to.
 */
void showFrame(uint16_t *bufferPtr);