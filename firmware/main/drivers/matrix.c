#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/dedic_gpio_cpu_ll.h"
#include "hal/gpio_ll.h"
#include <string.h>

#include "drivers/matrix.h"
#include "util/565_color.h"
#include "util/helpers.h"

static const char *TAG = "MATRIX_DRIVER";

#define shiftOutVal(_val)                                                      \
  ({                                                                           \
    dedic_gpio_cpu_ll_write_mask(0b01111111, (_val) | 0b00000000);             \
    dedic_gpio_cpu_ll_write_mask(0b01111111, (_val) | 0b01000000);             \
  })

#define shiftOutRow(_b, _o)                                                    \
  ({                                                                           \
    shiftOutVal(_b[(_o) + 0]);                                                 \
    shiftOutVal(_b[(_o) + 1]);                                                 \
    shiftOutVal(_b[(_o) + 2]);                                                 \
    shiftOutVal(_b[(_o) + 3]);                                                 \
    shiftOutVal(_b[(_o) + 4]);                                                 \
    shiftOutVal(_b[(_o) + 5]);                                                 \
    shiftOutVal(_b[(_o) + 6]);                                                 \
    shiftOutVal(_b[(_o) + 7]);                                                 \
    shiftOutVal(_b[(_o) + 8]);                                                 \
    shiftOutVal(_b[(_o) + 9]);                                                 \
    shiftOutVal(_b[(_o) + 10]);                                                \
    shiftOutVal(_b[(_o) + 11]);                                                \
    shiftOutVal(_b[(_o) + 12]);                                                \
    shiftOutVal(_b[(_o) + 13]);                                                \
    shiftOutVal(_b[(_o) + 14]);                                                \
    shiftOutVal(_b[(_o) + 15]);                                                \
    shiftOutVal(_b[(_o) + 16]);                                                \
    shiftOutVal(_b[(_o) + 17]);                                                \
    shiftOutVal(_b[(_o) + 18]);                                                \
    shiftOutVal(_b[(_o) + 19]);                                                \
    shiftOutVal(_b[(_o) + 20]);                                                \
    shiftOutVal(_b[(_o) + 21]);                                                \
    shiftOutVal(_b[(_o) + 22]);                                                \
    shiftOutVal(_b[(_o) + 23]);                                                \
    shiftOutVal(_b[(_o) + 24]);                                                \
    shiftOutVal(_b[(_o) + 25]);                                                \
    shiftOutVal(_b[(_o) + 26]);                                                \
    shiftOutVal(_b[(_o) + 27]);                                                \
    shiftOutVal(_b[(_o) + 28]);                                                \
    shiftOutVal(_b[(_o) + 29]);                                                \
    shiftOutVal(_b[(_o) + 30]);                                                \
    shiftOutVal(_b[(_o) + 31]);                                                \
    shiftOutVal(_b[(_o) + 32]);                                                \
    shiftOutVal(_b[(_o) + 33]);                                                \
    shiftOutVal(_b[(_o) + 34]);                                                \
    shiftOutVal(_b[(_o) + 35]);                                                \
    shiftOutVal(_b[(_o) + 36]);                                                \
    shiftOutVal(_b[(_o) + 37]);                                                \
    shiftOutVal(_b[(_o) + 38]);                                                \
    shiftOutVal(_b[(_o) + 39]);                                                \
    shiftOutVal(_b[(_o) + 40]);                                                \
    shiftOutVal(_b[(_o) + 41]);                                                \
    shiftOutVal(_b[(_o) + 42]);                                                \
    shiftOutVal(_b[(_o) + 43]);                                                \
    shiftOutVal(_b[(_o) + 44]);                                                \
    shiftOutVal(_b[(_o) + 45]);                                                \
    shiftOutVal(_b[(_o) + 46]);                                                \
    shiftOutVal(_b[(_o) + 47]);                                                \
    shiftOutVal(_b[(_o) + 48]);                                                \
    shiftOutVal(_b[(_o) + 49]);                                                \
    shiftOutVal(_b[(_o) + 50]);                                                \
    shiftOutVal(_b[(_o) + 51]);                                                \
    shiftOutVal(_b[(_o) + 52]);                                                \
    shiftOutVal(_b[(_o) + 53]);                                                \
    shiftOutVal(_b[(_o) + 54]);                                                \
    shiftOutVal(_b[(_o) + 55]);                                                \
    shiftOutVal(_b[(_o) + 56]);                                                \
    shiftOutVal(_b[(_o) + 57]);                                                \
    shiftOutVal(_b[(_o) + 58]);                                                \
    shiftOutVal(_b[(_o) + 59]);                                                \
    shiftOutVal(_b[(_o) + 60]);                                                \
    shiftOutVal(_b[(_o) + 61]);                                                \
    shiftOutVal(_b[(_o) + 62]);                                                \
    shiftOutVal(_b[(_o) + 63]);                                                \
  })

