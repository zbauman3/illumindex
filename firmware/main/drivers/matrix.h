#pragma once

#include "driver/dedic_gpio.h"
#include "driver/gptimer.h"
#include "esp_err.h"

// src    = 80,000,000hz          // APB CLOCK
// speed  = 40,000,000hz          // MATRIX_TIMER_RESOLUTION
// timer  = 40                    // MATRIX_TIMER_ALARM
// bit0   = speed / (timer * 2^0) // 0.001 ms
// bit1   = speed / (timer * 2^1) // 0.002 ms
// bit2   = speed / (timer * 2^2) // 0.004 ms
// bit3   = speed / (timer * 2^3) // 0.008 ms
// bit4   = speed / (timer * 2^4) // 0.016 ms
// bit5   = speed / (timer * 2^5) // 0.032 ms
// bit6   = speed / (timer * 2^6) // 0.064 ms
// bit7   = speed / (timer * 2^7) // 0.128 ms
// row    = bit0 + ... + bit7     // 0.255 ms
// screen = row × 32              // 8.16  ms
// hz     = screen to hz          // 122.549 Hz
//
// ## Description
//
// We start with a source clock speed of 80,000,000hz from the APB clock. We
// then select a prescale value by passing in the `MATRIX_TIMER_RESOLUTION`
// value. That creates a prescale of `2`, giving us a resolution of 0.025µs.
// This is the max value achievable because `2` is the minimum prescale value.
//
// Next, we select a "base" starting point for the timer alarm
// `MATRIX_TIMER_ALARM`. After some calculations, I landed on `40`, explained
// below.
//
// We output the value for bit0, and set the timer alarm count to
// `MATRIX_TIMER_ALARM`. Then we repeat this process for each subsequent bit,
// except we increase `MATRIX_TIMER_ALARM` by multiplying it by powers of 2,
// doubling the timer alarm value each time.
//
// After completing all 8 bits, we have completed a row in 0.255 ms (not
// including ISR overhead, more on that next). The data is output two rows at a
// time, meaning we need to do this 32 times for the entire screen refresh,
// giving us 8.16 ms for the full refresh cycle. Or 122.549 Hz.
//
// ## ISR overhead
//
// This calculation above does not include the time spent between stopping the
// timer and restarting it. This is non-trivial since this is where we shift out
// the entire row, and update the state of the matrix. This happens 8 times
// during the refresh cycle, once for each bit. I'll eventually calculate and
// include this, but for now it seems small enough not to push the display into
// flicker territory.
//
// I'm estimating the worst case for the overhead of each ISR is equal to the
// base timer alarm count, `MATRIX_TIMER_ALARM`. So if we add
// `MATRIX_TIMER_ALARM * 8` to each row, we get an additional `0.008 ms` of
// overhead for each row. This gives us a "worst case" refresh rate of 118.82Hz,
// although I think that is likely slower than reality.

#define MATRIX_TIMER_RESOLUTION 40000000
#define MATRIX_TIMER_ALARM 40
#define MATRIX_BIT_DEPTH 8

#define MATRIX_TIMER_ALARM_0 (MATRIX_TIMER_ALARM)
#define MATRIX_TIMER_ALARM_1 (MATRIX_TIMER_ALARM * 2)
#define MATRIX_TIMER_ALARM_2 (MATRIX_TIMER_ALARM * 4)
#define MATRIX_TIMER_ALARM_3 (MATRIX_TIMER_ALARM * 8)
#define MATRIX_TIMER_ALARM_4 (MATRIX_TIMER_ALARM * 16)
#define MATRIX_TIMER_ALARM_5 (MATRIX_TIMER_ALARM * 32)
#define MATRIX_TIMER_ALARM_6 (MATRIX_TIMER_ALARM * 64)
#define MATRIX_TIMER_ALARM_7 (MATRIX_TIMER_ALARM * 128)

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
esp_err_t matrixShow(MatrixHandle matrix, uint8_t *bufferRed,
                     uint8_t *bufferGreen, uint8_t *bufferBlue);