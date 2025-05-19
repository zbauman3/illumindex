#pragma once

#include "esp_err.h"
#include <stdbool.h>

#include "drivers/matrix.h"
#include "gfx/displayBuffer.h"
#include "lib/state.h"

typedef struct {
  MatrixHandle matrix;
  DisplayBufferHandle displayBuffer;
  StateHandle state;
} Display;

typedef Display *DisplayHandle;

esp_err_t displayInit(DisplayHandle *displayHandle,
                      MatrixInitConfig *matrixConfig);

void displayEnd(DisplayHandle display);
void displaySetup(DisplayHandle display);
void displayLoop(DisplayHandle display);