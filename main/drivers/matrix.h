#pragma once

#include "driver/dedic_gpio.h"
#include "driver/gptimer.h"
#include "esp_err.h"

#define MATRIX_RAW_BUFFER_SIZE (sizeof(uint16_t) * 64 * 32)

// 10MHz = .1µs resolution
#define MATRIX_TIMER_RESOLUTION 10000000
#define MATRIX_TIMER_ALARM_COUNT_5                                             \
  ((uint64_t)(MATRIX_TIMER_RESOLUTION / 16 / 1200))
#define MATRIX_TIMER_ALARM_COUNT_4 ((uint64_t)(MATRIX_TIMER_ALARM_COUNT_5 / 2))
#define MATRIX_TIMER_ALARM_COUNT_3 ((uint64_t)(MATRIX_TIMER_ALARM_COUNT_4 / 2))
#define MATRIX_TIMER_ALARM_COUNT_2 ((uint64_t)(MATRIX_TIMER_ALARM_COUNT_3 / 2))
#define MATRIX_TIMER_ALARM_COUNT_1 ((uint64_t)(MATRIX_TIMER_ALARM_COUNT_2 / 2))
#define MATRIX_TIMER_ALARM_COUNT_0 ((uint64_t)(MATRIX_TIMER_ALARM_COUNT_1 / 2))

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
  uint8_t rowNum;
  uint8_t bitNum;
} MatrixState;

typedef MatrixState *MatrixHandle;

esp_err_t matrixInit(MatrixHandle *matrix, MatrixInitConfig *config);

void showFrame(MatrixHandle matrix, uint16_t *buffer);