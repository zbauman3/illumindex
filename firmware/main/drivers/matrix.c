#include "freertos/FreeRTOS.h"

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
static portMUX_TYPE matrixSpinlock = portMUX_INITIALIZER_UNLOCKED;

static bool IRAM_ATTR
matrixTimerCallback(gptimer_handle_t timer,
                    const gptimer_alarm_event_data_t *edata, void *userData) {
  static uint8_t col;
  static uint16_t rowOffset;
  static uint8_t pixelByte;

  MatrixHandle matrix = (MatrixHandle)userData;
  rowOffset = matrix->rowNum * 64;

  for (col = 0; col < 64; col++) {
    SET_565_MATRIX_BYTE(
        // put the value into the variable
        pixelByte,
        // pull the "top" row from the frame buffer
        matrix->rawFrameBuffer[rowOffset + col],
        // pull the "bottom" row ...
        matrix->rawFrameBuffer[1024 + rowOffset + col],
        // just this loop's bit
        matrix->bitNum);

    // shift in each column. Clock is on the rising edge
    dedic_gpio_cpu_ll_write_mask(0b01111111, pixelByte | 0b00000000);
    // delay so the ICN2037 can keep up
    asm volatile("nop");
    dedic_gpio_cpu_ll_write_mask(0b01111111, pixelByte | 0b01000000);
  }

  // blank screen
  gpio_ll_set_level(&GPIO, matrix->pins->oe, 1);

  // set new address
  gpio_ll_set_level(&GPIO, matrix->pins->a0, matrix->rowNum & 0b0001 ? 1 : 0);
  gpio_ll_set_level(&GPIO, matrix->pins->a1, matrix->rowNum & 0b0010 ? 1 : 0);
  gpio_ll_set_level(&GPIO, matrix->pins->a2, matrix->rowNum & 0b0100 ? 1 : 0);
  gpio_ll_set_level(&GPIO, matrix->pins->a3, matrix->rowNum & 0b1000 ? 1 : 0);

  // latch, then reset all bundle outputs
  dedic_gpio_cpu_ll_write_mask(0b10000000, 0b00000000);
  // delay so the ICN2037 can keep up
  asm volatile("nop");
  dedic_gpio_cpu_ll_write_mask(0b10000000, 0b10000000);

  // show new row
  gpio_ll_set_level(&GPIO, matrix->pins->oe, 0);

  matrix->bitNum++;
  if (matrix->bitNum >= 6) {
    matrix->bitNum = 0;
    matrix->rowNum++;
    if (matrix->rowNum >= 16) {
      matrix->rowNum = 0;
    }
  }

  // reset bundle outputs
  dedic_gpio_cpu_ll_write_mask(0b11111111, 0b00000000);

  return false;
}

esp_err_t matrixInit(MatrixHandle *matrixHandle, MatrixInitConfig *config) {
  // allocate the the state
  MatrixHandle matrix = (MatrixHandle)malloc(sizeof(MatrixState));

  // misc setup
  matrix->rowNum = 0;
  matrix->bitNum = 0;

  // allocate and clear the frame buffers
  matrix->rawFrameBuffer = (uint16_t *)malloc(MATRIX_RAW_BUFFER_SIZE);
  memset(matrix->rawFrameBuffer, 0, MATRIX_RAW_BUFFER_SIZE);

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

  // timer alarms
  gptimer_alarm_config_t alarm_config;
  alarm_config.reload_count = 0;
  for (uint8_t i = 0; i < 6; i++) {
    switch (i) {
    case 0:
      alarm_config.alarm_count = MATRIX_TIMER_ALARM_COUNT_0;
      break;
    case 1:
      alarm_config.alarm_count = MATRIX_TIMER_ALARM_COUNT_1;
      break;
    case 2:
      alarm_config.alarm_count = MATRIX_TIMER_ALARM_COUNT_2;
      break;
    case 3:
      alarm_config.alarm_count = MATRIX_TIMER_ALARM_COUNT_3;
      break;
    case 4:
      alarm_config.alarm_count = MATRIX_TIMER_ALARM_COUNT_4;
      break;
    case 5:
      alarm_config.alarm_count = MATRIX_TIMER_ALARM_COUNT_5;
      break;
    }
    if (i == 5) {
      alarm_config.flags.auto_reload_on_alarm = true;
    } else {
      alarm_config.flags.auto_reload_on_alarm = false;
    }

    ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(matrix->timer, &alarm_config),
                        TAG, "Failed to create timer alarm");
  }

  // pass back the config
  *matrixHandle = matrix;

  return ESP_OK;
};

esp_err_t matrixStart(MatrixHandle matrix) {
  ESP_RETURN_ON_ERROR(gptimer_start(matrix->timer), TAG,
                      "START: Failed to start timer");
  return ESP_OK;
}

esp_err_t matrixStop(MatrixHandle matrix) {
  ESP_RETURN_ON_ERROR(gptimer_stop(matrix->timer), TAG,
                      "STOP: Failed to stop timer");
  return ESP_OK;
}

esp_err_t matrixEnd(MatrixHandle matrix) {
  ESP_RETURN_ON_ERROR(matrixStop(matrix), TAG, "END: Failed to stop.");
  gptimer_del_timer(matrix->timer);
  dedic_gpio_del_bundle(matrix->gpioBundle);
  free(matrix->pins);
  free(matrix->rawFrameBuffer);
  free(matrix);
  return ESP_OK;
}

esp_err_t matrixShow(MatrixHandle matrix, uint16_t *buffer) {
  esp_err_t ret = ESP_OK;
  taskENTER_CRITICAL(&matrixSpinlock);

  ESP_GOTO_ON_ERROR(gptimer_stop(matrix->timer), matrixShow_cleanup, TAG,
                    "RESET: Failed to stop timer");
  ESP_GOTO_ON_ERROR(gptimer_set_raw_count(matrix->timer, 0), matrixShow_cleanup,
                    TAG, "RESET: Failed to reset timer count");
  matrix->rowNum = 0;
  matrix->bitNum = 0;
  memcpy(matrix->rawFrameBuffer, buffer, MATRIX_RAW_BUFFER_SIZE);
  ESP_GOTO_ON_ERROR(gptimer_start(matrix->timer), matrixShow_cleanup, TAG,
                    "RESET: Failed to start timer");

matrixShow_cleanup:
  taskEXIT_CRITICAL(&matrixSpinlock);
  return ret;
}
