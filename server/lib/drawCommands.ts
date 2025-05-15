import { type Bitmap, mergeBitmaps } from "./bitmaps";
import { type CommandsArray, type AllCommands, type Point } from "./commands";
import { type Color565 } from "./util";
import { type FontSizeDetails, fontSizeDetailsMap, fontIsValidAscii, fontGetChunk } from "./font";

type DrawingState = { cursor: Point, color: Color565, font: FontSizeDetails }

const parseAndSetState = (state: DrawingState, command: AllCommands): void => {
  if ('position' in command && command.position) {
    state.cursor = { ...state.cursor, ...command.position };
  }
  if ('color' in command && command.color) {
    state.color = command.color;
  }
  if ('fontSize' in command && command.fontSize) {
    state.font = { ...state.font, ...fontSizeDetailsMap[command.fontSize] };
  }
}

const setMatrixValue = ({
  point,
  color,
  bitmap,
}: {
  point: Point,
  color: Color565,
  bitmap: Bitmap,
}) => {
  if (point.x >= bitmap.size.width || point.y >= bitmap.size.height) {
    console.log('Cannot set point', point);
    return;
  }

  bitmap.data[(point.y * bitmap.size.width) + point.x] = color;
}

const fastVertLine = ({
  point,
  to,
  color,
  bitmap,
}: {
  point: Point,
  to: number,
  color: Color565,
  bitmap: Bitmap
}): void => {
  const newPoint = { ...point };
  if (newPoint.y < to) {
    const tmp = to;
    to = newPoint.y;
    newPoint.y = tmp;
  }

  while (newPoint.y >= to) {
    setMatrixValue({ point: newPoint, color, bitmap });
    newPoint.y--;
  }
}

const fastHorizonLine = ({
  point,
  to,
  color,
  bitmap,
}: {
  point: Point,
  to: number,
  color: Color565,
  bitmap: Bitmap
}): void => {
  const newPoint = { ...point };
  if (newPoint.x < to) {
    const tmp = to;
    to = newPoint.x;
    newPoint.x = tmp;
  }

  while (newPoint.x >= to) {
    setMatrixValue({ point: newPoint, color, bitmap });
    newPoint.x--;
  }
}

const drawLine = ({
  from,
  to,
  color,
  bitmap
}: {
  from: Point,
  to: Point,
  color: Color565,
  bitmap: Bitmap
}): void => {
  if (from.x === to.x) {
    fastVertLine({ point: from, to: to.y, color, bitmap });
    return;
  }

  if (from.y === to.y) {
    fastHorizonLine({ point: from, to: to.x, color, bitmap });
    return;
  }


  const m = (to.y - from.y) / (to.x - from.x);
  let floatY = from.y;

  if (from.x <= to.x) {
    while (from.x <= to.x) {
      setMatrixValue({ point: { ...from, y: Math.round(floatY) }, color, bitmap });
      from.x++;
      floatY += m;
    }
  } else {
    while (from.x >= to.x) {
      setMatrixValue({ point: { ...from, y: Math.round(floatY) }, color, bitmap });
      from.x--;
      floatY -= m;
    }
  }
}

const lineFeed = (state: DrawingState): void => {
  state.cursor.x = 0;
  state.cursor.y += state.font.height;
}

const drawString = ({
  state,
  value,
  bitmap
}: { state: DrawingState, value: string, bitmap: Bitmap }) => {
  // the actual ascii character we are working on
  let asciiChar: number;
  // the starting point to draw bits at in the buffer
  let bufferStartIdx: number;
  // which bit of the font->width we are on working on
  let bitmapRowIdx: number;
  // the actual chunk value, pulled once to be used multiple
  let chunkVal: number;
  // the number of the bit we are working on, within the chunk. This is the
  // _number_, not the zero-based index
  let chunkBitN: number;

  // loop all the characters in the string
  for (let stringIndex = 0; stringIndex < value.length; stringIndex++) {
    asciiChar = value.charCodeAt(stringIndex);
    bitmapRowIdx = 0;
    // convert the buffers cursor to an index, where we start this char
    bufferStartIdx = (state.cursor.y * bitmap.size.width) + state.cursor.x;

    // If we have moved past the buffer's space, we can go ahead and end
    if ((state.cursor.y) >= bitmap.size.height) {
      return;
    }

    // if not a character that we support, change to `?`
    if (!fontIsValidAscii(asciiChar)) {
      console.warn("Unsupported ASCII character", asciiChar);
      asciiChar = 63;
    }

    for (let chunkIdx = 0; chunkIdx < state.font.chunksPerChar; chunkIdx++) {
      chunkVal = fontGetChunk(state.font.name, state.font, asciiChar, chunkIdx);

      for (chunkBitN = 1; chunkBitN <= state.font.bitsPerChunk; chunkBitN++) {
        // mask the chunk bit, then AND it to the chunk value. Use the result as
        // a boolean to check if we should set the value to the color or blank
        const setIndex = bufferStartIdx + bitmapRowIdx;
        const setValue = (chunkVal & (1 << (state.font.bitsPerChunk - chunkBitN))) > 0
          ? state.color
          : 0
        if (setIndex < bitmap.data.length) {
          bitmap.data[setIndex] = setValue;
        }

        // increase the rows index. Move down a line if at the end
        bitmapRowIdx++;
        if (bitmapRowIdx == state.font.width) {
          bitmapRowIdx = 0;
          bufferStartIdx += bitmap.size.width;
        }
      }
    }

    // we're done with this character, move to the next position.
    if (state.cursor.x + (state.font.width * 2) + state.font.spacing <= bitmap.size.width) {
      state.cursor.x += state.font.width + state.font.spacing;
    } else {
      lineFeed(state);
    }
  }
}

export const drawCommands = ({ bitmap, commands }: { bitmap: Bitmap, commands: CommandsArray }): Bitmap => {
  const state: DrawingState = {
    cursor: { x: 0, y: 0 },
    color: 0b0000011111111111,
    font: {
      ...fontSizeDetailsMap.md
    }
  };
  const loopBitmap: Bitmap = bitmap;

  for (const command of commands) {
    parseAndSetState(state, command);

    switch (command.type) {
      case "set-state":
        // already done above
        break;
      case "bitmap":
        loopBitmap.data = mergeBitmaps({
          base: loopBitmap,
          overlay: command,
          offsetX: state.cursor.x,
          offsetY: state.cursor.y
        }).data;
        break;
      case "line":
        drawLine({
          from: state.cursor,
          to: command.to,
          color: state.color,
          bitmap: loopBitmap
        });
        break;
      case "line-feed":
        lineFeed(state);
        break
      case "string":
        drawString({
          state,
          value: command.value,
          bitmap: loopBitmap
        });
        break
      default:
        console.warn("Unknown command", command);
        break
    }
  }

  return loopBitmap
}