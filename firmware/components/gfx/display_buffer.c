#include <math.h>
#include <memory.h>

#include "esp_log.h"

#include "color_utils.h"
#include "helper_utils.h"

#include "gfx/display_buffer.h"
#include "gfx/font.h"

const static char *TAG = "GFX:DISPLAY_BUFFER";

// allocates all memory required for the display_buffer_t and initiates the
// values
esp_err_t display_buffer_init(display_buffer_handle_t *db_handle, uint8_t width,
                              uint8_t height) {
  display_buffer_handle_t db =
      (display_buffer_handle_t)malloc(sizeof(display_buffer_t));
  if (db == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for display buffer");
    return ESP_ERR_NO_MEM;
  }

  display_buffer_set_color(db, 255, 255, 255);
  display_buffer_set_cursor(db, 0, 0);
  db->width = width;
  db->height = height;
  db->length = db->width * db->height;

  db->buffer_red = (uint8_t *)malloc(sizeof(uint8_t) * db->length);
  db->buffer_green = (uint8_t *)malloc(sizeof(uint8_t) * db->length);
  db->buffer_blue = (uint8_t *)malloc(sizeof(uint8_t) * db->length);

  if (db->buffer_red == NULL || db->buffer_green == NULL ||
      db->buffer_blue == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for display buffer colors");
    free(db->buffer_red);
    free(db->buffer_green);
    free(db->buffer_blue);
    free(db);
    return ESP_ERR_NO_MEM;
  }

  display_buffer_clear(db);

  ESP_ERROR_BUBBLE(font_init(&db->font));

  *db_handle = db;

  return ESP_OK;
}

// resets all values in the buffer to `0`
void display_buffer_clear(display_buffer_handle_t db) {
  memset(db->buffer_red, 0, sizeof(uint8_t) * db->length);
  memset(db->buffer_green, 0, sizeof(uint8_t) * db->length);
  memset(db->buffer_blue, 0, sizeof(uint8_t) * db->length);
}

// cleans up all memory associated with the buffer
void display_buffer_end(display_buffer_handle_t db) {
  font_end(db->font);
  free(db->buffer_red);
  free(db->buffer_green);
  free(db->buffer_blue);
  free(db);
}

// This will apply the provided string to the buffer, using the buffer's current
// cursor and font. The string will be wrapped until it is out of the matrix.
void display_buffer_draw_string(display_buffer_handle_t db, char *string) {
  const size_t stringLength = strlen(string);
  // the index within the string we are working on
  uint16_t stringIndex;
  // the actual ascii character we are working on
  char ascii_char;
  // the starting point to draw bits at in the buffer
  uint16_t bufferStartIdx;
  // which bit of the font->width we are on working on
  uint8_t bitmapRowIdx;
  // the index of the character's bitmap chunk we are working on
  uint8_t chunkIdx;
  // the actual chunk value, pulled once to be used multiple
  uint32_t chunkVal;
  // the number of the bit we are working on, within the chunk. This is the
  // _number_, not the zero-based index
  uint8_t chunkBitN;

  // loop all the characters in the string
  for (stringIndex = 0; stringIndex < stringLength; stringIndex++) {
    ascii_char = string[stringIndex];
    bitmapRowIdx = 0;
    // convert the buffers cursor to an index, where we start this char
    bufferStartIdx = display_buffer_cursor_to_index(db);

    // If we have moved past the buffer's space, we can go ahead and end
    if (!display_buffer_cursor_is_visible(db)) {
      return;
    }

    // if not a character that we support, change to `?`
    if (!font_is_valid_ascii(ascii_char)) {
      ESP_LOGW(TAG, "Unsupported ASCII character \"%d\"", ascii_char);
      ascii_char = 63;
    }

    for (chunkIdx = 0; chunkIdx < db->font->chunks_per_char; chunkIdx++) {
      chunkVal = font_get_chunk(db->font, ascii_char, chunkIdx);

      for (chunkBitN = 1; chunkBitN <= db->font->bits_per_chunk; chunkBitN++) {
        // mask the chunk bit, then AND it to the chunk value. Use the result as
        // a boolean to check if we should set the value to the color or blank
        if (chunkVal & _BV_1ULL(db->font->bits_per_chunk - chunkBitN)) {
          display_buffer_safe_set_value(db, bufferStartIdx + bitmapRowIdx,
                                        db->color_red, db->color_green,
                                        db->color_blue);
        } else {
          display_buffer_safe_set_value(db, bufferStartIdx + bitmapRowIdx, 0, 0,
                                        0);
        }

        // increase the rows index. Move down a line if at the end
        bitmapRowIdx++;
        if (bitmapRowIdx == db->font->width) {
          bitmapRowIdx = 0;
          bufferStartIdx += db->width;
        }
      }
    }

    // we're done with this character, move to the next position.
    display_buffer_next_char_wrap(db);
  }
}

