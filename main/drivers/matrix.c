#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

#include "drivers/matrix.h"
#include "util/helpers.h"
#include "util/time.h"

/**
 * TODO
 * - Convert all RTOS functions to static allocation
 */

static const char *TAG = "MATRIX_DRIVER";
static dedic_gpio_bundle_handle_t gpioColorBundle = NULL;
static TaskHandle_t transmitTaskHandle = NULL;
static TaskHandle_t processTaskHandle = NULL;
// The queue will just hold the number of each row in `sendFrameBuffer` that is
// ready to be processed. So the consumer of the queue will use `queueVal * 64`
// to get the offset that is ready to be sent
static QueueHandle_t sendFrameQueueHandle = NULL;
static uint8_t sendFrameBuffer[64 * 16];
// This handle is responsible to communication between the processing tasks and
// the original caller. The task holds the lock while it is processing a frame,
// and the caller holds the lock while it is setting a new frame.
static SemaphoreHandle_t frameBufferMutexHandle = NULL;
static uint16_t *frameBufferPtr;
// This allows the processing task to know if it is reprocessing the same frame
// multiple times. If it is just looping omn one frame it can opt out of
// reprocessing every time.
static bool frameBufferPtrIsNew = true;

void matrixTransmit(void *parameters) {
  // copy the input parameters so that we can continue to use them
  MatrixPins *pins;
  memcpy(pins, parameters, sizeof(MatrixPins));

  uint8_t row;
  uint8_t col;
  uint16_t sendFrameBufferOffset;

  while (true) {
    xQueueReceive(sendFrameQueueHandle, (void *)&row, portMAX_DELAY);

    sendFrameBufferOffset = row * 64;

    for (col = 0; col < 64; col++) {
      // shift in each column. Clock is on the rising edge
      dedic_gpio_bundle_write(gpioColorBundle, 0b01111111,
                              sendFrameBuffer[sendFrameBufferOffset + col] |
                                  0b00000000);
      dedic_gpio_bundle_write(gpioColorBundle, 0b01111111,
                              sendFrameBuffer[sendFrameBufferOffset + col] |
                                  0b01000000);
    }

    gpio_set_level(pins->oe, 1);

    gpio_set_level(pins->a0, row & 0b0001);
    gpio_set_level(pins->a1, row & 0b0010);
    gpio_set_level(pins->a2, row & 0b0100);
    gpio_set_level(pins->a3, row & 0b1000);

    // latch, then reset all bundle outputs
    dedic_gpio_bundle_write(gpioColorBundle, 0b10000000, 0b00000000);
    dedic_gpio_bundle_write(gpioColorBundle, 0b10000000, 0b10000000);
    dedic_gpio_bundle_write(gpioColorBundle, 0b11111111, 0b00000000);

    gpio_set_level(pins->oe, 0);

    delayMicrosecondsYield(5000);
  }
};

void matrixProcess() {
  uint8_t row;
  uint8_t col;
  uint16_t frameBufferPtrOffset;

  while (true) {
    xSemaphoreTake(frameBufferMutexHandle, portMAX_DELAY);

    for (row = 0; row < 16; row++) {
      // no need to reprocess the frame if we are just resending the same frame.
      if (frameBufferPtrIsNew) {
        frameBufferPtrOffset = row * 64;
        for (col = 0; col < 64; col++) {
          SET_565_MATRIX_BYTE(
              // place the processed byte into the send buffer
              sendFrameBuffer[frameBufferPtrOffset + col],
              // pull the "top" row from the frame buffer
              frameBufferPtr[frameBufferPtrOffset + col],
              // pull the "bottom" row ...
              frameBufferPtr[1024 + frameBufferPtrOffset + col],
              // for now, just the first bit
              0);
        }
      }

      xQueueSendToBack(sendFrameQueueHandle, (void *)&row, portMAX_DELAY);
      // yield so that the frame can be transmitted immediately
      taskYIELD();
    }

    // signal that this frame has been processed.
    // this allows saving time on repeat
    frameBufferPtrIsNew = false;
    xSemaphoreGive(frameBufferMutexHandle);
  }
};

esp_err_t matrixInit(MatrixPins pins) {
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

  BaseType_t createRes;

  ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "Unable to init matrix GPIO");

  ESP_RETURN_ON_ERROR(
      dedic_gpio_new_bundle(&gpio_bundle_color_config, &gpioColorBundle), TAG,
      "Unable to init matrix dedicated GPIO bundle");

  sendFrameQueueHandle = xQueueCreate(16, 1);

  ESP_RETURN_ON_FALSE(sendFrameQueueHandle != NULL, ESP_ERR_NO_MEM, TAG,
                      "Unable to create the \"sendFrameQueueHandle\" queue.");

  frameBufferMutexHandle = xSemaphoreCreateMutex();

  ESP_RETURN_ON_FALSE(frameBufferMutexHandle != NULL, ESP_ERR_NO_MEM, TAG,
                      "Unable to create the \"frameBufferMutexHandle\" mutex.");

  createRes = xTaskCreate(matrixTransmit, "matrixTransmit", (1024 * 4),
                          (void *)&pins, 1, &transmitTaskHandle);

  ESP_RETURN_ON_FALSE(createRes == pdPASS, ESP_ERR_NO_MEM, TAG,
                      "Unable to create the \"matrixTransmit\" task.");

  createRes = xTaskCreate(matrixProcess, "matrixProcess", (1024 * 4), NULL, 1,
                          &processTaskHandle);

  ESP_RETURN_ON_FALSE(createRes == pdPASS, ESP_ERR_NO_MEM, TAG,
                      "Unable to create the \"matrixProcess\" task.");

  return ESP_OK;
};

void showFrame(uint16_t *bufferPtr) {
  // wait for the current frame to stop being processed
  xSemaphoreTake(frameBufferMutexHandle, portMAX_DELAY);

  // set the new frame
  frameBufferPtr = bufferPtr;
  frameBufferPtrIsNew = true;

  // allow the processing to start
  xSemaphoreGive(frameBufferMutexHandle);
  // yield incase we are running on the same priority
  taskYIELD();
  // reclaim the mutex to wait for processing to complete before returning
  xSemaphoreTake(frameBufferMutexHandle, portMAX_DELAY);
  // give back the mutex to allow the processing task to "spin" on this frame
  xSemaphoreGive(frameBufferMutexHandle);
}