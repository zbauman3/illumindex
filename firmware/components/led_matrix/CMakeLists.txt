idf_component_register(
  SRCS "led_matrix.c"
  INCLUDE_DIRS "include"
  REQUIRES "driver" "esp_timer" "util"
  REQUIRED_IDF_TARGETS esp32s3
)