void display_buffer_draw_vert_line(display_buffer_handle_t db, uint8_t to) {
  // faster to write it twice ¯\_(ツ)_/¯
  if (db->cursor.y > to) {
    while (db->cursor.y > to) {
      if (display_buffer_point_is_visible(db, db->cursor.x, db->cursor.y)) {
        display_buffer_safe_set_value(
            db, display_buffer_point_to_index(db, db->cursor.x, db->cursor.y),
            db->color_red, db->color_green, db->color_blue);
      }

      db->cursor.y--;
    }
  } else {
    while (db->cursor.y < to) {
      if (display_buffer_point_is_visible(db, db->cursor.x, db->cursor.y)) {
        display_buffer_safe_set_value(
            db, display_buffer_point_to_index(db, db->cursor.x, db->cursor.y),
            db->color_red, db->color_green, db->color_blue);
      }

      db->cursor.y++;
    }
  }
  if (display_buffer_point_is_visible(db, db->cursor.x, db->cursor.y)) {
    display_buffer_safe_set_value(
        db, display_buffer_point_to_index(db, db->cursor.x, db->cursor.y),
        db->color_red, db->color_green, db->color_blue);
  }

  display_buffer_set_cursor(db, db->cursor.x, to);
}

void display_buffer_draw_horiz_line(display_buffer_handle_t db, uint8_t to) {
  // faster to write it twice ¯\_(ツ)_/¯
  if (db->cursor.x > to) {
    while (db->cursor.x >= to) {
      if (display_buffer_point_is_visible(db, db->cursor.x, db->cursor.y)) {
        display_buffer_safe_set_value(
            db, display_buffer_point_to_index(db, db->cursor.x, db->cursor.y),
            db->color_red, db->color_green, db->color_blue);
      }

      db->cursor.x--;
    }
  } else {
    while (db->cursor.x <= to) {
      if (display_buffer_point_is_visible(db, db->cursor.x, db->cursor.y)) {
        display_buffer_safe_set_value(
            db, display_buffer_point_to_index(db, db->cursor.x, db->cursor.y),
            db->color_red, db->color_green, db->color_blue);
      }

      db->cursor.x++;
    }
  }

  display_buffer_set_cursor(db, to, db->cursor.y);
}

void display_buffer_draw_diag_line(display_buffer_handle_t db, uint8_t to_x,
                                   uint8_t to_y) {
  float change =
      ((float)db->cursor.y - (float)to_y) / ((float)db->cursor.x - (float)to_x);
  float unroundedY = (float)db->cursor.y;

  // faster to write it twice ¯\_(ツ)_/¯
  if (db->cursor.x <= to_x) {
    while (db->cursor.x <= to_x) {
      if (display_buffer_point_is_visible(db, db->cursor.x,
                                          (uint8_t)round(unroundedY))) {
        display_buffer_safe_set_value(
            db,
            display_buffer_point_to_index(db, db->cursor.x,
                                          (uint8_t)round(unroundedY)),
            db->color_red, db->color_green, db->color_blue);
      }

      db->cursor.x++;
      unroundedY += change;
    }
  } else {
    while (db->cursor.x >= to_x) {
      if (display_buffer_point_is_visible(db, db->cursor.x,
                                          (uint8_t)round(unroundedY))) {
        display_buffer_safe_set_value(
            db,
            display_buffer_point_to_index(db, db->cursor.x,
                                          (uint8_t)round(unroundedY)),
            db->color_red, db->color_green, db->color_blue);
      }

      db->cursor.x--;
      unroundedY -= change;
    }
  }

  display_buffer_set_cursor(db, to_x, to_y);
}

// if you know the shape of your line already, you can save a few clocks by
// using the above `displayBufferDrawFast*` versions
void display_buffer_draw_line(display_buffer_handle_t db, uint8_t to_x,
                              uint8_t to_y) {
  if (db->cursor.x == to_x) {
    display_buffer_draw_vert_line(db, to_y);
  } else if (db->cursor.y == to_y) {
    display_buffer_draw_horiz_line(db, to_x);
  } else {
    display_buffer_draw_diag_line(db, to_x, to_y);
  }
}

