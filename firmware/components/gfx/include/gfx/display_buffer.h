#pragma once

#include "esp_err.h"
#include <inttypes.h>
#include <stdbool.h>

#include "gfx/font.h"

// validates that setting an index in the buffer is not an overflow
#define display_buffer_safe_set_value(db, index, red, green, blue)             \
  ({                                                                           \
    if ((index) < db->length) {                                                \
      db->buffer_red[(index)] = (red);                                         \
      db->buffer_green[(index)] = (green);                                     \
      db->buffer_blue[(index)] = (blue);                                       \
    }                                                                          \
  })

// moves one character and wraps if needed, but does not check that the new row
// (y) is within range
#define display_buffer_next_char_wrap(db)                                      \
  ({                                                                           \
    if (db->cursor.x + (db->font->width * 2) + db->font->spacing <=            \
        db->width) {                                                           \
      db->cursor.x += db->font->width + db->font->spacing;                     \
    } else {                                                                   \
      display_buffer_line_feed(db);                                            \
    }                                                                          \
  })

#define display_buffer_cursor_to_index(db)                                     \
  display_buffer_point_to_index(db, db->cursor.x, db->cursor.y)

#define display_buffer_point_to_index(db, x, y)                                \
  ((uint16_t)(((y) * db->width) + (x)))

#define display_buffer_cursor_is_visible(db)                                   \
  display_buffer_point_is_visible(db, db->cursor.x, db->cursor.y)

#define display_buffer_point_is_visible(db, x, y)                              \
  ((bool)((x) < db->width && (y) < db->height))

#define display_buffer_set_cursor(db, sx, sy)                                  \
  ({                                                                           \
    db->cursor.x = (sx);                                                       \
    db->cursor.y = (sy);                                                       \
  })

#define display_buffer_set_color(db, red, green, blue)                         \
  ({                                                                           \
    db->color_red = (red);                                                     \
    db->color_green = (green);                                                 \
    db->color_blue = (blue);                                                   \
  })

#define display_buffer_line_feed(db)                                           \
  ({                                                                           \
    db->cursor.x = 0;                                                          \
    db->cursor.y += db->font->height;                                          \
  })

typedef struct {
  uint8_t *buffer_red;
  uint8_t *buffer_green;
  uint8_t *buffer_blue;
  uint8_t width;
  uint8_t height;
  uint16_t length;
  font_handle_t font;
  // current drawing color red
  uint8_t color_red;
  // current drawing color green
  uint8_t color_green;
  // current drawing color blue
  uint8_t color_blue;
  struct {
    uint8_t x;
    uint8_t y;
  } cursor;
} display_buffer_t;

typedef display_buffer_t *display_buffer_handle_t;

esp_err_t display_buffer_init(display_buffer_handle_t *db_handle, uint8_t width,
                              uint8_t height);
void display_buffer_end(display_buffer_handle_t db_handle);
void display_buffer_clear(display_buffer_handle_t db_handle);
void display_buffer_draw_string(display_buffer_handle_t db, char *string);
void display_buffer_draw_vert_line(display_buffer_handle_t db, uint8_t to);
void display_buffer_draw_horiz_line(display_buffer_handle_t db, uint8_t to);
void display_buffer_draw_diag_line(display_buffer_handle_t db, uint8_t to_x,
                                   uint8_t to_y);
void display_buffer_draw_line(display_buffer_handle_t db, uint8_t to_x,
                              uint8_t to_y);
void display_buffer_draw_bitmap(display_buffer_handle_t db, uint8_t width,
                                uint8_t height, uint8_t *buffer_red,
                                uint8_t *buffer_green, uint8_t *buffer_blue,
                                bool draw_black);
void display_buffer_draw_graph(display_buffer_handle_t db, uint8_t width,
                               uint8_t height, uint8_t *values,
                               uint8_t bg_color_red, uint8_t bg_color_green,
                               uint8_t bg_color_blue);
void display_buffer_add_feedback(display_buffer_handle_t db,
                                 bool invalid_remote_state,
                                 bool invalid_commands,
                                 bool invalid_wifi_state);
