import {
  AnimationFrameCommand,
  fontSizeDetailsMap,
  type Command,
  type CommandApiResponse,
} from "@/lib"
import {
  getWeatherData,
  generateWeatherGraphs,
  weatherCodeToBitmap,
} from "./data/weather"
import { SCREEN } from "@/data/constants"

export const main = async (): Promise<CommandApiResponse> => {
  const dividerLineY =
    fontSizeDetailsMap.lg.height + fontSizeDetailsMap.sm.height + 1
  const commands: Command[] = [
    {
      type: "time",
      position: { x: 0, y: 0 },
      fontSize: "lg",
      color: {
        red: 255,
        green: 255,
        blue: 255,
      },
    },
    {
      type: "date",
      position: { x: 1, y: fontSizeDetailsMap.lg.height },
      fontSize: "sm",
    },
    {
      type: "line",
      position: {
        x: 0,
        y: dividerLineY,
      },
      to: {
        x: SCREEN.width,
        y: dividerLineY,
      },
      color: { red: 255, green: 255, blue: 255 },
    },
  ]

  try {
    const weather = await getWeatherData()
    const weatherGraphs = generateWeatherGraphs(weather)

    const tempMax = Math.max(...weather.hourly.temperature_2m)
    const tempMin = Math.min(...weather.hourly.temperature_2m)
    const couldSnow = (tempMax + tempMin) / 2 < 30
    const bitmapNameToText: Record<string, string> = {
      cloud: "Cloud",
      precipitation: couldSnow ? "Snow" : "Rain",
      temperature: "Temp",
      wind: "Wind",
    }
    const bitmapNameToUnit: Record<string, string> = {
      cloud: "%",
      precipitation: "%",
      temperature: "F",
      wind: "MPH",
    }
    const weatherBitmap = weatherCodeToBitmap({
      code: weather.current.weather_code,
      isDayTime: weather.current.is_day,
    })

    commands.push({
      type: "bitmap",
      position: {
        x: SCREEN.width - weatherBitmap.size.width - 2,
        y: dividerLineY + 3,
      },
      ...weatherBitmap,
    })

    commands.push({
      type: "animation",
      frames: Object.entries(weatherGraphs.graphs).map<AnimationFrameCommand[]>(
        ([name, graph]) => [
          {
            type: "string",
            value: bitmapNameToText[name],
            position: {
              x: 0,
              y: dividerLineY + 3,
            },
            fontSize: "sm",
            color: {
              red: 255,
              green: 255,
              blue: 255,
            },
          },
          {
            type: "string",
            value:
              Math.floor(graph.current).toString() +
              " " +
              bitmapNameToUnit[name],
            position: {
              x: 0,
              y: dividerLineY + 3 + fontSizeDetailsMap.sm.height + 2,
            },
          },
          {
            type: "string",
            value: Math.floor(graph.min) + "/" + Math.floor(graph.max),
            position: {
              x: 0,
              y: dividerLineY + 3 + (fontSizeDetailsMap.sm.height + 2) * 2,
            },
          },
          {
            type: "graph",
            position: {
              x: 0,
              y: SCREEN.height - weatherGraphs.size.height,
            },
            color: graph.color,
            backgroundColor: {
              red: 10,
              green: 10,
              blue: 10,
            },
            size: graph.size,
            values: graph.data,
          },
        ]
      ),
    })
  } catch (e) {
    console.error("Unable to generate weather data", e)
  }

  return {
    config: {
      animationDelay: 3000,
    },
    commands,
  }
}
