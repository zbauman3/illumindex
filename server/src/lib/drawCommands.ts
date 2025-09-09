import { mergeBitmaps } from "./bitmaps"
import { fontSizeDetailsMap, fontIsValidAscii, fontGetChunk } from "./font"
import type {
  Bitmap,
  Command,
  CommandApiResponse,
  Point,
  DrawingState,
  AnimationState,
  ColorRGB,
  Size,
} from "./types"

const parseAndSetState = (state: DrawingState, command: Command): void => {
  if ("position" in command && command.position) {
    state.cursor = { ...command.position }
  }

  if ("color" in command && command.color) {
    state.color = command.color
  }

  if ("fontSize" in command && command.fontSize) {
    state.font = { ...fontSizeDetailsMap[command.fontSize] }
  }
}

const setMatrixValue = ({
  point,
  color,
  bitmap,
}: {
  point: Point
  color: ColorRGB
  bitmap: Bitmap
}) => {
  if (point.x >= bitmap.size.width || point.y >= bitmap.size.height) {
    console.warn("Cannot set point", point)
    return
  }

  bitmap.data.red[point.y * bitmap.size.width + point.x] = color.red
  bitmap.data.green[point.y * bitmap.size.width + point.x] = color.green
  bitmap.data.blue[point.y * bitmap.size.width + point.x] = color.blue
}

const fastVerticalLine = ({
  point,
  to,
  color,
  bitmap,
}: {
  point: Point
  to: number
  color: ColorRGB
  bitmap: Bitmap
}): void => {
  const newPoint = { ...point }
  if (newPoint.y < to) {
    const tmp = to
    to = newPoint.y
    newPoint.y = tmp
  }

  while (newPoint.y >= to) {
    setMatrixValue({ point: newPoint, color, bitmap })
    newPoint.y--
  }
}

const fastHorizontalLine = ({
  point,
  to,
  color,
  bitmap,
}: {
  point: Point
  to: number
  color: ColorRGB
  bitmap: Bitmap
}): void => {
  const newPoint = { ...point }
  if (newPoint.x < to) {
    const tmp = to
    to = newPoint.x
    newPoint.x = tmp
  }

  while (newPoint.x >= to) {
    setMatrixValue({ point: newPoint, color, bitmap })
    newPoint.x--
  }
}

const drawLine = ({
  from,
  to,
  color,
  bitmap,
}: {
  from: Point
  to: Point
  color: ColorRGB
  bitmap: Bitmap
}): void => {
  if (from.x === to.x) {
    fastVerticalLine({ point: from, to: to.y, color, bitmap })
    return
  }

  if (from.y === to.y) {
    fastHorizontalLine({ point: from, to: to.x, color, bitmap })
    return
  }

  const m = (to.y - from.y) / (to.x - from.x)
  let floatY = from.y

  if (from.x <= to.x) {
    while (from.x <= to.x) {
      setMatrixValue({
        point: { ...from, y: Math.round(floatY) },
        color,
        bitmap,
      })
      from.x++
      floatY += m
    }
  } else {
    while (from.x >= to.x) {
      setMatrixValue({
        point: { ...from, y: Math.round(floatY) },
        color,
        bitmap,
      })
      from.x--
      floatY -= m
    }
  }
}

const lineFeed = (state: DrawingState): void => {
  state.cursor.x = 0
  state.cursor.y += state.font.height
}

const drawString = ({
  state,
  value,
  bitmap,
}: {
  state: DrawingState
  value: string
  bitmap: Bitmap
}) => {
  // the actual ascii character we are working on
  let asciiChar: number
  // the starting point to draw bits at in the buffer
  let bufferStartIdx: number
  // which bit of the font->width we are on working on
  let bitmapRowIdx: number
  // the actual chunk value, pulled once to be used multiple
  let chunkVal: number
  // the number of the bit we are working on, within the chunk. This is the
  // _number_, not the zero-based index
  let chunkBitN: number

  // loop all the characters in the string
  for (let stringIndex = 0; stringIndex < value.length; stringIndex++) {
    asciiChar = value.charCodeAt(stringIndex)
    bitmapRowIdx = 0
    // convert the buffers cursor to an index, where we start this char
    bufferStartIdx = state.cursor.y * bitmap.size.width + state.cursor.x

    // If we have moved past the buffer's space, we can go ahead and end
    if (state.cursor.y >= bitmap.size.height) {
      return
    }

    // if not a character that we support, change to `?`
    if (!fontIsValidAscii(asciiChar)) {
      console.warn("Unsupported ASCII character", asciiChar)
      asciiChar = 63
    }

    for (let chunkIdx = 0; chunkIdx < state.font.chunksPerChar; chunkIdx++) {
      chunkVal = fontGetChunk({
        asciiChar,
        size: state.font.name,
        chunk: chunkIdx,
      })

      for (chunkBitN = 1; chunkBitN <= state.font.bitsPerChunk; chunkBitN++) {
        // mask the chunk bit, then AND it to the chunk value. Use the result as
        // a boolean to check if we should set the value to the color or blank
        const setIndex = bufferStartIdx + bitmapRowIdx
        const setValue =
          (chunkVal & (1 << (state.font.bitsPerChunk - chunkBitN))) != 0
            ? state.color
            : { red: 0, green: 0, blue: 0 }
        if (setIndex < bitmap.data.red.length) {
          bitmap.data.red[setIndex] = setValue.red
          bitmap.data.green[setIndex] = setValue.green
          bitmap.data.blue[setIndex] = setValue.blue
        }

        // increase the rows index. Move down a line if at the end
        bitmapRowIdx++
        if (bitmapRowIdx == state.font.width) {
          bitmapRowIdx = 0
          bufferStartIdx += bitmap.size.width
        }
      }
    }

    // we're done with this character, move to the next position.
    if (
      state.cursor.x + state.font.width * 2 + state.font.spacing <=
      bitmap.size.width
    ) {
      state.cursor.x += state.font.width + state.font.spacing
    } else {
      lineFeed(state)
    }
  }
}

