#pragma once

#include "esp_err.h"
#include <stdbool.h>

typedef struct {
  bool is_dev_mode;
  char *command_endpoint;
  uint16_t fetch_interval;
  bool invalid_remote_state;
  bool invalid_commands;
  bool invalid_wifi_state;
  uint16_t loop_seconds;
  uint8_t fetch_failure_count;
  uint8_t wifi_failure_count;
} state_t;

typedef state_t *state_handle_t;

esp_err_t state_init(state_handle_t *state_handle);
void state_end(state_handle_t state);
esp_err_t state_fetch_remote(state_handle_t state);