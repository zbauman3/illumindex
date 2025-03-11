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
      type: 'string',
      value: `Trash: ${trashInfo.trash.dayShort}`,
    }))
  }

  if (trashInfo.trash && trashInfo.recycling) {
    commands.push(createCommand({
      type: 'line-feed',
    }))
  }

  if (trashInfo.recycling) {
    commands.push(createCommand({
      type: 'string',
      value: `Recycle: ${trashInfo.recycling.dayShort}`,
    }))
  }

  return commands
}