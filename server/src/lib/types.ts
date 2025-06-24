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

export type State = {
  color?: number
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
  data: number[]
  size: Size
}

export type CommandSetState = State & {
  type: "set-state"
}

export type CommandNewLine = {
  type: "line-feed"
}

export type CommandAnimation = {
  type: "animation"
  position: Point
  size: Size
  delay: number
  frames: number[][]
}

export type Command =
  | CommandString
  | CommandLine
  | CommandBitmap
  | CommandSetState
  | CommandNewLine
  | CommandAnimation

export type Bitmap = Pick<CommandBitmap, "size" | "data">

export type DrawingState = {
  cursor: Point
  color: number
  font: FontSizeDetails
}
