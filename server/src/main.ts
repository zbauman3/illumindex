import {
  CommandAnimation,
  createBitmap,
  drawCommands,
  fontSizeDetailsMap,
  type Command,
  type CommandApiResponse,
} from "@/lib"
import {
  getWeatherData,
  generateWeatherGraphBitmaps,
  // weatherCodeToBitmap,
} from "./data/weather"
import { SCREEN } from "@/data/constants"

export const main = async (): Promise<CommandApiResponse> => {
  const commands: Command[] = []

  try {
    const weather = await getWeatherData()
    const weatherGraphBitmaps = generateWeatherGraphBitmaps(weather)

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
      position: {
        x: 0,
        y: 0,
      },
      size: {
        width: SCREEN.width,
        height: SCREEN.height,
      },
      frames: Object.entries(weatherGraphBitmaps.bitmaps)
        .map(
          ([name, graph]) =>
            drawCommands({
              bitmap: createBitmap(SCREEN.width, SCREEN.height),
              config: {
                animationDelay: 1000,
              },
              allAnimationStates: [],
              commands: [
                {
                  type: "string",
                  value: bitmapNameToText[name],
                  position: {
                    x: 0,
                    y:
                      SCREEN.height -
                      weatherGraphBitmaps.height -
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
                  type: "bitmap",
                  position: {
                    x: 0,
                    y: SCREEN.height - weatherGraphBitmaps.height,
                  },
                  ...graph.bitmap,
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
              ],
            }).data
        )
        .reduce(
          (acc, loopFrame) => {
            acc.red.push(loopFrame.red)
            acc.green.push(loopFrame.green)
            acc.blue.push(loopFrame.blue)
            return acc
          },
          {
            red: [],
            green: [],
            blue: [],
          } as CommandAnimation["frames"]
        ),
    })

    commands.push(
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
        type: "line-feed",
      },
      {
        type: "date",
        fontSize: "sm",
      }
    )
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
