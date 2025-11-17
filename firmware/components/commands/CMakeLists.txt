idf_component_register(
  SRCS "commands.c"
  INCLUDE_DIRS "include"
  REQUIRES "gfx" "json"
  PRIV_REQUIRES "time_util"
)