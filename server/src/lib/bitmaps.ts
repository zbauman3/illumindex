import type { Bitmap } from "./types"

export const cloneBitmap = (bitmap: Bitmap): Bitmap => ({
  size: { ...bitmap.size },
  data: {
    red: [...bitmap.data.red],
    green: [...bitmap.data.green],
    blue: [...bitmap.data.blue],
  },
})

export const createBitmap = (width: number, height: number): Bitmap => ({
  size: {
    width,
    height,
  },
  data: {
    red: new Array(width * height).fill(0),
    green: new Array(width * height).fill(0),
    blue: new Array(width * height).fill(0),
  },
})

const mergeTwoBitmaps = ({
  base,
  overlay,
  offsetX,
  offsetY,
}: {
  base: Bitmap
  overlay: Bitmap
  offsetX: number
  offsetY: number
}): Bitmap => {
  const data = {
    red: [...base.data.red],
    green: [...base.data.green],
    blue: [...base.data.blue],
  }
  for (let y = 0; y < overlay.size.height; y++) {
    for (let x = 0; x < overlay.size.width; x++) {
      const baseIndex = (y + offsetY) * base.size.width + (x + offsetX)
      const overlayIndex = y * overlay.size.width + x

      if (overlay.data.red[overlayIndex] !== 0) {
        data.red[baseIndex] = overlay.data.red[overlayIndex]!
      }
      if (overlay.data.green[overlayIndex] !== 0) {
        data.green[baseIndex] = overlay.data.green[overlayIndex]!
      }
      if (overlay.data.blue[overlayIndex] !== 0) {
        data.blue[baseIndex] = overlay.data.blue[overlayIndex]!
      }
    }
  }

  return {
    size: base.size,
    data,
  }
}

export const mergeBitmaps = ({
  base,
  overlays,
  offsetX,
  offsetY,
}: {
  base: Bitmap
  overlays: Bitmap[]
  offsetX: number
  offsetY: number
}) =>
  overlays.reduce(
    (acc, overlay) =>
      mergeTwoBitmaps({
        base: acc,
        overlay,
        offsetX,
        offsetY,
      }),
    base
  )