void display_buffer_draw_bitmap(display_buffer_handle_t db, uint8_t width,
                                uint8_t height, uint8_t *buffer_red,
                                uint8_t *buffer_green, uint8_t *buffer_blue,
                                bool draw_black) {
  for (uint8_t row = 0; row < height; row++) {
    for (uint8_t col = 0; col < width; col++) {
      if (draw_black == false && buffer_red[(row * width) + col] == 0 &&
          buffer_green[(row * width) + col] == 0 &&
          buffer_blue[(row * width) + col] == 0) {
        continue;
      }

      display_buffer_safe_set_value(
          db,
          display_buffer_point_to_index(db, db->cursor.x + col,
                                        db->cursor.y + row),
          buffer_red[(row * width) + col], buffer_green[(row * width) + col],
          buffer_blue[(row * width) + col]);
    }
  }
}

void display_buffer_draw_graph(display_buffer_handle_t db, uint8_t width,
                               uint8_t height, uint8_t *values,
                               uint8_t bg_color_red, uint8_t bg_color_green,
                               uint8_t bg_color_blue) {
  uint8_t cursorStartX = db->cursor.x;
  uint8_t cursorStartY = db->cursor.y;

  uint8_t prevColorRed = db->color_red;
  uint8_t prevColorGreen = db->color_green;
  uint8_t prevColorBlue = db->color_blue;

  // first draw the background
  display_buffer_set_color(db, bg_color_red, bg_color_green, bg_color_blue);
  for (uint8_t y = 0; y < height; y++) {
    display_buffer_set_cursor(db, cursorStartX, cursorStartY + y);
    display_buffer_draw_horiz_line(db, cursorStartX + width - 1);
  }

  // reset the color
  display_buffer_set_color(db, prevColorRed, prevColorGreen, prevColorBlue);

  // now draw the values
  for (uint8_t x = 0; x < width; x++) {
    uint8_t value = MAX(MIN(values[x], height), 0);
    if (value == 0) {
      continue;
    }
    display_buffer_set_cursor(db, cursorStartX + x, cursorStartY + height - 1);
    display_buffer_draw_vert_line(db, cursorStartY + height - value);
  }
}

void display_buffer_add_feedback(display_buffer_handle_t db,
                                 bool invalid_remote_state,
                                 bool invalid_commands, bool invalid_wifi_state,
                                 bool is_dev_mode) {
  if (is_dev_mode) {
    display_buffer_safe_set_value(db, 0, 0, 0, 255);
  }

  if (invalid_remote_state) {
    display_buffer_safe_set_value(db, 2, 0, 255, 0);
  }

  if (invalid_wifi_state || invalid_commands) {
    uint8_t originalCursorX = db->cursor.x;
    uint8_t originalCursorY = db->cursor.y;
    uint8_t green_blue[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    db->cursor.x = db->width - 10;
    db->cursor.y = db->height - 10;

    if (invalid_wifi_state) {

      uint8_t red[] = {0,   0,   0,   0,   200, 0, 0,   0,   0,   0,   0, 200,
                       200, 0,   200, 200, 0,   0, 200, 200, 0,   0,   0, 0,
                       0,   200, 200, 0,   0,   0, 200, 200, 200, 0,   0, 0,
                       0,   200, 200, 0,   0,   0, 200, 200, 0,   0,   0, 0,
                       0,   200, 0,   0,   0,   0, 0,   0,   200, 200, 0, 200,
                       200, 0,   0,   0,   0,   0, 0,   0,   0,   0,   0, 0,
                       0,   0,   0,   0,   200, 0, 0,   0,   0};
      display_buffer_draw_bitmap(db, 9, 9, red, green_blue, green_blue, false);
    } else {
      uint8_t red[] = {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   200, 0,   0,
          0,   0,   0,   0,   0,   156, 200, 200, 0,   200, 200, 0,   0,   0,
          156, 156, 200, 200, 156, 200, 200, 0,   156, 156, 156, 156, 156, 156,
          156, 200, 200, 156, 156, 156, 156, 156, 156, 156, 200, 200, 0,   156,
          156, 156, 156, 156, 156, 156, 0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};
      display_buffer_draw_bitmap(db, 9, 9, red, green_blue, green_blue, false);
    }

    db->cursor.x = originalCursorX;
    db->cursor.y = originalCursorY;
  }
}