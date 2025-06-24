import z from "zod"

const remoteStateSchema = z.object({
  isDevMode: z.boolean(),
  commandEndpoint: z.string(),
  fetchInterval: z.number().int().min(10).max(65535),
})
export type RemoteState = z.infer<typeof remoteStateSchema>

const defaultState = ((): RemoteState => {
  if (typeof process.env.ILLUMINDEX_DEFAULT_STATE === "string") {
    try {
      const parsedDefaultState = remoteStateSchema.safeParse(
        JSON.parse(process.env.ILLUMINDEX_DEFAULT_STATE)
      )
      if (parsedDefaultState.success) {
        return parsedDefaultState.data
      }
      console.error("Invalid default state:", parsedDefaultState.error.message)
    } catch (e) {
      console.error(
        `Failed to parse ILLUMINDEX_DEFAULT_STATE: "${process.env.ILLUMINDEX_DEFAULT_STATE}"`,
        e
      )
    }
  }

  return {
    isDevMode: false,
    commandEndpoint: `https://illumindex.vercel.app/api/command?token=${process.env.SITE_PASSWORD}`,
    fetchInterval: 60 * 5,
  }
})()

export { defaultState }
