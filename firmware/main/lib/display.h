#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_err.h"
#include <stdbool.h>

#include "drivers/matrix.h"
#include "gfx/displayBuffer.h"
#include "lib/state.h"

typedef struct {
  MatrixHandle matrix;
  DisplayBufferHandle displayBuffer;
  StateHandle state;
  TaskHandle_t mainTaskHandle;
  TaskHandle_t animationTaskHandle;
  char *lastEtag;
} Display;

typedef Display *DisplayHandle;

esp_err_t displayInit(DisplayHandle *displayHandle,
                      MatrixInitConfig *matrixConfig);

void displayEnd(DisplayHandle display);
esp_err_t displayStart(DisplayHandle display);