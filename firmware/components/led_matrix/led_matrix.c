#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "hal/dedic_gpio_cpu_ll.h"
#include "hal/gpio_ll.h"
#include <string.h>

#include "color_utils.h"
#include "helper_utils.h"

#include "led_matrix.h"

static const char *TAG = "LED_MATRIX";

#define shift_out_val(_val)                                                    \
  ({                                                                           \
    dedic_gpio_cpu_ll_write_mask(0b01111111, (_val));                          \
    asm volatile("nop"); /* delay so the ICN2037 can keep up */                \
    dedic_gpio_cpu_ll_write_mask(0b01111111, (_val) | 0b01000000);             \
  })

#define shift_out_row(_b, _o)                                                  \
  ({                                                                           \
    shift_out_val(_b[(_o) + 0]);                                               \
    shift_out_val(_b[(_o) + 1]);                                               \
    shift_out_val(_b[(_o) + 2]);                                               \
    shift_out_val(_b[(_o) + 3]);                                               \
    shift_out_val(_b[(_o) + 4]);                                               \
    shift_out_val(_b[(_o) + 5]);                                               \
    shift_out_val(_b[(_o) + 6]);                                               \
    shift_out_val(_b[(_o) + 7]);                                               \
    shift_out_val(_b[(_o) + 8]);                                               \
    shift_out_val(_b[(_o) + 9]);                                               \
    shift_out_val(_b[(_o) + 10]);                                              \
    shift_out_val(_b[(_o) + 11]);                                              \
    shift_out_val(_b[(_o) + 12]);                                              \
    shift_out_val(_b[(_o) + 13]);                                              \
    shift_out_val(_b[(_o) + 14]);                                              \
    shift_out_val(_b[(_o) + 15]);                                              \
    shift_out_val(_b[(_o) + 16]);                                              \
    shift_out_val(_b[(_o) + 17]);                                              \
    shift_out_val(_b[(_o) + 18]);                                              \
    shift_out_val(_b[(_o) + 19]);                                              \
    shift_out_val(_b[(_o) + 20]);                                              \
    shift_out_val(_b[(_o) + 21]);                                              \
    shift_out_val(_b[(_o) + 22]);                                              \
    shift_out_val(_b[(_o) + 23]);                                              \
    shift_out_val(_b[(_o) + 24]);                                              \
    shift_out_val(_b[(_o) + 25]);                                              \
    shift_out_val(_b[(_o) + 26]);                                              \
    shift_out_val(_b[(_o) + 27]);                                              \
    shift_out_val(_b[(_o) + 28]);                                              \
    shift_out_val(_b[(_o) + 29]);                                              \
    shift_out_val(_b[(_o) + 30]);                                              \
    shift_out_val(_b[(_o) + 31]);                                              \
    shift_out_val(_b[(_o) + 32]);                                              \
    shift_out_val(_b[(_o) + 33]);                                              \
    shift_out_val(_b[(_o) + 34]);                                              \
    shift_out_val(_b[(_o) + 35]);                                              \
    shift_out_val(_b[(_o) + 36]);                                              \
    shift_out_val(_b[(_o) + 37]);                                              \
    shift_out_val(_b[(_o) + 38]);                                              \
    shift_out_val(_b[(_o) + 39]);                                              \
    shift_out_val(_b[(_o) + 40]);                                              \
    shift_out_val(_b[(_o) + 41]);                                              \
    shift_out_val(_b[(_o) + 42]);                                              \
    shift_out_val(_b[(_o) + 43]);                                              \
    shift_out_val(_b[(_o) + 44]);                                              \
    shift_out_val(_b[(_o) + 45]);                                              \
    shift_out_val(_b[(_o) + 46]);                                              \
    shift_out_val(_b[(_o) + 47]);                                              \
    shift_out_val(_b[(_o) + 48]);                                              \
    shift_out_val(_b[(_o) + 49]);                                              \
    shift_out_val(_b[(_o) + 50]);                                              \
    shift_out_val(_b[(_o) + 51]);                                              \
    shift_out_val(_b[(_o) + 52]);                                              \
    shift_out_val(_b[(_o) + 53]);                                              \
    shift_out_val(_b[(_o) + 54]);                                              \
    shift_out_val(_b[(_o) + 55]);                                              \
    shift_out_val(_b[(_o) + 56]);                                              \
    shift_out_val(_b[(_o) + 57]);                                              \
    shift_out_val(_b[(_o) + 58]);                                              \
    shift_out_val(_b[(_o) + 59]);                                              \
    shift_out_val(_b[(_o) + 60]);                                              \
    shift_out_val(_b[(_o) + 61]);                                              \
    shift_out_val(_b[(_o) + 62]);                                              \
    shift_out_val(_b[(_o) + 63]);                                              \
  })

