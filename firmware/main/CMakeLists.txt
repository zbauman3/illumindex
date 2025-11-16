idf_component_register(
  SRCS "main.c" "lib/commands.c" "lib/state.c" "lib/display.c"
  INCLUDE_DIRS "."
  REQUIRED_IDF_TARGETS esp32s3
)