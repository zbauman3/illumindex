"use client"

import { useEffect, useState } from "react"
import { BitmapComponent } from "@/components/Bitmap"
import {
  Bitmap,
  Command,
  createBitmap,
  drawCommands,
  findAnimation,
  applyAnimation,
} from "@/lib"

export const AnimatedBitmap = ({ commands }: { commands: Command[] }) => {
  const [bitmap, setBitmap] = useState<Bitmap>(() => createBitmap(64, 64))

  useEffect(() => {
    const newBitmap = createBitmap(64, 64)
    const updateBitmap = drawCommands({ bitmap: newBitmap, commands })

    setBitmap(updateBitmap)

    const animation = findAnimation(commands)
    if (!animation) {
      return
    }

    const firstFrame = applyAnimation({
      animation,
      bitmap: updateBitmap,
      lastIndex: 0,
    })

    setBitmap(firstFrame.bitmap)
    let lastIndex = firstFrame.lastIndex

    const interval = setInterval(() => {
      const frame = applyAnimation({
        animation,
        bitmap: updateBitmap,
        lastIndex,
      })
      lastIndex = frame.lastIndex
      setBitmap(frame.bitmap)
    }, animation.delay)

    return () => {
      clearInterval(interval)
    }
  }, [commands])

  return (
    <div
      style={{
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "center",
        alignItems: "center",
      }}
    >
      <BitmapComponent bitmap={bitmap} />
    </div>
  )
}
