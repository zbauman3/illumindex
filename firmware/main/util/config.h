#pragma once

#include "driver/gpio.h"
#include "esp_http_client.h"

// green and blue are switched here due to the `ICN2037`
#define MATRIX_BLUE_1 GPIO_NUM_18  // A0
#define MATRIX_RED_1 GPIO_NUM_17   // A1
#define MATRIX_GREEN_1 GPIO_NUM_16 // A2
#define MATRIX_BLUE_2 GPIO_NUM_15  // A3
#define MATRIX_RED_2 GPIO_NUM_14   // A4
#define MATRIX_GREEN_2 GPIO_NUM_8  // A5
#define MATRIX_CLOCK GPIO_NUM_36   // SCK
#define MATRIX_LATCH GPIO_NUM_5    // 5
#define MATRIX_OE GPIO_NUM_6       // 6
#define MATRIX_ADDR_A GPIO_NUM_9   // 9
#define MATRIX_ADDR_B GPIO_NUM_10  // 10
#define MATRIX_ADDR_C GPIO_NUM_11  // 11
#define MATRIX_ADDR_D GPIO_NUM_12  // 12

#define API_ENDPOINT_URL "https://illumindex.vercel.app/api/matrix-test"
#define API_ENDPOINT_METHOD HTTP_METHOD_GET