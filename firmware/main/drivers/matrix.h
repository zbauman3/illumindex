#pragma once

#include "driver/dedic_gpio.h"
#include "driver/gptimer.h"
#include "esp_err.h"

// src        = 80,000,000hz  |
// speed      = 40,000,000hz  |
// prescale   = src / speed   | 2
// timer      = 272           |
// period     = speed / timer | 0.0068ms
// row        = period * 32   | 0.2176 ms
// hz         = row * 32      | 143.6121323529 Hz

// 40MHz = .025µs resolution. This is max.
#define MATRIX_TIMER_RESOLUTION 40000000
// 0.0068ms period. 143Hz
#define MATRIX_TIMER_ALARM 272
#define MATRIX_BIT_DEPTH 6

// if using a 5-bit address matrix, a4 MUST be set
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
  // if using a 5-bit address matrix, a4 MUST be set
  uint8_t a4;

  uint8_t latch;
  uint8_t clock;
  uint8_t oe;
} MatrixPins;

typedef struct {
  MatrixPins pins;
  uint8_t width;
  uint8_t height;
} MatrixInitConfig;

typedef struct {
  MatrixPins *pins;
  dedic_gpio_bundle_handle_t gpioBundle;
  gptimer_handle_t timer;
  uint8_t *displayBuffer;
  uint8_t rowNum;
  uint8_t bitNum;
  uint8_t bitNumInc;
  uint8_t width;
  uint8_t height;
  uint8_t halfHeight;
  uint16_t splitOffset;
  bool fiveBitAddress;
  uint16_t currentBufferOffset;
} MatrixState;

typedef MatrixState *MatrixHandle;

esp_err_t matrixInit(MatrixHandle *matrix, MatrixInitConfig *config);
esp_err_t matrixStart(MatrixHandle matrix);
esp_err_t matrixStop(MatrixHandle matrix);
esp_err_t matrixEnd(MatrixHandle matrix);
esp_err_t matrixShow(MatrixHandle matrix, uint16_t *buffer);