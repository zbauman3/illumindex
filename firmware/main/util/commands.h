#pragma once

#include "gfx/displayBuffer.h"

// -------- Shared across all commands

typedef struct {
  uint16_t color;
  uint16_t posX;
  uint16_t posY;
  FontSize fontSize;
} CommandState;

#define COMMAND_TYPE_STRING 0
#define COMMAND_TYPE_LINE 1
#define COMMAND_TYPE_BITMAP 2
#define COMMAND_TYPE_SETSTATE 3
#define COMMAND_TYPE_LINEFEED 4
#define COMMAND_TYPE_ANIMATION 5
#define COMMAND_TYPE_TIME 6
#define COMMAND_TYPE_DATE 7

typedef enum {
  typeString = COMMAND_TYPE_STRING,
  typeLine = COMMAND_TYPE_LINE,
  typeBitmap = COMMAND_TYPE_BITMAP,
  typeSetState = COMMAND_TYPE_SETSTATE,
  typeLineFeed = COMMAND_TYPE_LINEFEED,
  typeAnimation = COMMAND_TYPE_ANIMATION,
  typeTime = COMMAND_TYPE_TIME,
  typeDate = COMMAND_TYPE_DATE,
} CommandType;

// -------- Individual Commands

typedef struct {
  CommandState *state;
  char *value;
} CommandString;

typedef struct {
  CommandState *state;
  uint16_t toX;
  uint16_t toY;
} CommandLine;

typedef struct {
  CommandState *state;
  uint16_t height;
  uint16_t width;
  uint16_t *data;
} CommandBitmap;

typedef struct {
  CommandState *state;
} CommandSetState;

typedef struct {
  // nothing
} CommandLineFeed;

typedef struct {
  uint16_t posX;
  uint16_t posY;
  uint16_t height;
  uint16_t width;
  uint16_t frameCount;
  uint16_t *frames;
} CommandAnimation;

typedef struct {
  CommandState *state;
} CommandTime;

typedef struct {
  CommandState *state;
} CommandDate;

// -------- high-level usage structs

typedef union {
  CommandString *string;
  CommandLine *line;
  CommandBitmap *bitmap;
  CommandSetState *setState;
  CommandLineFeed *lineFeed;
  CommandAnimation *animation;
  CommandTime *time;
  CommandDate *date;
} CommandValueUnion;

typedef struct {
  CommandType type;
  CommandValueUnion value;
} Command;

typedef Command *CommandHandle;

typedef struct {
  CommandHandle command;
  CommandHandle next;
} CommandListNode;

typedef struct {
  CommandListNode *head;
  CommandListNode *tail;
} CommandList;

typedef CommandList *CommandListHandle;

esp_err_t parseAndShowCommands(DisplayBufferHandle db, char *data,
                               size_t length);