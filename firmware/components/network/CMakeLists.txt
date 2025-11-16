idf_component_register(
  SRCS "wifi.c" "fetch.c"
  INCLUDE_DIRS "include"
  REQUIRES "esp_http_client"
  PRIV_REQUIRES "esp_wifi" "mbedtls" "esp_event" "esp-tls" "util"
  REQUIRED_IDF_TARGETS esp32s3
)