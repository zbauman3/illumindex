import { createCommand, type AllCommands } from "./lib/commands";
import { trashDayCollector } from "./lib/trashDayCollector";
import { CommandsConfig } from "./lib/storage";
import { getWeatherData } from "./lib/weather";
import * as bitmaps from "./lib/bitmaps";


const getWeatherOutput = async (commands: AllCommands[]) => {
  const wetherData = await getWeatherData();
  const currentWeather = wetherData[0];
  // const nextWeather = wetherData[1];

  if (currentWeather) {
    commands.push(createCommand({
      type: 'set-state',
      position: {
        x: 0,
        y: 32
      }
    }))
    commands.push(createCommand({
      type: 'string',
      value: `Temp ${currentWeather.temperature}F`,
    }))
  }
}

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

  commands.push(createCommand({
    type: 'bitmap',
    position: {
      x: 1,
      y: 33
    },
    ...bitmaps.sun,
  }))

  return commands
}

export const mainDev = async (config: CommandsConfig): Promise<AllCommands[]> => {
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

  await getWeatherOutput(commands);

  return commands
}