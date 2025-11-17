#pragma once

#include "esp_err.h"
#include <stdbool.h>

typedef struct {
  bool is_dev_mode;
  char *commandEndpoint;
  uint16_t fetchInterval;
  bool remote_state_invalid;
  bool commands_invalid;
  uint16_t loopSeconds;
  uint8_t fetchFailureCount;
} state_t;

typedef state_t *state_handle_t;

esp_err_t state_init(state_handle_t *state_handle);
void state_end(state_handle_t state);
esp_err_t state_fetch_remote(state_handle_t state);