static bool IRAM_ATTR
matrixTimerCallback(gptimer_handle_t timer,
                    const gptimer_alarm_event_data_t *edata, void *userData) {
  static MatrixHandle matrix;
  static uint16_t rowOffset;

  matrix = (MatrixHandle)userData;
  rowOffset = (matrix->rowNum * matrix->width) +
              (matrix->bitNum * matrix->width * matrix->height);

  shiftOutRow(matrix->outputBuffer, rowOffset);

  // blank screen
  gpio_ll_set_level(&GPIO, matrix->pins->oe, 1);

  // set new address
  gpio_ll_set_level(&GPIO, matrix->pins->a0, matrix->rowNum & 0b00001);
  gpio_ll_set_level(&GPIO, matrix->pins->a1, matrix->rowNum & 0b00010);
  gpio_ll_set_level(&GPIO, matrix->pins->a2, matrix->rowNum & 0b00100);
  gpio_ll_set_level(&GPIO, matrix->pins->a3, matrix->rowNum & 0b01000);
  if (matrix->addrBits == 5) {
    gpio_ll_set_level(&GPIO, matrix->pins->a4, matrix->rowNum & 0b10000);
  }

  // latch, then reset all bundle outputs
  dedic_gpio_cpu_ll_write_mask(0b10000000, 0b00000000);
  // delay so the ICN2037 can keep up
  asm volatile("nop");
  dedic_gpio_cpu_ll_write_mask(0b10000000, 0b10000000);

  // show new row
  gpio_ll_set_level(&GPIO, matrix->pins->oe, 0);

  matrix->bitNumInc++;
  if (matrix->bitNumInc == 65) {
    matrix->bitNumInc = 0;
  }

  if (matrix->bitNumInc == 32 || matrix->bitNumInc == 16 ||
      matrix->bitNumInc == 8 || matrix->bitNumInc == 4 ||
      matrix->bitNumInc == 2 || matrix->bitNumInc == 1) {
    matrix->bitNum++;
    if (matrix->bitNum >= MATRIX_BIT_DEPTH) {
      matrix->bitNum = 0;
      matrix->rowNum++;
      if (matrix->rowNum >= (uint8_t)(matrix->height / 2)) {
        matrix->rowNum = 0;
      }
    }
  }

  // reset bundle outputs
  dedic_gpio_cpu_ll_write_mask(0b11111111, 0b00000000);

  return false;
}

