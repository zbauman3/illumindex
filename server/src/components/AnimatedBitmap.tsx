"use client"

import { useEffect, useState } from "react"
import { BitmapComponent } from "@/components/Bitmap"
import {
  Bitmap,
  CommandApiResponse,
  createBitmap,
  drawCommands,
  createNewAnimationsState,
} from "@/lib"

export const AnimatedBitmap = ({ commands, config }: CommandApiResponse) => {
  const [bitmap, setBitmap] = useState<Bitmap>(() => createBitmap(64, 64))

  useEffect(() => {
    const allAnimationStates = createNewAnimationsState(commands)
    const applyBitmap = () => {
      const newBitmap = createBitmap(64, 64)
      const updateBitmap = drawCommands({
        bitmap: newBitmap,
        commands,
        allAnimationStates,
      })

      setBitmap(updateBitmap)
    }

    const intTime = setInterval(applyBitmap, config.animationDelay)
    applyBitmap()

    return () => {
      clearInterval(intTime)
    }
  }, [commands, config])

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
