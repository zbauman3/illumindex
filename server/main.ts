import { createCommand, type AllCommands } from "./lib/commands";
import { trashDayCollector } from "./lib/trashDayCollector";
import { CommandsConfig } from "./lib/storage";

export const main = async (config: CommandsConfig): Promise<AllCommands[]> => {
  const commands: AllCommands[] = [
    createCommand({
      type: 'set-state',
      color: config.colorTheme,
      fontSize: 'md',
      position: {
        x: 0,
        y: 0
      }
    })
  ];

  const trashInfo = await trashDayCollector();

  if (trashInfo.trash) {
    commands.push(createCommand({
      type: 'bitmap',
      size: {
        height: 16,
        width: 16,
      },
      data: [
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, config.colorTheme, config.colorTheme, 0x0000, 0x0000, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, 0x0000, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, config.colorTheme, config.colorTheme, 0x0000, 0x0000, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, config.colorTheme, config.colorTheme, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      ]
    }))
    commands.push(createCommand({
      type: 'set-state',
      position: {
        x: 16,
        y: 4
      }
    }))
    commands.push(createCommand({
      type: 'string',
      value: `${trashInfo.trash.dayShort}`,
    }))
  }

  if (trashInfo.trash && trashInfo.recycling) {
    commands.push(createCommand({
      type: 'set-state',
      position: {
        x: 0,
        y: 17
      }
    }))
  }

  if (trashInfo.recycling) {
    commands.push(createCommand({
      type: 'string',
      value: `Recycl ${trashInfo.recycling.dayShort}`,
    }))
  }

  return commands
}