#pragma once

#include "esp_err.h"

#define ESP_ERROR_BUBBLE(x)                                                    \
  ({                                                                           \
    esp_err_t err = x;                                                         \
    if (err != ESP_OK) {                                                       \
      return err;                                                              \
    }                                                                          \
  })