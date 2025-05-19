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
} State;

typedef State *StateHandle;

esp_err_t stateInit(StateHandle *stateHandle);
void stateEnd(StateHandle state);
esp_err_t stateFetchRemote(StateHandle state);