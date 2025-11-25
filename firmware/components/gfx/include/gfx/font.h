#pragma once

#include <inttypes.h>

#include "esp_err.h"

#define FONT_SIZE_SM 0
#define FONT_SIZE_MD 1
#define FONT_SIZE_LG 2

// supported ASCII range min
#define FONT_ASCII_MIN 32
// supported ASCII range max
#define FONT_ASCII_MAX 126

#define font_ascii_to_index(ascii) (uint8_t)(ascii - FONT_ASCII_MIN)
#define font_is_valid_ascii(ascii)                                             \
  (ascii <= FONT_ASCII_MAX && ascii >= FONT_ASCII_MIN)
#define font_is_valid_chunk(font, ascii) ((ascii) <= (font)->chunks_per_char)

typedef enum {
  font_size_sm = FONT_SIZE_SM,
  font_size_md = FONT_SIZE_MD,
  font_size_lg = FONT_SIZE_LG,
} font_size_t;

typedef struct {
  font_size_t size;
  uint8_t width;
  uint8_t height;
  uint8_t chunks_per_char;
  uint8_t bits_per_chunk;
  uint8_t bits_per_char;
  uint8_t spacing;
} font_t;

typedef font_t *font_handle_t;

esp_err_t font_init(font_handle_t *font_handle);
void font_set_size(font_handle_t font, font_size_t size);
void font_end(font_handle_t font);
uint32_t font_get_chunk(font_handle_t font, char ascii, uint8_t chunk);
