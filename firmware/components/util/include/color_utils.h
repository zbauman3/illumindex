#pragma once

#include <inttypes.h>

// Bit values for each color channel in a single byte.
// Used with "define concatenation" to select bits.
#define BV_COLOR_0 0b00000001
#define BV_COLOR_1 0b00000010
#define BV_COLOR_2 0b00000100
#define BV_COLOR_3 0b00001000
#define BV_COLOR_4 0b00010000
#define BV_COLOR_5 0b00100000
#define BV_COLOR_6 0b01000000
#define BV_COLOR_7 0b10000000

// Given two RGB values, this determines how far we should shift a given bit to
// get it into a single byte as `0,0,R1,G1,B1,R2,G2,B2` so that we can shift it
// out in parallel on the 8-bit serial bus.
// Used with "define concatenation" to select bits.
#define SHIFT_HIGH_BIT_RED_0 << 5
#define SHIFT_HIGH_BIT_RED_1 << 4
#define SHIFT_HIGH_BIT_RED_2 << 3
#define SHIFT_HIGH_BIT_RED_3 << 2
#define SHIFT_HIGH_BIT_RED_4 << 1
#define SHIFT_HIGH_BIT_RED_5 << 0
#define SHIFT_HIGH_BIT_RED_6 >> 1
#define SHIFT_HIGH_BIT_RED_7 >> 2
#define SHIFT_HIGH_BIT_GREEN_0 << 4
#define SHIFT_HIGH_BIT_GREEN_1 << 3
#define SHIFT_HIGH_BIT_GREEN_2 << 2
#define SHIFT_HIGH_BIT_GREEN_3 << 1
#define SHIFT_HIGH_BIT_GREEN_4 << 0
#define SHIFT_HIGH_BIT_GREEN_5 >> 1
#define SHIFT_HIGH_BIT_GREEN_6 >> 2
#define SHIFT_HIGH_BIT_GREEN_7 >> 3
#define SHIFT_HIGH_BIT_BLUE_0 << 3
#define SHIFT_HIGH_BIT_BLUE_1 << 2
#define SHIFT_HIGH_BIT_BLUE_2 << 1
#define SHIFT_HIGH_BIT_BLUE_3 << 0
#define SHIFT_HIGH_BIT_BLUE_4 >> 1
#define SHIFT_HIGH_BIT_BLUE_5 >> 2
#define SHIFT_HIGH_BIT_BLUE_6 >> 3
#define SHIFT_HIGH_BIT_BLUE_7 >> 4
// low bits
#define SHIFT_LOW_BIT_RED_0 << 2
#define SHIFT_LOW_BIT_RED_1 << 1
#define SHIFT_LOW_BIT_RED_2 << 0
#define SHIFT_LOW_BIT_RED_3 >> 1
#define SHIFT_LOW_BIT_RED_4 >> 2
#define SHIFT_LOW_BIT_RED_5 >> 3
#define SHIFT_LOW_BIT_RED_6 >> 4
#define SHIFT_LOW_BIT_RED_7 >> 5
#define SHIFT_LOW_BIT_GREEN_0 << 1
#define SHIFT_LOW_BIT_GREEN_1 << 0
#define SHIFT_LOW_BIT_GREEN_2 >> 1
#define SHIFT_LOW_BIT_GREEN_3 >> 2
#define SHIFT_LOW_BIT_GREEN_4 >> 3
#define SHIFT_LOW_BIT_GREEN_5 >> 4
#define SHIFT_LOW_BIT_GREEN_6 >> 5
#define SHIFT_LOW_BIT_GREEN_7 >> 6
#define SHIFT_LOW_BIT_BLUE_0 << 0
#define SHIFT_LOW_BIT_BLUE_1 >> 1
#define SHIFT_LOW_BIT_BLUE_2 >> 2
#define SHIFT_LOW_BIT_BLUE_3 >> 3
#define SHIFT_LOW_BIT_BLUE_4 >> 4
#define SHIFT_LOW_BIT_BLUE_5 >> 5
#define SHIFT_LOW_BIT_BLUE_6 >> 6
#define SHIFT_LOW_BIT_BLUE_7 >> 7

