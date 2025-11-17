#pragma once

#include <stdbool.h>

#include "esp_err.h"

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

// should be called after the network is initialized
// initializes the SNTP client and starts it
// uses the default NTP server pool "pool.ntp.org"
// configures it to renew servers after getting a new IP address
// and to use the first server from the DHCP response
// sets the IP event to renew servers when the STA gets an IP address
esp_err_t time_util_init();

// updates the global time variable with the current time
void time_util_get(time_util_info_t *time_info);