import z from "zod";

export const remoteStateSchema = z.object({
  isDevMode: z.boolean(),
  devModeEndpoint: z.string(),
});
export type RemoteState = z.infer<typeof remoteStateSchema>;

const defaultState: RemoteState = (() => {
  if (typeof process.env.ILLUMINDEX_DEFAULT_STATE === "string") {
    try {
      const parsedDefaultState = remoteStateSchema.safeParse(JSON.parse(process.env.ILLUMINDEX_DEFAULT_STATE));
      if (parsedDefaultState.success) {
        return parsedDefaultState.data;
      }
      console.error("Invalid default state:", parsedDefaultState.error.message);
    } catch (e) {
      console.error(`Failed to parse ILLUMINDEX_DEFAULT_STATE: "${process.env.ILLUMINDEX_DEFAULT_STATE}"`, e);
    }
  }

  return { isDevMode: false, devModeEndpoint: "" };
})()

export { defaultState }