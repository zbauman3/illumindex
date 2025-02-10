#pragma once

#include "esp_attr.h"
#include <stdint.h>

/**
 * This performs a blocking delay via `nop` instructions using
 * `esp_timer_get_time` until the desired delay has passed.
 */
void IRAM_ATTR delayMicroseconds(uint32_t us);