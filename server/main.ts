import { createCommand, type AllCommands } from "./lib/commands";
import { trashDayCollector } from "./lib/trashDayCollector";
import * as bitmaps from "./lib/bitmaps";
import { FontSize } from "./lib/font";

export const main = async (): Promise<AllCommands[]> => {
  const color = 0b0000011111111111;

  const commands: AllCommands[] = [
    createCommand({
      type: 'set-state',
      color: color,
      fontSize: FontSize.fontSizeMd,
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
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, color, color, color, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, color, color, color, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, color, color, color, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, color, color, color, color, color, color, color, color, color, color, color, 0x0000, 0x0000, 0x0000,
        0x0000, color, color, color, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, color, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, color, 0x0000, color, color, color, color, color, color, 0x0000, color, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, color, 0x0000, color, color, color, color, color, color, 0x0000, color, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, color, 0x0000, 0x0000, 0x0000, color, color, 0x0000, 0x0000, 0x0000, color, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, color, 0x0000, 0x0000, 0x0000, color, color, 0x0000, 0x0000, 0x0000, color, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, color, 0x0000, 0x0000, 0x0000, color, color, 0x0000, 0x0000, 0x0000, color, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, color, color, color, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, color, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, color, color, color, color, color, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, color, 0x0000, 0x0000, 0x0000,
        0x0000, color, color, 0x0000, 0x0000, color, color, 0x0000, 0x0000, 0x0000, 0x0000, color, color, 0x0000, 0x0000, 0x0000,
        0x0000, color, color, 0x0000, 0x0000, color, color, 0x0000, 0x0000, 0x0000, color, color, color, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, color, color, color, color, color, color, color, color, color, color, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, color, color, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
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

  commands.push(createCommand({
    type: 'bitmap',
    position: {
      x: 1,
      y: 33
    },
    ...bitmaps.mergeManyBitmaps({ base: bitmaps.sun, overlays: [bitmaps.cloudDark], offsetX: 0, offsetY: 0 }),
  }))

  return commands
}