// used for quick access to the timer count values
static uint16_t DRAM_ATTR timer_count_values[8] = {
    LED_MATRIX_TIMER_ALARM_0, LED_MATRIX_TIMER_ALARM_1,
    LED_MATRIX_TIMER_ALARM_2, LED_MATRIX_TIMER_ALARM_3,
    LED_MATRIX_TIMER_ALARM_4, LED_MATRIX_TIMER_ALARM_5,
    LED_MATRIX_TIMER_ALARM_6, LED_MATRIX_TIMER_ALARM_7};

static bool IRAM_ATTR led_matrix_timer_callback(
    gptimer_handle_t timer, const gptimer_alarm_event_data_t *event_data,
    void *user_data) {
  static led_matrix_handle_t matrix;
  matrix = (led_matrix_handle_t)user_data;

  gptimer_stop(matrix->timer);

  matrix->bitNum++;
  if (matrix->bitNum >= LED_MATRIX_BIT_DEPTH) {
    matrix->bitNum = 0;
    matrix->rowNum++;
    if (matrix->rowNum >= matrix->halfHeight) {
      matrix->rowNum = 0;
    }
  }

  matrix->currentBufferOffset =
      (matrix->rowNum * matrix->width) +
      (matrix->bitNum * matrix->width * matrix->height);

  shift_out_row(matrix->buffer, matrix->currentBufferOffset);

  // blank screen
  gpio_ll_set_level(&GPIO, matrix->pins->oe, 1);

  // saves some cycles
  if (matrix->bitNum == 0) {
    // set new address.
    gpio_ll_set_level(&GPIO, matrix->pins->a0, matrix->rowNum & 0b00001);
    gpio_ll_set_level(&GPIO, matrix->pins->a1, matrix->rowNum & 0b00010);
    gpio_ll_set_level(&GPIO, matrix->pins->a2, matrix->rowNum & 0b00100);
    gpio_ll_set_level(&GPIO, matrix->pins->a3, matrix->rowNum & 0b01000);
    if (matrix->fiveBitAddress) {
      gpio_ll_set_level(&GPIO, matrix->pins->a4, matrix->rowNum & 0b10000);
    }
  }

  // latch, then reset all bundle outputs
  dedic_gpio_cpu_ll_write_mask(0b10000000, 0b00000000);
  asm volatile("nop"); // delay so the ICN2037 can keep up
  dedic_gpio_cpu_ll_write_mask(0b10000000, 0b10000000);

  // show new row
  gpio_ll_set_level(&GPIO, matrix->pins->oe, 0);

  gptimer_set_raw_count(matrix->timer, timer_count_values[matrix->bitNum]);
  gptimer_start(matrix->timer);

  return false;
}

