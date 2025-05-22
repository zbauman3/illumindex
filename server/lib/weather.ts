import z from "zod";

process.env.TZ = 'America/New_York'
// https://open-meteo.com/en/docs
// https://open-meteo.com/en/docs?latitude=40.037340675826286&longitude=-83.0197630420404&timezone=America%2FNew_York&forecast_days=3&hourly=temperature_2m,precipitation_probability,cloud_cover,wind_speed_10m,precipitation,wind_gusts_10m,uv_index,is_day&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timeformat=unixtime&current=temperature_2m,is_day,precipitation,rain,showers,snowfall,weather_code,cloud_cover,wind_speed_10m,wind_direction_10m,wind_gusts_10m&daily=sunrise,sunset,temperature_2m_max,temperature_2m_min#data_sources

const endpoint = process.env.WEATHER_ENDPOINT;
if (typeof endpoint !== 'string') {
  throw new Error('No weather API endpoint set');
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
  //   wind_gusts_10m: "mp/h"
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
    time: z.number().transform((t) => new Date(t * 1000)).array(),
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
    time: z.number().transform((t) => new Date(t * 1000)).array(),
    sunrise: z.number().transform((t) => new Date(t * 1000)).array(),
    sunset: z.number().transform((t) => new Date(t * 1000)).array(),
    temperature_2m_max: z.number().array(),
    temperature_2m_min: z.number().array(),
  })
});

export const getWeatherData = async () => {
  process.env.TZ = 'America/New_York'
  const response = await fetch(endpoint);

  if (!response.ok) {
    throw new Error(`Bad weather API response "${response.status}".`)
  }

  const json = await response.json();
  const parsedJson = weatherApiSchema.parse(json);

  const reformatted = {
    ...parsedJson,
    daily: {
      time: parsedJson.daily.time[0]!,
      sunrise: parsedJson.daily.sunrise[0]!,
      sunset: parsedJson.daily.sunset[0]!,
      temperature_2m_max: parsedJson.daily.temperature_2m_max[0]!,
      temperature_2m_min: parsedJson.daily.temperature_2m_min[0]!,
    }
  }

  return reformatted;
}

const groupValues = (input: number[]): number[] => {
  const ret: number[] = []
  for (let group = 0; group < 12; group++) {
    let groupedValue = 0;
    for (let value = 0; value < 2; value++) {
      groupedValue += input[(group * 2) + value]!;
    }
    ret.push(groupedValue / 2);
  }

  return ret;
}

export const generateGraphValues = ({
  data,
  defaultMax,
  defaultMin,
  graphHeight,
}: {
  data: number[],
  defaultMax: number,
  defaultMin: number,
  graphHeight: number
}) => {
  const maxTemp = Math.max(data.reduce((acc, temp) => (temp > acc ? temp : acc), 0), defaultMax);
  const minTemp = Math.min(data.reduce((acc, temp) => (temp < acc ? temp : acc), 1000), defaultMin);
  const tempRange = maxTemp - minTemp;
  const grouped = groupValues(data).map((temp) => {
    const valP = (temp - minTemp) / tempRange;
    return Math.round(valP * graphHeight);
  })

  return grouped
}