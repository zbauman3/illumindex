#pragma once

#include "esp_err.h"

typedef struct MatrixPins {
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

void showFrame(uint16_t *bufferPtr);