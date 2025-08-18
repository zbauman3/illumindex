#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"

#include "lib/time.h"
#include "util/error_helpers.h"

static const char *TAG = "TIME";

esp_err_t timeInit() {
  ESP_LOGD(TAG, "Initializing time module");
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  config.start = true;
  config.server_from_dhcp = false;
  config.renew_servers_after_new_IP = true;
  config.index_of_first_server = 0;
  config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;

  ESP_ERROR_BUBBLE(esp_netif_sntp_init(&config));

  setenv("TZ", CONFIG_TIMEZONE_STRING, 1);
  tzset();

  return ESP_OK;
}

void timeGet(TimeInfo *timeInfo) {
  time_t now;
  struct tm cTimeinfo;
  time(&now);
  localtime_r(&now, &cTimeinfo);

  timeInfo->second = cTimeinfo.tm_sec;
  timeInfo->minute = cTimeinfo.tm_min;
  timeInfo->hour12 = cTimeinfo.tm_hour % 12;
  timeInfo->isPM = cTimeinfo.tm_hour >= 12;
  timeInfo->hour24 = cTimeinfo.tm_hour;
  timeInfo->dayOfMonth = cTimeinfo.tm_mday;
  timeInfo->month = cTimeinfo.tm_mon + 1;    // tm_mon is 0-11
  timeInfo->year = cTimeinfo.tm_year + 1900; // tm_year is years since 1900
  timeInfo->dayOfWeek =
      cTimeinfo.tm_wday + 1; // tm_wday is 0-6, starting at Sunday
  timeInfo->dayOfYear =
      cTimeinfo.tm_yday + 1; // tm_yday is days since Jan 1, starting at 0
  timeInfo->isDST = cTimeinfo.tm_isdst != 0; // Daylight Saving Time flag
}