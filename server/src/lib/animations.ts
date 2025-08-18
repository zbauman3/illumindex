import { AnimationState, Command } from "./types"

export const createNewAnimationsState = (
  commands: Command[]
): AnimationState[] => {
  const allAnimationsState: AnimationState[] = []
  commands.forEach((command) => {
    if (command.type === "animation") {
      allAnimationsState.push({
        // start from the last one, so the first one is shown next
        lastShowFrame: command.frames.length - 1,
        frameCount: command.frames.length,
      })
    }
  })
  return allAnimationsState
}
