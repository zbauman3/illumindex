// #include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/dedic_gpio_ll.h"
#include "hal/gpio_ll.h"
#include <string.h>

#include "drivers/matrix.h"
#include "util/565_color.h"
#include "util/helpers.h"

static const char *TAG = "MATRIX_DRIVER";
static dedic_gpio_bundle_handle_t gpioColorBundle = NULL;

static volatile bool frameBufferPtrIsNew = true;
static volatile uint16_t *frameBufferPtr;
static volatile uint8_t sendFrameBuffer[64 * 16];

static MatrixPins usePins;
static volatile uint8_t row;

static bool IRAM_ATTR alarm_cb(gptimer_handle_t timer,
                               const gptimer_alarm_event_data_t *edata,
                               void *user_data) {
  if (frameBufferPtr == NULL) {
    return false;
  }

  // uint8_t row;
  uint8_t col;
  uint16_t sendFrameBufferOffset;

  // for (row = 0; row < 16; row++) {
  sendFrameBufferOffset = row * 64;

  for (col = 0; col < 64; col++) {
    SET_565_MATRIX_BYTE(
        // place the processed byte into the send buffer
        sendFrameBuffer[sendFrameBufferOffset + col],
        // pull the "top" row from the frame buffer
        frameBufferPtr[sendFrameBufferOffset + col],
        // pull the "bottom" row ...
        frameBufferPtr[1024 + sendFrameBufferOffset + col],
        // for now, just the first bit
        0);

    // shift in each column. Clock is on the rising edge
    dedic_gpio_ll_write_mask(&DEDIC_GPIO, 0b01111111,
                             sendFrameBuffer[sendFrameBufferOffset + col] |
                                 0b00000000);

    dedic_gpio_ll_write_mask(&DEDIC_GPIO, 0b01111111,
                             sendFrameBuffer[sendFrameBufferOffset + col] |
                                 0b01000000);
  }

  gpio_ll_set_level(&GPIO, usePins.oe, 1);

  gpio_ll_set_level(&GPIO, usePins.a0, row & 0b0001 ? 1 : 0);
  gpio_ll_set_level(&GPIO, usePins.a1, row & 0b0010 ? 1 : 0);
  gpio_ll_set_level(&GPIO, usePins.a2, row & 0b0100 ? 1 : 0);
  gpio_ll_set_level(&GPIO, usePins.a3, row & 0b1000 ? 1 : 0);

  // latch, then reset all bundle outputs
  dedic_gpio_ll_write_mask(&DEDIC_GPIO, 0b10000000, 0b00000000);
  dedic_gpio_ll_write_mask(&DEDIC_GPIO, 0b10000000, 0b10000000);
  dedic_gpio_ll_write_mask(&DEDIC_GPIO, 0b11111111, 0b00000000);

  gpio_ll_set_level(&GPIO, usePins.oe, 0);
  // }

  row++;
  if (row >= 16) {
    row = 0;
  }

  return false;
}

esp_err_t matrixInit(MatrixPins pins) {
  memcpy(&usePins, &pins, sizeof(MatrixPins));

  gpio_config_t io_conf = {
      .pin_bit_mask = _BV_1ULL(pins.oe) | _BV_1ULL(pins.a0) |
                      _BV_1ULL(pins.a1) | _BV_1ULL(pins.a2) | _BV_1ULL(pins.a3),
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pull_down_en = 0,
      .pull_up_en = 0,
  };

  const int bundle_color_pins[8] = {
      pins.b1, pins.g1, pins.r1,    pins.b2,
      pins.g2, pins.r2, pins.clock, pins.latch,
  };

  dedic_gpio_bundle_config_t gpio_bundle_color_config = {
      .gpio_array = bundle_color_pins,
      .array_size = sizeof(bundle_color_pins) / sizeof(bundle_color_pins[0]),
      .flags = {.out_en = 1},
  };

  ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "Unable to init matrix GPIO");

  ESP_RETURN_ON_ERROR(
      dedic_gpio_new_bundle(&gpio_bundle_color_config, &gpioColorBundle), TAG,
      "Unable to init matrix dedicated GPIO bundle");

  gptimer_handle_t gptimer = NULL;
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = 1000000, // 1MHz, 1 tick=1us
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

  gptimer_event_callbacks_t cbs = {
      .on_alarm = alarm_cb,
  };

  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, &pins));

  ESP_LOGI(TAG, "Enable timer");
  ESP_ERROR_CHECK(gptimer_enable(gptimer));

  ESP_LOGI(TAG, "Start timer, stop it at alarm event");
  gptimer_alarm_config_t alarm_config1 = {
      .alarm_count = 100,
      .reload_count = 0,
      .flags = {.auto_reload_on_alarm = true}};
  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config1));

  ESP_ERROR_CHECK(gptimer_start(gptimer));

  ESP_LOGI(TAG, "Timer Running");

  return ESP_OK;
};

void showFrame(uint16_t *bufferPtr) {
  // set the new frame
  frameBufferPtr = bufferPtr;
  frameBufferPtrIsNew = true;

  while (true) {
    ESP_LOGI(TAG, "LOOP!");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}