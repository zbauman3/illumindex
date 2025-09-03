import {
  Bitmap,
  Command,
  createBitmap,
  drawCommands,
  generateGraphValues,
  Size,
  ColorRGB,
  // mergeBitmaps,
} from "@/lib"
import { scaleUpGraphValues, smoothGraphValues } from "@/lib/graphing"
import z from "zod"
import { SCREEN } from "./constants"
// import * as bitmaps from "./bitmaps"

// https://open-meteo.com/en/docs?latitude=41.94235335717608&longitude=-87.6400647248335&daily=sunrise,sunset,temperature_2m_max,temperature_2m_min&hourly=temperature_2m,precipitation_probability,cloud_cover,wind_speed_10m,precipitation,wind_gusts_10m,is_day&current=temperature_2m,is_day,precipitation,cloud_cover,wind_speed_10m,wind_direction_10m,wind_gusts_10m,weather_code&timezone=America%2FChicago&forecast_days=3&timeformat=unixtime&wind_speed_unit=mph&temperature_unit=fahrenheit&precipitation_unit=inch&forecast_hours=24
// https://api.open-meteo.com/v1/forecast?latitude=41.94235335717608&longitude=-87.6400647248335&daily=sunrise,sunset,temperature_2m_max,temperature_2m_min&hourly=temperature_2m,precipitation_probability,cloud_cover,wind_speed_10m,precipitation,wind_gusts_10m,is_day&current=temperature_2m,is_day,precipitation,cloud_cover,wind_speed_10m,wind_direction_10m,wind_gusts_10m,weather_code&timezone=America%2FChicago&forecast_days=3&timeformat=unixtime&wind_speed_unit=mph&temperature_unit=fahrenheit&precipitation_unit=inch&forecast_hours=24
const endpoint = process.env.WEATHER_ENDPOINT
if (typeof endpoint !== "string") {
  throw new Error("No weather API endpoint set")
}

export type WeatherData = Awaited<ReturnType<typeof getWeatherData>>

type WeatherGraphBitmapData = {
  bitmap: Bitmap
  scaleMax: number
  scaleMin: number
  max: number
  min: number
}

export const weatherCodeToBitmap = ({
  code,
  isDayTime,
}: {
  code: number
  isDayTime: boolean
}): Bitmap => {
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  const overlays: Bitmap[] = []
  switch (code) {
    case 0: // "clear"
    case 1: // "mainly_clear"
      if (isDayTime) {
        // overlays.push(bitmaps.sunRays)
      }
      break
    case 2: // "partly_cloudy"
    case 3: // "overcast"
    case 45: // "fog"
    case 48: // "fog"
      // overlays.push(bitmaps.cloud)
      break
    case 51: // "drizzle_light"
    case 53: // "drizzle_moderate"
    case 55: // "drizzle_dense"
    case 61: // "rain_slight"
    case 63: // "rain_moderate"
    case 65: // "rain_heavy"
    case 80: // "rain_showers_slight"
    case 81: // "rain_showers_moderate"
    case 82: // "rain_showers_violent"
      // overlays.push(bitmaps.cloud)
      // overlays.push(bitmaps.rain)
      break
    case 56: // "freezing_drizzle_light"
    case 57: // "freezing_drizzle_dense"
    case 66: // "freezing_rain_light"
    case 67: // "freezing_rain_heavy"
    case 71: // "snow_slight"
    case 73: // "snow_moderate"
    case 75: // "snow_heavy"
    case 77: // "snow_grains"
    case 85: // "snow_showers_slight"
    case 86: // "snow_showers_heavy"
      // overlays.push(bitmaps.cloud)
      // overlays.push(bitmaps.snow)
      break
    case 95: // "thunderstorm"
    case 96: // "thunderstorm_slight_hail"
    case 99: // "thunderstorm_heavy_hail"
      // overlays.push(bitmaps.cloud)
      // overlays.push(bitmaps.rain)
      // overlays.push(bitmaps.lightning)
      break
  }

  // return mergeBitmaps({
  //   base: isDayTime ? bitmaps.sun : bitmaps.moon,
  //   overlays,
  //   offsetX: 0,
  //   offsetY: 0,
  // })

  return {
    size: {
      height: 30,
      width: 30,
    },
    data: {
      red: [],
      green: [],
      blue: [],
    },
  }
}

const weatherApiSchema = z.object({
  utc_offset_seconds: z.number(),
  timezone: z.string(),
  timezone_abbreviation: z.string(),
  // current_units: {
  //   time: "unixtime",
  //   interval: "seconds",
  //   temperature_2m: "°F",
  //   is_day: "",
  //   precipitation: "inch",
  //   cloud_cover: "%",
  //   wind_speed_10m: "mp/h",
  //   wind_direction_10m: "°",
  //   wind_gusts_10m: "mp/h",
  //   weather_code: 'wmo code'
  // },
  current: z.object({
    time: z.number().transform((t) => new Date(t * 1000)),
    interval: z.number(),
    temperature_2m: z.number(),
    is_day: z.number().transform(Boolean),
    precipitation: z.number(),
    cloud_cover: z.number(),
    wind_speed_10m: z.number(),
    wind_direction_10m: z.number(),
    wind_gusts_10m: z.number(),
    weather_code: z.number(),
  }),
  // "hourly_units": {
  //   "time": "unixtime",
  //   "temperature_2m": "°F",
  //   "precipitation_probability": "%",
  //   "cloud_cover": "%",
  //   "wind_speed_10m": "mp/h",
  //   "precipitation": "inch",
  //   "wind_gusts_10m": "mp/h",
  //   "is_day": ""
  // },
  hourly: z.object({
    time: z
      .number()
      .transform((t) => new Date(t * 1000))
      .array(),
    temperature_2m: z.number().array(),
    precipitation_probability: z.number().array(),
    cloud_cover: z.number().array(),
    wind_speed_10m: z.number().array(),
    precipitation: z.number().array(),
    wind_gusts_10m: z.number().array(),
    is_day: z.number().transform(Boolean).array(),
  }),
  // daily_units: {
  //   "time": "unixtime",
  //   "sunrise": "unixtime",
  //   "sunset": "unixtime",
  //   "temperature_2m_max": "°F",
  //   "temperature_2m_min": "°F"
  // },
  daily: z.object({
    time: z
      .number()
      .transform((t) => new Date(t * 1000))
      .array(),
    sunrise: z
      .number()
      .transform((t) => new Date(t * 1000))
      .array(),
    sunset: z
      .number()
      .transform((t) => new Date(t * 1000))
      .array(),
    temperature_2m_max: z.number().array(),
    temperature_2m_min: z.number().array(),
  }),
})