// Allocates the resources for a matrix and masses back a handle
esp_err_t matrixInit(MatrixHandle *matrixHandle, MatrixInitConfig *config) {
  // allocate the the state
  MatrixHandle matrix = (MatrixHandle)malloc(sizeof(MatrixState));

  // misc setup
  matrix->rowNum = 0;
  matrix->bitNum = 0;
  matrix->bitNumInc = 0;
  matrix->width = config->width;
  matrix->height = config->height;
  matrix->splitOffset = (matrix->height / 2) * matrix->width;

  if (matrix->height > 32) {
    matrix->addrBits = 5;
  } else {
    matrix->addrBits = 4;
  }

  // allocate and clear the frame buffers
  matrix->processBuffer = (uint8_t *)malloc(sizeof(uint8_t) * matrix->width *
                                            matrix->height * MATRIX_BIT_DEPTH);
  matrix->outputBuffer = (uint8_t *)malloc(sizeof(uint8_t) * matrix->width *
                                           matrix->height * MATRIX_BIT_DEPTH);
  memset(matrix->processBuffer, 0,
         sizeof(uint8_t) * matrix->width * matrix->height * MATRIX_BIT_DEPTH);
  memset(matrix->outputBuffer, 0,
         sizeof(uint8_t) * matrix->width * matrix->height * MATRIX_BIT_DEPTH);

  // allocate and copy pins
  matrix->pins = (MatrixPins *)malloc(sizeof(MatrixPins));
  memcpy(matrix->pins, &config->pins, sizeof(MatrixPins));

  // setup GPIO pins
  gpio_config_t io_conf = {
      .pin_bit_mask = _BV_1ULL(matrix->pins->oe) | _BV_1ULL(matrix->pins->a0) |
                      _BV_1ULL(matrix->pins->a1) | _BV_1ULL(matrix->pins->a2) |
                      _BV_1ULL(matrix->pins->a3),
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pull_down_en = 0,
      .pull_up_en = 0,
  };

  if (matrix->addrBits == 5) {
    io_conf.pin_bit_mask |= _BV_1ULL(matrix->pins->a4);
  }

  ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "Failed to init matrix GPIO");

  // setup the dedicated GPIO
  const int bundle_color_pins[8] = {
      matrix->pins->b2,    matrix->pins->g2,    matrix->pins->r2,
      matrix->pins->b1,    matrix->pins->g1,    matrix->pins->r1,
      matrix->pins->clock, matrix->pins->latch,
  };
  dedic_gpio_bundle_config_t gpio_bundle_color_config = {
      .gpio_array = bundle_color_pins,
      .array_size = sizeof(bundle_color_pins) / sizeof(bundle_color_pins[0]),
      .flags = {.out_en = 1},
  };
  ESP_RETURN_ON_ERROR(
      dedic_gpio_new_bundle(&gpio_bundle_color_config, &matrix->gpioBundle),
      TAG, "Failed to init matrix dedicated GPIO bundle");

  // setup the timer
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = MATRIX_TIMER_RESOLUTION,
      .intr_priority = 3,
  };
  ESP_RETURN_ON_ERROR(gptimer_new_timer(&timer_config, &matrix->timer), TAG,
                      "Failed to create new timer");
  // timer alarm callback
  gptimer_event_callbacks_t cbs = {
      .on_alarm = matrixTimerCallback,
  };
  ESP_RETURN_ON_ERROR(
      gptimer_register_event_callbacks(matrix->timer, &cbs, matrix), TAG,
      "Failed to register timer alarm callback");
  // enable the timer
  ESP_RETURN_ON_ERROR(gptimer_enable(matrix->timer), TAG,
                      "Failed to enable timer");

  // timer alarm
  gptimer_alarm_config_t alarm_config = {
      .alarm_count = MATRIX_TIMER_ALARM,
      .reload_count = 0,
      .flags.auto_reload_on_alarm = true,
  };
  ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(matrix->timer, &alarm_config),
                      TAG, "Failed to create timer alarm");

  // pass back the config
  *matrixHandle = matrix;

  return ESP_OK;
};

// Starts the hardware resources associated with the matrix
esp_err_t matrixStart(MatrixHandle matrix) {
  ESP_RETURN_ON_ERROR(gptimer_start(matrix->timer), TAG,
                      "START: Failed to start timer");
  return ESP_OK;
}

// Stops the hardware resources associated with the matrix
esp_err_t matrixStop(MatrixHandle matrix) {
  ESP_RETURN_ON_ERROR(gptimer_stop(matrix->timer), TAG,
                      "STOP: Failed to stop timer");
  return ESP_OK;
}

// Stops the hardware resources associated with the matrix and frees all
// internal memory
esp_err_t matrixEnd(MatrixHandle matrix) {
  ESP_RETURN_ON_ERROR(matrixStop(matrix), TAG, "END: Failed to stop.");
  gptimer_del_timer(matrix->timer);
  dedic_gpio_del_bundle(matrix->gpioBundle);
  free(matrix->pins);
  free(matrix->processBuffer);
  free(matrix->outputBuffer);
  free(matrix);
  return ESP_OK;
}

// Shows a `buffer` in the `matrix`
esp_err_t matrixShow(MatrixHandle matrix, uint16_t *buffer) {
  uint16_t rowAndBitOffset;
  uint16_t rowOffset;
  uint8_t row;
  uint8_t col;
  uint8_t bitNum;

  for (bitNum = 0; bitNum < MATRIX_BIT_DEPTH; bitNum++) {
    for (row = 0; row < matrix->height; row++) {
      rowOffset = (row * matrix->width);
      rowAndBitOffset = rowOffset + (bitNum * matrix->width * matrix->height);
      for (col = 0; col < matrix->width; col++) {
        SET_565_MATRIX_BYTE(
            // put the value into the variable
            matrix->processBuffer[rowAndBitOffset + col],
            // pull the "top" row from the frame buffer
            buffer[rowOffset + col],
            // pull the "bottom" row ...
            buffer[matrix->splitOffset + rowOffset + col],
            // just this loop's bit
            bitNum);
      }
    }
  }

  memcpy(matrix->outputBuffer, matrix->processBuffer,
         sizeof(uint8_t) * matrix->width * matrix->height * MATRIX_BIT_DEPTH);

  return ESP_OK;
}
