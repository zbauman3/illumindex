#pragma once

#include <stdbool.h>

#include "esp_err.h"

#include "gfx/font.h"

// -------- Shared across all commands

// 1000MS
#define COMMAND_CONFIG_ANIMATION_DELAY_DEFAULT 1000

typedef struct {
  uint16_t animation_delay;
} command_config_t;

#define COMMAND_STATE_FLAGS_COLOR (1 << 0)
#define COMMAND_STATE_FLAGS_POSITION (1 << 1)
#define COMMAND_STATE_FLAGS_FONT (1 << 2)

#define command_state_has_color(state)                                         \
  ((bool)((state)->flags & COMMAND_STATE_FLAGS_COLOR))
#define command_state_has_position(state)                                      \
  ((bool)((state)->flags & COMMAND_STATE_FLAGS_POSITION))
#define command_state_has_font(state)                                          \
  ((bool)((state)->flags & COMMAND_STATE_FLAGS_FONT))

#define command_state_set_flag_color(state)                                    \
  (state)->flags |= COMMAND_STATE_FLAGS_COLOR;
#define command_state_set_flag_position(state)                                 \
  (state)->flags |= COMMAND_STATE_FLAGS_POSITION;
#define command_state_set_flag_font(state)                                     \
  (state)->flags |= COMMAND_STATE_FLAGS_FONT;
#define command_state_clear_flag_color(state)                                  \
  (state)->flags &= ~COMMAND_STATE_FLAGS_COLOR;
#define command_state_clear_flag_position(state)                               \
  (state)->flags &= ~COMMAND_STATE_FLAGS_POSITION;
#define command_state_clear_flag_font(state)                                   \
  (state)->flags &= ~COMMAND_STATE_FLAGS_FONT;

typedef struct {
  uint8_t flags;
  uint8_t color_red;
  uint8_t color_green;
  uint8_t color_blue;
  uint8_t pos_x;
  uint8_t pos_y;
  font_size_t font_size;
} command_state_t;

#define COMMAND_TYPE_STRING 0
#define COMMAND_TYPE_LINE 1
#define COMMAND_TYPE_BITMAP 2
#define COMMAND_TYPE_SETSTATE 3
#define COMMAND_TYPE_LINEFEED 4
#define COMMAND_TYPE_ANIMATION 5
#define COMMAND_TYPE_TIME 6
#define COMMAND_TYPE_DATE 7
#define COMMAND_TYPE_GRAPH 8

typedef enum {
  type_string = COMMAND_TYPE_STRING,
  type_line = COMMAND_TYPE_LINE,
  type_bitmap = COMMAND_TYPE_BITMAP,
  type_set_state = COMMAND_TYPE_SETSTATE,
  type_line_feed = COMMAND_TYPE_LINEFEED,
  type_animation = COMMAND_TYPE_ANIMATION,
  type_time = COMMAND_TYPE_TIME,
  type_date = COMMAND_TYPE_DATE,
  type_graph = COMMAND_TYPE_GRAPH,
} command_type_enum_t;

// -------- Individual Commands

typedef struct {
  command_state_t *state;
  char *value;
} command_value_string_t;

typedef struct {
  command_state_t *state;
  uint8_t to_x;
  uint8_t to_y;
} command_value_line_t;

typedef struct {
  command_state_t *state;
  uint8_t height;
  uint8_t width;
  uint8_t *data_red;
  uint8_t *data_green;
  uint8_t *data_blue;
} command_value_bitmap_t;

typedef struct {
  command_state_t *state;
} command_value_set_state_t;

typedef struct {
  // nothing
} command_value_line_feed_t;

typedef struct {
  uint16_t frame_count;
  uint16_t last_show_frame;
  // have to use the full struct here due to the typedef not being defined yet
  struct command_list_t **frames;
} command_value_animation_t;

typedef struct {
  command_state_t *state;
} command_value_time_t;

typedef struct {
  command_state_t *state;
} command_value_date_t;

typedef struct {
  command_state_t *state;
  uint8_t height;
  uint8_t width;
  uint8_t *values;
  uint8_t bg_color_red;
  uint8_t bg_color_green;
  uint8_t bg_color_blue;
} command_value_graph_t;

// -------- high-level usage structs/fns

typedef union {
  command_value_string_t *string;
  command_value_line_t *line;
  command_value_bitmap_t *bitmap;
  command_value_set_state_t *set_state;
  command_value_line_feed_t *line_feed;
  command_value_animation_t *animation;
  command_value_time_t *time;
  command_value_date_t *date;
  command_value_graph_t *graph;
} command_values_union_t;

typedef struct {
  command_type_enum_t type;
  command_values_union_t value;
} command_t;

typedef command_t *command_handle_t;

typedef struct command_list_node_t {
  command_handle_t command;
  struct command_list_node_t *next;
} command_list_node_t;

typedef struct command_list_t {
  command_config_t config;
  command_list_node_t *head;
  command_list_node_t *tail;
} command_list_t;

typedef command_list_t *command_list_handle_t;

esp_err_t command_state_init(command_state_t **state_handle);
esp_err_t command_list_init(command_list_handle_t *command_list_handle);
esp_err_t command_list_node_init(command_list_handle_t command_list,
                                 command_type_enum_t type,
                                 command_handle_t *command_handle);
void command_list_end(command_list_handle_t command_list);
esp_err_t command_list_parse(command_list_handle_t *command_list_handle,
                             char *data, size_t length);