#pragma once

#include <inttypes.h>

#define FONT_SIZE_SM 0
#define FONT_SIZE_MD 1
#define FONT_SIZE_LG 2

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
} Font;

typedef Font *FontHandle;

void fontInit(FontHandle *fontHandle, FontSize size);
uint32_t fontGetChunk(FontHandle font, uint16_t index);
void fontSet(FontHandle font, FontSize size);
void fontEnd(FontHandle font);