export const getWeatherData = async () => {
  const response = await fetch(endpoint)

  if (!response.ok) {
    throw new Error(`Bad weather API response "${response.status}".`)
  }

  const json = await response.json()
  const parsedJson = weatherApiSchema.parse(json)

  const reformatted = {
    ...parsedJson,
    daily: {
      time: parsedJson.daily.time[0]!,
      sunrise: parsedJson.daily.sunrise[0]!,
      sunset: parsedJson.daily.sunset[0]!,
      temperature_2m_max: parsedJson.daily.temperature_2m_max[0]!,
      temperature_2m_min: parsedJson.daily.temperature_2m_min[0]!,
    },
  }

  return reformatted
}

export const generateWeatherGraphBitmaps = (
  data: WeatherData
): {
  bitmaps: {
    cloud: WeatherGraphBitmapData
    wind: WeatherGraphBitmapData
    precipitation: WeatherGraphBitmapData
    temperature: WeatherGraphBitmapData
  }
} & Size => {
  const height = 16
  const width = SCREEN.width

  const graphDataToBitmap = (graphData: number[], color: ColorRGB) => {
    const graphCommands = graphData.map<Command>((val, i) => ({
      type: "line",
      color,
      position: {
        x: i,
        y: height - 1,
      },
      to: {
        x: i,
        y: height - 1 - val,
      },
    }))

    return drawCommands({
      bitmap: createBitmap(width, height),
      commands: graphCommands,
      config: {
        animationDelay: 1000,
      },
      allAnimationStates: [],
    })
  }

  // generate graph data

  const cloudGraphValues = generateGraphValues({
    data: data.hourly.cloud_cover,
    defaultMax: 100, // percent
    defaultMin: 0,
    height,
  })

  const windGraphValues = generateGraphValues({
    data: data.hourly.wind_speed_10m,
    defaultMax: 30, // mph
    defaultMin: 0,
    height,
  })

  const precipitationGraphValues = generateGraphValues({
    data: data.hourly.precipitation_probability,
    defaultMax: 50, // inches
    defaultMin: 0,
    height,
  })

  const minTemp = Math.min(...data.hourly.temperature_2m)
  const temperatureGraphValues = generateGraphValues({
    data: data.hourly.temperature_2m,
    defaultMax: 100, // Fahrenheit
    defaultMin: minTemp - 10,
    height,
  })

  // generate bitmaps from graph data

  const cloudBitmap = graphDataToBitmap(
    scaleUpGraphValues({
      data: cloudGraphValues.data,
      width: width,
    }),
    // dark gray for cloud cover
    {
      red: 120,
      green: 120,
      blue: 120,
    }
  )

  const windBitmap = graphDataToBitmap(
    smoothGraphValues(
      scaleUpGraphValues({
        data: windGraphValues.data,
        width: width,
      })
    ),
    // light gray for wind speed
    {
      red: 255,
      green: 250,
      blue: 190,
    }
  )

  const precipitationBitmap = graphDataToBitmap(
    scaleUpGraphValues({
      data: precipitationGraphValues.data,
      width: width,
    }),
    // light blue for precipitation probability
    {
      red: 80,
      green: 100,
      blue: 255,
    }
  )

  const temperatureBitmap = graphDataToBitmap(
    smoothGraphValues(
      scaleUpGraphValues({
        data: temperatureGraphValues.data,
        width: width,
      })
    ),
    // orange for temperature
    {
      red: 255,
      green: 150,
      blue: 50,
    }
  )

  return {
    width,
    height,
    bitmaps: {
      cloud: {
        bitmap: cloudBitmap,
        scaleMax: cloudGraphValues.scaleMax,
        max: cloudGraphValues.max,
        scaleMin: cloudGraphValues.scaleMin,
        min: cloudGraphValues.min,
      },
      wind: {
        bitmap: windBitmap,
        scaleMax: windGraphValues.scaleMax,
        max: windGraphValues.max,
        scaleMin: windGraphValues.scaleMin,
        min: windGraphValues.min,
      },
      precipitation: {
        bitmap: precipitationBitmap,
        scaleMax: precipitationGraphValues.scaleMax,
        max: precipitationGraphValues.max,
        scaleMin: precipitationGraphValues.scaleMin,
        min: precipitationGraphValues.min,
      },
      temperature: {
        bitmap: temperatureBitmap,
        scaleMax: temperatureGraphValues.scaleMax,
        max: temperatureGraphValues.max,
        scaleMin: temperatureGraphValues.scaleMin,
        min: temperatureGraphValues.min,
      },
    },
  }
}
