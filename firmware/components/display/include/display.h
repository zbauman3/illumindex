#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_err.h"
#include <stdbool.h>

#include "commands.h"
#include "gfx/display_buffer.h"
#include "led_matrix.h"
#include "state.h"

typedef struct {
  led_matrix_handle_t matrix;
  display_buffer_handle_t display_buffer;
  state_handle_t state;
  TaskHandle_t main_task_handle;
  TaskHandle_t animation_task_handle;
  char *last_etag;
  command_list_handle_t commands;
} display_t;

typedef display_t *display_handle_t;

esp_err_t display_init(display_handle_t *display_handle,
                       led_matrix_config_t *led_matrix_config);
void display_end(display_handle_t display);
esp_err_t display_start(display_handle_t display);