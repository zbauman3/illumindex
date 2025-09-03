export type RemoteState = {
  isDevMode: boolean
  commandEndpoint: string
  fetchInterval: number
}

export type FontSize = "sm" | "md" | "lg"

export type FontSizeDetails = {
  width: number
  height: number
  bitsPerChunk: number
  chunksPerChar: number
  spacing: number
  name: FontSize
}

export type ColorRGB = {
  red: number
  green: number
  blue: number
}

export type State = {
  color?: ColorRGB
  fontSize?: FontSize
  position?: {
    x: number
    y: number
  }
}

export type Point = {
  x: number
  y: number
}

export type Size = {
  width: number
  height: number
}

export type CommandString = State & {
  type: "string"
  value: string
}

export type CommandLine = State & {
  type: "line"
  to: Point
}

export type CommandBitmap = State & {
  type: "bitmap"
  data: {
    red: number[]
    green: number[]
    blue: number[]
  }
  size: Size
}

export type CommandSetState = State & {
  type: "set-state"
}

export type CommandLineFeed = {
  type: "line-feed"
}

export type CommandAnimation = {
  type: "animation"
  position: Point
  size: Size
  frames: {
    red: number[][]
    green: number[][]
    blue: number[][]
  }
}

export type AnimationState = {
  frameCount: number
  lastShowFrame: number
}

export type CommandTime = State & {
  type: "time"
}

export type CommandDate = State & {
  type: "date"
}

export type Command =
  | CommandString
  | CommandLine
  | CommandBitmap
  | CommandSetState
  | CommandLineFeed
  | CommandAnimation
  | CommandTime
  | CommandDate

export type Bitmap = Pick<CommandBitmap, "size" | "data">

export type DrawingState = {
  cursor: Point
  color: ColorRGB
  font: FontSizeDetails
}

type CommandsConfig = {
  animationDelay: number
}

export type CommandApiResponse = {
  config: CommandsConfig
  commands: Command[]
}
