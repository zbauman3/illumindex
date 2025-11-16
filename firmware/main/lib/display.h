#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_err.h"
#include <stdbool.h>

#include "gfx/display_buffer.h"
#include "led_matrix.h"

#include "lib/commands.h"
#include "lib/state.h"

typedef struct {
  led_matrix_handle_t matrix;
  display_buffer_handle_t displayBuffer;
  state_handle_t state;
  TaskHandle_t mainTaskHandle;
  TaskHandle_t animationTaskHandle;
  char *lastEtag;
  command_list_handle_t commands;
} display_t;

typedef display_t *display_handle_t;

esp_err_t display_init(display_handle_t *displayHandle,
                       led_matrix_config_t *matrixConfig);
void display_end(display_handle_t display);
esp_err_t display_start(display_handle_t display);