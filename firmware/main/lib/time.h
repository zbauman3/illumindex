#pragma once

#include <sys/time.h>
#include <time.h>

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
  int dayOfWeek;
  int dayOfYear;
  bool isDST;
} TimeInfo;

typedef TimeInfo *TimeInfoHandle;

// should be called after the network is initialized
// initializes the SNTP client and starts it
// uses the default NTP server pool "pool.ntp.org"
// configures it to renew servers after getting a new IP address
// and to use the first server from the DHCP response
// sets the IP event to renew servers when the STA gets an IP address
esp_err_t time_init();

// gets the current time and fills the provided TimeInfo structure
void time_get(TimeInfoHandle timeInfo);