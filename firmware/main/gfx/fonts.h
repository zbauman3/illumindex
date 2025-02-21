#pragma once

#include <inttypes.h>

#define FONT_SIZE_SM 0
#define FONT_SIZE_MD 1
#define FONT_SIZE_LG 2

#define FONT_ASCII_MIN 32
#define FONT_ASCII_MAX 126

#define fontAsciiToIndex(ascii) (uint8_t)(ascii - FONT_ASCII_MIN)
#define fontIsValidAscii(ascii)                                                \
  (ascii <= FONT_ASCII_MAX && ascii >= FONT_ASCII_MIN)
#define fontIsValidChunk(font, ascii) (chunk <= font->chunksPerChar)

typedef enum {
  fontSizeSm = FONT_SIZE_SM,
  fontSizeMd = FONT_SIZE_MD,
  fontSizeLg = FONT_SIZE_LG,
} FontSize;

typedef struct {
  FontSize size;
  uint8_t width;
  uint8_t height;
  uint8_t chunksPerChar;
  uint8_t bitsPerChunk;
  uint8_t bitPerChar;
  uint16_t color;
  uint8_t spacing;
} Font;

typedef Font *FontHandle;

void fontInit(FontHandle *fontHandle);
void fontSetSize(FontHandle font, FontSize size);
void fontSetColor(FontHandle font, uint16_t color);
void fontEnd(FontHandle font);

uint32_t fontGetChunk(FontHandle font, char ascii, uint8_t chunk);
