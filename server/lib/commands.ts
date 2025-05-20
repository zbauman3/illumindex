import z from "zod";

import { FontSize } from "./font";

const commandState = z.object({
  color: z.number().optional(),
  fontSize: z.union([
    z.literal(FontSize.fontSizeSm),
    z.literal(FontSize.fontSizeMd),
    z.literal(FontSize.fontSizeLg)]
  ).optional(),
  position: z.object({
    x: z.number(),
    y: z.number()
  }).optional()
})

export type State = Required<z.infer<typeof commandState>>
export type Point = State['position']

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

export type CommandAnimation = z.infer<typeof commandAnimation>
export const commandAnimation = z.object({
  type: z.literal('animation'),
  delay: z.number(),
  position: commandState.shape.position.refine((a) => !!a),
  size: z.object({
    width: z.number(),
    height: z.number()
  }),
  frames: z.array(z.number().array())
})

export type AllCommands = z.infer<typeof allCommands>
export const allCommands = z.union([
  commandString,
  commandLine,
  commandBitmap,
  commandNewLine,
  commandSetState,
  commandAnimation
])

export type CommandsArray = z.infer<typeof commandsArray>
export const commandsArray = allCommands.array();

export const createCommand = (config: AllCommands): AllCommands => allCommands.parse(config)