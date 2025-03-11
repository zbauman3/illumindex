import z from "zod";

const commandState = z.object({
  color: z.number().optional(),
  fontSize: z.union([
    z.literal('sm'),
    z.literal('md'),
    z.literal('lg')]
  ).optional(),
  position: z.object({
    x: z.number(),
    y: z.number()
  }).optional()
})

export type CommandString = z.infer<typeof commandString>;
export const commandString = commandState.extend({
  type: z.literal('string'),
  value: z.string()
})

export type CommandLine = z.infer<typeof commandLine>;
export const commandLine = commandState.extend({
  type: z.literal('line'),
  to: z.object({
    x: z.number(),
    y: z.number()
  })
})

export type CommandBitmap = z.infer<typeof commandBitmap>
export const commandBitmap = commandState.extend({
  type: z.literal('bitmap'),
  data: z.number().array(),
  size: z.object({
    width: z.number(),
    height: z.number()
  })
}).transform((config) => {
  if (config.data.length !== (config.size.height * config.size.width)) {
    console.error(`bitmap command does not have matching length. Filling with empty data.`, {
      length: config.data.length,
      size: (config.size.height * config.size.width)
    })

    config.data = new Array(config.size.height * config.size.width).fill(0);
  }

  return config;
})

/** based on the last set font size */
export type CommandNewLine = z.infer<typeof commandNewLine>
/** based on the last set font size */
export const commandNewLine = z.object({
  type: z.literal('line-feed')
})

export type CommandSetState = z.infer<typeof commandSetState>
export const commandSetState = commandState.extend({
  type: z.literal('set-state'),
})

export type AllCommands = z.infer<typeof allCommands>
export const allCommands = z.union([
  commandString,
  commandLine,
  commandBitmap,
  commandNewLine,
  commandSetState,
])

export type CommandsArray = z.infer<typeof commandsArray>
export const commandsArray = allCommands.array();

export const createCommand = (config: AllCommands): AllCommands => allCommands.parse(config)