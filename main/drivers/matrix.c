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
#include "util/565_color.h"
#include "util/helpers.h"
#include "util/time.h"

static const char *TAG = "MATRIX_DRIVER";
static dedic_gpio_bundle_handle_t gpioColorBundle = NULL;
static TaskHandle_t matrixTransmitTaskHandle = NULL;
static TaskHandle_t matrixProcessTaskHandle = NULL;
// The queue will just hold the number of each row in `prsdFrameBuffer` that is
// ready to be processed. So the consumer of the queue will use `queueVal * 64`
// to get the offset that is ready to be sent
static QueueHandle_t prsdFrameQueueHandle = NULL;
static uint8_t prsdFrameBuffer[64 * 16];

static SemaphoreHandle_t frameMutexHandle = NULL;
static uint16_t frameBuffer[64 * 32];

void matrixTransmit() {
  uint8_t row;
  uint16_t prsdFrameBufferOffset;
  BaseType_t queueResult;

  while (true) {
    queueResult =
        xQueueReceive(prsdFrameQueueHandle, (void *)&row, portMAX_DELAY);

    if (queueResult != pdTRUE) {
      ESP_LOGE(TAG,
               "Unable to receive queue value from \"prsdFrameQueueHandle\"");
    }

    prsdFrameBufferOffset = row * 64;

    for (uint8_t column = 0; column < 64; column++) {
      dedic_gpio_bundle_write(gpioColorBundle, 0b01111111,
                              prsdFrameBuffer[prsdFrameBufferOffset + column] |
                                  0b00000000);
      dedic_gpio_bundle_write(gpioColorBundle, 0b01111111,
                              prsdFrameBuffer[prsdFrameBufferOffset + column] |
                                  0b01000000);
    }

    gpio_set_level(MATRIX_OE, 1);

    gpio_set_level(MATRIX_ADDR_A, row & 0b0001);
    gpio_set_level(MATRIX_ADDR_B, row & 0b0010);
    gpio_set_level(MATRIX_ADDR_C, row & 0b0100);
    gpio_set_level(MATRIX_ADDR_D, row & 0b1000);

    // latch
    dedic_gpio_bundle_write(gpioColorBundle, 0b10000000, 0b00000000);
    dedic_gpio_bundle_write(gpioColorBundle, 0b10000000, 0b10000000);

    gpio_set_level(MATRIX_OE, 0);

    delayMicrosecondsYield(5000);
  }
};

void matrixProcess() {
  while (true) {
    xSemaphoreTake(frameMutexHandle, portMAX_DELAY);

    for (uint8_t row = 0; row < 16; row++) {
      for (uint8_t column = 0; column < 64; column++) {
        SET_565_MATRIX_BYTE(prsdFrameBuffer[row + column],
                            frameBuffer[row + column],
                            frameBuffer[1024 + row + column], 0);
      }

      xQueueSendToBack(prsdFrameQueueHandle, (void *)&row, portMAX_DELAY);
      taskYIELD();
    }

    xSemaphoreGive(frameMutexHandle);
    taskYIELD();
  }
};

