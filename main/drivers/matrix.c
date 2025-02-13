#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
  static uint16_t sendFrameBufferOffset;

  MatrixHandle matrix = (MatrixHandle)userData;
  sendFrameBufferOffset = matrix->rowNum * 64;

  // no need to continue to process this frame if we've already processed
  // every value. We can skip it after an initial pass
  if (!matrix->completedBuffer) {
    for (col = 0; col < 64; col++) {
      SET_565_MATRIX_BYTE(
          // put the value into the variable
          matrix->frameBuffer[sendFrameBufferOffset + col],
          // pull the "top" row from the frame buffer
          matrix->rawFrameBuffer[sendFrameBufferOffset + col],
          // pull the "bottom" row ...
          matrix->rawFrameBuffer[1024 + sendFrameBufferOffset + col],
          // for now, just the first bit
          0);
    }
  }

  for (col = 0; col < 64; col++) {
    // shift in each column. Clock is on the rising edge
    dedic_gpio_cpu_ll_write_mask(
        0b01111111,
        matrix->frameBuffer[sendFrameBufferOffset + col] | 0b00000000);
    dedic_gpio_cpu_ll_write_mask(
        0b01111111,
        matrix->frameBuffer[sendFrameBufferOffset + col] | 0b01000000);
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
  dedic_gpio_cpu_ll_write_mask(0b10000000, 0b10000000);
  dedic_gpio_cpu_ll_write_mask(0b11111111, 0b00000000);

  // show new row
  gpio_ll_set_level(&GPIO, matrix->pins->oe, 0);

  matrix->rowNum++;
  if (matrix->rowNum >= 16) {
    matrix->rowNum = 0;
    // we are now at the end, we can signal that we've completed processing
    matrix->completedBuffer = true;
  }

  return false;
}

esp_err_t matrixInit(MatrixHandle *matrixHandle, MatrixInitConfig *config) {
  // allocate the the state
  MatrixHandle matrix = (MatrixHandle)malloc(sizeof(MatrixState));

  // misc setup
  matrix->rowNum = 0;
  matrix->completedBuffer = false;

  // allocate and clear the frame buffers
  matrix->rawFrameBuffer = (uint16_t *)malloc(MATRIX_RAW_BUFFER_SIZE);
  memset(matrix->rawFrameBuffer, 0, MATRIX_RAW_BUFFER_SIZE);
  matrix->frameBuffer = (uint8_t *)malloc(MATRIX_PROC_BUFFER_SIZE);
  memset(matrix->frameBuffer, 0, MATRIX_PROC_BUFFER_SIZE);

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
  ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "Unable to init matrix GPIO");

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
      TAG, "Unable to init matrix dedicated GPIO bundle");

  // setup the timer
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = MATRIX_TIMER_RESOLUTION,
  };
  ESP_RETURN_ON_ERROR(gptimer_new_timer(&timer_config, &matrix->timer), TAG,
                      "Unable to create new timer");
  // timer alarm callback
  gptimer_event_callbacks_t cbs = {
      .on_alarm = matrixTimerCallback,
  };
  ESP_RETURN_ON_ERROR(
      gptimer_register_event_callbacks(matrix->timer, &cbs, matrix), TAG,
      "Unable to register timer alarm callback");
  // enable the timer
  ESP_RETURN_ON_ERROR(gptimer_enable(matrix->timer), TAG,
                      "Unable to enable timer");
  // timer alrm
  gptimer_alarm_config_t alarm_config1 = {
      .alarm_count = MATRIX_TIMER_ALARM_COUNT,
      .reload_count = 0,
      .flags = {.auto_reload_on_alarm = true},
  };
  ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(matrix->timer, &alarm_config1),
                      TAG, "Unable to create timer alarm");
  // start!
  ESP_RETURN_ON_ERROR(gptimer_start(matrix->timer), TAG,
                      "Unable to start timer");

  // pass back the config
  *matrixHandle = matrix;

  return ESP_OK;
};

void showFrame(MatrixHandle matrix, uint16_t *buffer) {
  taskENTER_CRITICAL(&matrixSpinlock);
  memcpy(matrix->rawFrameBuffer, buffer, MATRIX_RAW_BUFFER_SIZE);
  matrix->rowNum = 0;
  matrix->completedBuffer = false;
  taskEXIT_CRITICAL(&matrixSpinlock);
}