const drawGraph = ({
  state,
  size,
  values,
  bitmap,
}: {
  state: DrawingState
  size: Size
  values: number[]
  bitmap: Bitmap
}) => {
  if (values.length < size.width) {
    throw new Error("Not enough values to fill graph width")
  }

  const maxValue = Math.max(...values)
  const minValue = Math.min(...values)
  if (maxValue > size.height) {
    throw new Error("Max value exceeds graph height")
  }
  if (minValue < 0) {
    throw new Error("Min value is below zero")
  }

  values.forEach((value, x) => {
    const parsedValue = Math.min(Math.max(value, 0), size.height)
    drawLine({
      from: { x: state.cursor.x + x, y: state.cursor.y + size.height - 1 },
      to: {
        x: state.cursor.x + x,
        y: state.cursor.y + size.height - parsedValue,
      },
      color: state.color,
      bitmap,
    })
  })
}

/**
 * This function takes a bitmap and a list of commands, and draws the
 * commands onto the bitmap.
 */
export const drawCommands = ({
  bitmap,
  commands,
  allAnimationStates,
}: {
  bitmap: Bitmap
  allAnimationStates: AnimationState[]
} & CommandApiResponse): Bitmap => {
  const state: DrawingState = {
    cursor: { x: 0, y: 0 },
    color: { red: 255, green: 255, blue: 255 },
    font: {
      ...fontSizeDetailsMap.md,
    },
  }
  const loopBitmap: Bitmap = bitmap
  const animationCount = 0

  for (const command of commands) {
    parseAndSetState(state, command)

    switch (command.type) {
      case "set-state":
        // already done above
        break
      case "animation":
        const animationState = allAnimationStates[animationCount]
        if (!animationState) {
          throw new Error("Missing animation state")
        }

        animationState.lastShowFrame++
        if (animationState.lastShowFrame >= animationState.frameCount) {
          animationState.lastShowFrame = 0
        }

        const prevX = state.cursor.x
        const prevY = state.cursor.y

        state.cursor.x = command.position.x
        state.cursor.y = command.position.y

        loopBitmap.data = mergeBitmaps({
          base: loopBitmap,
          overlays: [
            {
              data: {
                red: command.frames.red[animationState.lastShowFrame],
                green: command.frames.green[animationState.lastShowFrame],
                blue: command.frames.blue[animationState.lastShowFrame],
              },
              size: command.size,
            },
          ],
          offsetX: state.cursor.x,
          offsetY: state.cursor.y,
        }).data

        state.cursor.x = prevX
        state.cursor.y = prevY
        break
      case "bitmap":
        loopBitmap.data = mergeBitmaps({
          base: loopBitmap,
          overlays: [command],
          offsetX: state.cursor.x,
          offsetY: state.cursor.y,
        }).data
        break
      case "line":
        drawLine({
          from: state.cursor,
          to: command.to,
          color: state.color,
          bitmap: loopBitmap,
        })
        break
      case "line-feed":
        lineFeed(state)
        break
      case "string":
        drawString({
          state,
          value: command.value,
          bitmap: loopBitmap,
        })
        break
      case "time":
        const timeStr = new Date().toLocaleTimeString("en-US", {
          timeStyle: "short",
        })
        drawString({
          state,
          value: timeStr,
          bitmap: loopBitmap,
        })
        break
      case "date":
        const dateStr = new Date().toLocaleDateString("en-US", {
          year: "numeric",
          month: "short",
          day: "2-digit",
        })
        drawString({
          state,
          value: dateStr,
          bitmap: loopBitmap,
        })
        break
      case "graph":
        drawGraph({
          state,
          size: command.size,
          values: command.values,
          bitmap: loopBitmap,
        })
        break
      default:
        console.warn("Unknown command", command)
        break
    }
  }

  return loopBitmap
}
