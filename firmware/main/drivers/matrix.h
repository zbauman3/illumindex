#pragma once

#include "driver/dedic_gpio.h"
#include "driver/gptimer.h"
#include "esp_err.h"

// 10MHz = .1µs resolution
#define MATRIX_TIMER_RESOLUTION 10000000
#define MATRIX_TIMER_ALARM_COUNT 6
#define MATRIX_TIMER_ALARMS(_var, _height)                                     \
  uint64_t _var[6];                                                            \
  ({                                                                           \
    _var[5] = ((uint64_t)(MATRIX_TIMER_RESOLUTION / ((_height) / 2) / 1200));  \
    _var[4] = ((uint64_t)(_var[5] / 2));                                       \
    _var[3] = ((uint64_t)(_var[4] / 2));                                       \
    _var[2] = ((uint64_t)(_var[3] / 2));                                       \
    _var[1] = ((uint64_t)(_var[2] / 2));                                       \
    _var[0] = ((uint64_t)(_var[1] / 2));                                       \
  })

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
  uint8_t width;
  uint8_t height;
} MatrixState;

typedef MatrixState *MatrixHandle;

/** Allocates the resources for a matrix and masses back a handle */
esp_err_t matrixInit(MatrixHandle *matrix, MatrixInitConfig *config,
                     uint8_t width, uint8_t height);
/** Starts the hardware resources associated with the matrix */
esp_err_t matrixStart(MatrixHandle matrix);
/** Stops the hardware resources associated with the matrix */
esp_err_t matrixStop(MatrixHandle matrix);
/** Stops the hardware resources associated with the matrix and frees all
 * internal memory */
esp_err_t matrixEnd(MatrixHandle matrix);
/**
 * Shows a `buffer` in the `matrix`.
 *
 * @param matrix a matrix handle to a matrix that has already been initiated
 * @param buffer a pointer to an array of 565 colors that are
 * `MATRIX_RAW_BUFFER_SIZE` size
 */
esp_err_t matrixShow(MatrixHandle matrix, uint16_t *buffer);