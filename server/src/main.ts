import {
  AnimationFrameCommand,
  fontSizeDetailsMap,
  type Command,
  type CommandApiResponse,
} from "@/lib"
import {
  getWeatherData,
  generateWeatherGraphs,
  // weatherCodeToBitmap,
} from "./data/weather"
import { SCREEN } from "@/data/constants"

export const main = async (): Promise<CommandApiResponse> => {
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

    commands.push({
      type: "animation",
      frames: Object.entries(weatherGraphs.graphs).map<AnimationFrameCommand[]>(
        ([name, graph]) => [
          {
            type: "string",
            value: bitmapNameToText[name],
            position: {
              x: 0,
              y:
                SCREEN.height -
                weatherGraphs.size.height -
                fontSizeDetailsMap.sm.height,
            },
            fontSize: "sm",
            color: {
              red: 255,
              green: 255,
              blue: 255,
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
              red: 50,
              green: 50,
              blue: 50,
            },
            size: graph.size,
            values: graph.data,
          },
          // {
          //   type: "bitmap",
          //   position: {
          //     x: 33,
          //     y: SCREEN.height - weatherGraphBitmaps.height - 30,
          //   },
          //   ...weatherCodeToBitmap({
          //     code: weather.current.weather_code,
          //     isDayTime: weather.current.is_day,
          //   }),
          // },
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
