#pragma once

#include "esp_err.h"
#include <stdbool.h>

typedef struct {
  bool isDevMode;
  char *commandEndpoint;
  uint16_t fetchInterval;
} RemoteState;

typedef RemoteState *RemoteStateHandle;

esp_err_t remoteStateInit(RemoteStateHandle *remoteStateHandle);
esp_err_t remoteStateParse(RemoteStateHandle remoteState, char *data,
                           size_t length);