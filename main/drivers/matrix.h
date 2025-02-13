#pragma once

#include "driver/dedic_gpio.h"
#include "driver/gptimer.h"
#include "esp_err.h"

#define MATRIX_RAW_BUFFER_SIZE (sizeof(uint16_t) * 64 * 32)
#define MATRIX_PROC_BUFFER_SIZE (sizeof(uint8_t) * 64 * 16)

#define MATRIX_REFRESH_RATE 240
// 1MHz = 1µs resolution
#define MATRIX_TIMER_RESOLUTION 1000000
// interrupt every N 1µs
#define MATRIX_TIMER_ALARM_COUNT                                               \
  ((uint64_t)(MATRIX_TIMER_RESOLUTION / 16 / MATRIX_REFRESH_RATE))

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

typedef struct {
  MatrixPins pins;
} MatrixInitConfig;

typedef struct {
  MatrixPins *pins;
  dedic_gpio_bundle_handle_t gpioBundle;
  gptimer_handle_t timer;
  uint16_t *rawFrameBuffer;
  uint8_t *frameBuffer;
  uint8_t rowNum;
  bool completedBuffer;
} MatrixState;

typedef MatrixState *MatrixHandle;

esp_err_t matrixInit(MatrixHandle *matrix, MatrixInitConfig *config);

void showFrame(MatrixHandle matrix, uint16_t *buffer);