import { createCommand, type AllCommands } from "./lib/commands";
import { fontSizeDetailsMap } from "./lib/font";
import { generateGraphValues, getWeatherData } from "./lib/weather";
import { rgbTo565 } from "./lib/util";

export const main = async (): Promise<AllCommands[]> => {

  const commands: AllCommands[] = [];

  commands.push(createCommand({
    type: 'animation',
    position: {
      x: 2,
      y: 2
    },
    delay: 1000,
    size: {
      width: 2,
      height: 2
    },
    frames: [
      [rgbTo565(255, 0, 0), rgbTo565(255, 0, 0), rgbTo565(255, 0, 0), rgbTo565(255, 0, 0)],
      [rgbTo565(0, 255, 0), rgbTo565(0, 255, 0), rgbTo565(0, 255, 0), rgbTo565(0, 255, 0)],
      [rgbTo565(0, 0, 255), rgbTo565(0, 0, 255), rgbTo565(0, 0, 255), rgbTo565(0, 0, 255)],
    ]
  }))

  try {
    const weather = await getWeatherData();
    const graphHeight = 15;
    const graphBottom = 62;
    // const graphTop = graphBottom - graphHeight - 1; // -1 for padding
    const graphLeft = 1;
    const graphRight = graphLeft + (12 * 4); // includes 1 padding

    commands.push(createCommand({
      type: 'string',
      value: `${Math.round(weather.daily.temperature_2m_min)}`,
      fontSize: 'sm',
      position: {
        x: weather.daily.temperature_2m_min >= 100 ? graphRight : graphRight + 2,
        y: graphBottom - fontSizeDetailsMap.sm.height + 2
      },
    }))

    commands.push(createCommand({
      type: 'string',
      value: `${Math.round(weather.daily.temperature_2m_max)}`,
      fontSize: 'sm',
      position: {
        x: weather.daily.temperature_2m_max >= 100 ? graphRight : graphRight + 2,
        y: graphBottom - fontSizeDetailsMap.sm.height - fontSizeDetailsMap.sm.height
      },
    }))

    const groupedPrec = generateGraphValues({
      data: weather.hourly.precipitation_probability,
      defaultMax: 100, // percent
      defaultMin: 0,
      graphHeight: graphHeight
    });
    const groupedCloud = generateGraphValues({
      data: weather.hourly.cloud_cover,
      defaultMax: 100, // percent
      defaultMin: 0,
      graphHeight: graphHeight
    });
    const groupedWind = generateGraphValues({
      data: weather.hourly.wind_speed_10m,
      defaultMax: 30, // mph
      defaultMin: 0,
      graphHeight: graphHeight
    });

    groupedPrec.forEach((prec, index) => {
      const cloud = groupedCloud[index]!;
      const wind = groupedWind[index]!;
      const offsetStart = graphLeft + (index * 4);

      commands.push(createCommand({
        type: 'line',
        color: rgbTo565(0, 100, 255),
        position: {
          x: offsetStart + 0,
          y: 62
        },
        to: {
          x: offsetStart + 0,
          y: 62 - prec
        }
      }))
      commands.push(createCommand({
        type: 'line',
        color: rgbTo565(255, 255, 255),
        position: {
          x: offsetStart + 1,
          y: 62
        },
        to: {
          x: offsetStart + 1,
          y: 62 - cloud
        }
      }))
      commands.push(createCommand({
        type: 'line',
        color: rgbTo565(255, 240, 50),
        position: {
          x: offsetStart + 2,
          y: 62
        },
        to: {
          x: offsetStart + 2,
          y: 62 - wind
        }
      }))
    })

  } catch (e) {
    console.error('Unable to generate weather data', e)
  }

  return commands
}
