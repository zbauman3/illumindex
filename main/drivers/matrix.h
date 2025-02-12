#pragma once

#include "driver/dedic_gpio.h"
#include "driver/gptimer.h"
#include "esp_err.h"

#define MATRIX_RAW_BUFFER_SIZE (sizeof(uint16_t) * 64 * 32)

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

typedef struct MatrixInitConfig {
  MatrixPins pins;
} MatrixInitConfig;

typedef struct MatrixState {
  MatrixPins *pins;
  dedic_gpio_bundle_handle_t gpioBundle;
  uint16_t *frameBuffer;
  uint8_t nextRow;
  gptimer_handle_t timer;
} MatrixState;

typedef MatrixState *MatrixHandle;

esp_err_t matrixInit(MatrixHandle *matrix, MatrixInitConfig *config);

void showFrame(MatrixHandle matrix, uint16_t *buffer);