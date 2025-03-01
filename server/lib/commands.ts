type CommandState = {
  color?: number,
  fontSize?: 'sm' | 'md' | 'lg',
  position?: {
    x: number,
    y: number
  }
}


export type CommandString = CommandState & {
  type: 'string',
  value: string,
}

export type CommandLine = CommandState & {
  type: 'line',
  to: {
    x: number,
    y: number
  }
}

export type CommandBitmap = CommandState & {
  type: 'bitmap',
  data: number[],
  size: {
    width: number,
    height: number
  }
}

/** based on the last set font size */
export type CommandNewLine = {
  type: 'line-feed',
}

export type CommandSetState = CommandState & {
  type: 'set-state',
}

export type AllCommands = CommandString | CommandLine | CommandBitmap | CommandNewLine | CommandSetState

export const createCommand = (config: AllCommands): AllCommands => {
  if (config.type === 'bitmap' && config.data.length !== (config.size.height * config.size.width)) {
    console.error(`bitmap command does not have matching length. Filling with empty data.`, {
      length: config.data.length,
      size: (config.size.height * config.size.width)
    })

    config.data = new Array(config.size.height * config.size.width).fill(0);
  }


  return config;
}