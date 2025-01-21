
#include "util/time.h" // for macros

#include "freertos/task.h"

void IRAM_ATTR delayMicroseconds(uint32_t us) {
  uint32_t currentUs = micros();
  if (us) {
    uint32_t end = (currentUs + us);

    // if overflow
    if (currentUs > end) {
      while (MICROS() > end) {
        asm volatile("nop");
      }
    }

    while (MICROS() < end) {
      asm volatile("nop");
    }
  }
}

void IRAM_ATTR delayMicrosecondsYield(uint32_t us) {
  uint32_t currentUs = micros();
  taskYIELD();
  if (us) {
    uint32_t end = (currentUs + us);

    // if overflow
    if (currentUs > end) {
      while (MICROS() > end) {
        taskYIELD();
      }
    }

    while (MICROS() < end) {
      taskYIELD();
    }
  }
}