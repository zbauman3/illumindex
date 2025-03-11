import { createClient } from 'redis';
import z from "zod";

import { commandsArray, type CommandsArray } from "./commands";
import { ensure565Color, rgbTo565 } from './util';

export const redis = await createClient({
  url: process.env.REDIS_URL
})
  .on('error', err => console.log('Redis Client Error', err))
  .connect();

export type CommandsConfig = z.infer<typeof commandsConfig>;
export const commandsConfig = z.object({
  dataSource: z.union([z.literal('storage'), z.literal('source-code')]),
  colorTheme: z.number().transform(ensure565Color)
})

export const storageKeys = {
  commandsConfig: 'commands-config',
  commands: 'commands'
} as const;

export const defaultCommanConfig: CommandsConfig = {
  colorTheme: rgbTo565(0, 255, 156),
  dataSource: 'source-code'
}

export const getCommandsConfig = async (): Promise<CommandsConfig> => {
  try {
    const value = await redis.get(storageKeys.commandsConfig);
    return commandsConfig.parse(JSON.parse(value || '{}'));
  } catch (e) {
    console.error(`Unable to get ${storageKeys.commandsConfig}`, e);

    return defaultCommanConfig
  }
}

export const setCommandsConfig = async (config: CommandsConfig) => {
  try {
    await redis.set(storageKeys.commandsConfig, JSON.stringify(config));
  } catch (e) {
    console.error(`Unable to set ${storageKeys.commandsConfig}`, e);
  }
}

export const getCommands = async (): Promise<CommandsArray> => {
  try {
    const value = await redis.get(storageKeys.commands);
    return commandsArray.parse(JSON.parse(value || ''));
  } catch (e) {
    console.error(`Unable to get ${storageKeys.commands}`, e);

    return []
  }
}

export const setCommands = async (commands: CommandsArray) => {
  try {
    await redis.set(storageKeys.commands, JSON.stringify(commands));
  } catch (e) {
    console.error(`Unable to set ${storageKeys.commands}`, e);
  }
}