#include <math.h>
#include <memory.h>

#include "esp_log.h"

#include "color_utils.h"
#include "gfx/display_buffer.h"
#include "gfx/font.h"
#include "helper_utils.h"

const static char *TAG = "DISPLAY_BUFFER";

// allocates all memory required for the display_buffer_t and initiates the
// values
esp_err_t display_buffer_init(display_buffer_handle_t *displayBufferHandle,
                              uint8_t width, uint8_t height) {
  display_buffer_handle_t db =
      (display_buffer_handle_t)malloc(sizeof(display_buffer_t));

  display_buffer_set_color(db, 255, 255, 255);
  display_buffer_set_cursor(db, 0, 0);
  db->width = width;
  db->height = height;
  db->length = db->width * db->height;

  db->bufferRed = (uint8_t *)malloc(sizeof(uint8_t) * db->length);
  db->bufferGreen = (uint8_t *)malloc(sizeof(uint8_t) * db->length);
  db->bufferBlue = (uint8_t *)malloc(sizeof(uint8_t) * db->length);
  display_buffer_clear(db);

  font_init(&db->font);

  *displayBufferHandle = db;

  return ESP_OK;
}

// resets all values in the buffer to `0`
void display_buffer_clear(display_buffer_handle_t db) {
  memset(db->bufferRed, 0, sizeof(uint8_t) * db->length);
  memset(db->bufferGreen, 0, sizeof(uint8_t) * db->length);
  memset(db->bufferBlue, 0, sizeof(uint8_t) * db->length);
}

// cleans up all memory associated with the buffer
void display_buffer_end(display_buffer_handle_t db) {
  font_end(db->font);
  free(db->bufferRed);
  free(db->bufferGreen);
  free(db->bufferBlue);
  free(db);
}

// This will apply the provided string to the buffer, using the buffer's current
// cursor and font. The string will be wrapped until it is out of the matrix.
void display_buffer_draw_string(display_buffer_handle_t db, char *string) {
  const size_t stringLength = strlen(string);
  // the index within the string we are working on
  uint16_t stringIndex;
  // the actual ascii character we are working on
  char asciiChar;
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
    asciiChar = string[stringIndex];
    bitmapRowIdx = 0;
    // convert the buffers cursor to an index, where we start this char
    bufferStartIdx = display_buffer_cursor_to_index(db);

    // If we have moved past the buffer's space, we can go ahead and end
    if (!display_buffer_cursor_is_visible(db)) {
      return;
    }

    // if not a character that we support, change to `?`
    if (!font_is_valid_ascii(asciiChar)) {
      ESP_LOGW(TAG, "Unsupported ASCII character \"%d\"", asciiChar);
      asciiChar = 63;
    }

    for (chunkIdx = 0; chunkIdx < db->font->chunksPerChar; chunkIdx++) {
      chunkVal = font_get_chunk(db->font, asciiChar, chunkIdx);

      for (chunkBitN = 1; chunkBitN <= db->font->bitsPerChunk; chunkBitN++) {
        // mask the chunk bit, then AND it to the chunk value. Use the result as
        // a boolean to check if we should set the value to the color or blank
        if (chunkVal & _BV_1ULL(db->font->bitsPerChunk - chunkBitN)) {
          display_buffer_safe_set_value(db, bufferStartIdx + bitmapRowIdx,
                                        db->colorRed, db->colorGreen,
                                        db->colorBlue);
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
            db->colorRed, db->colorGreen, db->colorBlue);
      }

      db->cursor.y--;
    }
  } else {
    while (db->cursor.y < to) {
      if (display_buffer_point_is_visible(db, db->cursor.x, db->cursor.y)) {
        display_buffer_safe_set_value(
            db, display_buffer_point_to_index(db, db->cursor.x, db->cursor.y),
            db->colorRed, db->colorGreen, db->colorBlue);
      }

      db->cursor.y++;
    }
  }
  if (display_buffer_point_is_visible(db, db->cursor.x, db->cursor.y)) {
    display_buffer_safe_set_value(
        db, display_buffer_point_to_index(db, db->cursor.x, db->cursor.y),
        db->colorRed, db->colorGreen, db->colorBlue);
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
            db->colorRed, db->colorGreen, db->colorBlue);
      }

      db->cursor.x--;
    }
  } else {
    while (db->cursor.x <= to) {
      if (display_buffer_point_is_visible(db, db->cursor.x, db->cursor.y)) {
        display_buffer_safe_set_value(
            db, display_buffer_point_to_index(db, db->cursor.x, db->cursor.y),
            db->colorRed, db->colorGreen, db->colorBlue);
      }

      db->cursor.x++;
    }
  }

  display_buffer_set_cursor(db, to, db->cursor.y);
}

