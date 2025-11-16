#pragma once

#include "esp_err.h"
#include <stdbool.h>

typedef struct {
  bool isDevMode;
  char *commandEndpoint;
  uint16_t fetchInterval;
  bool remoteStateInvalid;
  bool commandsInvalid;
  uint16_t loopSeconds;
  uint8_t fetchFailureCount;
} state_t;

typedef state_t *state_handle_t;

esp_err_t state_init(state_handle_t *stateHandle);
void state_end(state_handle_t state);
esp_err_t state_fetch_remote(state_handle_t state);