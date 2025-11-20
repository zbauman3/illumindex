idf_component_register(
  SRCS "display.c"
  INCLUDE_DIRS "include"
  REQUIRES "commands" "gfx" "led_matrix" "state"
  PRIV_REQUIRES "network" "time_util" "util"
)