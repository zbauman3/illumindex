import z from "zod";

const unitCodeSchema = z.string().transform((value) => value.replace(/^[^:]+:/, '').toLowerCase());
const quantitativeValueSchema = z.object({
  value: z.number().optional().nullable(),
  maxValue: z.number().optional().nullable(),
  minValue: z.number().optional().nullable(),
  // "A string denoting a unit of measure, expressed in the format \"{unit}\" or \"{namespace}:{unit}\".
  //Units with the namespace \"wmo\" or \"wmoUnit\" are defined in the World Meteorological Organization
  // Codes Registry at http://codes.wmo.int/common/unit and should be canonically resolvable to
  // http://codes.wmo.int/common/unit/{unit}. Units with the namespace \"nwsUnit\" are currently custom
  // and do not align to any standard. Units with no namespace or the namespace \"uc\" are compliant
  // with the Unified Code for Units of Measure syntax defined at https://unitsofmeasure.org/. This
  // also aligns with recent versions of the Geographic Markup Language (GML) standard, the IWXXM
  // standard, and OGC Observations and Measurements v2.0 (ISO/DIS 19156). Namespaced units are\
  // considered deprecated. We will be aligning API to use the same standards as GML/IWXXM in
  // the future.
  unitCode: unitCodeSchema,
})

/**
 * {
 *   "name": "Friday",
 *   "startTime": "2025-03-21T10:00:00.000Z",
 *   "endTime": "2025-03-21T22:00:00.000Z",
 *   "isDaytime": true,
 *   "temperature": 53,
 *   "shortForecast": "Sunny",
 *   "probabilityOfPrecipitation": {
 *     "value": null,
 *     "unitCode": "percent"
 *   },
 *   "windSpeed": {
 *     "maxValue": 22.224,
 *     "minValue": 5.556,
 *     "unitCode": "km_h-1"
 *   },
 *   "windDirection": "W"
 * }
 */
const periodSchema = z.object({
  /** Tonight, Thursday, Thursday Night, Wednesday, Wednesday Night */
  name: z.string(),
  startTime: z.string().transform((value) => new Date(value)),
  endTime: z.string().transform((value) => new Date(value)),
  isDaytime: z.boolean(),
  /** converted to F */
  temperature: quantitativeValueSchema.transform(({ unitCode, value }) => {
    if (!value) { return 0; }
    return /(deg)?C$/i.test(unitCode) ? (value * (9 / 5)) + 32 : value
  }),
  /**
   * "Chance Rain Showers then Partly Cloudy", "Chance Rain Showers", "Mostly Sunny then Slight Chance Rain Showers",
   * "Chance Rain And Snow", "Mostly Cloudy then Slight Chance Rain Showers", "Sunny"
   */
  shortForecast: z.string(),
  probabilityOfPrecipitation: quantitativeValueSchema.optional().nullable(),
  windSpeed: quantitativeValueSchema.optional().nullable(),
  windDirection: z.union([
    z.literal("N"),
    z.literal("NNE"),
    z.literal("NE"),
    z.literal("ENE"),
    z.literal("E"),
    z.literal("ESE"),
    z.literal("SE"),
    z.literal("SSE"),
    z.literal("S"),
    z.literal("SSW"),
    z.literal("SW"),
    z.literal("WSW"),
    z.literal("W"),
    z.literal("WNW"),
    z.literal("NW"),
    z.literal("NNW"),
  ]),
});

const weatherDataResponseSchema = z.object({
  periods: z.array(periodSchema),
}).transform(({ periods }) => periods)
export type WeatherDataResponse = z.infer<typeof weatherDataResponseSchema>

export const getWeatherData = async () => {
  const url = new URL(`https://api.weather.gov/gridpoints/ILN/84,84/forecast?units=us`);
  const response = await fetch(url, {
    headers: {
      Accept: 'application/ld+json',
      'Feature-Flags': 'forecast_temperature_qv,forecast_wind_speed_qv'
    }
  })

  if (!response.ok) {
    throw new Error(`Invalid response from ${url.host} - ${response.status}`)
  }

  const json = await response.json();
  const parsedResponse = weatherDataResponseSchema.parse(json);
  return parsedResponse;
}


// const getWeatherOutput = async (commands: AllCommands[]) => {
//   const wetherData = await getWeatherData();
//   const currentWeather = wetherData[0];
//   // const nextWeather = wetherData[1];

//   if (currentWeather) {
//     commands.push(createCommand({
//       type: 'set-state',
//       position: {
//         x: 0,
//         y: 32
//       }
//     }))
//     commands.push(createCommand({
//       type: 'string',
//       value: `Temp ${currentWeather.temperature}F`,
//     }))
//   }
// }