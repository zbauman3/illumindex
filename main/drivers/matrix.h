#pragma once

#define MATRIX_ADDR_A GPIO_NUM_18  // A0
#define MATRIX_ADDR_B GPIO_NUM_17  // A1
#define MATRIX_ADDR_C GPIO_NUM_16  // A2
#define MATRIX_ADDR_D GPIO_NUM_15  // A3
#define MATRIX_LATCH GPIO_NUM_8    // A5
#define MATRIX_CLOCK GPIO_NUM_5    // 5
#define MATRIX_OE GPIO_NUM_14      // A4
#define MATRIX_RED_1 GPIO_NUM_12   // 12
#define MATRIX_GREEN_1 GPIO_NUM_36 // SCK
#define MATRIX_BLUE_1 GPIO_NUM_11  // 11
#define MATRIX_RED_2 GPIO_NUM_9    // 9
#define MATRIX_GREEN_2 GPIO_NUM_10 // 10
#define MATRIX_BLUE_2 GPIO_NUM_6   // 6
#define MATRIX_UNUSED_1 45         // 8-bit mode requires all are set.
#define MATRIX_UNUSED_2 42         // Setting these to unused pins.

esp_err_t matrix_init();

void displayTest();