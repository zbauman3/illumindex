
#include "util/time.h" // for macros

#include "freertos/task.h"

void delayMicroseconds(uint32_t us) {
  uint32_t currentUs = micros();
  if (us) {
    uint32_t end = (currentUs + us);

    // if overflow
    if (currentUs > end) {
      while (micros() > end) {
        asm volatile("nop");
      }
    }

    while (micros() < end) {
      asm volatile("nop");
    }
  }
}

void delayMicrosecondsYield(uint32_t us) {
  uint32_t currentUs = micros();
  taskYIELD();
  if (us) {
    uint32_t end = (currentUs + us);

    // if overflow
    if (currentUs > end) {
      while (micros() > end) {
        taskYIELD();
      }
    }

    while (micros() < end) {
      taskYIELD();
    }
  }
}