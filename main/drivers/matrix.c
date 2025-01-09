#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>

#include "drivers/matrix.h"
#include "util/565_color.h"
#include "util/helpers.h"

// bus master, and 8-bit mode
#define SPI_BUS_FLAGS (SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_OCTAL)
// half-duplex is required for 8-bit mode
#define SPI_DEVICE_FLAGS (SPI_DEVICE_HALFDUPLEX)
// 8-bit mode, and 8-bit command/address. CMD/ADDR isn't required, but it makes
// those happen in the same fashion as a data transfer
#define SPI_TRANS_FLAGS                                                        \
  (SPI_TRANS_MODE_OCT | SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR)

static const char *TAG = "MATRIX_DRIVER";
static spi_device_handle_t spi;

esp_err_t matrix_init() {
  esp_err_t ret;

  gpio_config_t io_conf = {
      .pin_bit_mask = _BV_1ULL(MATRIX_LATCH) | _BV_1ULL(MATRIX_OE) |
                      _BV_1ULL(MATRIX_ADDR_A) | _BV_1ULL(MATRIX_ADDR_B) |
                      _BV_1ULL(MATRIX_ADDR_C) | _BV_1ULL(MATRIX_ADDR_D),
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pull_down_en = 0,
      .pull_up_en = 0,
  };

  ret = gpio_config(&io_conf);
  ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG,
                      "Error initializing the SPI GPIO");

  // the bits for blue/green are swapped due to the hardware
  spi_bus_config_t buscfg = {
      .flags = SPI_BUS_FLAGS,
      .max_transfer_sz = 64,
      .sclk_io_num = MATRIX_CLOCK,
      .data0_io_num = MATRIX_GREEN_2,
      .data1_io_num = MATRIX_BLUE_2,
      .data2_io_num = MATRIX_RED_2,
      .data3_io_num = MATRIX_GREEN_1,
      .data4_io_num = MATRIX_BLUE_1,
      .data5_io_num = MATRIX_RED_1,
      .data6_io_num = MATRIX_UNUSED_1,
      .data7_io_num = MATRIX_UNUSED_2,
      .intr_flags = ESP_INTR_FLAG_IRAM,
  };

  spi_device_interface_config_t devcfg = {
      .flags = SPI_DEVICE_FLAGS,
      .clock_speed_hz = 30 * 1000 * 1000, // 30 MHz is max for `ICN2037`
      .mode = 1,                          //
      .spics_io_num = -1,                 // not using CS
      .queue_size = 1,
  };

  ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
  ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG,
                      "Error initializing the SPI bus");

  ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
  ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG,
                      "Error initializing the SPI device");

  return ESP_OK;
};

void displayTest() {
  ESP_LOGI(TAG, "Sending data");

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
    SET_565_SPI_BYTE(buff[i], topRow[i], bottomRow[i], 0);
  }

  spi_transaction_t *inTrans;
  spi_transaction_t outTrans;
  memset(&outTrans, 0, sizeof(outTrans));
  outTrans.flags = SPI_TRANS_FLAGS;
  outTrans.length = 64 * 8;
  outTrans.rx_buffer = NULL;
  outTrans.tx_buffer = buff;

  while (true) {

    for (uint8_t i = 0; i < 16; i++) {
      // esp_err_t ret = ;
      // ESP_RETURN_VOID_ON_FALSE(ret == ESP_OK, TAG, "Error transmitting");

      ESP_ERROR_CHECK(spi_device_queue_trans(spi, &outTrans, portMAX_DELAY));
      ESP_ERROR_CHECK(
          spi_device_get_trans_result(spi, &inTrans, portMAX_DELAY));

      gpio_set_level(MATRIX_OE, 1);

      gpio_set_level(MATRIX_ADDR_A, i & 0b0001 ? 1 : 0);
      gpio_set_level(MATRIX_ADDR_B, i & 0b0010 ? 1 : 0);
      gpio_set_level(MATRIX_ADDR_C, i & 0b0100 ? 1 : 0);
      gpio_set_level(MATRIX_ADDR_D, i & 0b1000 ? 1 : 0);

      gpio_set_level(MATRIX_LATCH, 1);
      gpio_set_level(MATRIX_LATCH, 0);

      gpio_set_level(MATRIX_OE, 0);

      // vTaskDelay(0);
    }
  }
  heap_caps_free(buff);

  ESP_LOGI(TAG, "Done sending");
};