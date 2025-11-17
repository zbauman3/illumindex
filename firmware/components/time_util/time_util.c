#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include <sys/time.h>
#include <time.h>

#include "helper_utils.h"

#include "time_util.h"

static const char *TAG = "TIME_UTIL";

// initialize the time module, setting up SNTP and timezone
esp_err_t time_util_init() {
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

void time_util_get(time_util_info_t *time_info) {
  time_t now;
  struct tm cTimeinfo;
  time(&now);
  localtime_r(&now, &cTimeinfo);

  time_info->second = cTimeinfo.tm_sec;
  time_info->minute = cTimeinfo.tm_min;
  time_info->hour12 = cTimeinfo.tm_hour % 12;
  time_info->isPM = cTimeinfo.tm_hour >= 12;
  time_info->hour24 = cTimeinfo.tm_hour;
  time_info->dayOfMonth = cTimeinfo.tm_mday;
  time_info->month = cTimeinfo.tm_mon + 1;    // tm_mon is 0-11
  time_info->year = cTimeinfo.tm_year + 1900; // tm_year is years since 1900
  time_info->dayOfWeek =
      cTimeinfo.tm_wday + 1; // tm_wday is 0-6, starting at Sunday
  time_info->dayOfYear =
      cTimeinfo.tm_yday + 1; // tm_yday is days since Jan 1, starting at 0
  time_info->isDST = cTimeinfo.tm_isdst != 0; // Daylight Saving Time flag
}