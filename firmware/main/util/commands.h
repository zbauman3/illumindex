#pragma once

#include "gfx/displayBuffer.h"

esp_err_t parseAndShowCommands(DisplayBufferHandle db, char *data,
                               size_t length);