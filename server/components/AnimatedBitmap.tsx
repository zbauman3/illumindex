'use client';

import { useEffect, useState } from "react";
import { BitmapComponent } from "../components/Bitmap";
import { Bitmap, createEmptyBitmap } from "../lib/bitmaps";
import { drawCommands, getAnimationCommand, doAnimationCommand } from "../lib/drawCommands";
import { type CommandsArray } from "@/lib/commands";

export const AnimatedBitmap = ({ commands }: { commands: CommandsArray }) => {
  const [bitmap, setBitmap] = useState<Bitmap>(() => createEmptyBitmap(64, 64))

  useEffect(() => {
    const newBitmap = createEmptyBitmap(64, 64);
    const updateBitmap = drawCommands({ bitmap: newBitmap, commands });

    setBitmap(updateBitmap);

    const animation = getAnimationCommand(commands);
    if (!animation) {
      console.log('No animation');
      return;
    }

    const firstFrame = doAnimationCommand({
      animation,
      bitmap: updateBitmap,
      lastIndex: 0
    })

    setBitmap(firstFrame.bitmap);
    let lastIndex = firstFrame.lastIndex;

    const interval = setInterval(() => {
      const frame = doAnimationCommand({
        animation,
        bitmap: updateBitmap,
        lastIndex
      })
      lastIndex = frame.lastIndex;
      setBitmap(frame.bitmap);
    }, animation.delay);

    return () => { clearInterval(interval); }
  }, [commands])

  return (
    <div
      style={{
        display: 'flex',
        flexDirection: 'row',
        flexWrap: 'nowrap',
        justifyContent: 'center',
        alignItems: 'center',
      }}
    >
      <BitmapComponent bitmap={bitmap} />
    </div>
  )
}