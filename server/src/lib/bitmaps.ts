import type { Bitmap } from "./types"

export const cloneBitmap = (bitmap: Bitmap): Bitmap => ({
  size: { ...bitmap.size },
  data: [...bitmap.data],
})

export const createBitmap = (width: number, height: number): Bitmap => ({
  size: {
    width,
    height,
  },
  data: new Array(width * height).fill(0),
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
  const data = [...base.data]
  for (let y = 0; y < overlay.size.height; y++) {
    for (let x = 0; x < overlay.size.width; x++) {
      const baseIndex = (y + offsetY) * base.size.width + (x + offsetX)
      const overlayIndex = y * overlay.size.width + x

      if (overlay.data[overlayIndex] !== 0) {
        data[baseIndex] = overlay.data[overlayIndex]!
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
