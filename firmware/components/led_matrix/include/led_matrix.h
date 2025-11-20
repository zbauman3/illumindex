#pragma once

#include "driver/dedic_gpio.h"
#include "driver/gptimer.h"
#include "esp_err.h"

// cpu_f     = 240,000,000hz             // CPU CLOCK
// cycles    = 2,280                     // via cpu_hal_get_cycle_count
// oneBit    = cycles / cpu_f            // 0.0095 ms
// miscTime  = 0.001  ms                 // time between alarm & handler
// oneByte   = ((oneBit + miscTime) * 8) // 0.084  ms

// src       = 80,000,000hz              // APB CLOCK
// speed     = 40,000,000hz              // LED_MATRIX_TIMER_RESOLUTION
// timer     = 28                        // LED_MATRIX_TIMER_ALARM
// bit0      = speed / (timer * 2^0)     // 0.0007 ms
// bit1      = speed / (timer * 2^1)     // 0.0014 ms
// bit2      = speed / (timer * 2^2)     // 0.0028 ms
// bit3      = speed / (timer * 2^3)     // 0.0056 ms
// bit4      = speed / (timer * 2^4)     // 0.0112 ms
// bit5      = speed / (timer * 2^5)     // 0.0224 ms
// bit6      = speed / (timer * 2^6)     // 0.0448 ms
// bit7      = speed / (timer * 2^7)     // 0.0896 ms
// rowTimers = bit0 + ... + bit7         // 0.1785 ms
// row       = rowTimers + oneByte       // 0.2625 ms
// screen    = row × 32                  // 8.4    ms
// hz        = screen to hz              // 119.05 Hz
//
// ## Description
//
// We start with a source clock speed of 80,000,000hz from the APB clock. We
// then select a prescale value by passing in the `LED_MATRIX_TIMER_RESOLUTION`
// value. That creates a prescale of `2`, giving us a resolution of `0.025µs`.
// This is the max value achievable because `2` is the minimum prescale value.
//
// Next, we select a "base" starting point for the timer alarm
// `LED_MATRIX_TIMER_ALARM`. After some calculations, I landed on `28`,
// to achieve a reasonable refresh rate explained below.
//
// We output the value for bit0, and set the timer alarm count to
// `LED_MATRIX_TIMER_ALARM`. Then we repeat this process for each subsequent
// bit, except we increase `LED_MATRIX_TIMER_ALARM` by multiplying it by powers
// of 2, doubling the timer alarm value each time. After completing all 8 bits,
// we have completed a row in `0.1785 ms` (not including ISR overhead, more on
// that next).
//
// The ISR takes 2,280 CPU cycles as measured by `cpu_hal_get_cycle_count()`.
// There's some overhead in the low-level logic before/after calling our ISR.
// Estimating overhead is difficult, but a worst-case scenario (and accurate to
// real-life averaged measurements I took) is about `1µs`. Using the CPU
// frequency we can calculate that the ISR and overhead take about `0.0105 ms`.
// Multiplying that by 8 (one for each bit in a row's byte) give us an added
// `0.084 ms` per row.
//
// Adding together the ISR duration and the timer duration, we get each row
// outputing once per `0.2625 ms`. We address the LED matrix two rows at a time,
// so we need to do this 32 times for a full screen refresh. This results in
// a total screen refresh time of `8.4 ms`, or about a `119.05 Hz` refresh rate.

#define LED_MATRIX_TIMER_RESOLUTION 40000000
#define LED_MATRIX_TIMER_ALARM 28
#define LED_MATRIX_BIT_DEPTH 8

#define LED_MATRIX_TIMER_ALARM_0 (LED_MATRIX_TIMER_ALARM)
#define LED_MATRIX_TIMER_ALARM_1 (LED_MATRIX_TIMER_ALARM * 2)
#define LED_MATRIX_TIMER_ALARM_2 (LED_MATRIX_TIMER_ALARM * 4)
#define LED_MATRIX_TIMER_ALARM_3 (LED_MATRIX_TIMER_ALARM * 8)
#define LED_MATRIX_TIMER_ALARM_4 (LED_MATRIX_TIMER_ALARM * 16)
#define LED_MATRIX_TIMER_ALARM_5 (LED_MATRIX_TIMER_ALARM * 32)
#define LED_MATRIX_TIMER_ALARM_6 (LED_MATRIX_TIMER_ALARM * 64)
#define LED_MATRIX_TIMER_ALARM_7 (LED_MATRIX_TIMER_ALARM * 128)

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
} led_matrix_pins_t;

typedef struct {
  led_matrix_pins_t pins;
  uint8_t width;
  uint8_t height;
} led_matrix_config_t;

typedef struct {
  led_matrix_pins_t *pins;
  dedic_gpio_bundle_handle_t gpio_bundle;
  gptimer_handle_t timer;
  uint8_t *buffer;
  uint8_t rowNum;
  uint8_t bitNum;
  uint8_t width;
  uint8_t height;
  uint8_t halfHeight;
  uint16_t splitOffset;
  bool fiveBitAddress;
  uint16_t currentBufferOffset;
} led_matrix_state_t;

typedef led_matrix_state_t *led_matrix_handle_t;

esp_err_t led_matrix_init(led_matrix_handle_t *matrix,
                          led_matrix_config_t *config);
esp_err_t led_matrix_start(led_matrix_handle_t matrix);
esp_err_t led_matrix_stop(led_matrix_handle_t matrix);
esp_err_t led_matrix_end(led_matrix_handle_t matrix);
esp_err_t led_matrix_show(led_matrix_handle_t matrix, uint8_t *buffer_red,
                          uint8_t *buffer_green, uint8_t *buffer_blue);