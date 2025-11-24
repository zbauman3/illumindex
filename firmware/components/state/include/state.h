#pragma once

#include "esp_err.h"
#include <stdbool.h>

typedef struct {
  char *command_endpoint;
  // if the remote state was not fetched or parsed correctly
  bool invalid_remote_state;
  // if the commands were not fetched or parsed correctly
  bool invalid_commands;
  // if the wifi has disconnected/failed to connect
  bool invalid_wifi_state;
  // incremented for each fetch failure
  uint8_t fetch_failure_count;
  // how often to fetch the remote state, in seconds.
  // compared against fetch_loop_seconds to determine when to fetch.
  uint16_t fetch_interval;
  // how many seconds have passed in the fetch loop.
  // since the loop is 1 second, this is incremented every second.
  uint16_t fetch_loop_seconds;
  // how many seconds have passed in the fetch loop.
  // since the loop is 1 second, this is incremented every second.
  uint16_t remote_state_seconds;
} state_t;

typedef state_t *state_handle_t;

esp_err_t state_init(state_handle_t *state_handle);
void state_end(state_handle_t state);
esp_err_t state_fetch_remote(state_handle_t state);