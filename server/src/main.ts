import {
  createBitmap,
  drawCommands,
  fontSizeDetailsMap,
  rgbTo565,
  type Command,
} from "@/lib"
import { generateGraphValues } from "@/lib"
import { getWeatherData } from "./data/weather"

export const main = async (): Promise<Command[]> => {
  const commands: Command[] = []

  try {
    const weather = await getWeatherData()
    const graphHeight = 15
    const graphWidth = weather.hourly.cloud_cover.length
    const graphBottom = 63
    const graphTop = graphBottom - graphHeight + 1
    const graphLeft = 0
    const graphRight = graphLeft + graphWidth

    console.log({
      graphTop,
      graphLeft,
      last: graphTop + graphHeight,
      graphWidth,
    })

    commands.push({
      type: "string",
      value: `${Math.round(weather.daily.temperature_2m_min)}`,
      fontSize: "sm",
      position: {
        x:
          weather.daily.temperature_2m_min >= 100 ? graphRight : graphRight + 2,
        y: graphBottom - fontSizeDetailsMap.sm.height + 2,
      },
    })

    commands.push({
      type: "string",
      value: `${Math.round(weather.daily.temperature_2m_max)}`,
      fontSize: "sm",
      position: {
        x:
          weather.daily.temperature_2m_max >= 100 ? graphRight : graphRight + 2,
        y:
          graphBottom -
          fontSizeDetailsMap.sm.height -
          fontSizeDetailsMap.sm.height,
      },
    })

    const groupedPrec = generateGraphValues({
      data: weather.hourly.precipitation_probability,
      defaultMax: 100, // percent
      defaultMin: 0,
      graphHeight: graphHeight,
    })
    const groupedCloud = generateGraphValues({
      data: weather.hourly.cloud_cover,
      defaultMax: 100, // percent
      defaultMin: 0,
      graphHeight: graphHeight,
    })
    const groupedWind = generateGraphValues({
      data: weather.hourly.wind_speed_10m,
      defaultMax: 30, // mph
      defaultMin: 0,
      graphHeight: graphHeight,
    })

    commands.push({
      type: "animation",
      delay: 1000,
      position: {
        x: graphLeft,
        y: graphTop,
      },
      size: {
        width: graphWidth,
        height: graphHeight,
      },
      frames: [groupedCloud, groupedWind, groupedPrec].map((group) => {
        return drawCommands({
          bitmap: createBitmap(graphWidth, graphHeight),
          commands: group.map(
            (val, i): Command => ({
              type: "line",
              color: rgbTo565(0, 100, 255),
              // TODO: is this from/to actually valid?
              position: {
                x: i,
                y: graphHeight - 1,
              },
              to: {
                x: i,
                y: graphHeight - 1 - val,
              },
            })
          ),
        }).data
      }),
    })
  } catch (e) {
    console.error("Unable to generate weather data", e)
  }

  return commands
}