esp_err_t matrix_init() {
  gpio_config_t io_conf = {
      .pin_bit_mask = _BV_1ULL(MATRIX_OE) | _BV_1ULL(MATRIX_ADDR_A) |
                      _BV_1ULL(MATRIX_ADDR_B) | _BV_1ULL(MATRIX_ADDR_C) |
                      _BV_1ULL(MATRIX_ADDR_D),
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pull_down_en = 0,
      .pull_up_en = 0,
  };

  const int bundle_color_pins[8] = {
      MATRIX_BLUE_1,  MATRIX_GREEN_1, MATRIX_RED_1, MATRIX_BLUE_2,
      MATRIX_GREEN_2, MATRIX_RED_2,   MATRIX_CLOCK, MATRIX_LATCH,
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

  prsdFrameQueueHandle = xQueueCreate(16, 1);

  ESP_RETURN_ON_FALSE(prsdFrameQueueHandle != NULL, ESP_ERR_NO_MEM, TAG,
                      "Unable to create the \"prsdFrameQueueHandle\" queue.");

  frameMutexHandle = xSemaphoreCreateMutex();

  ESP_RETURN_ON_FALSE(frameMutexHandle != NULL, ESP_ERR_NO_MEM, TAG,
                      "Unable to create the \"frameMutexHandle\" mutex.");

  createRes = xTaskCreate(matrixTransmit, "matrixTransmit", (1024 * 4), NULL,
                          MATRIX_TASK_PRIORITY, &matrixTransmitTaskHandle);

  ESP_RETURN_ON_FALSE(createRes == pdPASS, ESP_ERR_NO_MEM, TAG,
                      "Unable to create the \"matrixTransmit\" task.");

  createRes = xTaskCreate(matrixProcess, "matrixProcess", (1024 * 4), NULL,
                          MATRIX_TASK_PRIORITY, &matrixProcessTaskHandle);

  ESP_RETURN_ON_FALSE(createRes == pdPASS, ESP_ERR_NO_MEM, TAG,
                      "Unable to create the \"matrixProcess\" task.");

  return ESP_OK;
};

void displayTest() {
  uint16_t topRow[64] = {
      BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,
      BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,
      BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,
      BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,
      BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,
      BV_565_RED_0, // 21 red
      BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0,
      BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0,
      BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0,
      BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0,
      BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0,
      BV_565_GREEN_0,
      BV_565_GREEN_0, // 22 green
      BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,
      BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,
      BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,
      BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,
      BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,
      BV_565_BLUE_0 // 21 blue
  };

  uint16_t bottomRow[64] = {
      BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,
      BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,
      BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,
      BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,
      BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,  BV_565_BLUE_0,
      BV_565_BLUE_0, // 21 blue
      BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0,
      BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0,
      BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0,
      BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0,
      BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0, BV_565_GREEN_0,
      BV_565_GREEN_0,
      BV_565_GREEN_0, // 22 green
      BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,
      BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,
      BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,
      BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,
      BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,   BV_565_RED_0,
      BV_565_RED_0 // 21 red
  };

  uint8_t *buff = heap_caps_malloc(sizeof(uint8_t) * 64, MALLOC_CAP_DMA);
  for (uint8_t i = 0; i < 64; i++) {
    SET_565_MATRIX_BYTE(buff[i], topRow[i], bottomRow[i], 0);
  }

  uint64_t loops = 0;
  int64_t us_start = esp_timer_get_time();
  int64_t us_end;
  float hrz;

  while (true) {
    for (uint8_t row = 0; row < 16; row++) {
      for (uint8_t col = 0; col < 64; col++) {
        dedic_gpio_bundle_write(gpioColorBundle, 0b01111111,
                                buff[col] | 0b00000000);
        dedic_gpio_bundle_write(gpioColorBundle, 0b01111111,
                                buff[col] | 0b01000000);
      }

      gpio_set_level(MATRIX_OE, 1);

      gpio_set_level(MATRIX_ADDR_A, row & 0b0001);
      gpio_set_level(MATRIX_ADDR_B, row & 0b0010);
      gpio_set_level(MATRIX_ADDR_C, row & 0b0100);
      gpio_set_level(MATRIX_ADDR_D, row & 0b1000);

      dedic_gpio_bundle_write(gpioColorBundle, 0b10000000, 0b00000000);
      dedic_gpio_bundle_write(gpioColorBundle, 0b10000000, 0b10000000);

      gpio_set_level(MATRIX_OE, 0);
      taskYIELD();
    }

    if (loops == 10999) {
      us_end = esp_timer_get_time();
      hrz =
          (float)11000 / (float)(((float)(us_end - us_start) / (float)1000000));

      ESP_LOGI(TAG, "100 loops in. Time: %lld. HRz: %f", (us_end - us_start),
               hrz);

      us_start = us_end;
      loops = 0;
    } else {
      loops++;
    }
  }

  heap_caps_free(buff);

  ESP_LOGI(TAG, "Done sending");
};