// This takes six `uint8_t` values representing two RGB colors – top row (HIGH)
// and bottom row (LOW) – and a `uint8_t` representing which bit to grab from
// each color. It then generates a `uint8_t` for use on an 8-bit serial bus. The
// value is packed into the byte in the following order:
// `0,0,R1,G1,B1,R2,G2,B2`.
#define GET_MATRIX_BYTE(red_h, green_h, blue_h, red_l, green_l, blue_l, bit)   \
  (((uint8_t)((red_h & BV_COLOR_##bit) SHIFT_HIGH_BIT_RED_##bit)) |            \
   ((uint8_t)((green_h & BV_COLOR_##bit) SHIFT_HIGH_BIT_GREEN_##bit)) |        \
   ((uint8_t)((blue_h & BV_COLOR_##bit) SHIFT_HIGH_BIT_BLUE_##bit)) |          \
   ((uint8_t)((red_l & BV_COLOR_##bit) SHIFT_LOW_BIT_RED_##bit)) |             \
   ((uint8_t)((green_l & BV_COLOR_##bit) SHIFT_LOW_BIT_GREEN_##bit)) |         \
   ((uint8_t)((blue_l & BV_COLOR_##bit) SHIFT_LOW_BIT_BLUE_##bit)))

// This takes six `uint8_t` values representing two RGB colors – top row (HIGH)
// and bottom row (LOW) – and a `uint8_t` representing which bit to grab from
// each color. It then generates a `uint8_t` for use on an 8-bit serial bus and
// assigns it to the provided `varName`. The value is packed into the byte in
// the following order: `0,0,R1,G1,B1,R2,G2,B2`.
#define SET_MATRIX_BYTE(varName, red_h, green_h, blue_h, red_l, green_l,       \
                        blue_l, bit)                                           \
  switch (bit) {                                                               \
  case 7: {                                                                    \
    varName =                                                                  \
        GET_MATRIX_BYTE(red_h, green_h, blue_h, red_l, green_l, blue_l, 7);    \
    break;                                                                     \
  }                                                                            \
  case 6: {                                                                    \
    varName =                                                                  \
        GET_MATRIX_BYTE(red_h, green_h, blue_h, red_l, green_l, blue_l, 6);    \
    break;                                                                     \
  }                                                                            \
  case 5: {                                                                    \
    varName =                                                                  \
        GET_MATRIX_BYTE(red_h, green_h, blue_h, red_l, green_l, blue_l, 5);    \
    break;                                                                     \
  }                                                                            \
  case 4: {                                                                    \
    varName =                                                                  \
        GET_MATRIX_BYTE(red_h, green_h, blue_h, red_l, green_l, blue_l, 4);    \
    break;                                                                     \
  }                                                                            \
  case 3: {                                                                    \
    varName =                                                                  \
        GET_MATRIX_BYTE(red_h, green_h, blue_h, red_l, green_l, blue_l, 3);    \
    break;                                                                     \
  }                                                                            \
  case 2: {                                                                    \
    varName =                                                                  \
        GET_MATRIX_BYTE(red_h, green_h, blue_h, red_l, green_l, blue_l, 2);    \
    break;                                                                     \
  }                                                                            \
  case 1: {                                                                    \
    varName =                                                                  \
        GET_MATRIX_BYTE(red_h, green_h, blue_h, red_l, green_l, blue_l, 1);    \
    break;                                                                     \
  }                                                                            \
  case 0:                                                                      \
  default: {                                                                   \
    varName =                                                                  \
        GET_MATRIX_BYTE(red_h, green_h, blue_h, red_l, green_l, blue_l, 0);    \
    break;                                                                     \
  }                                                                            \
  }
