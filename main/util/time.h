#pragma once

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include <stdint.h>

#define micros() (uint32_t)(esp_timer_get_time())

/**
 * This performs a blocking delay via `nop` instructions using
 * `esp_timer_get_time` until the desired delay has passed.
 */
void IRAM_ATTR delayMicroseconds(uint32_t us);

/**
 * This performs a semi-blocking delay via `taskYIELD` using
 * `esp_timer_get_time` until the desired delay has passed. This allows tasks of
 * equal or greater priority to run while this tasks waits in an otherwise
 * blocking manor. Using `taskYIELD` means that the delay time is a _minimum_
 * time to delay, the actual time may be longer depending on other tasks that
 * may run.
 */
void IRAM_ATTR delayMicrosecondsYield(uint32_t us);
