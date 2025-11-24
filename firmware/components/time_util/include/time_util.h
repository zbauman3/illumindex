#pragma once

#include <stdbool.h>

#include "esp_err.h"

// Just a nicer wrapper around `struct tm`.
typedef struct {
  int second;
  int minute;
  int hour12;
  int hour24;
  bool isPM;
  int dayOfMonth;
  int month;
  int year;
  // this is the day of the week (1-7, Sunday-Saturday)
  int dayOfWeek;
  int dayOfYear;
  bool isDST;
} time_util_info_t;

esp_err_t time_util_init();

void time_util_get(time_util_info_t *time_info);