void display_buffer_draw_diag_line(display_buffer_handle_t db, uint8_t toX,
                                   uint8_t toY) {
  float change =
      ((float)db->cursor.y - (float)toY) / ((float)db->cursor.x - (float)toX);
  float unroundedY = (float)db->cursor.y;

  // faster to write it twice ¯\_(ツ)_/¯
  if (db->cursor.x <= toX) {
    while (db->cursor.x <= toX) {
      if (display_buffer_point_is_visible(db, db->cursor.x,
                                          (uint8_t)round(unroundedY))) {
        display_buffer_safe_set_value(
            db,
            display_buffer_point_to_index(db, db->cursor.x,
                                          (uint8_t)round(unroundedY)),
            db->colorRed, db->colorGreen, db->colorBlue);
      }

      db->cursor.x++;
      unroundedY += change;
    }
  } else {
    while (db->cursor.x >= toX) {
      if (display_buffer_point_is_visible(db, db->cursor.x,
                                          (uint8_t)round(unroundedY))) {
        display_buffer_safe_set_value(
            db,
            display_buffer_point_to_index(db, db->cursor.x,
                                          (uint8_t)round(unroundedY)),
            db->colorRed, db->colorGreen, db->colorBlue);
      }

      db->cursor.x--;
      unroundedY -= change;
    }
  }

  display_buffer_set_cursor(db, toX, toY);
}

// if you know the shape of your line already, you can save a few clocks by
// using the above `displayBufferDrawFast*` versions
void display_buffer_draw_line(display_buffer_handle_t db, uint8_t toX,
                              uint8_t toY) {
  if (db->cursor.x == toX) {
    display_buffer_draw_vert_line(db, toY);
  } else if (db->cursor.y == toY) {
    display_buffer_draw_horiz_line(db, toX);
  } else {
    display_buffer_draw_diag_line(db, toX, toY);
  }
}

void display_buffer_draw_bitmap(display_buffer_handle_t db, uint8_t width,
                                uint8_t height, uint8_t *bufferRed,
                                uint8_t *bufferGreen, uint8_t *bufferBlue) {
  for (uint8_t row = 0; row < height; row++) {
    for (uint8_t col = 0; col < width; col++) {
      display_buffer_safe_set_value(
          db,
          display_buffer_point_to_index(db, db->cursor.x + col,
                                        db->cursor.y + row),
          bufferRed[(row * width) + col], bufferGreen[(row * width) + col],
          bufferBlue[(row * width) + col]);
    }
  }
}

void display_buffer_draw_graph(display_buffer_handle_t db, uint8_t width,
                               uint8_t height, uint8_t *values,
                               uint8_t bgColorRed, uint8_t bgColorGreen,
                               uint8_t bgColorBlue) {
  uint8_t cursorStartX = db->cursor.x;
  uint8_t cursorStartY = db->cursor.y;

  uint8_t prevColorRed = db->colorRed;
  uint8_t prevColorGreen = db->colorGreen;
  uint8_t prevColorBlue = db->colorBlue;

  // first draw the background
  display_buffer_set_color(db, bgColorRed, bgColorGreen, bgColorBlue);
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
                                 bool remoteStateInvalid, bool commandsInvalid,
                                 bool isDevMode) {
  if (isDevMode) {
    display_buffer_safe_set_value(db, 0, 0, 0, 255);
  }

  if (remoteStateInvalid) {
    display_buffer_safe_set_value(db, 2, 255, 255, 0);
  }

  if (commandsInvalid) {
    display_buffer_safe_set_value(db, 4, 255, 0, 0);
  }
}