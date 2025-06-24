import type { Command, CommandAnimation, Bitmap } from "./types"
import { drawCommands } from "./drawCommands"
import { cloneBitmap, createBitmap, mergeBitmaps } from "./bitmaps"

/**
 * Creates an animation command (i.g. bitmap frames) from a set of commands.
 * This allows the animation of other types of commands, such as lines or strings.
 *
 * Using animations for drawing the entire screen costs more memory on the receiving
 * end, but allows for more complex animations. So ideally you should only use
 * animations for parts of the screen that need to be animated, and use
 * `drawCommands` for the rest of the screen.
 */
export const createAnimation = ({
  delay,
  frames,
  position,
  size,
}: Pick<CommandAnimation, "delay" | "position" | "size"> & {
  frames: Command[][]
}): CommandAnimation => {
  const outputFrames: CommandAnimation["frames"] = frames.map(
    (commands) =>
      drawCommands({
        bitmap: createBitmap(size.width, size.height),
        commands,
      }).data
  )

  return {
    delay,
    position,
    size,
    type: "animation",
    frames: outputFrames,
  }
}

export const cloneAnimation = (
  animation: CommandAnimation
): CommandAnimation => ({
  ...animation,
  position: { ...animation.position },
  size: { ...animation.size },
  frames: animation.frames.map((frame) => [...frame]),
})

export const findAnimation = (
  commands: Command[]
): CommandAnimation | undefined =>
  commands.find((command) => command.type === "animation")

/**
 * Applies an animation command to a bitmap, returning a new bitmap
 * with the animation applied and the index of the last applied frame.
 */
export const applyAnimation = ({
  bitmap,
  animation,
  lastIndex,
}: {
  /** the bitmap to apply the animation to */
  bitmap: Bitmap
  /** the animation command to apply */
  animation: CommandAnimation
  /** the index of the previously applied animation frame */
  lastIndex: number
}): {
  /** the new bitmap with the animation applied */
  bitmap: Bitmap
  /** the index of the animation frame that was applied to this bitmap */
  lastIndex: number
} => {
  const newBitmap: Bitmap = {
    ...bitmap,
    data: [...bitmap.data],
    size: { ...bitmap.size },
  }

  let nextIndex = lastIndex + 1
  if (nextIndex >= animation.frames.length) {
    nextIndex = 0
  }

  const animationBitmap: Bitmap = {
    data: animation.frames[nextIndex]!,
    size: { ...animation.size },
  }

  newBitmap.data = mergeBitmaps({
    base: newBitmap,
    overlays: [animationBitmap],
    offsetX: animation.position.x,
    offsetY: animation.position.y,
  }).data

  return {
    bitmap: newBitmap,
    lastIndex: nextIndex,
  }
}

/**
 * Creates an animation loop that applies the given animation to the bitmap
 * on every frame, calling the provided callback with the updated bitmap.
 *
 * The function returns a cleanup function that stops the animation loop
 * and returns the last index of the applied frame.
 */
export const createAnimationLoop = ({
  bitmap,
  animation,
  callback,
}: {
  /** the bitmap to apply the animation to */
  bitmap: Bitmap
  /** the animation command to apply */
  animation: CommandAnimation
  /** The function to call with teh updated bitmap on every new frame */
  callback: (bitmap: Bitmap, frame: number) => void
}) => {
  const baseBitmap = cloneBitmap(bitmap)
  const baseAnimation = cloneAnimation(animation)
  // pretend that we've just shown the last frame, so the next frame will be the first one
  let lastIndex = animation.frames.length - 1
  const doAnimationLoop = () => {
    const loopFrame = applyAnimation({
      bitmap: baseBitmap,
      animation: baseAnimation,
      lastIndex,
    })

    lastIndex = loopFrame.lastIndex
    callback(loopFrame.bitmap, loopFrame.lastIndex)
  }

  doAnimationLoop()
  const timerId = setInterval(doAnimationLoop, baseAnimation.delay)

  return () => {
    clearInterval(timerId)
    return lastIndex
  }
}
