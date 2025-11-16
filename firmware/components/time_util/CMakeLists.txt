idf_component_register(
  SRCS "time_util.c"
  INCLUDE_DIRS "include"
  PRIV_REQUIRES "util" "esp_netif" "lwip"
)