// Allocates the resources for a matrix and masses back a handle
esp_err_t led_matrix_init(led_matrix_handle_t *matrix_handle,
                          led_matrix_config_t *config) {
  // allocate the the state. Must be in IRAM for the interrupt handler
  led_matrix_handle_t matrix = (led_matrix_handle_t)heap_caps_malloc(
      sizeof(led_matrix_state_t), MALLOC_CAP_INTERNAL);

  // misc setup
  matrix->rowNum = 0;
  matrix->bitNum = 0;
  matrix->width = config->width;
  matrix->height = config->height;
  matrix->halfHeight = matrix->height / 2;
  matrix->currentBufferOffset = 0;
  matrix->splitOffset = (matrix->height / 2) * matrix->width;
  matrix->fiveBitAddress = matrix->height > 32;

  // allocate/clear the frame buffer. Must be in IRAM for the interrupt handler
  matrix->buffer = (uint8_t *)heap_caps_malloc(
      sizeof(uint8_t) * matrix->width * matrix->height * LED_MATRIX_BIT_DEPTH,
      MALLOC_CAP_INTERNAL);
  memset(matrix->buffer, 0,
         sizeof(uint8_t) * matrix->width * matrix->height *
             LED_MATRIX_BIT_DEPTH);

  // allocate and copy pins. Must be in IRAM for the interrupt handler
  matrix->pins = (led_matrix_pins_t *)heap_caps_malloc(
      sizeof(led_matrix_pins_t), MALLOC_CAP_INTERNAL);
  memcpy(matrix->pins, &config->pins, sizeof(led_matrix_pins_t));

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

  if (matrix->fiveBitAddress) {
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
      dedic_gpio_new_bundle(&gpio_bundle_color_config, &matrix->gpio_bundle),
      TAG, "Failed to init matrix dedicated GPIO bundle");

  // setup the timer
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_DOWN,
      .resolution_hz = LED_MATRIX_TIMER_RESOLUTION,
      .intr_priority = 3,
  };
  ESP_RETURN_ON_ERROR(gptimer_new_timer(&timer_config, &matrix->timer), TAG,
                      "Failed to create new timer");
  // timer alarm callback
  gptimer_event_callbacks_t cbs = {
      .on_alarm = led_matrix_timer_callback,
  };
  ESP_RETURN_ON_ERROR(
      gptimer_register_event_callbacks(matrix->timer, &cbs, matrix), TAG,
      "Failed to register timer alarm callback");
  // enable the timer
  ESP_RETURN_ON_ERROR(gptimer_enable(matrix->timer), TAG,
                      "Failed to enable timer");

  // timer alarm
  gptimer_alarm_config_t alarm_config = {
      // counting down, so we set a value and count to 0
      .alarm_count = 0,
      .reload_count = LED_MATRIX_TIMER_ALARM,
      .flags.auto_reload_on_alarm = false,
  };
  ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(matrix->timer, &alarm_config),
                      TAG, "Failed to create timer alarm");

  ESP_RETURN_ON_ERROR(
      gptimer_set_raw_count(matrix->timer, LED_MATRIX_TIMER_ALARM), TAG,
      "Failed to set timer raw count");

  // pass back the config
  *matrix_handle = matrix;

  return ESP_OK;
};

// Starts the hardware resources associated with the matrix
esp_err_t led_matrix_start(led_matrix_handle_t matrix) {
  ESP_RETURN_ON_ERROR(gptimer_start(matrix->timer), TAG,
                      "START: Failed to start timer");
  return ESP_OK;
}

// Stops the hardware resources associated with the matrix
esp_err_t led_matrix_stop(led_matrix_handle_t matrix) {
  ESP_RETURN_ON_ERROR(gptimer_stop(matrix->timer), TAG,
                      "STOP: Failed to stop timer");
  return ESP_OK;
}

// Stops the hardware resources associated with the matrix and frees all
// internal memory
esp_err_t led_matrix_end(led_matrix_handle_t matrix) {
  ESP_RETURN_ON_ERROR(led_matrix_stop(matrix), TAG, "END: Failed to stop.");
  gptimer_del_timer(matrix->timer);
  dedic_gpio_del_bundle(matrix->gpio_bundle);
  free(matrix->pins);
  free(matrix->buffer);
  free(matrix);
  return ESP_OK;
}

// Shows a `buffer` in the `matrix`
esp_err_t led_matrix_show(led_matrix_handle_t matrix, uint8_t *buffer_red,
                          uint8_t *buffer_green, uint8_t *buffer_blue) {
  uint16_t rowAndBitOffset;
  uint16_t rowOffset;
  uint8_t row;
  uint8_t col;
  uint8_t bitNum;

  for (bitNum = 0; bitNum < LED_MATRIX_BIT_DEPTH; bitNum++) {
    for (row = 0; row < matrix->halfHeight; row++) {
      rowOffset = (row * matrix->width);
      rowAndBitOffset = rowOffset + (bitNum * matrix->width * matrix->height);
      for (col = 0; col < matrix->width; col++) {
        SET_MATRIX_BYTE(
            // put the value into the variable
            matrix->buffer[rowAndBitOffset + col],
            // pull the "top" red from the frame buffer
            buffer_red[rowOffset + col],
            // pull the "top" green from the frame buffer
            buffer_green[rowOffset + col],
            // pull the "top" blue from the frame buffer
            buffer_blue[rowOffset + col],
            // pull the "bottom" ...
            buffer_red[matrix->splitOffset + rowOffset + col],
            buffer_green[matrix->splitOffset + rowOffset + col],
            buffer_blue[matrix->splitOffset + rowOffset + col],
            // just this loop's bit
            bitNum);
      }
    }
  }

  return ESP_OK;
}