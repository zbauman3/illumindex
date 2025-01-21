#pragma once

// This file contains helpers for working with 565 colors:
// https://en.wikipedia.org/wiki/List_of_monochrome_and_RGB_color_formats#16-bit_RGB_(also_known_as_RGB565)
// Ideally, these helpers should be mostly compile-time utilities, with minimal
// run-time logic.

// The bitmasks for 565 colors
#define BV_565_RED_0 0b0000100000000000U
#define BV_565_RED_1 0b0001000000000000U
#define BV_565_RED_2 0b0010000000000000U
#define BV_565_RED_3 0b0100000000000000U
#define BV_565_RED_4 0b1000000000000000U
#define BV_565_GREEN_0 0b0000000000100000U
#define BV_565_GREEN_1 0b0000000001000000U
#define BV_565_GREEN_2 0b0000000010000000U
#define BV_565_GREEN_3 0b0000000100000000U
#define BV_565_GREEN_4 0b0000001000000000U
#define BV_565_GREEN_5 0b0000010000000000U
#define BV_565_BLUE_0 0b0000000000000001U
#define BV_565_BLUE_1 0b0000000000000010U
#define BV_565_BLUE_2 0b0000000000000100U
#define BV_565_BLUE_3 0b0000000000001000U
#define BV_565_BLUE_4 0b0000000000010000U

// Given two 565 `uint16_t` values, this determines how far we should shift one
// of the bits to get it into a single byte as `0,0,R1,G1,B1,R2,G2,B2`
#define SHIFT_565_RED_LOW_BIT_0 >> 9
#define SHIFT_565_RED_LOW_BIT_1 >> 10
#define SHIFT_565_RED_LOW_BIT_2 >> 11
#define SHIFT_565_RED_LOW_BIT_3 >> 12
#define SHIFT_565_RED_LOW_BIT_4 >> 13
#define SHIFT_565_GREEN_LOW_BIT_0 >> 4
#define SHIFT_565_GREEN_LOW_BIT_1 >> 5
#define SHIFT_565_GREEN_LOW_BIT_2 >> 6
#define SHIFT_565_GREEN_LOW_BIT_3 >> 7
#define SHIFT_565_GREEN_LOW_BIT_4 >> 8
#define SHIFT_565_GREEN_LOW_BIT_5 >> 9
#define SHIFT_565_BLUE_LOW_BIT_0 >> 0
#define SHIFT_565_BLUE_LOW_BIT_1 >> 1
#define SHIFT_565_BLUE_LOW_BIT_2 >> 2
#define SHIFT_565_BLUE_LOW_BIT_3 >> 3
#define SHIFT_565_BLUE_LOW_BIT_4 >> 4
#define SHIFT_565_RED_HIGH_BIT_0 >> 6
#define SHIFT_565_RED_HIGH_BIT_1 >> 7
#define SHIFT_565_RED_HIGH_BIT_2 >> 8
#define SHIFT_565_RED_HIGH_BIT_3 >> 9
#define SHIFT_565_RED_HIGH_BIT_4 >> 10
#define SHIFT_565_GREEN_HIGH_BIT_0 >> 1
#define SHIFT_565_GREEN_HIGH_BIT_1 >> 2
#define SHIFT_565_GREEN_HIGH_BIT_2 >> 3
#define SHIFT_565_GREEN_HIGH_BIT_3 >> 4
#define SHIFT_565_GREEN_HIGH_BIT_4 >> 5
#define SHIFT_565_GREEN_HIGH_BIT_5 >> 6
#define SHIFT_565_BLUE_HIGH_BIT_0 << 3
#define SHIFT_565_BLUE_HIGH_BIT_1 << 2
#define SHIFT_565_BLUE_HIGH_BIT_2 << 1
#define SHIFT_565_BLUE_HIGH_BIT_3 >> 0
#define SHIFT_565_BLUE_HIGH_BIT_4 >> 1

// bits within a matrix byte
#define BV_MATRIX_RED_1 0b00100000
#define BV_MATRIX_GREEN_1 0b00010000
#define BV_MATRIX_BLUE_1 0b00001000
#define BV_MATRIX_RED_2 0b00000100
#define BV_MATRIX_GREEN_2 0b00000010
#define BV_MATRIX_BLUE_2 0b00000001

// This takes 2 `uint16_t` values representing two 565 colors (top row and
// bottom row) and a `uint8_t` representing which bit to grab from each color.
// It then generates a `uint8_t` for use on an 8-bit SPI bus. The value is
// packed into the byte in the following order: `0,0,R1,G1,B1,R2,G2,B2`.
#define GET_565_MATRIX_BYTE(one, two, bit)                                     \
  (((uint8_t)((one & BV_565_RED_##bit) SHIFT_565_RED_HIGH_BIT_##bit)) |        \
   ((uint8_t)((one & BV_565_GREEN_##bit) SHIFT_565_GREEN_HIGH_BIT_##bit)) |    \
   ((uint8_t)((one & BV_565_BLUE_##bit) SHIFT_565_BLUE_HIGH_BIT_##bit)) |      \
   ((uint8_t)((two & BV_565_RED_##bit) SHIFT_565_RED_LOW_BIT_##bit)) |         \
   ((uint8_t)((two & BV_565_GREEN_##bit) SHIFT_565_GREEN_LOW_BIT_##bit)) |     \
   ((uint8_t)((two & BV_565_BLUE_##bit) SHIFT_565_BLUE_LOW_BIT_##bit)))

// This takes a variable name, two `uint16_t` values representing two 565 colors
// (top row and bottom row), and a `uint8_t` representing which bit to grab from
// each color. It then generates a `uint8_t` for use on an 8-bit SPI bus. The
// value is packed into a byte and assigned to the variable in the following
// order : `0,0,R1,G1,B1,R2,G2,B2`.
#define SET_565_MATRIX_BYTE(varName, topColor, bottomColor, bit)               \
  switch (bit) {                                                               \
  case 4: {                                                                    \
    varName = GET_565_MATRIX_BYTE(topColor, bottomColor, 4);                   \
    break;                                                                     \
  }                                                                            \
  case 3: {                                                                    \
    varName = GET_565_MATRIX_BYTE(topColor, bottomColor, 3);                   \
    break;                                                                     \
  }                                                                            \
  case 2: {                                                                    \
    varName = GET_565_MATRIX_BYTE(topColor, bottomColor, 2);                   \
    break;                                                                     \
  }                                                                            \
  case 1: {                                                                    \
    varName = GET_565_MATRIX_BYTE(topColor, bottomColor, 1);                   \
    break;                                                                     \
  }                                                                            \
  case 0:                                                                      \
  default: {                                                                   \
    varName = GET_565_MATRIX_BYTE(topColor, bottomColor, 0);                   \
    break;                                                                     \
  }                                                                            \
  }
