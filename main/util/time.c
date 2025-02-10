
#include "esp_timer.h"

void delayMicroseconds(uint32_t us) {
  uint64_t m = (uint64_t)esp_timer_get_time();
  if (us) {
    uint64_t e = (m + us);
    if (m > e) { // overflow
      while ((uint64_t)esp_timer_get_time() > e) {
        asm volatile("nop");
      }
    }
    while ((uint64_t)esp_timer_get_time() < e) {
      asm volatile("nop");
    }
  }
}
